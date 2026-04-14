#include <amxmodx>
#include <easy_http>

#pragma semicolon 1

/*
 * Integration test suite for requests that are still in flight during a map change.
 *
 * The suite persists its state to:
 *   addons/amxmodx/data/ezhttp_mapchange_test.ini
 *
 * By default it runs one scenario, changes the map, verifies that the server
 * survived, sends a probe request on the next map, and only then advances to
 * the next scenario.
 *
 * Useful server commands:
 *   ezhttp_mapchange_test_reset
 *   ezhttp_mapchange_test_start
 */

#define PLUGIN_NAME "EasyHttp MapChange Test"
#define PLUGIN_VERSION "1.0"
#define PLUGIN_AUTHOR "Wilian M."

#define STATE_FILE_PATH "addons/amxmodx/data/ezhttp_mapchange_test.ini"

#define PHASE_IDLE "idle"
#define PHASE_VERIFY_WAIT "verify_wait"
#define PHASE_PROBE_PENDING "probe_pending"
#define PHASE_DONE "done"

#define TASK_START_SCENARIO 4101
#define TASK_PERFORM_CHANGELEVEL 4102
#define TASK_VERIFY_AFTER_CHANGE 4103
#define TASK_NEXT_SCENARIO 4104
#define TASK_EXECUTE_CHANGELEVEL 4105

#define CALLBACK_KIND_INFLIGHT 1
#define CALLBACK_KIND_PROBE 2

#define REQUEST_TIMEOUT_BUFFER_SEC 12

enum MapChangeScenarioId
{
    Scenario_CancelSingleMain = 0,
    Scenario_ForgetSingleMain,
    Scenario_CancelStressMain,
    Scenario_ForgetStressMain,
    Scenario_CancelSequentialQueue,
    Scenario_ForgetSequentialQueue,
    Scenario_Count
};

new const g_ScenarioNames[Scenario_Count][] = {
    "cancel_single_main",
    "forget_single_main",
    "cancel_stress_main",
    "forget_stress_main",
    "cancel_sequential_queue",
    "forget_sequential_queue"
};

new const EzHttpPluginEndBehaviour:g_ScenarioBehaviours[Scenario_Count] = {
    EZH_CANCEL_REQUEST,
    EZH_FORGET_REQUEST,
    EZH_CANCEL_REQUEST,
    EZH_FORGET_REQUEST,
    EZH_CANCEL_REQUEST,
    EZH_FORGET_REQUEST
};

new const g_ScenarioRequestCounts[Scenario_Count] = {
    1,
    1,
    6,
    6,
    4,
    4
};

new const bool:g_ScenarioUsesQueue[Scenario_Count] = {
    false,
    false,
    false,
    false,
    true,
    true
};

new g_CvarAutostart;
new g_CvarBaseUrl;
new g_CvarChangeAfter;
new g_CvarVerifyWait;
new g_CvarNextScenarioDelay;
new g_CvarRequestDelay;
new g_CvarScenarioRepeats;
new g_CvarStressRequests;
new g_CvarQueueRequests;
new g_CvarProbeRequests;
new g_CvarTargetMap;

new g_StatePhase[32];
new g_StateScenarioName[64];
new g_StateCurrentMap[64];
new g_StateTargetMap[64];
new g_StateLastResult[32];
new g_StateLastError[192];

new g_StateScenarioIndex;
new g_StateScenarioRepeat;
new g_StatePassCount;
new g_StateFailCount;
new g_StateStartedRequests;
new g_StateDispatchFailures;
new g_StateUnexpectedCallbacks;
new bool:g_StateProbeOk;

new g_RuntimeProbeExpected;
new g_RuntimeProbeCompleted;
new g_RuntimeProbeFailures;
new g_RuntimeProbeDispatchFailures;

public plugin_init()
{
    register_plugin(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR);

    g_CvarAutostart = register_cvar("ezhttp_mapchange_test_autostart", "1");
    g_CvarBaseUrl = register_cvar("ezhttp_mapchange_test_base_url", "https://httpbin.org");
    g_CvarChangeAfter = register_cvar("ezhttp_mapchange_test_change_after", "1.0");
    g_CvarVerifyWait = register_cvar("ezhttp_mapchange_test_verify_wait", "8.0");
    g_CvarNextScenarioDelay = register_cvar("ezhttp_mapchange_test_next_scenario_delay", "2.0");
    g_CvarRequestDelay = register_cvar("ezhttp_mapchange_test_request_delay", "6");
    g_CvarScenarioRepeats = register_cvar("ezhttp_mapchange_test_repeat", "1");
    g_CvarStressRequests = register_cvar("ezhttp_mapchange_test_stress_requests", "6");
    g_CvarQueueRequests = register_cvar("ezhttp_mapchange_test_queue_requests", "4");
    g_CvarProbeRequests = register_cvar("ezhttp_mapchange_test_probe_requests", "1");
    g_CvarTargetMap = register_cvar("ezhttp_mapchange_test_target_map", "");

    register_srvcmd("ezhttp_mapchange_test_reset", "cmd_reset_state");
    register_srvcmd("ezhttp_mapchange_test_start", "cmd_start_suite");
}

public plugin_cfg()
{
    load_state();

    if (equal(g_StatePhase, PHASE_VERIFY_WAIT) || equal(g_StatePhase, PHASE_PROBE_PENDING))
    {
        announce_resume();
        set_task(get_pcvar_float(g_CvarVerifyWait), "begin_post_map_verification", TASK_VERIFY_AFTER_CHANGE);
        return;
    }

    if (equal(g_StatePhase, PHASE_DONE))
    {
        print_suite_summary("suite already completed");
        return;
    }

    if (g_StateScenarioIndex >= _:Scenario_Count)
    {
        finish_suite("scenario index is already complete");
        return;
    }

    if (get_pcvar_num(g_CvarAutostart))
    {
        server_print("[ez_http_mapchange_test] Starting suite at scenario %d (%s)",
            g_StateScenarioIndex,
            g_ScenarioNames[MapChangeScenarioId:g_StateScenarioIndex]
        );
        set_task(2.0, "start_current_scenario", TASK_START_SCENARIO);
    }
    else
    {
        server_print("[ez_http_mapchange_test] Autostart disabled. Use server command 'ezhttp_mapchange_test_start'.");
    }
}

public plugin_end()
{
    log_amx("[ez_http_mapchange_test] plugin_end phase=%s scenario=%s", g_StatePhase, g_StateScenarioName);
}

public cmd_reset_state()
{
    remove_runtime_tasks();
    reset_state();
    save_state();

    server_print("[ez_http_mapchange_test] State reset. File: %s", STATE_FILE_PATH);
    log_amx("[ez_http_mapchange_test] state reset");
}

public cmd_start_suite()
{
    remove_runtime_tasks();
    reset_state();
    save_state();

    server_print("[ez_http_mapchange_test] Starting suite from the first scenario.");
    log_amx("[ez_http_mapchange_test] suite started manually");

    set_task(1.0, "start_current_scenario", TASK_START_SCENARIO);
}

public start_current_scenario()
{
    if (g_StateScenarioIndex >= _:Scenario_Count)
    {
        finish_suite("all scenarios already processed");
        return;
    }

    new MapChangeScenarioId:scenario = MapChangeScenarioId:g_StateScenarioIndex;
    new EzHttpOptions:opt = ezhttp_create_options();
    new EzHttpQueue:queue_id = EzHttpQueue:0;
    new url[256];
    new data[3];
    new request_count = get_request_count_for_scenario(scenario);
    new request_delay = get_pcvar_num(g_CvarRequestDelay);
    new repeat_total = get_scenario_repeat_count();

    begin_scenario_state(scenario);

    ezhttp_option_set_plugin_end_behaviour(opt, g_ScenarioBehaviours[scenario]);
    ezhttp_option_set_timeout(opt, (request_delay + REQUEST_TIMEOUT_BUFFER_SEC) * 1000);
    ezhttp_option_set_connect_timeout(opt, 5000);

    if (g_ScenarioUsesQueue[scenario])
    {
        queue_id = ezhttp_create_queue();
        ezhttp_option_set_queue(opt, queue_id);
    }

    for (new i = 0; i < request_count; ++i)
    {
        build_inflight_url(url, charsmax(url), scenario, i);

        data[0] = CALLBACK_KIND_INFLIGHT;
        data[1] = _:scenario;
        data[2] = i;

        new EzHttpRequest:request_id = ezhttp_get(
            url,
            "mapchange_request_complete",
            opt,
            data,
            sizeof(data)
        );

        if (request_id != EzHttpRequest:0)
        {
            ++g_StateStartedRequests;
        }
        else
        {
            ++g_StateDispatchFailures;
            set_last_error_fmt("Dispatch failed for request slot %d in scenario %s", i, g_ScenarioNames[scenario]);
        }
    }

    save_state();

    server_print(
        "[ez_http_mapchange_test] Scenario %s repeat %d/%d armed: started=%d failed_dispatch=%d queue=%d behaviour=%d",
        g_StateScenarioName,
        g_StateScenarioRepeat + 1,
        repeat_total,
        g_StateStartedRequests,
        g_StateDispatchFailures,
        _:queue_id,
        _:g_ScenarioBehaviours[scenario]
    );
    log_amx(
        "[ez_http_mapchange_test] scenario=%s repeat=%d/%d started=%d dispatch_failures=%d use_queue=%d behaviour=%d",
        g_StateScenarioName,
        g_StateScenarioRepeat + 1,
        repeat_total,
        g_StateStartedRequests,
        g_StateDispatchFailures,
        g_ScenarioUsesQueue[scenario],
        _:g_ScenarioBehaviours[scenario]
    );

    if (g_StateStartedRequests == 0)
    {
        finalize_current_scenario(false, "no requests were dispatched");
        return;
    }

    set_task(get_pcvar_float(g_CvarChangeAfter), "perform_map_change", TASK_PERFORM_CHANGELEVEL);
}

public perform_map_change()
{
    if (!equal(g_StatePhase, PHASE_VERIFY_WAIT))
        return;

    resolve_target_map(g_StateTargetMap, charsmax(g_StateTargetMap));
    save_state();

    server_print(
        "[ez_http_mapchange_test] Changing map from %s to %s during scenario %s",
        g_StateCurrentMap,
        g_StateTargetMap,
        g_StateScenarioName
    );
    log_amx(
        "[ez_http_mapchange_test] changelevel current=%s target=%s scenario=%s",
        g_StateCurrentMap,
        g_StateTargetMap,
        g_StateScenarioName
    );

    /*
     * Avoid forcing the map change synchronously from inside the current AMXX public.
     * Let the function unwind first, then queue the actual changelevel command on a
     * short follow-up task so AMXX does not keep stale public metadata on the stack.
     */
    set_task(0.1, "execute_changelevel_command", TASK_EXECUTE_CHANGELEVEL);
}

public execute_changelevel_command()
{
    if (!equal(g_StatePhase, PHASE_VERIFY_WAIT))
        return;

    server_cmd("changelevel %s", g_StateTargetMap);
}

public begin_post_map_verification()
{
    if (g_StateScenarioIndex >= _:Scenario_Count)
    {
        finish_suite("verification resumed with no scenarios left");
        return;
    }

    if (!equal(g_StatePhase, PHASE_VERIFY_WAIT) && !equal(g_StatePhase, PHASE_PROBE_PENDING))
        return;

    new MapChangeScenarioId:scenario = MapChangeScenarioId:g_StateScenarioIndex;
    new EzHttpOptions:opt = ezhttp_create_options();
    new url[256];
    new data[3];
    new probe_count = get_probe_request_count();

    g_RuntimeProbeExpected = 0;
    g_RuntimeProbeCompleted = 0;
    g_RuntimeProbeFailures = 0;
    g_RuntimeProbeDispatchFailures = 0;

    ezhttp_option_set_timeout(opt, 10000);
    ezhttp_option_set_connect_timeout(opt, 5000);

    copy(g_StatePhase, charsmax(g_StatePhase), PHASE_PROBE_PENDING);
    save_state();

    for (new i = 0; i < probe_count; ++i)
    {
        build_probe_url(url, charsmax(url), scenario, i);

        data[0] = CALLBACK_KIND_PROBE;
        data[1] = _:scenario;
        data[2] = i;

        new EzHttpRequest:request_id = ezhttp_get(
            url,
            "mapchange_request_complete",
            opt,
            data,
            sizeof(data)
        );

        if (request_id == EzHttpRequest:0)
        {
            ++g_RuntimeProbeDispatchFailures;
            set_last_error_fmt(
                "Probe dispatch failed after map change for scenario %s slot=%d",
                g_ScenarioNames[scenario],
                i
            );
            continue;
        }

        ++g_RuntimeProbeExpected;

        server_print(
            "[ez_http_mapchange_test] Probe %d/%d dispatched for scenario %s",
            i + 1,
            probe_count,
            g_StateScenarioName
        );
        log_amx(
            "[ez_http_mapchange_test] probe dispatched scenario=%s repeat=%d/%d slot=%d request_id=%d",
            g_StateScenarioName,
            g_StateScenarioRepeat + 1,
            get_scenario_repeat_count(),
            i,
            _:request_id
        );
    }

    if (g_RuntimeProbeExpected == 0)
    {
        finalize_current_scenario(false, "probe request dispatch failed after map change");
        return;
    }
}

public mapchange_request_complete(EzHttpRequest:request_id, const data[])
{
    new callback_kind = data[0];
    new callback_scenario = data[1];
    new callback_slot = data[2];

    if (callback_kind == CALLBACK_KIND_INFLIGHT)
    {
        ++g_StateUnexpectedCallbacks;
        set_last_error_fmt(
            "Unexpected in-flight callback in phase=%s scenario=%s callback_scenario=%d slot=%d err=%d",
            g_StatePhase,
            g_StateScenarioName,
            callback_scenario,
            callback_slot,
            _:ezhttp_get_error_code(request_id)
        );
        save_state();

        server_print(
            "[ez_http_mapchange_test] Unexpected in-flight callback: scenario=%s callback_scenario=%d slot=%d error=%d",
            g_StateScenarioName,
            callback_scenario,
            callback_slot,
            _:ezhttp_get_error_code(request_id)
        );
        log_amx(
            "[ez_http_mapchange_test] unexpected callback phase=%s scenario=%s callback_scenario=%d slot=%d error=%d",
            g_StatePhase,
            g_StateScenarioName,
            callback_scenario,
            callback_slot,
            _:ezhttp_get_error_code(request_id)
        );
        return;
    }

    if (callback_kind != CALLBACK_KIND_PROBE)
    {
        set_last_error_fmt("Unknown callback kind %d", callback_kind);
        finalize_current_scenario(false, "unknown callback kind received");
        return;
    }

    if (callback_scenario != g_StateScenarioIndex)
    {
        set_last_error_fmt(
            "Probe callback scenario mismatch: current=%d callback=%d",
            g_StateScenarioIndex,
            callback_scenario
        );
        finalize_current_scenario(false, "probe callback scenario mismatch");
        return;
    }

    if (!verify_probe_response(request_id, MapChangeScenarioId:callback_scenario, callback_slot))
    {
        ++g_RuntimeProbeFailures;
    }

    ++g_RuntimeProbeCompleted;
    g_StateProbeOk =
        g_RuntimeProbeCompleted == g_RuntimeProbeExpected &&
        g_RuntimeProbeFailures == 0 &&
        g_RuntimeProbeDispatchFailures == 0;

    if (g_RuntimeProbeCompleted < g_RuntimeProbeExpected)
        return;

    finalize_current_scenario(
        g_StateProbeOk,
        g_StateProbeOk ? "all post-map probes succeeded" : "one or more post-map probes failed"
    );
}

stock begin_scenario_state(MapChangeScenarioId:scenario)
{
    get_mapname(g_StateCurrentMap, charsmax(g_StateCurrentMap));
    resolve_target_map(g_StateTargetMap, charsmax(g_StateTargetMap));

    copy(g_StatePhase, charsmax(g_StatePhase), PHASE_VERIFY_WAIT);
    copy(g_StateScenarioName, charsmax(g_StateScenarioName), g_ScenarioNames[scenario]);
    copy(g_StateLastResult, charsmax(g_StateLastResult), "running");
    g_StateLastError[0] = '^0';
    g_StateStartedRequests = 0;
    g_StateDispatchFailures = 0;
    g_StateUnexpectedCallbacks = 0;
    g_StateProbeOk = false;
    g_RuntimeProbeExpected = 0;
    g_RuntimeProbeCompleted = 0;
    g_RuntimeProbeFailures = 0;
    g_RuntimeProbeDispatchFailures = 0;

    save_state();
}

stock bool:verify_probe_response(EzHttpRequest:request_id, MapChangeScenarioId:scenario, probe_slot)
{
    new EzHttpErrorCode:error_code = ezhttp_get_error_code(request_id);
    if (error_code != EZH_OK)
    {
        set_last_error_fmt(
            "Probe error for scenario %s slot=%d: err=%d",
            g_ScenarioNames[scenario],
            probe_slot,
            _:error_code
        );
        return false;
    }

    new http_code = ezhttp_get_http_code(request_id);
    if (http_code != 204)
    {
        new response_preview[160];
        ezhttp_get_data(request_id, response_preview, charsmax(response_preview));
        trim(response_preview);

        set_last_error_fmt(
            "Probe HTTP code mismatch for scenario %s slot=%d: code=%d body=%s",
            g_ScenarioNames[scenario],
            probe_slot,
            http_code,
            response_preview
        );
        return false;
    }

    g_StateLastError[0] = '^0';
    return true;
}

stock finalize_current_scenario(bool:probe_step_succeeded, const reason[])
{
    new bool:scenario_passed =
        probe_step_succeeded &&
        g_StateDispatchFailures == 0 &&
        g_StateUnexpectedCallbacks == 0 &&
        g_RuntimeProbeDispatchFailures == 0;

    copy(g_StateLastResult, charsmax(g_StateLastResult), scenario_passed ? "passed" : "failed");

    if (!scenario_passed && !strlen(g_StateLastError))
        copy(g_StateLastError, charsmax(g_StateLastError), reason);

    if (scenario_passed)
        ++g_StatePassCount;
    else
        ++g_StateFailCount;

    server_print(
        "[ez_http_mapchange_test] Scenario %s repeat %d/%d %s. started=%d dispatch_failures=%d unexpected_callbacks=%d probe_ok=%d probes=%d/%d probe_failures=%d probe_dispatch_failures=%d",
        g_StateScenarioName,
        g_StateScenarioRepeat + 1,
        get_scenario_repeat_count(),
        scenario_passed ? "PASSED" : "FAILED",
        g_StateStartedRequests,
        g_StateDispatchFailures,
        g_StateUnexpectedCallbacks,
        g_StateProbeOk,
        g_RuntimeProbeCompleted,
        g_RuntimeProbeExpected,
        g_RuntimeProbeFailures,
        g_RuntimeProbeDispatchFailures
    );
    log_amx(
        "[ez_http_mapchange_test] scenario=%s repeat=%d/%d result=%s started=%d dispatch_failures=%d unexpected_callbacks=%d probe_ok=%d probe_completed=%d probe_expected=%d probe_failures=%d probe_dispatch_failures=%d reason=%s error=%s",
        g_StateScenarioName,
        g_StateScenarioRepeat + 1,
        get_scenario_repeat_count(),
        scenario_passed ? "passed" : "failed",
        g_StateStartedRequests,
        g_StateDispatchFailures,
        g_StateUnexpectedCallbacks,
        g_StateProbeOk,
        g_RuntimeProbeCompleted,
        g_RuntimeProbeExpected,
        g_RuntimeProbeFailures,
        g_RuntimeProbeDispatchFailures,
        reason,
        g_StateLastError
    );

    if (g_StateScenarioRepeat + 1 < get_scenario_repeat_count())
    {
        ++g_StateScenarioRepeat;
    }
    else
    {
        g_StateScenarioRepeat = 0;
        ++g_StateScenarioIndex;
    }

    if (g_StateScenarioIndex >= _:Scenario_Count)
    {
        finish_suite(reason);
        return;
    }

    copy(g_StatePhase, charsmax(g_StatePhase), PHASE_IDLE);
    save_state();

    set_task(get_pcvar_float(g_CvarNextScenarioDelay), "start_current_scenario", TASK_NEXT_SCENARIO);
}

stock finish_suite(const reason[])
{
    remove_runtime_tasks();

    copy(g_StatePhase, charsmax(g_StatePhase), PHASE_DONE);
    g_StateTargetMap[0] = '^0';

    if (!strlen(g_StateLastResult))
        copy(g_StateLastResult, charsmax(g_StateLastResult), "done");

    save_state();
    print_suite_summary(reason);
}

stock print_suite_summary(const reason[])
{
    server_print("[ez_http_mapchange_test] Suite finished. pass=%d fail=%d reason=%s state=%s",
        g_StatePassCount,
        g_StateFailCount,
        reason,
        STATE_FILE_PATH
    );
    log_amx(
        "[ez_http_mapchange_test] suite finished pass=%d fail=%d reason=%s state=%s",
        g_StatePassCount,
        g_StateFailCount,
        reason,
        STATE_FILE_PATH
    );
}

stock announce_resume()
{
    server_print(
        "[ez_http_mapchange_test] Resuming after map change: scenario=%s repeat=%d/%d phase=%s pass=%d fail=%d",
        g_StateScenarioName,
        g_StateScenarioRepeat + 1,
        get_scenario_repeat_count(),
        g_StatePhase,
        g_StatePassCount,
        g_StateFailCount
    );
    log_amx(
        "[ez_http_mapchange_test] resume scenario=%s repeat=%d/%d phase=%s pass=%d fail=%d",
        g_StateScenarioName,
        g_StateScenarioRepeat + 1,
        get_scenario_repeat_count(),
        g_StatePhase,
        g_StatePassCount,
        g_StateFailCount
    );
}

stock remove_runtime_tasks()
{
    remove_task(TASK_START_SCENARIO);
    remove_task(TASK_PERFORM_CHANGELEVEL);
    remove_task(TASK_EXECUTE_CHANGELEVEL);
    remove_task(TASK_VERIFY_AFTER_CHANGE);
    remove_task(TASK_NEXT_SCENARIO);
}

stock reset_state()
{
    copy(g_StatePhase, charsmax(g_StatePhase), PHASE_IDLE);
    g_StateScenarioName[0] = '^0';
    g_StateCurrentMap[0] = '^0';
    g_StateTargetMap[0] = '^0';
    copy(g_StateLastResult, charsmax(g_StateLastResult), "not_started");
    g_StateLastError[0] = '^0';
    g_StateScenarioIndex = 0;
    g_StateScenarioRepeat = 0;
    g_StatePassCount = 0;
    g_StateFailCount = 0;
    g_StateStartedRequests = 0;
    g_StateDispatchFailures = 0;
    g_StateUnexpectedCallbacks = 0;
    g_StateProbeOk = false;
    g_RuntimeProbeExpected = 0;
    g_RuntimeProbeCompleted = 0;
    g_RuntimeProbeFailures = 0;
    g_RuntimeProbeDispatchFailures = 0;
}

stock load_state()
{
    reset_state();

    if (!file_exists(STATE_FILE_PATH))
        return;

    new line[256];
    new key[64];
    new value[192];
    new line_no = 0;
    new line_len = 0;

    while (read_file(STATE_FILE_PATH, line_no++, line, charsmax(line), line_len))
    {
        trim(line);

        if (!line[0] || line[0] == ';' || line[0] == '#' || line[0] == '[')
            continue;

        if (!split_key_value(line, key, charsmax(key), value, charsmax(value)))
            continue;

        if (equal(key, "phase"))
            copy(g_StatePhase, charsmax(g_StatePhase), value);
        else if (equal(key, "scenario_name"))
            copy(g_StateScenarioName, charsmax(g_StateScenarioName), value);
        else if (equal(key, "current_map"))
            copy(g_StateCurrentMap, charsmax(g_StateCurrentMap), value);
        else if (equal(key, "target_map"))
            copy(g_StateTargetMap, charsmax(g_StateTargetMap), value);
        else if (equal(key, "last_result"))
            copy(g_StateLastResult, charsmax(g_StateLastResult), value);
        else if (equal(key, "last_error"))
            copy(g_StateLastError, charsmax(g_StateLastError), value);
        else if (equal(key, "scenario_index"))
            g_StateScenarioIndex = str_to_num(value);
        else if (equal(key, "scenario_repeat"))
            g_StateScenarioRepeat = str_to_num(value);
        else if (equal(key, "pass_count"))
            g_StatePassCount = str_to_num(value);
        else if (equal(key, "fail_count"))
            g_StateFailCount = str_to_num(value);
        else if (equal(key, "started_requests"))
            g_StateStartedRequests = str_to_num(value);
        else if (equal(key, "dispatch_failures"))
            g_StateDispatchFailures = str_to_num(value);
        else if (equal(key, "unexpected_callbacks"))
            g_StateUnexpectedCallbacks = str_to_num(value);
        else if (equal(key, "probe_ok"))
            g_StateProbeOk = bool:str_to_num(value);
    }
}

stock save_state()
{
    new line[256];

    delete_file(STATE_FILE_PATH);

    write_file(STATE_FILE_PATH, "[ez_http_mapchange_test]", -1);

    formatex(line, charsmax(line), "phase=%s", g_StatePhase);
    write_file(STATE_FILE_PATH, line, -1);

    formatex(line, charsmax(line), "scenario_index=%d", g_StateScenarioIndex);
    write_file(STATE_FILE_PATH, line, -1);

    formatex(line, charsmax(line), "scenario_repeat=%d", g_StateScenarioRepeat);
    write_file(STATE_FILE_PATH, line, -1);

    formatex(line, charsmax(line), "scenario_name=%s", g_StateScenarioName);
    write_file(STATE_FILE_PATH, line, -1);

    formatex(line, charsmax(line), "current_map=%s", g_StateCurrentMap);
    write_file(STATE_FILE_PATH, line, -1);

    formatex(line, charsmax(line), "target_map=%s", g_StateTargetMap);
    write_file(STATE_FILE_PATH, line, -1);

    formatex(line, charsmax(line), "pass_count=%d", g_StatePassCount);
    write_file(STATE_FILE_PATH, line, -1);

    formatex(line, charsmax(line), "fail_count=%d", g_StateFailCount);
    write_file(STATE_FILE_PATH, line, -1);

    formatex(line, charsmax(line), "started_requests=%d", g_StateStartedRequests);
    write_file(STATE_FILE_PATH, line, -1);

    formatex(line, charsmax(line), "dispatch_failures=%d", g_StateDispatchFailures);
    write_file(STATE_FILE_PATH, line, -1);

    formatex(line, charsmax(line), "unexpected_callbacks=%d", g_StateUnexpectedCallbacks);
    write_file(STATE_FILE_PATH, line, -1);

    formatex(line, charsmax(line), "probe_ok=%d", _:g_StateProbeOk);
    write_file(STATE_FILE_PATH, line, -1);

    formatex(line, charsmax(line), "last_result=%s", g_StateLastResult);
    write_file(STATE_FILE_PATH, line, -1);

    formatex(line, charsmax(line), "last_error=%s", g_StateLastError);
    write_file(STATE_FILE_PATH, line, -1);
}

stock bool:split_key_value(line[], key[], key_len, value[], value_len)
{
    new pos = contain(line, "=");
    if (pos < 0)
        return false;

    line[pos] = '^0';

    copy(key, key_len, line);
    copy(value, value_len, line[pos + 1]);

    trim(key);
    trim(value);

    return true;
}

stock build_inflight_url(url[], max_len, MapChangeScenarioId:scenario, request_slot)
{
    new base_url[128];
    get_base_url(base_url, charsmax(base_url));

    formatex(
        url,
        max_len,
        "%s/delay/%d?scenario=%s&request=%d",
        base_url,
        get_pcvar_num(g_CvarRequestDelay),
        g_ScenarioNames[scenario],
        request_slot
    );
}

stock build_probe_url(url[], max_len, MapChangeScenarioId:scenario, probe_slot)
{
    new base_url[128];
    get_base_url(base_url, charsmax(base_url));

    formatex(
        url,
        max_len,
        "%s/status/204?probe=%s&slot=%d",
        base_url,
        g_ScenarioNames[scenario],
        probe_slot
    );
}

stock get_request_count_for_scenario(MapChangeScenarioId:scenario)
{
    switch (scenario)
    {
        case Scenario_CancelSingleMain, Scenario_ForgetSingleMain:
            return 1;
        case Scenario_CancelStressMain, Scenario_ForgetStressMain:
        {
            new count = get_pcvar_num(g_CvarStressRequests);
            return count > 0 ? count : g_ScenarioRequestCounts[scenario];
        }
        case Scenario_CancelSequentialQueue, Scenario_ForgetSequentialQueue:
        {
            new count = get_pcvar_num(g_CvarQueueRequests);
            return count > 0 ? count : g_ScenarioRequestCounts[scenario];
        }
    }

    return g_ScenarioRequestCounts[scenario];
}

stock get_scenario_repeat_count()
{
    new count = get_pcvar_num(g_CvarScenarioRepeats);
    return count > 0 ? count : 1;
}

stock get_probe_request_count()
{
    new count = get_pcvar_num(g_CvarProbeRequests);
    return count > 0 ? count : 1;
}

stock get_base_url(buffer[], max_len)
{
    get_pcvar_string(g_CvarBaseUrl, buffer, max_len);
    trim(buffer);

    new len = strlen(buffer);
    while (len > 0 && buffer[len - 1] == '/')
    {
        buffer[--len] = '^0';
    }
}

stock resolve_target_map(buffer[], max_len)
{
    get_pcvar_string(g_CvarTargetMap, buffer, max_len);
    trim(buffer);

    if (!strlen(buffer))
        get_mapname(buffer, max_len);
}

stock set_last_error_fmt(const fmt[], any:...)
{
    vformat(g_StateLastError, charsmax(g_StateLastError), fmt, 2);
}

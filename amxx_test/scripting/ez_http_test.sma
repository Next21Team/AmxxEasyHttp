#include <amxmodx>
#include <utest>
#include <easy_http>

#pragma semicolon 1

#define EZHTTP_OPTION_SET_TEST_DATA(%1)\
    ezhttp_option_set_user_data(%1, __test, sizeof(__test));

#define EZHTTP_OPTION_EXTRACT_TEST_DATA(%1)\
    new __test[_TestT];\
    ezhttp_get_user_data(%1, __test);


TEST_LIST_ASYNC = {
    { "test_get_parameters",    "test parameters in get request" },
    { "test_post_form",         "test post form" },
    { "test_post_body",         "test post body plain/text" },
    { "test_post_body_json",    "test post body json" },
    { "test_user_agent",        "test user agent" },
    { "test_headers",           "test headers" },
    { "test_cookies",           "test cookies" },
    { "test_auth",              "test auth" },
    { "test_save_to_file",      "test save to file" },
    { "test_fail_by_timeout",   "test timeout" },
    { "test_options_reuse_concurrent", "test reusing the same options in concurrent requests" },
    { "test_user_data_snapshot", "test request keeps user data snapshot from dispatch time" },
    { "test_destroy_options_after_dispatch", "test options can be destroyed after dispatch" },
    { "test_ftp_download",      "test ftp download" },
    { "test_ftp_download_wildcard", "test ftp download wildcard" },
    { "test_ftp_returns_request_id", "test ftp natives return a valid request id" },
    { "test_ftp_upload",        "test ftp upload" },
    { "test_ftp_upload2",       "test ftp upload by uri with special chars in credentials" },
    TEST_LIST_END
};

#define FTP_WILDCARD_DOWNLOAD_DIR "ezhttp_test_ftp_wildcard"
#define FTP_WILDCARD_FILE_1 "ezhttp_test_ftp_wildcard/hello-2.12.tar.gz.sig"
#define FTP_WILDCARD_FILE_2 "ezhttp_test_ftp_wildcard/hello-2.12.1.tar.gz.sig"
#define FTP_WILDCARD_FILE_3 "ezhttp_test_ftp_wildcard/hello-2.12.2.tar.gz.sig"
#define FTP_RETURN_ID_FILE "ezhttp_test_ftp_download_return_id.txt"

new g_CvarEzHttpTestAutostart;
new g_CvarEzHttpTestBaseUrl;

new g_TestOptionsReuseConcurrent[_TestT];
new g_TestOptionsReuseConcurrentCompleted = 0;
new bool:g_TestOptionsReuseConcurrentFinished = false;

new g_TestUserDataSnapshot[_TestT];
new g_TestUserDataSnapshotCompleted = 0;
new bool:g_TestUserDataSnapshotFinished = false;

new bool:g_TestDestroyOptionsAfterDispatchDestroyed = false;
new EzHttpRequest:g_TestDestroyOptionsAfterDispatchRequestId = EzHttpRequest:0;

new bool:g_TestFtpReturnsRequestIdExistsAtDispatch = false;
new EzHttpRequest:g_TestFtpReturnsRequestIdRequestId = EzHttpRequest:0;

stock copy_test_state(dest[_TestT], const source[_TestT])
{
    for (new i = 0; i < _TestT; ++i)
        dest[i] = source[i];
}

stock finish_async_aggregate_test(test[_TestT])
{
    utest_run_async_one_test_end(test, g_utlist_async);
    utest_run_async_one_test_begin(g_utlist_async);
}

stock bool:prepare_json_response(test[_TestT], EzHttpRequest:request_id, &EzJSON:json_root, line, const fail_message[] = "request must succeed")
{
    json_root = EzInvalid_JSON;

    new EzHttpErrorCode:error_code = ezhttp_get_error_code(request_id);
    _test_assert(test, error_code == EZH_OK, line, fail_message);
    if (error_code != EZH_OK)
        return false;

    json_root = ezhttp_parse_json_response(request_id);
    if (json_root == EzInvalid_JSON)
        log_invalid_json_response(request_id, line);

    _test_assert(test, json_root != EzInvalid_JSON, line, "response must be valid JSON");

    return json_root != EzInvalid_JSON;
}

stock log_invalid_json_response(EzHttpRequest:request_id, line)
{
    new http_code = ezhttp_get_http_code(request_id);
    new content_type[96];
    new response_preview[192];

    if (!ezhttp_get_headers(request_id, "Content-Type", content_type, charsmax(content_type)))
        copy(content_type, charsmax(content_type), "<missing>");

    ezhttp_get_data(request_id, response_preview, charsmax(response_preview));
    trim(response_preview);

    log_amx(
        "[ez_http_test] invalid json at line=%d http_code=%d content_type=%s body=%s",
        line,
        http_code,
        content_type,
        response_preview
    );
    server_print(
        "[ez_http_test] invalid json at line=%d http_code=%d content_type=%s body=%s",
        line,
        http_code,
        content_type,
        response_preview
    );
}

stock build_test_url(url[], max_len, const path[])
{
    get_pcvar_string(g_CvarEzHttpTestBaseUrl, url, max_len);
    trim(url);

    new len = strlen(url);
    while (len > 0 && url[len - 1] == '/')
    {
        url[--len] = '^0';
    }

    formatex(url[len], max_len - len, "%s", path);
}

stock cleanup_ftp_wildcard_download_dir()
{
    delete_file(FTP_WILDCARD_FILE_1);
    delete_file(FTP_WILDCARD_FILE_2);
    delete_file(FTP_WILDCARD_FILE_3);
    rmdir(FTP_WILDCARD_DOWNLOAD_DIR);
}

public plugin_init()
{
    register_plugin("EasyHttp Test", "Polarhigh", "1.2");

    g_CvarEzHttpTestAutostart = register_cvar("ezhttp_test_autostart", "1");
    g_CvarEzHttpTestBaseUrl = register_cvar("ezhttp_test_base_url", "https://httpbin.org");

    if (get_pcvar_num(g_CvarEzHttpTestAutostart))
        set_task(2.0, "run_tests");
}

public plugin_end()
{
    log_amx("[ez_http_test] plugin_end");
}

public run_tests()
{
    UTEST_RUN_ASYNC(UT_VERBOSE)
}

START_ASYNC_TEST(test_get_parameters) 
{
    new EzHttpOptions:opt = ezhttp_create_options();
    new url[256];

    ezhttp_option_add_url_parameter(opt, "MyParam1", "ParamVal1");
    ezhttp_option_add_url_parameter(opt, "MyParam2", "ParamVal2");
    
    EZHTTP_OPTION_SET_TEST_DATA(opt)

    build_test_url(url, charsmax(url), "/get");
    ezhttp_get(url, "test_get_parameters_complete", opt);
}

public test_get_parameters_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    new tmp_data[256];

    new EzJSON:json_root;
    if (!prepare_json_response(__test, request_id, json_root, __LINE__))
    {
        END_ASYNC_TEST()
        return;
    }

    new EzJSON:json_args = ezjson_object_get_value(json_root, "args");
    
    server_print("test_get_parameters request elapsed: %f", ezhttp_get_elapsed(request_id));

    // asserts

    ASSERT_TRUE_MSG(ezjson_object_get_count(json_args) == 2, "expected 2 arguments");

    ezjson_object_get_string(json_args, "MyParam1", tmp_data, charsmax(tmp_data));
    ASSERT_TRUE(equal(tmp_data, "ParamVal1"));

    ezjson_object_get_string(json_args, "MyParam2", tmp_data, charsmax(tmp_data));
    ASSERT_TRUE(equal(tmp_data, "ParamVal2"));

    //

    ezjson_free(json_args);
    ezjson_free(json_root);

    END_ASYNC_TEST()
}

START_ASYNC_TEST(test_fail_by_timeout)
{
    new EzHttpOptions:opt = ezhttp_create_options();
    new url[256];
    ezhttp_option_set_timeout(opt, 2 * 1000);

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    build_test_url(url, charsmax(url), "/delay/5");
    ezhttp_get(url, "test_get_fail_by_timeout_complete", opt);
}

public test_get_fail_by_timeout_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    server_print("test_fail_by_timeout request elapsed: %f", ezhttp_get_elapsed(request_id));

    // asserts

    ASSERT_TRUE_MSG(ezhttp_get_error_code(request_id) == EZH_OPERATION_TIMEDOUT, "operation must be timeouted");

    //

    END_ASYNC_TEST()
}

START_ASYNC_TEST(test_options_reuse_concurrent)
{
    copy_test_state(g_TestOptionsReuseConcurrent, __test);
    g_TestOptionsReuseConcurrentCompleted = 0;
    g_TestOptionsReuseConcurrentFinished = false;

    new EzHttpOptions:opt = ezhttp_create_options();
    ezhttp_option_set_header(opt, "X-Reused-Option", "shared-header");

    new data_a[1] = { 1 };
    new data_b[1] = { 2 };
    new url_a[256];
    new url_b[256];

    build_test_url(url_a, charsmax(url_a), "/delay/1?request=reuse_a");
    build_test_url(url_b, charsmax(url_b), "/delay/1?request=reuse_b");

    new EzHttpRequest:request_a = ezhttp_get(
        url_a,
        "test_options_reuse_concurrent_complete",
        opt,
        data_a,
        sizeof(data_a)
    );
    new EzHttpRequest:request_b = ezhttp_get(
        url_b,
        "test_options_reuse_concurrent_complete",
        opt,
        data_b,
        sizeof(data_b)
    );

    _test_assert(g_TestOptionsReuseConcurrent, request_a != EzHttpRequest:0, __LINE__, "first request id must not be zero");
    _test_assert(g_TestOptionsReuseConcurrent, request_b != EzHttpRequest:0, __LINE__, "second request id must not be zero");

    if (request_a == EzHttpRequest:0 || request_b == EzHttpRequest:0)
    {
        g_TestOptionsReuseConcurrentFinished = true;
        finish_async_aggregate_test(g_TestOptionsReuseConcurrent);
    }
}

public test_options_reuse_concurrent_complete(EzHttpRequest:request_id, const data[])
{
    if (g_TestOptionsReuseConcurrentFinished)
        return;

    new EzJSON:json_root;
    if (!prepare_json_response(g_TestOptionsReuseConcurrent, request_id, json_root, __LINE__))
    {
        if (++g_TestOptionsReuseConcurrentCompleted == 2 && !g_TestOptionsReuseConcurrentFinished)
        {
            g_TestOptionsReuseConcurrentFinished = true;
            finish_async_aggregate_test(g_TestOptionsReuseConcurrent);
        }
        return;
    }

    new EzJSON:json_args = ezjson_object_get_value(json_root, "args");
    new EzJSON:json_headers = ezjson_object_get_value(json_root, "headers");

    new request_name[64];
    new header_value[64];
    ezjson_object_get_string(json_args, "request", request_name, charsmax(request_name));
    ezjson_object_get_string(json_headers, "X-Reused-Option", header_value, charsmax(header_value));

    _test_assert(g_TestOptionsReuseConcurrent, equal(header_value, "shared-header"), __LINE__, "reused options header was not preserved");

    switch (data[0])
    {
        case 1: _test_assert(g_TestOptionsReuseConcurrent, equal(request_name, "reuse_a"), __LINE__, "first callback returned unexpected request marker");
        case 2: _test_assert(g_TestOptionsReuseConcurrent, equal(request_name, "reuse_b"), __LINE__, "second callback returned unexpected request marker");
        default: _test_assert(g_TestOptionsReuseConcurrent, false, __LINE__, "unexpected callback payload");
    }

    ezjson_free(json_headers);
    ezjson_free(json_args);
    ezjson_free(json_root);

    if (++g_TestOptionsReuseConcurrentCompleted == 2 && !g_TestOptionsReuseConcurrentFinished)
    {
        g_TestOptionsReuseConcurrentFinished = true;
        finish_async_aggregate_test(g_TestOptionsReuseConcurrent);
    }
}

START_ASYNC_TEST(test_user_data_snapshot)
{
    copy_test_state(g_TestUserDataSnapshot, __test);
    g_TestUserDataSnapshotCompleted = 0;
    g_TestUserDataSnapshotFinished = false;

    new EzHttpOptions:opt = ezhttp_create_options();
    new url_a[256];
    new url_b[256];

    new user_data_a[1] = { 111 };
    new request_data_a[1] = { 1 };
    ezhttp_option_set_user_data(opt, user_data_a, sizeof(user_data_a));
    build_test_url(url_a, charsmax(url_a), "/delay/1?request=user_data_a");

    new EzHttpRequest:request_a = ezhttp_get(
        url_a,
        "test_user_data_snapshot_complete",
        opt,
        request_data_a,
        sizeof(request_data_a)
    );

    new user_data_b[1] = { 222 };
    new request_data_b[1] = { 2 };
    ezhttp_option_set_user_data(opt, user_data_b, sizeof(user_data_b));
    build_test_url(url_b, charsmax(url_b), "/delay/1?request=user_data_b");

    new EzHttpRequest:request_b = ezhttp_get(
        url_b,
        "test_user_data_snapshot_complete",
        opt,
        request_data_b,
        sizeof(request_data_b)
    );

    _test_assert(g_TestUserDataSnapshot, request_a != EzHttpRequest:0, __LINE__, "first request id must not be zero");
    _test_assert(g_TestUserDataSnapshot, request_b != EzHttpRequest:0, __LINE__, "second request id must not be zero");

    if (request_a == EzHttpRequest:0 || request_b == EzHttpRequest:0)
    {
        g_TestUserDataSnapshotFinished = true;
        finish_async_aggregate_test(g_TestUserDataSnapshot);
    }
}

public test_user_data_snapshot_complete(EzHttpRequest:request_id, const data[])
{
    if (g_TestUserDataSnapshotFinished)
        return;

    _test_assert(g_TestUserDataSnapshot, ezhttp_get_error_code(request_id) == EZH_OK, __LINE__, "request must succeed");

    new request_user_data[1];
    ezhttp_get_user_data(request_id, request_user_data);

    switch (data[0])
    {
        case 1: _test_assert(g_TestUserDataSnapshot, request_user_data[0] == 111, __LINE__, "first request lost its original user data snapshot");
        case 2: _test_assert(g_TestUserDataSnapshot, request_user_data[0] == 222, __LINE__, "second request did not receive updated user data snapshot");
        default: _test_assert(g_TestUserDataSnapshot, false, __LINE__, "unexpected callback payload");
    }

    if (++g_TestUserDataSnapshotCompleted == 2 && !g_TestUserDataSnapshotFinished)
    {
        g_TestUserDataSnapshotFinished = true;
        finish_async_aggregate_test(g_TestUserDataSnapshot);
    }
}

START_ASYNC_TEST(test_post_form)
{
    new EzHttpOptions:opt = ezhttp_create_options();
    new url[256];

    ezhttp_option_add_form_payload(opt, "MyFormEntry1", "FormVal1");
    ezhttp_option_add_form_payload(opt, "MyFormEntry2", "FormVal2");

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    build_test_url(url, charsmax(url), "/post");
    ezhttp_post(url, "test_post_form_complete", opt);
}

public test_post_form_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    new EzJSON:json_root;
    if (!prepare_json_response(__test, request_id, json_root, __LINE__))
    {
        END_ASYNC_TEST()
        return;
    }

    new EzJSON:json_form = ezjson_object_get_value(json_root, "form");

    server_print("test_post_form request elapsed: %f", ezhttp_get_elapsed(request_id));

    // asserts

    new tmp_data[256];

    ASSERT_TRUE(ezjson_object_get_count(json_form) == 2);
    
    ezjson_object_get_string(json_form, "MyFormEntry1", tmp_data, charsmax(tmp_data));
    ASSERT_TRUE(equal(tmp_data, "FormVal1"));

    ezjson_object_get_string(json_form, "MyFormEntry2", tmp_data, charsmax(tmp_data));
    ASSERT_TRUE(equal(tmp_data, "FormVal2"));

    //

    ezjson_free(json_form);
    ezjson_free(json_root);

    END_ASYNC_TEST()
}

START_ASYNC_TEST(test_post_body)
{
    new EzHttpOptions:opt = ezhttp_create_options();
    new url[256];

    ezhttp_option_set_body(opt, "MyBody");
    ezhttp_option_set_header(opt, "Content-Type", "text/plain");

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    build_test_url(url, charsmax(url), "/post");
    ezhttp_post(url, "test_post_body_complete", opt);
}

public test_post_body_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    server_print("test_post_body request elapsed: %f", ezhttp_get_elapsed(request_id));

    new EzJSON:json_root;
    if (!prepare_json_response(__test, request_id, json_root, __LINE__))
    {
        END_ASYNC_TEST()
        return;
    }

    // asserts

    new body_data[256];
    ezjson_object_get_string(json_root, "data", body_data, charsmax(body_data));
    ASSERT_TRUE(equal(body_data, "MyBody"));

    //

    ezjson_free(json_root);

    END_ASYNC_TEST()
}

START_ASYNC_TEST(test_post_body_json)
{
    new EzHttpOptions:opt = ezhttp_create_options();
    new url[256];

    new EzJSON:json_root = ezjson_init_object();
    ezjson_object_set_string(json_root, "StringField", "TestValue");
    ezjson_object_set_number(json_root, "NumberField", 21);

    ezhttp_option_set_body_from_json(opt, json_root);
    ezjson_free(json_root);
    ezhttp_option_set_header(opt, "Content-Type", "application/json");

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    build_test_url(url, charsmax(url), "/anything");
    ezhttp_post(url, "test_post_body_json_complete", opt);
}

public test_post_body_json_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    new EzJSON:json_root;
    if (!prepare_json_response(__test, request_id, json_root, __LINE__))
    {
        END_ASYNC_TEST()
        return;
    }

    new EzJSON:json_data = ezjson_object_get_value(json_root, "json");

    server_print("test_post_body_json request elapsed: %f", ezhttp_get_elapsed(request_id));

    // asserts
    new tmp_data[256];

    ezjson_object_get_string(json_data, "StringField", tmp_data, charsmax(tmp_data));
    ASSERT_TRUE(equal(tmp_data, "TestValue"));

    ASSERT_TRUE(ezjson_object_get_number(json_data, "NumberField") == 21);
    //

    ezjson_free(json_data);
    ezjson_free(json_root);

    END_ASYNC_TEST()
}

START_ASYNC_TEST(test_user_agent)
{
    new EzHttpOptions:opt = ezhttp_create_options();
    new url[256];
    ezhttp_option_set_user_agent(opt, "Easy HTTP User-Agent");

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    build_test_url(url, charsmax(url), "/post");
    ezhttp_post(url, "test_user_agent_complete", opt);
}

public test_user_agent_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    new EzJSON:json_root;
    if (!prepare_json_response(__test, request_id, json_root, __LINE__))
    {
        END_ASYNC_TEST()
        return;
    }

    new EzJSON:json_headers = ezjson_object_get_value(json_root, "headers");

    server_print("test_user_agent request elapsed: %f", ezhttp_get_elapsed(request_id));

    // asserts

    new user_agent[256];
    ezjson_object_get_string(json_headers, "User-Agent", user_agent, charsmax(user_agent));
    ASSERT_TRUE(equal(user_agent, "Easy HTTP User-Agent"));

    //

    ezjson_free(json_headers);
    ezjson_free(json_root);

    END_ASYNC_TEST()
}

START_ASYNC_TEST(test_headers)
{
    new EzHttpOptions:opt = ezhttp_create_options();
    new url[256];
    ezhttp_option_set_header(opt, "Myheader1", "HeaderVal1");
    ezhttp_option_set_header(opt, "Myheader2", "HeaderVal2");

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    build_test_url(url, charsmax(url), "/post");
    ezhttp_post(url, "test_headers_complete", opt);
}

public test_headers_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    new EzJSON:json_root;
    if (!prepare_json_response(__test, request_id, json_root, __LINE__))
    {
        END_ASYNC_TEST()
        return;
    }

    new EzJSON:json_headers = ezjson_object_get_value(json_root, "headers");

    server_print("test_headers request elapsed: %f", ezhttp_get_elapsed(request_id));

    // asserts

    new tmp_data[256];

    ezjson_object_get_string(json_headers, "Myheader1", tmp_data, charsmax(tmp_data));
    ASSERT_TRUE(equal(tmp_data, "HeaderVal1"));

    ezjson_object_get_string(json_headers, "Myheader2", tmp_data, charsmax(tmp_data));
    ASSERT_TRUE(equal(tmp_data, "HeaderVal2"));

    //

    ezjson_free(json_headers);
    ezjson_free(json_root);

    END_ASYNC_TEST()
}

START_ASYNC_TEST(test_cookies)
{
    new EzHttpOptions:opt = ezhttp_create_options();
    new url[256];
    ezhttp_option_set_cookie(opt, "Mycookie1", "CookieVal1");
    ezhttp_option_set_cookie(opt, "Mycookie2", "CookieVal20");
    ezhttp_option_set_cookie(opt, "Mycookie2", "CookieVal2");

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    build_test_url(url, charsmax(url), "/cookies");
    ezhttp_get(url, "test_cookies_complete", opt);
}

public test_cookies_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    new EzJSON:json_root;
    if (!prepare_json_response(__test, request_id, json_root, __LINE__))
    {
        END_ASYNC_TEST()
        return;
    }

    new EzJSON:json_cookies = ezjson_object_get_value(json_root, "cookies");

    server_print("test_cookies request elapsed: %f", ezhttp_get_elapsed(request_id));

    // asserts

    new tmp_data[256];

    ezjson_object_get_string(json_cookies, "Mycookie1", tmp_data, charsmax(tmp_data));
    ASSERT_TRUE(equal(tmp_data, "CookieVal1"));

    ezjson_object_get_string(json_cookies, "Mycookie2", tmp_data, charsmax(tmp_data));
    ASSERT_TRUE(equal(tmp_data, "CookieVal2"));

    //

    ezjson_free(json_cookies);
    ezjson_free(json_root);

    END_ASYNC_TEST()
}

START_ASYNC_TEST(test_save_to_file)
{
    new EzHttpOptions:opt = ezhttp_create_options();
    new url[256];

    ezhttp_option_set_body(opt, "MyBody");
    ezhttp_option_set_header(opt, "Content-Type", "text/plain");

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    build_test_url(url, charsmax(url), "/robots.txt");
    ezhttp_get(url, "test_save_to_file_complete", opt);
}

public test_save_to_file_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    ezhttp_save_data_to_file(request_id, "addons/amxmodx/test_save.txt");

    server_print("test_save_to_file request elapsed: %f", ezhttp_get_elapsed(request_id));

    // asserts

    new text[128];

    read_file("addons/amxmodx/test_save.txt", 0, text, charsmax(text));
    ASSERT_TRUE(equal(text, "User-agent: *"));

    read_file("addons/amxmodx/test_save.txt", 1, text, charsmax(text));
    ASSERT_TRUE(equal(text, "Disallow: /deny"));

    //

    unlink("addons/amxmodx/test_save.txt");

    END_ASYNC_TEST()
}

START_ASYNC_TEST(test_auth)
{
    new EzHttpOptions:opt = ezhttp_create_options();
    new url[256];
    ezhttp_option_set_auth(opt, "user1", "pswd1");

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    build_test_url(url, charsmax(url), "/basic-auth/user1/pswd1");
    ezhttp_get(url, "test_auth_complete", opt);
}

public test_auth_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    server_print("test_auth_complete request elapsed: %f", ezhttp_get_elapsed(request_id));

    new EzJSON:json_root;
    if (!prepare_json_response(__test, request_id, json_root, __LINE__))
    {
        END_ASYNC_TEST()
        return;
    }

    // asserts

    ASSERT_TRUE(ezjson_object_get_bool(json_root, "authenticated"));

    new user[16];
    ezjson_object_get_string(json_root, "user", user, charsmax(user));
    ASSERT_TRUE(equal(user, "user1"));

    //

    ezjson_free(json_root);

    END_ASYNC_TEST()
}

START_ASYNC_TEST(test_ftp_download)
{
    new EzHttpOptions:opt = ezhttp_create_options();

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    ezhttp_ftp_download2("ftp://demo:password@test.rebex.net/readme.txt", "ezhttp_test_ftp_download_readme.txt", "test_ftp_download_complete", EZH_UNSECURE, opt);
}

public test_ftp_download_complete(EzHttpRequest:request_id)
{
    const expected_file_size = 379;

    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    // asserts

    ASSERT_INT_EQ(EzHttpErrorCode:EZH_OK, ezhttp_get_error_code(request_id));
    new size = filesize("ezhttp_test_ftp_download_readme.txt");
    ASSERT_INT_EQ_MSG(expected_file_size, size, fmt("expected %d but was %d", expected_file_size, size));

    delete_file("ezhttp_test_ftp_download_readme.txt");
    END_ASYNC_TEST()
}

START_ASYNC_TEST(test_destroy_options_after_dispatch)
{
    new EzHttpOptions:opt = ezhttp_create_options();
    new url[256];
    ezhttp_option_set_header(opt, "X-Destroyed-Option", "still-works");

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    build_test_url(url, charsmax(url), "/delay/1?request=destroy_after_dispatch");
    g_TestDestroyOptionsAfterDispatchRequestId = ezhttp_get(
        url,
        "test_destroy_options_after_dispatch_complete",
        opt
    );
    g_TestDestroyOptionsAfterDispatchDestroyed = ezhttp_destroy_options(opt);

    ASSERT_INT_NEQ_MSG(EzHttpRequest:0, g_TestDestroyOptionsAfterDispatchRequestId, "request id must not be zero");

    if (g_TestDestroyOptionsAfterDispatchRequestId == EzHttpRequest:0)
    {
        END_ASYNC_TEST()
    }
}

public test_destroy_options_after_dispatch_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    ASSERT_TRUE_MSG(g_TestDestroyOptionsAfterDispatchDestroyed, "options must be destroyable after dispatch");
    ASSERT_INT_EQ(g_TestDestroyOptionsAfterDispatchRequestId, request_id);
    
    new EzJSON:json_root;
    if (!prepare_json_response(__test, request_id, json_root, __LINE__))
    {
        END_ASYNC_TEST()
        return;
    }

    new EzJSON:json_args = ezjson_object_get_value(json_root, "args");
    new EzJSON:json_headers = ezjson_object_get_value(json_root, "headers");

    new request_name[64];
    new header_value[64];
    ezjson_object_get_string(json_args, "request", request_name, charsmax(request_name));
    ezjson_object_get_string(json_headers, "X-Destroyed-Option", header_value, charsmax(header_value));

    ASSERT_STRING_EQ(request_name, "destroy_after_dispatch");
    ASSERT_STRING_EQ(header_value, "still-works");

    ezjson_free(json_headers);
    ezjson_free(json_args);
    ezjson_free(json_root);

    END_ASYNC_TEST()
}

START_ASYNC_TEST(test_ftp_download_wildcard)
{
    new EzHttpOptions:opt = ezhttp_create_options();

    cleanup_ftp_wildcard_download_dir();
    EZHTTP_OPTION_SET_TEST_DATA(opt)

    ezhttp_ftp_download("", "", "ftp.gnu.org", "gnu/hello/hello-2.12*.tar.gz.sig", FTP_WILDCARD_DOWNLOAD_DIR, "test_ftp_download_wildcard_complete", EZH_UNSECURE, opt);
}

public test_ftp_download_wildcard_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    // asserts

    ASSERT_INT_EQ(EzHttpErrorCode:EZH_OK, ezhttp_get_error_code(request_id));
    ASSERT_TRUE_MSG(dir_exists(FTP_WILDCARD_DOWNLOAD_DIR), "wildcard download directory was not created");
    ASSERT_TRUE_MSG(file_exists(FTP_WILDCARD_FILE_1), "expected hello-2.12.tar.gz.sig to be downloaded");
    ASSERT_TRUE_MSG(file_exists(FTP_WILDCARD_FILE_2), "expected hello-2.12.1.tar.gz.sig to be downloaded");
    ASSERT_TRUE_MSG(file_exists(FTP_WILDCARD_FILE_3), "expected hello-2.12.2.tar.gz.sig to be downloaded");

    cleanup_ftp_wildcard_download_dir();
    END_ASYNC_TEST()
}

START_ASYNC_TEST(test_ftp_returns_request_id)
{
    new EzHttpOptions:opt = ezhttp_create_options();

    delete_file(FTP_RETURN_ID_FILE);
    EZHTTP_OPTION_SET_TEST_DATA(opt)

    g_TestFtpReturnsRequestIdRequestId = ezhttp_ftp_download2(
        "ftp://demo:password@test.rebex.net/readme.txt",
        FTP_RETURN_ID_FILE,
        "test_ftp_returns_request_id_complete",
        EZH_UNSECURE,
        opt
    );
    g_TestFtpReturnsRequestIdExistsAtDispatch =
        g_TestFtpReturnsRequestIdRequestId != EzHttpRequest:0 &&
        ezhttp_is_request_exists(g_TestFtpReturnsRequestIdRequestId);

    ASSERT_INT_NEQ_MSG(EzHttpRequest:0, g_TestFtpReturnsRequestIdRequestId, "ftp request id must not be zero");

    if (g_TestFtpReturnsRequestIdRequestId == EzHttpRequest:0)
    {
        END_ASYNC_TEST()
    }
}

public test_ftp_returns_request_id_complete(EzHttpRequest:request_id)
{
    const expected_file_size = 379;

    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    ASSERT_INT_NEQ(EzHttpRequest:0, g_TestFtpReturnsRequestIdRequestId);
    ASSERT_TRUE_MSG(g_TestFtpReturnsRequestIdExistsAtDispatch, "returned ftp request id must exist after dispatch");
    ASSERT_INT_EQ(g_TestFtpReturnsRequestIdRequestId, request_id);
    ASSERT_INT_EQ(EzHttpErrorCode:EZH_OK, ezhttp_get_error_code(request_id));

    new size = filesize(FTP_RETURN_ID_FILE);
    ASSERT_INT_EQ_MSG(expected_file_size, size, fmt("expected %d but was %d", expected_file_size, size));

    delete_file(FTP_RETURN_ID_FILE);
    END_ASYNC_TEST()
}

START_ASYNC_TEST(test_ftp_upload)
{
    new EzHttpOptions:opt = ezhttp_create_options();

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    ezhttp_ftp_upload("anonymous", "test@example.com", "ftp.cs.brown.edu", "incoming/ezhttp_test_cstrike.ico", "cstrike.ico", "test_ftp_upload_complete", EZH_UNSECURE, opt);
}

public test_ftp_upload_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    // asserts

    ASSERT_INT_EQ(EzHttpErrorCode:EZH_OK, ezhttp_get_error_code(request_id));

    END_ASYNC_TEST()
}

START_ASYNC_TEST(test_ftp_upload2)
{
    new EzHttpOptions:opt = ezhttp_create_options();

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    ezhttp_ftp_upload2("ftp://anonymous:test@example.com@ftp.cs.brown.edu/incoming/ezhttp_test_cstrike_uri.ico", "cstrike.ico", "test_ftp_upload2_complete", EZH_UNSECURE, opt);
}

public test_ftp_upload2_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    // asserts

    ASSERT_INT_EQ(EzHttpErrorCode:EZH_OK, ezhttp_get_error_code(request_id));

    END_ASYNC_TEST()
}

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
    { "test_get_parameters", "test parameters in get request" },
    { "test_post_form", "test post form" },
    { "test_post_body", "test post body plain/text" },
    { "test_user_agent", "test user agent" },
    { "test_headers", "test headers" },
    { "test_cookies", "test cookies" },
    { "test_auth", "test auth" },
    { "test_save_to_file", "test save to file" },
    { "test_fail_by_timeout", "test timeout" },
    TEST_LIST_END
};

public plugin_init()
{
    register_plugin("EasyHttp Test", "Polarhigh", "1.1");

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

    ezhttp_option_add_url_parameter(opt, "MyParam1", "ParamVal1");
    ezhttp_option_add_url_parameter(opt, "MyParam2", "ParamVal2");
    
    EZHTTP_OPTION_SET_TEST_DATA(opt)

    ezhttp_get("https://httpbin.org/get", "test_get_parameters_complete", opt);
}

public test_get_parameters_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    new tmp_data[256];

    new EzJSON:json_root = ezhttp_parse_json_response(request_id);
    new EzJSON:json_args = ezjson_object_get_value(json_root, "args");
    
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
    ezhttp_option_set_timeout(opt, 2 * 1000);

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    ezhttp_get("https://httpbin.org/delay/5", "test_get_fail_by_timeout_complete", opt);
}

public test_get_fail_by_timeout_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    // asserts

    ASSERT_TRUE_MSG(ezhttp_get_error_code(request_id) == EZH_OPERATION_TIMEDOUT, "operation must be timeouted");

    //

    END_ASYNC_TEST()
}

START_ASYNC_TEST(test_post_form)
{
    new EzHttpOptions:opt = ezhttp_create_options();

    ezhttp_option_add_form_payload(opt, "MyFormEntry1", "FormVal1");
    ezhttp_option_add_form_payload(opt, "MyFormEntry2", "FormVal2");

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    ezhttp_post("https://httpbin.org/post", "test_post_form_complete", opt);
}

public test_post_form_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    new EzJSON:json_root = ezhttp_parse_json_response(request_id);
    new EzJSON:json_form = ezjson_object_get_value(json_root, "form");

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

    ezhttp_option_set_body(opt, "MyBody");
    ezhttp_option_set_header(opt, "Content-Type", "text/plain");

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    ezhttp_post("https://httpbin.org/post", "test_post_body_complete", opt);
}

public test_post_body_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    new EzJSON:json_root = ezhttp_parse_json_response(request_id);

    // asserts

    new body_data[256];
    ezjson_object_get_string(json_root, "data", body_data, charsmax(body_data));
    ASSERT_TRUE(equal(body_data, "MyBody"));

    //

    ezjson_free(json_root);

    END_ASYNC_TEST()
}

START_ASYNC_TEST(test_user_agent)
{
    new EzHttpOptions:opt = ezhttp_create_options();
    ezhttp_option_set_user_agent(opt, "Easy HTTP User-Agent");

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    ezhttp_post("https://httpbin.org/post", "test_user_agent_complete", opt);
}

public test_user_agent_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    new EzJSON:json_root = ezhttp_parse_json_response(request_id);
    new EzJSON:json_headers = ezjson_object_get_value(json_root, "headers");

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
    ezhttp_option_set_header(opt, "Myheader1", "HeaderVal1");
    ezhttp_option_set_header(opt, "Myheader2", "HeaderVal2");

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    ezhttp_post("https://httpbin.org/post", "test_headers_complete", opt);
}

public test_headers_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    new EzJSON:json_root = ezhttp_parse_json_response(request_id);
    new EzJSON:json_headers = ezjson_object_get_value(json_root, "headers");

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
    ezhttp_option_set_cookie(opt, "Mycookie1", "CookieVal1");
    ezhttp_option_set_cookie(opt, "Mycookie2", "CookieVal2");

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    ezhttp_get("https://httpbin.org/cookies", "test_cookies_complete", opt);
}

public test_cookies_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    new EzJSON:json_root = ezhttp_parse_json_response(request_id);
    new EzJSON:json_cookies = ezjson_object_get_value(json_root, "cookies");

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

    ezhttp_option_set_body(opt, "MyBody");
    ezhttp_option_set_header(opt, "Content-Type", "text/plain");

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    ezhttp_get("https://httpbin.org/robots.txt", "test_save_to_file_complete", opt);
}

public test_save_to_file_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    ezhttp_save_data_to_file(request_id, "addons/amxmodx/test_save.txt");

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
    ezhttp_option_set_auth(opt, "user1", "pswd1");

    EZHTTP_OPTION_SET_TEST_DATA(opt)

    ezhttp_get("https://httpbin.org/basic-auth/user1/pswd1", "test_auth_complete", opt);
}

public test_auth_complete(EzHttpRequest:request_id)
{
    EZHTTP_OPTION_EXTRACT_TEST_DATA(request_id)

    new EzJSON:json_root = ezhttp_parse_json_response(request_id);

    // asserts

    ASSERT_TRUE(ezjson_object_get_bool(json_root, "authenticated"));

    new user[16];
    ezjson_object_get_string(json_root, "user", user, charsmax(user));
    ASSERT_TRUE(equal(user, "user1"));

    //

    ezjson_free(json_root);

    END_ASYNC_TEST()
}

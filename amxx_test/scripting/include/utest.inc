#if defined _utest_included
    #endinput
#endif
#define _utest_included

/**
 * inspired by https://github.com/mity/cutest
 * inspired by https://github.com/libcheck/check
 */

/**
 * Maximum length of unit test name
 */
#define TEST_NAME_MAX_LEN 64

/**
 * Maximum length of function name
 */
#define TEST_FUNC_MAX_LEN 66

/**
 * Maximum length of unit test description
 */
#define TEST_DESCRIPTION_MAX_LEN 255

/**
 * Need stop if one of the unit test failed
 */
#define UT_STOP_AFTER_FIRST_FAIL (1 << 0)

/**
 * Need to show more info
 */
#define UT_VERBOSE (1 << 1)

/**
 * Default flag (verbose + stop testing on first failure)
 */
#define UT_DEFAULT (UT_VERBOSE | UT_STOP_AFTER_FIRST_FAIL)

/**
 * Scope struct
 */
enum _:_ScopeT {
    _es_name[TEST_FUNC_MAX_LEN],
    _es_total,
    _es_flags,
    _es_skipped,
    _es_passed,
    _es_failed
};

/**
 * Types of unit test's result execution
 */
enum TestResultT {
    TEST_OK = 0,
    TEST_FAIL,
    TEST_SKIP,
    TEST_UNDEFINED
};

/**
 * Test struct
 */
enum _:_TestT {
    _et_name[TEST_NAME_MAX_LEN],
    _et_desc[TEST_DESCRIPTION_MAX_LEN],
    _et_func[TEST_FUNC_MAX_LEN],
    _et_assert_number,
    TestResultT:_et_result
};

/**
 * Macro that helps to mark previous test as last
 */
#define TEST_LIST_END { 0 }

/**
 * List of unit tests (required)
 */
#define TEST_LIST stock g_utlist[][_TestT]

/**
 * List of async unit tests (required)
 */
#define TEST_LIST_ASYNC stock g_utlist_async[][_TestT]

/**
 * Start a unit test with START_TEST(unit_test_name)
 * end with END_TEST (is not required)
 * Have to use braces within ?
 */
#define START_TEST(%1) public __%1(__test[_TestT])

/**
 * END unit test
 */
#define END_TEST

#define START_ASYNC_TEST(%1) START_TEST(%1)
    
#define END_ASYNC_TEST()\
    utest_run_async_one_test_end(__test, g_utlist_async);\
    utest_run_async_one_test_begin(g_utlist_async);

/**
 * @param flags cell
 */
#define UTEST_RUN(%1) utest_run(g_utlist, %1);

/**
 * @param flags cell
 */
#define UTEST_RUN_ASYNC(%1)\
    utest_run_async_begin(%1);\
    utest_run_async_one_test_begin(g_utlist_async);

/**
 * Skip unit est
 * @param reason
 */
#define SKIP_TEST(%1)\
    server_print(">>> Skip test suite %s (%s)", __test[_et_name], %1);\
    __test[_et_result] = TEST_SKIP;\
    return

/**
 * @param ut_list[][_TestT]
 * @param flags
 */
stock utest_run(ut_list[][_TestT], flags = UT_DEFAULT)
{
    new TestResultT:res = TEST_UNDEFINED

    new scope[_ScopeT] = {"", 0}
    new stop = false

    server_print("======UTEST RUN=======")

    for (new i = 0; ut_list[i][_et_name] != 0 && !stop; ++i) {
        ++scope[_es_total]
        ut_list[i][_et_result] = TEST_OK

        format(ut_list[i][_et_func], TEST_FUNC_MAX_LEN, "__%s", ut_list[i][_et_name])
        if (!callfunc_begin(ut_list[i][_et_func])) {
            /* TODO log error*/
        }

        callfunc_push_array(_:ut_list[i], _TestT)

        server_print(
            ">>> Running test suite [%s]: %s",
            ut_list[i][_et_name],
            ut_list[i][_et_desc]
        );

        callfunc_end()

        res = ut_list[i][_et_result]
        switch (res) {
            case TEST_OK: ++scope[_es_passed];
            case TEST_FAIL: {
                ++scope[_es_failed];
                stop = flags & UT_STOP_AFTER_FIRST_FAIL
            }
            case TEST_SKIP: ++scope[_es_skipped];
        }
    }

    if (flags & UT_VERBOSE) {
        server_print("=======SUMMARY:========")
        server_print("Count of all unit tests: %d", scope[_es_total])
        server_print("Count of ok tests: %d", scope[_es_passed])
        server_print("Count of failed tests: %d", scope[_es_failed])
        server_print("Count of skipped tests: %d", scope[_es_skipped])
    }

    if (scope[_es_failed]) {
        server_print(
            "FAILED: %d of %d unit tests have failed",
            scope[_es_failed],
            scope[_es_total]
        )
    } else {
        server_print("SUCCESS: all unit tests have passed")
    }

    server_print("==========END==========")
}

new g_AsyncScope[_ScopeT] = {"", 0}
new g_AsyncStop = false
new g_AsyncIteration = 0
new g_AsyncFlags = 0

stock utest_run_async_begin(flags = UT_DEFAULT)
{
    g_AsyncScope[_es_name][0] = '^0';
    g_AsyncScope[_es_total] = 0;
    g_AsyncStop = false
    g_AsyncFlags = flags

    server_print("======UTEST RUN ASYNC=======")

    g_AsyncIteration = 0
}

stock utest_run_async_one_test_begin(ut_list[][_TestT])
{
    new i = g_AsyncIteration;
    if (ut_list[i][_et_name] == 0 || g_AsyncStop)
    {
        utest_run_async_end();
        return;
    }

    ++g_AsyncScope[_es_total]
    ut_list[i][_et_result] = TEST_OK

    format(ut_list[i][_et_func], TEST_FUNC_MAX_LEN, "__%s", ut_list[i][_et_name])
    if (!callfunc_begin(ut_list[i][_et_func])) {
        /* TODO log error*/
    }

    callfunc_push_array(_:ut_list[i], _TestT)

    server_print(
        ">>> Running test suite [%s]: %s",
        ut_list[i][_et_name],
        ut_list[i][_et_desc]
    );

    callfunc_end()
}

stock utest_run_async_one_test_end(const local_test_data[_TestT], ut_list[][_TestT])
{   
    new i = g_AsyncIteration;

    // copy local data to global
    ut_list[i][_et_result] = local_test_data[_et_result];

    new TestResultT:res = ut_list[i][_et_result]
    switch (res) {
        case TEST_OK: ++g_AsyncScope[_es_passed];
        case TEST_FAIL: {
            ++g_AsyncScope[_es_failed];
            g_AsyncStop = g_AsyncFlags & UT_STOP_AFTER_FIRST_FAIL
        }
        case TEST_SKIP: ++g_AsyncScope[_es_skipped];
    }

    g_AsyncIteration++;
}

stock utest_run_async_end()
{
    if (g_AsyncFlags & UT_VERBOSE) {
        server_print("=======SUMMARY:========")
        server_print("Count of all unit tests: %d", g_AsyncScope[_es_total])
        server_print("Count of ok tests: %d", g_AsyncScope[_es_passed])
        server_print("Count of failed tests: %d", g_AsyncScope[_es_failed])
        server_print("Count of skipped tests: %d", g_AsyncScope[_es_skipped])
    }

    if (g_AsyncScope[_es_failed]) {
        server_print(
            "FAILED: %d of %d unit tests have failed",
            g_AsyncScope[_es_failed],
            g_AsyncScope[_es_total]
        )
    } else {
        server_print("SUCCESS: all unit tests have passed")
    }

    server_print("==========END==========")
}

/**
 * If condition is false, the test fails
 * @param %1 Boolean condition
 */
#define ASSERT_TRUE(%1) _test_assert(__test, (any:%1), __LINE__)

/**
 * If condition is false, the test fails
 * @param %1 Boolean condition
 * @param %2 Message to print if test fails
 */
#define ASSERT_TRUE_MSG(%1,%2) _test_assert(__test, (any:%1), __LINE__, (%2))

/**
 * If condition is true, the test fails
 * @param %1 Boolean condition
 * @param %2 Message to print if test fails
 */
#define ASSERT_FALSE_MSG(%1,%2) ASSERT_TRUE(!%1,%2)

/**
 * If condition is true, the test fails
 * @param %1 Boolean condition
 */
#define ASSERT_FALSE(%1) ASSERT_FALSE_MSG(%1,"")

/**
 * If condition (X OP Y) is false, the test fails
 * @param %1 X cell
 * @param %2 OP sign
 * @param %3 Y cell
 * @param %4 Message to print if test fails
 */
#define _ASSERT_INT_MSG(%1,%2,%3,%4) ASSERT_TRUE_MSG(cell:%1%2cell:%3,%4)

/**
 * If not %1 == %2, the test fails
 * @param %1 cell
 * @param %2 cell
 * @param %3 Message to print if test fails
 */
#define ASSERT_INT_EQ_MSG(%1,%2,%3) _ASSERT_INT_MSG(%1,==,%2,%3)

/**
 * If not %1 == %2, the test fails
 * @param %1 cell
 * @param %2 cell
 */
#define ASSERT_INT_EQ(%1,%2) ASSERT_INT_EQ_MSG(%1,%2,"")

/**
 * If not %1 != %2, the test fails
 * @param %1 cell
 * @param %2 cell
 * @param %3 Message to print if test fails
 */
#define ASSERT_INT_NEQ_MSG(%1,%2,%3) _ASSERT_INT_MSG(%1,!=,%2,%3)

/**
 * If not %1 != %2, the test fails
 * @param %1 cell
 * @param %2 cell
 */
#define ASSERT_INT_NEQ(%1,%2) ASSERT_INT_NEQ_MSG(%1,%2,"")

/**
 * If not %1 > %2, the test fails
 * @param %1 cell
 * @param %2 cell
 * @param %3 Message to print if test fails
 */
#define ASSERT_INT_GT_MSG(%1,%2,%3) _ASSERT_INT_MSG(%1,>,%2,%3)

/**
 * If not %1 > %2, the test fails
 * @param %1 cell
 * @param %2 cell
 */
#define ASSERT_INT_GT(%1,%2) ASSERT_INT_GT_MSG(%1,%2,"")

/**
 * If not %1 >= %2, the test fails
 * @param %1 cell
 * @param %2 cell
 * @param %3 Message to print if test fails
 */
#define ASSERT_INT_GE_MSG(%1,%2,%3) _ASSERT_INT_MSG(%1,>=,%2,%3)

/**
 * If not %1 >= %2, the test fails
 * @param %1 cell
 * @param %2 cell
 */
#define ASSERT_INT_GE(%1,%2) ASSERT_INT_GE_MSG(%1,%2,"")

/**
 * If not %1 < %2, the test fails
 * @param %1 cell
 * @param %2 cell
 * @param %3 Message to print if test fails
 */
#define ASSERT_INT_LT_MSG(%1,%2,%3) _ASSERT_INT(%1,<,%2,%3)

/**
 * If not %1 < %2, the test fails
 * @param %1 cell
 * @param %2 cell
 */
#define ASSERT_INT_LT(%1,%2) ASSERT_INT_LT_MSG(%1,%2,"")

/**
 * If not %1 <= %2, the test fails
 * @param %1 cell
 * @param %2 cell
 * @param %3 Message to print if test fails
 */
#define ASSERT_INT_LE_MSG(%1,%2,%3) _ASSERT_INT(%1,<=,%2,%3)

/**
 * If not %1 <= %2, the test fails
 * @param %1 cell
 * @param %2 cell
 */
#define ASSERT_INT_LE(%1,%2) ASSERT_INT_LE_MSG(%1,%2,"")

/**
 * If not %1 == %2, the test fails
 * @param %1 String
 * @param %2 String
 * @param %3 Message to print if test fails
 */
#define ASSERT_STRING_EQ_MSG(%1,%2,%3) ASSERT_TRUE_MSG(equal(%1,%2),%3)

/**
 * If not %1 == %2, the test fails
 * @param %1 String
 * @param %2 String
 */
#define ASSERT_STRING_EQ(%1,%2) ASSERT_STRING_EQ_MSG(%1,%2,"")

/**
 * If not %1 != %2, the test fails
 * @param %1 String
 * @param %2 String
 * @param %3 Message to print if test fails
 */
#define ASSERT_STRING_NEQ_MSG(%1,%2,%3) ASSERT_FALSE_MSG(equal(%1,%2),%3)

/**
 * If not %1 != %2, the test fails
 * @param %1 String
 * @param %2 String
 */
#define ASSERT_STRING_NEQ(%1,%2) ASSERT_STRING_NEQ_MSG(%1,%2,"")

/**
 * Simple assert implementation
 * @param test _TestT
 * @param cond cell
 * @param line cell
 * @param message String
 */
stock _test_assert(test[_TestT], cond, line, const message[] = "")
{
    new number = ++test[_et_assert_number]

    if (cond) {
        server_print("+ [%s] OK #%d", test[_et_name], number)
    } else {
        new msg[256] = ""
        if (strlen(message)) {
            formatex(msg, sizeof(msg), ", %s", message)
        }

        server_print(
            "- [%s] FAIL #%d LINE %d%s",
            test[_et_name],
            number,
            line,
            msg
        )
        test[_et_result] = TEST_FAIL
    }
}

#undef TEST_FUNC_MAX_LEN
#undef TEST_NAME_MAX_LEN
#undef TEST_DESCRIPTION_MAX_LEN
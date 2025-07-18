#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <stdio.h>
#include <sys/time.h>

#include "driver.h"
#include "__helper.h"

#define QUOTE1(x) #x
#define QUOTE(x) QUOTE1(x)
#define SCRIPT1(x) x##_script
#define SCRIPT(x) SCRIPT1(x)

// TODO: (EWS) Why are there all these #defines and #undefs below?
// It makes the maintenance of the tests very difficult.
#define SUITE basic_suite
#define jobs_cmd "jobs"
#define printers_cmd "printers"
#define help_cmd "help"
#define quit_cmd "quit"

/*---------------------------test quit cmd------------------------------------*/
#define TEST_NAME quit_cmd_test
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,                     modifiers,          timeout,    before,    after
    {  NULL,            INIT_EVENT,                 0,                  HND_MSEC,   NULL,      NULL },
    {  quit_cmd,        FINI_EVENT,                 EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,                  0,                  TEN_MSEC,   NULL,      NULL }
};


Test(SUITE, TEST_NAME, .init=test_setup, .fini = test_teardown, .timeout = 5)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef TEST_NAME


/*---------------------------test unknown cmd error----------------------------*/
#define TEST_NAME unknown_cmd_test
#define unknown_cmd "unknown"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,                     modifiers,          timeout,    before,    after
    {  NULL,            INIT_EVENT,                 0,                  HND_MSEC,   NULL,      NULL },
    {  unknown_cmd,     CMD_ERROR_EVENT,            0,                  HND_MSEC,   NULL,      NULL },
    {  quit_cmd,        FINI_EVENT,                 EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,                  0,                  TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .init=test_setup, .fini = test_teardown, .timeout = 5)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef unknown_cmd
#undef TEST_NAME

/*---------------------------test help cmd------------------------------------*/
#define TEST_NAME help_cmd_test
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,                 modifiers,          timeout,    before,    after
    {  NULL,            INIT_EVENT,             0,                  HND_MSEC,   NULL,      NULL },
    {  help_cmd,        CMD_OK_EVENT,           0,                  HND_MSEC,   NULL,      NULL },
    {  quit_cmd,        FINI_EVENT,             EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,              0,                  TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .init=test_setup, .fini = test_teardown, .timeout = 5)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef TEST_NAME

/*---------------------------test type cmd--------------------------------*/
#define TEST_NAME type_cmd_ok_test
#define type_cmd "type pdf"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,                     modifiers,          timeout,    before,    after
    {  NULL,            INIT_EVENT,                 0,                  HND_MSEC,   NULL,      NULL },
    {  type_cmd,        TYPE_DEFINED_EVENT,         EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  quit_cmd,        FINI_EVENT,                 EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,                  0,                  TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .init=test_setup, .fini = test_teardown, .timeout = 5)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef type_cmd
#undef TEST_NAME

/*---------------------------test printer cmd--------------------------------*/
#define TEST_NAME printer_cmd_ok_test
#define type_cmd "type pdf"
#define printer_cmd "printer Alice pdf"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,                         modifiers,          timeout,    before,    after
    {  NULL,            INIT_EVENT,                     0,                  HND_MSEC,   NULL,      NULL },
    {  type_cmd,        TYPE_DEFINED_EVENT,             EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  printer_cmd,     PRINTER_DEFINED_EVENT,          EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  quit_cmd,        FINI_EVENT,                     EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,                      0,                  TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .init=test_setup, .fini = test_teardown, .timeout = 5)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef type_cmd
#undef printer_cmd
#undef TEST_NAME

/*---------------------------test conversion cmd--------------------------------*/
#define TEST_NAME conversion_cmd_ok_test
#define type_cmd_1 "type aaa"
#define type_cmd_2 "type bbb"
#define conversion_cmd "conversion aaa bbb util/convert aaa bbb"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,                expect,                     modifiers,          timeout,    before,    after
    {  NULL,                INIT_EVENT,                 0,                  HND_MSEC,   NULL,      NULL },
    {  type_cmd_1,          TYPE_DEFINED_EVENT,         EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  type_cmd_2,          TYPE_DEFINED_EVENT,         EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  conversion_cmd,      CONVERSION_DEFINED_EVENT,   EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  quit_cmd,            FINI_EVENT,                 EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  NULL,                EOF_EVENT,                  0,                  TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .init=test_setup, .fini = test_teardown, .timeout = 5)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef type_cmd_1
#undef type_cmd_2
#undef conversion_cmd
#undef TEST_NAME

/*---------------------------test jobs cmd----------------------------------------*/
#define TEST_NAME jobs_cmd_test
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,                 modifiers,          timeout,    before,    after
    {  NULL,            INIT_EVENT,             0,                  HND_MSEC,   NULL,      NULL },
    {  jobs_cmd,        CMD_OK_EVENT,           0,                  HND_MSEC,   NULL,      NULL },
    {  quit_cmd,        FINI_EVENT,             EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,              0,                  TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .init=test_setup, .fini = test_teardown, .timeout = 5)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef TEST_NAME

/*---------------------------test enable cmd----------------------------------------*/
#define TEST_NAME enable_cmd_ok_test
#define type_cmd "type aaa"
#define printer_cmd "printer alice aaa"
#define enable_cmd "enable alice"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,                         modifiers,          timeout,    before,    after
    {  NULL,            INIT_EVENT,                     0,                  HND_MSEC,   NULL,      NULL },
    {  type_cmd,        TYPE_DEFINED_EVENT,             EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  printer_cmd,     PRINTER_DEFINED_EVENT,          EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  enable_cmd,      PRINTER_STATUS_EVENT,           EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  quit_cmd,        FINI_EVENT,                     EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,                      0,                  TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .init=test_setup, .fini = test_teardown, .timeout = 5)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef type_cmd
#undef printer_cmd
#undef enable_cmd
#undef TEST_NAME

/*---------------------------test print cmd----------------------------------------*/
#define TEST_NAME print_cmd_ok_test
#define type_cmd "type aaa"
#define print_cmd "print test_scripts/testfile.aaa"
static COMMAND SCRIPT(TEST_NAME)[] = {
    // send,            expect,                         modifiers,          timeout,    before,    after
    {  NULL,            INIT_EVENT,                     0,                  HND_MSEC,   NULL,      NULL },
    {  type_cmd,        TYPE_DEFINED_EVENT,             EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  print_cmd,       JOB_CREATED_EVENT,              EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  quit_cmd,        FINI_EVENT,                     EXPECT_SKIP_OTHER,  HND_MSEC,   NULL,      NULL },
    {  NULL,            EOF_EVENT,                      0,                  TEN_MSEC,   NULL,      NULL }
};

Test(SUITE, TEST_NAME, .init=test_setup, .fini = test_teardown, .timeout = 5)
{
    int err, status;
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    char *argv[] = {TEST_EXECUTABLE, NULL};
    err = run_test(name, argv[0], argv, SCRIPT(TEST_NAME), &status);
    assert_proper_exit_status(err, status);
}
#undef type_cmd
#undef print_cmd
#undef TEST_NAME


add_simple_test(TestScopeGuard "")
add_simple_test(TestScopeGuardDefuse "")

add_simple_test(TestProcessHelper "distplusplus::common::process_helper")
add_simple_test(TestProcessHelperBad "distplusplus::common::process_helper")
add_simple_test(TestProcessHelperStdout "distplusplus::common::process_helper")
add_simple_test(TestProcessHelperStderr "distplusplus::common::process_helper")
add_simple_test(TestProcessHelperEnv "distplusplus::common::process_helper")

add_simple_test(TestTempfileCreate "distplusplus::common::tempfile")
add_simple_test(TestTempfileDelete "distplusplus::common::tempfile")
add_simple_test(TestTempfileNoCleanup "distplusplus::common::tempfile")
add_simple_test(TestTempfileContent "distplusplus::common::tempfile")
add_simple_test(TestTempfileNoName "distplusplus::common::tempfile")
add_simple_test(TestTempfileNoBasename "distplusplus::common::tempfile")

add_simple_test(TestMapCompiler "distplusplus::common")
add_simple_test(TestMapGRPCStatus "distplusplus::common")

add_simple_test(TestAssertAndRaiseGood "distplusplus::common")
add_simple_test(TestAssertAndRaiseBad "distplusplus::common")

add_simple_test(TestInitBoostLogging "distplusplus::common")

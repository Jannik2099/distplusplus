add_simple_test(TestParser "distplusplus::server::parser")
add_simple_test(TestParserBad "distplusplus::server::parser")

add_simple_test(TestParserCPPArgsSingle "distplusplus::server::parser")
add_simple_test(TestParserCPPArgsSingleStartsWith "distplusplus::server::parser")
add_simple_test(TestParserCPPArgsMulti "distplusplus::server::parser")
add_simple_test(TestParserCPPArgsMultiBad "distplusplus::server::parser")

add_simple_test(TestParserNoDistributeSingle "distplusplus::server::parser")
add_simple_test(TestParserNoDistributeSingleStartsWith "distplusplus::server::parser")
add_simple_test(TestParserNoDistributeMulti "distplusplus::server::parser")

add_simple_test(TestParserCannotProcessSignal "distplusplus::server::parser")

add_bash_test(TestServerStartup)
add_bash_test(TestServerNoListenAddress)
add_bash_test(TestServerReserveAndReap)
add_bash_test(TestServerReserveAndServe)

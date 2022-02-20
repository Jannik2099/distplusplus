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
add_bash_test(TestServerReserveAndReap)
add_bash_test(TestServerReserveAndServe)
add_bash_test(TestServerBadCompiler)
add_bash_test(TestServerDisallowedCompiler)
add_bash_test(TestServerNoReservation)
add_bash_test(TestServerNoDistribute)
add_bash_test(TestServerDecompressBad)
add_bash_test(TestServerDecompressNONE)
add_bash_test(TestServerDecompresszstd)

add_executable(distplusplus-server "main.cpp")
add_executable(distplusplus::server::distplusplus-server ALIAS distplusplus-server)

add_library(config-server OBJECT "config.cpp")
add_library(distplusplus::server::config ALIAS config-server)
target_link_libraries(config-server distplusplus::common distplusplus::common::compression_helper Boost::log Boost::program_options)
obj_propagate_targets(config-server)

add_library(parser-server OBJECT "parser.cpp")
add_library(distplusplus::server::parser ALIAS parser-server)
target_link_libraries(parser-server Boost::log)
obj_propagate_targets(parser-server)

add_library(server OBJECT "server.cpp")
add_library(distplusplus::server::server ALIAS server)
target_link_libraries(server Boost::log distplusplus::common distplusplus::common::compression_helper distplusplus::common::process_helper distplusplus::common::tempfile distplusplus::protobuf distplusplus::grpc distplusplus::server::parser)
obj_propagate_targets(server)


target_link_libraries(distplusplus-server server distplusplus::common parser-server config-server distplusplus::protobuf distplusplus::grpc PkgConfig::protobuf PkgConfig::grpc++ Boost::filesystem Boost::log)

install(TARGETS distplusplus-server RUNTIME)

if(ZSTD)
    target_compile_definitions(distplusplus-server PRIVATE "DISTPLUSPLUS_WITH_ZSTD")
endif()

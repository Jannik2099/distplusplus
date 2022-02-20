add_executable(distplusplus-server "main.cpp")
add_executable(distplusplus::server::distplusplus-server ALIAS distplusplus-server)

add_library(parser-server OBJECT "parser.cpp")
add_library(distplusplus::server::parser ALIAS parser-server)
target_link_libraries(parser-server Boost::log -ltbb)
obj_propagate_targets(parser-server)

add_library(server OBJECT "server.cpp")
add_library(distplusplus::server::server ALIAS server)
target_link_libraries(server Boost::log distplusplus::common distplusplus::common::compression_helper distplusplus::protobuf distplusplus::grpc distplusplus::server::parser)
obj_propagate_targets(server)


target_link_libraries(distplusplus-server server distplusplus::common parser-server distplusplus::protobuf distplusplus::grpc PkgConfig::protobuf PkgConfig::grpc++ Boost::filesystem Boost::log Boost::program_options -ltbb)

install(TARGETS distplusplus-server RUNTIME)

if(ZSTD)
    target_compile_definitions(distplusplus-server PRIVATE "DISTPLUSPLUS_WITH_ZSTD")
endif()

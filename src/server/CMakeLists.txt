add_executable(distplusplus-server "main.cpp")
add_executable(distplusplus::server::distplusplus-server ALIAS distplusplus-server)

add_library(server OBJECT "server.cpp")
add_library(distplusplus::server::server ALIAS server)

add_library(parser-server OBJECT "parser.cpp")
add_library(distplusplus::server::parser ALIAS parser-server)

target_link_libraries(distplusplus-server server distplusplus::common parser-server protobuf PkgConfig::protobuf grpc PkgConfig::grpc Boost::filesystem Boost::log -ltbb)

install(PROGRAMS distplusplus-server TYPE BIN)
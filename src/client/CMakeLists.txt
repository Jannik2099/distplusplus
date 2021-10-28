add_executable(distplusplus "main.cpp")
add_executable(distplusplus::client::distplusplus ALIAS distplusplus)

add_library(parser-client OBJECT "parser.cpp")
add_library(distplusplus::client::parser ALIAS parser-client)

add_library(client OBJECT "client.cpp")
add_library(distplusplus::client::client ALIAS client)

add_library(config-client OBJECT "config.cpp")
add_library(distplusplus::client::config ALIAS config-client)

target_link_libraries(distplusplus distplusplus::client::client distplusplus::client::parser distplusplus::client::config distplusplus::common protobuf PkgConfig::protobuf grpc PkgConfig::grpc Boost::filesystem Boost::log)

install(TARGETS distplusplus DESTINATION bin)

add_executable(distplusplus "main.cpp")
add_executable(distplusplus::client::distplusplus ALIAS distplusplus)

add_library(parser-client OBJECT "parser.cpp")
add_library(distplusplus::client::parser ALIAS parser-client)
target_link_libraries(parser-client distplusplus::common -ltbb)
obj_propagate_targets(parser-client)

add_library(config-client OBJECT "config.cpp")
add_library(distplusplus::client::config ALIAS config-client)
target_link_libraries(config-client Boost::log)
obj_propagate_targets(config-client)

add_library(client OBJECT "client.cpp")
add_library(distplusplus::client::client ALIAS client)
target_link_libraries(client distplusplus::protobuf distplusplus::grpc PkgConfig::lmdb)
obj_propagate_targets(client)

target_link_libraries(distplusplus distplusplus::client::client distplusplus::client::parser distplusplus::client::config distplusplus::common distplusplus::common distplusplus::protobuf distplusplus::grpc distplusplus::common::compression_helper Boost::log -ltbb)

install(TARGETS distplusplus RUNTIME)

target_compile_definitions(config-client PRIVATE $<TARGET_PROPERTY:distplusplus::common::compression_helper,INTERFACE_COMPILE_DEFINITIONS>)

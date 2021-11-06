add_library(common OBJECT "common.cpp")
add_library(distplusplus::common ALIAS common)
target_link_libraries(common Boost::log)
obj_propagate_targets(common)

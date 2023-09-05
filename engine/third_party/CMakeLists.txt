set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(YAML_BUILD_SHARED_LIBS off)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLFW_INCLUDE_NONE")

add_subdirectory(fmt)
add_subdirectory(glfw)
add_subdirectory(glm)
add_subdirectory(yaml_cpp)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

add_subdirectory(fmt)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLFW_INCLUDE_NONE")
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(glfw)

add_subdirectory(glm)

add_subdirectory(magic_enum)

add_subdirectory(vma)

set(YAML_BUILD_SHARED_LIBS off)
add_subdirectory(yaml_cpp)
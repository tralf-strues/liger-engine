include("${CMAKE_HOME_DIRECTORY}/cmake/compile_options_main.cmake")
add_subdirectory(third_party)

include("${CMAKE_HOME_DIRECTORY}/cmake/compile_options_warnings.cmake")
add_subdirectory(src)

target_link_libraries(liger
  PUBLIC
    fmt
    glfw
    glm
    yaml-cpp
  )

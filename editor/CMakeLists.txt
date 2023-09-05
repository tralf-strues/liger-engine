include("${CMAKE_HOME_DIRECTORY}/cmake/compile_options_main.cmake")
add_subdirectory(third_party)

include("${CMAKE_HOME_DIRECTORY}/cmake/compile_options_warnings.cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../")
add_subdirectory(src)

target_link_libraries(liger-editor
  PUBLIC
    liger

    fmt
    glfw
    glm
    yaml-cpp
  )

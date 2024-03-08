include("${CMAKE_HOME_DIRECTORY}/cmake/compile_options_main.cmake")
add_subdirectory(third_party)

include("${CMAKE_HOME_DIRECTORY}/cmake/compile_options_warnings.cmake")
add_subdirectory(src)

target_include_directories(liger PUBLIC "third_party/taskflow")

target_link_libraries(liger
  PUBLIC
    fmt
    glfw
    glm
    magic_enum
    VulkanMemoryAllocator
    vulkan-headers
    vulkan-utility-libraries
    volk
    yaml-cpp
  )

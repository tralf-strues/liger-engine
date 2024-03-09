include("${CMAKE_HOME_DIRECTORY}/cmake/compile_options_main.cmake")
include("${CMAKE_HOME_DIRECTORY}/cmake/compile_options_warnings.cmake")

add_subdirectory(third_party)
add_subdirectory(src)

string(REPLACE " " ";" LIGER_COMPILE_OPTIONS "${LIGER_COMPILE_FLAGS} ${LIGER_WARNING_FLAGS}")
target_compile_options(liger PRIVATE ${LIGER_COMPILE_OPTIONS})

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

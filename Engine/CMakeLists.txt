include("${CMAKE_HOME_DIRECTORY}/Cmake/CompileOptions.cmake")

add_subdirectory(ThirdParty)

add_library(liger-engine STATIC)

target_compile_options(liger-engine PRIVATE ${LIGER_COMPILE_FLAGS})
target_link_options(liger-engine PRIVATE ${LIGER_LINK_FLAGS})

message("-- Liger-Engine flags (LIGER_COMPILE_FLAGS): ${LIGER_COMPILE_FLAGS}")
message("-- Liger-Engine flags (LIGER_LINK_FLAGS): ${LIGER_LINK_FLAGS}")

file(GLOB_RECURSE LIGER_ENGINE_SOURCE_PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h
)

file(GLOB_RECURSE LIGER_ENGINE_SOURCE_PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c
)

target_include_directories(liger-engine
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/taskflow
)

target_sources(liger-engine
  PUBLIC
    ${LIGER_ENGINE_SOURCE_PUBLIC}
  PRIVATE
    ${LIGER_ENGINE_SOURCE_PRIVATE}
)

target_link_libraries(liger-engine
  PUBLIC
    fmt
    glfw
    glm
    magic_enum

  PRIVATE
    VulkanMemoryAllocator
    vulkan-headers
    vulkan-utility-libraries
    volk
    yaml-cpp
)

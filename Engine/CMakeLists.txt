include("../Cmake/CompileOptions.cmake")

add_subdirectory(ThirdParty)

add_library(liger-engine STATIC)
add_library(Liger-Engine::liger-engine ALIAS liger-engine)

target_compile_options(liger-engine PRIVATE ${LIGER_COMPILE_FLAGS})
target_link_options(liger-engine PRIVATE ${LIGER_LINK_FLAGS})

message("-- Liger-Engine flags (LIGER_COMPILE_FLAGS): ${LIGER_COMPILE_FLAGS}")
message("-- Liger-Engine flags (LIGER_LINK_FLAGS): ${LIGER_LINK_FLAGS}")

file(GLOB_RECURSE LIGER_ENGINE_SOURCE_PUBLIC
  Include/*.hpp
  Include/*.h
)

file(GLOB_RECURSE LIGER_ENGINE_SOURCE_PRIVATE
  Source/*.hpp
  Source/*.h
  Source/*.cpp
  Source/*.c
)

target_include_directories(liger-engine
  PUBLIC
    $<INSTALL_INTERFACE:Include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
)

target_sources(liger-engine
  PUBLIC
    ${LIGER_ENGINE_SOURCE_PUBLIC}
  PRIVATE
    ${LIGER_ENGINE_SOURCE_PRIVATE}
)

target_link_libraries(liger-engine
  PUBLIC
    EnTT::EnTT
    fmt
    glfw
    glm
    magic_enum
    Taskflow

  PRIVATE
    glslang glslang-default-resource-limits SPIRV
    VulkanMemoryAllocator
    vulkan-headers
    vulkan-utility-libraries
    volk
    yaml-cpp
)

install(
  TARGETS
    liger-engine

    EnTT
    fmt
    glfw
    glm
    glslang OSDependent MachineIndependent GenericCodeGen glslang-default-resource-limits SPIRV
    magic_enum
    Taskflow
    VulkanMemoryAllocator
    vulkan-headers
    vulkan-utility-libraries
    volk
    yaml-cpp

  EXPORT liger-engine-targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(DIRECTORY Include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

export(EXPORT liger-engine-targets
  FILE ${CMAKE_CURRENT_BINARY_DIR}/Liger-EngineConfig.cmake
  NAMESPACE Liger-Engine::
)

export(PACKAGE Liger-Engine)
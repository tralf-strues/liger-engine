include("../Cmake/CompileOptions.cmake")

set(BUILD_SHARED_LIBS OFF)

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
    assimp
    EnTT::EnTT
    fmt
    glfw
    glm
    magic_enum
    Taskflow

  PRIVATE
    # shaderc
    # shaderc glslang glslang-default-resource-limits SPIRV
    glslang glslang-default-resource-limits SPIRV
    stb_image
    VulkanMemoryAllocator
    volk
    vulkan-headers
    vulkan-utility-libraries
    yaml-cpp
)

install(
  TARGETS
    liger-engine

    assimp zlibstatic
    EnTT
    fmt
    glfw
    glm
    magic_enum
    glslang OSDependent MachineIndependent GenericCodeGen glslang-default-resource-limits SPIRV
    # shaderc shaderc_util glslang OSDependent MachineIndependent GenericCodeGen glslang-default-resource-limits SPIRV
    # shaderc shaderc_util glslang OSDependent MachineIndependent GenericCodeGen SPIRV
    stb_image
    Taskflow
    VulkanMemoryAllocator
    volk
    vulkan-headers
    vulkan-utility-libraries
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
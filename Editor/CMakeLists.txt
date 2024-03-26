include("../Cmake/CompileOptions.cmake")

add_subdirectory(ThirdParty)

add_executable(liger-editor)
set_target_properties(liger-editor PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../")

target_compile_options(liger-editor PRIVATE ${LIGER_COMPILE_FLAGS})
target_link_options(liger-editor PRIVATE ${LIGER_LINK_FLAGS})

message("-- Liger-Editor flags (LIGER_COMPILE_FLAGS): ${LIGER_COMPILE_FLAGS}")
message("-- Liger-Editor flags (LIGER_LINK_FLAGS): ${LIGER_LINK_FLAGS}")

file(GLOB_RECURSE LIGER_EDITOR_SOURCE_PUBLIC
  Include/*.hpp
  Include/*.h
)

file(GLOB_RECURSE LIGER_EDITOR_SOURCE_PRIVATE
  Source/*.hpp
  Source/*.h
  Source/*.cpp
  Source/*.c
)

target_include_directories(liger-editor
  PUBLIC
    Include
)

target_sources(liger-editor
  PUBLIC
    ${LIGER_EDITOR_SOURCE_PUBLIC}
  PRIVATE
    ${LIGER_EDITOR_SOURCE_PRIVATE}
)

target_link_libraries(liger-editor
  PUBLIC
    liger-engine
)

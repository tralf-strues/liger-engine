include("${CMAKE_HOME_DIRECTORY}/Cmake/CompileOptions.cmake")

add_subdirectory(ThirdParty)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../")
add_executable(liger-editor)

target_compile_options(liger-editor PRIVATE ${LIGER_COMPILE_FLAGS})
target_link_options(liger-editor PRIVATE ${LIGER_LINK_FLAGS})

message("-- Liger-Editor flags (LIGER_COMPILE_FLAGS): ${LIGER_COMPILE_FLAGS}")
message("-- Liger-Editor flags (LIGER_LINK_FLAGS): ${LIGER_LINK_FLAGS}")

file(GLOB_RECURSE LIGER_EDITOR_SOURCE_PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h
)

file(GLOB_RECURSE LIGER_EDITOR_SOURCE_PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c
)

target_include_directories(liger-editor
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
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

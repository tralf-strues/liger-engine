cmake_minimum_required(VERSION 3.21)

set(LIGER_PROJECT_NAME Empty-Project)
set(LIGER_ENGINE_PATH "" CACHE PATH "Path to Liger Engine")

project(${LIGER_PROJECT_NAME} LANGUAGES C CXX)

find_package(Threads REQUIRED)
find_package(Liger-Engine REQUIRED)

include("${LIGER_ENGINE_PATH}/Cmake/CompileOptions.cmake")

add_executable(${LIGER_PROJECT_NAME})
set_target_properties(${LIGER_PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../")

target_compile_options(${LIGER_PROJECT_NAME} PRIVATE ${LIGER_COMPILE_FLAGS})
target_link_options(${LIGER_PROJECT_NAME} PRIVATE ${LIGER_LINK_FLAGS})

file(GLOB_RECURSE SOURCE_PRIVATE
  Source/*.hpp
  Source/*.h
  Source/*.cpp
  Source/*.c
)

target_sources(${LIGER_PROJECT_NAME} PRIVATE ${SOURCE_PRIVATE})

target_link_libraries(${LIGER_PROJECT_NAME} PUBLIC Liger-Engine::liger-engine)
add_executable(liger-sandbox)

file(GLOB_RECURSE SANDBOX_INCLUDE *.hpp *.h)
file(GLOB_RECURSE SANDBOX_SOURCE *.cpp *.c)

target_include_directories(liger-sandbox
  PUBLIC
    .
  )

target_sources(liger-sandbox
  PUBLIC
    ${SANDBOX_INCLUDE}
  PRIVATE
    test_dag.cpp
  )
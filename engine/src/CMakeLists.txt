add_library(liger)

file(GLOB_RECURSE LIGER_INCLUDE *.hpp *.h)
file(GLOB_RECURSE LIGER_SOURCE *.cpp *.c)

target_include_directories(liger
  PUBLIC
    .
  )

target_sources(liger
  PUBLIC
    ${LIGER_INCLUDE}
  PRIVATE
    ${LIGER_SOURCE}
  )
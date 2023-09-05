add_executable(liger-editor)

file(GLOB_RECURSE LIGER_EDITOR_INCLUDE *.hpp *.h)
file(GLOB_RECURSE LIGER_EDITOR_SOURCE *.cpp *.c)

target_include_directories(liger-editor
  PUBLIC
    .
  )

target_sources(liger-editor
  PUBLIC
    ${LIGER_EDITOR_INCLUDE}
  PRIVATE
    ${LIGER_EDITOR_SOURCE}
  )
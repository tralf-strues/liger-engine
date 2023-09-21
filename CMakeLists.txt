cmake_minimum_required(VERSION 3.21)

project(liger-engine)

add_subdirectory(engine)
add_subdirectory(editor)
add_subdirectory(sandbox)

# Documentation
option(LIGER_BUILD_DOCS "Build Doxygen documentation" ON)

find_package(Doxygen)
if (DOXYGEN_FOUND AND LIGER_BUILD_DOCS)
  set(DOXYGEN_IN ${CMAKE_HOME_DIRECTORY}/docs/Doxyfile)
  set(DOXYGEN_OUT ${CMAKE_HOME_DIRECTORY}/docs/docs/Doxyfile)

  configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
  message("Doxygen build started")

  add_custom_target(doc_doxygen ALL
      COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
      WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM)
elseif (NOT DOXYGEN_FOUND AND LIGER_BUILD_DOCS)
  message("Doxygen need to be installed to generate the doxygen documentation")
endif ()
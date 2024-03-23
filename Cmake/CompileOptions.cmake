set(CMAKE_CXX_STANDARD 20)

message("-- Compiler ID C++: ${CMAKE_CXX_COMPILER_ID}")

set(LIGER_COMPILE_FLAGS "")
set(LIGER_LINK_FLAGS "")

function(add_liger_compile_flags flags)
  set(LIGER_COMPILE_FLAGS "${LIGER_COMPILE_FLAGS} ${flags}" PARENT_SCOPE)
endfunction()

function(add_liger_link_flags flags)
  set(LIGER_LINK_FLAGS "${LIGER_LINK_FLAGS} ${flags}" PARENT_SCOPE)
endfunction()

# C++ version and standard library
add_liger_compile_flags("-std=c++20 -fms-extensions")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_liger_compile_flags("-stdlib=libc++")
endif()

# Debug/Release
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_liger_compile_flags("-DLIGER_DEBUG_MODE")

  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_liger_compile_flags("/Od")
  else()
    add_liger_compile_flags("-g -O0")
  endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_liger_compile_flags("-DLIGER_RELEASE_MODE")

  string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
  string(REPLACE "/D_DEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_liger_compile_flags("/O2")
  else()
    add_liger_compile_flags("-O2")
  endif()
endif()

# Sanitizers (Linux and MacOS only)
option(LIGER_BUILD_WITH_ASAN "Enable address sanitizer (Linux and MacOS only)" OFF)
option(LIGER_BUILD_WITH_TSAN "Enable thread sanitizer (Linux and MacOS only)" OFF)

if(LIGER_BUILD_WITH_ASAN AND LIGER_BUILD_WITH_TSAN)
  message(FATAL_ERROR "Asan and tsan can not be enabled at the same time")
endif()

if((LIGER_BUILD_WITH_ASAN OR LIGER_BUILD_WITH_TSAN) AND WIN32)
  message(FATAL_ERROR "Asan and tsan can not be enabled on Windows")
endif()

if(LIGER_BUILD_WITH_ASAN)
  message("-- Address sanitizer enabled")
  add_liger_compile_flags("-fsanitize=address")
  add_liger_link_flags("-fsanitize=address")
else()
  message("-- Address sanitizer disabled")
endif()

if(LIGER_BUILD_WITH_TSAN)
  message("-- Thread sanitizer enabled")
  add_liger_compile_flags("-fsanitize=thread")
  add_liger_link_flags("-fsanitize=thread")
else()
  message("-- Thread sanitizer disabled")
endif()

# Warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_liger_compile_flags("/Wall")
else()
  add_liger_compile_flags("-Wall")
  add_liger_compile_flags("-Wextra")
  add_liger_compile_flags("-Wpedantic")
  add_liger_compile_flags("-Wno-missing-field-initializers")
  add_liger_compile_flags("-Wno-nullability-extension")
  add_liger_compile_flags("-Wno-nullability-completeness")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_liger_compile_flags("-Wno-changes-meaning")
endif()

# Convert to have ; as separators
string(REPLACE " " ";" LIGER_COMPILE_FLAGS "${LIGER_COMPILE_FLAGS}")
string(REPLACE " " ";" LIGER_LINK_FLAGS "${LIGER_LINK_FLAGS}")

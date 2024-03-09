set(CMAKE_CXX_STANDARD 20)

message("Compiler ID C++: ${CMAKE_CXX_COMPILER_ID}")

set(LIGER_COMPILE_FLAGS "-std=c++20 -fms-extensions")

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(LIGER_COMPILE_FLAGS "${LIGER_COMPILE_FLAGS} -stdlib=libc++")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(LIGER_COMPILE_FLAGS "${LIGER_COMPILE_FLAGS} -fpermissive")
endif()

# Debug/Release
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(LIGER_COMPILE_FLAGS "${LIGER_COMPILE_FLAGS} -DLIGER_DEBUG_MODE")

  if(WIN32)
    set(LIGER_COMPILE_FLAGS "${LIGER_COMPILE_FLAGS} /Od")
  else()
    set(LIGER_COMPILE_FLAGS "${LIGER_COMPILE_FLAGS} -g")
    set(LIGER_COMPILE_FLAGS "${LIGER_COMPILE_FLAGS} -O0")
  endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(LIGER_COMPILE_FLAGS "${LIGER_COMPILE_FLAGS} -DLIGER_RELEASE_MODE")

  string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
  string(REPLACE "/D_DEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

  if(WIN32)
    set(LIGER_COMPILE_FLAGS "${LIGER_COMPILE_FLAGS} /O2")
  else()
    set(LIGER_COMPILE_FLAGS "${LIGER_COMPILE_FLAGS} -O2")
  endif()
endif()

# Sanitizers (Linux and MacOS only)
option(LIGER_BUILD_WITH_ASAN "enable address sanitizer (Linux and MacOS only)" OFF)
option(LIGER_BUILD_WITH_TSAN "enable thread sanitizer (Linux and MacOS only)" OFF)

if(LIGER_BUILD_WITH_ASAN AND LIGER_BUILD_WITH_TSAN)
  message(FATAL_ERROR "asan and tsan can not be enabled at the same time")
endif()

if((LIGER_BUILD_WITH_ASAN OR LIGER_BUILD_WITH_TSAN) AND WIN32)
  message(FATAL_ERROR "asan and tsan can not be enabled on Windows")
endif()

if(LIGER_BUILD_WITH_ASAN)
  message("-- Address sanitizer enabled")
  set(LIGER_COMPILE_FLAGS "${LIGER_COMPILE_FLAGS} -fsanitize=address")
  add_link_options("-fsanitize=address")
else()
  message("-- Address sanitizer disabled")
endif()

if(LIGER_BUILD_WITH_TSAN)
  message("-- Thread sanitizer enabled")
  set(LIGER_COMPILE_FLAGS "${LIGER_COMPILE_FLAGS} -fsanitize=thread")
  add_link_options("-fsanitize=thread")
else()
  message("-- Thread sanitizer disabled")
endif()

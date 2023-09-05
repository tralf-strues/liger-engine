set(CMAKE_CXX_STANDARD 17)

# Debug/Release
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options("-DLIGER_DEBUG_MODE")

  if(WIN32)
    add_compile_options("/Od")    
  else()
    add_compile_options("-g")
    add_compile_options("-O0")
  endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options("-DLIGER_RELEASE_MODE")

  string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

  if(WIN32)
    add_compile_options("/O2")
  else()
    add_compile_options("-O2")
  endif()
endif()

# Sanitizers (Linux and MacOS only)
option(BUILD_WITH_ASAN "enable address sanitizer (Linux and MacOS only)" OFF)
option(BUILD_WITH_TSAN "enable thread sanitizer (Linux and MacOS only)" OFF)

if(BUILD_WITH_ASAN AND BUILD_WITH_TSAN)
  message(FATAL_ERROR "asan and tsan can not be enabled at the same time")
endif()

if((BUILD_WITH_ASAN OR BUILD_WITH_TSAN) AND WIN32)
  message(FATAL_ERROR "asan and tsan can not be enabled on Windows")
endif()

if(BUILD_WITH_ASAN)
  message("-- Address sanitizer enabled")
  add_compile_options("-fsanitize=address")
  add_link_options("-fsanitize=address")
else()
  message("-- Address sanitizer disabled")
endif()

if(BUILD_WITH_TSAN)
  message("-- Thread sanitizer enabled")
  add_compile_options("-fsanitize=thread")
  add_link_options("-fsanitize=thread")
else()
  message("-- Thread sanitizer disabled")
endif()

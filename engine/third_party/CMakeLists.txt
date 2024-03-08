set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

find_package(Vulkan REQUIRED)

add_subdirectory(fmt)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLFW_INCLUDE_NONE")
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(glfw)

add_subdirectory(glm)

add_subdirectory(magic_enum)

add_subdirectory(vma)

set(VOLK_STATIC_DEFINES "-DVK_ENABLE_BETA_EXTENSIONS")

if (WIN32)
  set(VOLK_STATIC_DEFINES "${VOLK_STATIC_DEFINES} -DVK_USE_PLATFORM_WIN32_KHR")
elseif(APPLE)
  set(VOLK_STATIC_DEFINES "${VOLK_STATIC_DEFINES} -DVK_USE_PLATFORM_METAL_EXT")
elseif(UNIX)
  include(FindPkgConfig)

  pkg_check_modules(X11 x11)
  if (X11_FOUND)
      set(VOLK_STATIC_DEFINES "${VOLK_STATIC_DEFINES} -DVK_USE_PLATFORM_XLIB_KHR")
  endif()

  pkg_check_modules(WAYLAND wayland-client)
  if (WAYLAND_FOUND)
      set(VOLK_STATIC_DEFINES "${VOLK_STATIC_DEFINES} -DVK_USE_PLATFORM_WAYLAND_KHR")
  endif()
endif()

add_subdirectory(volk)

set(YAML_BUILD_SHARED_LIBS off)
add_subdirectory(yaml_cpp)

set(LIGER_ADDITIONAL_INCLUDE_DIRS "${Vulkan_INCLUDE_DIRS}" PARENT_SCOPE)
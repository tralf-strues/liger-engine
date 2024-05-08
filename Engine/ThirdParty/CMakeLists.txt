set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# entt
add_subdirectory(entt)

# fmt
add_subdirectory(fmt)

# glfw
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGLFW_INCLUDE_NONE")
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(glfw)

# glm
add_subdirectory(glm)

# magic-enum
add_subdirectory(magic_enum)

# shaderc
add_subdirectory(shaderc)

# taskflow
set(TF_BUILD_TESTS OFF)
set(TF_BUILD_EXAMPLES OFF)
add_subdirectory(taskflow)

# vma
add_subdirectory(vma)

# vulkan-headers
add_library(vulkan-headers INTERFACE)
set(VULKAN_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/vulkan-headers/include)
target_sources(vulkan-headers INTERFACE ${VULKAN_INCLUDE_DIRS}/vulkan/vulkan.h)
target_include_directories(vulkan-headers INTERFACE ${VULKAN_INCLUDE_DIRS})

# vulkan-utility-libraries
add_library(vulkan-utility-libraries INTERFACE)
set(VUL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/vulkan-utility-libraries/include)
target_include_directories(vulkan-utility-libraries INTERFACE ${VUL_INCLUDE_DIRS})

# volk
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

set(VULKAN_HEADERS_INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vulkan-headers/)
add_subdirectory(volk)

# yaml-cpp
set(YAML_BUILD_SHARED_LIBS off)
add_subdirectory(yaml_cpp)

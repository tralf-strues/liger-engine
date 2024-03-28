# Liger Engine
![Linux/MacOS build status](https://github.com/tralf-strues/liger-engine/actions/workflows/build.yml/badge.svg?branch=main)

Liger engine is a personal learning game engine, currently in the early stages development.

## Build
Currently the engine works on Linux and MacOS.

### Installing the engine
```bash
git clone --recursive https://github.com/tralf-strues/liger-engine
./build_unix.sh {debug|release}
```

### Create game project
```bash
python new_project.py --name "<PROJECT_NAME>" --path "<PROJECT_PATH>" --liger_path "<ENGINE_PATH>"
```

### Dependencies
| Name                                                                                 | Notes     |
|--------------------------------------------------------------------------------------|-----------|
| [fmt](https://github.com/fmtlib/fmt)                                                 | Submodule |
| [glfw](https://github.com/glfw/glfw)                                                 | Submodule |
| [glm](https://github.com/g-truc/glm)                                                 | Submodule |
| [magic_enum](https://github.com/Neargye/magic_enum)                                  | Submodule |
| [taskflow](https://github.com/taskflow/taskflow)                                     | Submodule |
| [VMA](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)             | Submodule |
| [volk](https://github.com/zeux/volk)                                                 | Submodule |
| [vulkan-headers](https://github.com/KhronosGroup/Vulkan-Headers)                     | Submodule |
| [vulkan-utility-libraries](https://github.com/KhronosGroup/Vulkan-Utility-Libraries) | Submodule |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp)                                       | Submodule |

## Wiki
[Here](https://github.com/tralf-strues/liger-engine/wiki) I'm going to write short pages on the design decisions I'm making during development of the engine.

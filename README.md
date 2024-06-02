# Liger Engine
![Linux/MacOS build status](https://github.com/tralf-strues/liger-engine/actions/workflows/build_linux_and_macos.yml/badge.svg?branch=main)

Liger engine is a personal learning game engine, currently in the early stages development.

## Build
Currently the engine works only on Windows and Linux. MacOS is currently not supported until MoltenVK supports Vulkan 1.3

### Installing the engine
#### Linux & ~~MacOS~~
```bash
git clone --recursive https://github.com/tralf-strues/liger-engine
./build_unix.sh {debug|release}
```

#### Windows (Microsoft Visual Studio)
1. Clone repository
```bash
git clone --recursive https://github.com/tralf-strues/liger-engine
```
2. Open the cloned folder as CMake project in MVS
3. Configure CMake in MVS
4. Build->Build all
5. Build->Install Liger-Engine

### Create game project
```bash
python new_project.py --name "<PROJECT_NAME>" --path "<PROJECT_PATH>" --liger_path "<ENGINE_PATH>"
```

#### Windows (Microsoft Visual Studio)
1. Open generated project in MVS
2. Go to Manage Configurations and add the following CMake command arguments (this will definitely be improved in the future):
```
-DCMAKE_EXPORT_COMPILE_COMMANDS=true -DLIGER_ENGINE_PATH=<<YOUR_PATH>> -DLiger-Engine_DIR=<<YOUR_PATH>>/out/build/x64-{Debug|Release}/Engine -DSPIRV-Tools_DIR=<<YOUR_PATH>>/out/install/x64-{Debug|Release}/SPIRV-Tools/cmake -DSPIRV-Tools-opt_DIR=<<YOUR_PATH>>/out/install/x64-{Debug|Release}/SPIRV-Tools-opt/cmake
```

### Dependencies
| Name                                                                                 | Notes     |
|--------------------------------------------------------------------------------------|-----------|
| [assimp](https://github.com/assimp/assimp)                                           | Submodule |
| [entt](https://github.com/skypjack/entt)                                             | Submodule |
| [fmt](https://github.com/fmtlib/fmt)                                                 | Submodule |
| [glfw](https://github.com/glfw/glfw)                                                 | Submodule |
| [glm](https://github.com/g-truc/glm)                                                 | Submodule |
| [glslang](https://github.com/KhronosGroup/glslang)                                   | Submodule |
| [magic_enum](https://github.com/Neargye/magic_enum)                                  | Submodule |
| [stb-image](https://github.com/nothings/stb)                                         |     -     |
| [taskflow](https://github.com/taskflow/taskflow)                                     | Submodule |
| [vma](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)             | Submodule |
| [volk](https://github.com/zeux/volk)                                                 | Submodule |
| [vulkan-headers](https://github.com/KhronosGroup/Vulkan-Headers)                     | Submodule |
| [vulkan-utility-libraries](https://github.com/KhronosGroup/Vulkan-Utility-Libraries) | Submodule |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp)                                       | Submodule |

## Wiki
[Here](https://github.com/tralf-strues/liger-engine/wiki) I'm going to write short pages on the design decisions I'm making during development of the engine.

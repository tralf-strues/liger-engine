#!/bin/bash
set -e

mkdir -p .liger_log

# cd Engine/ThirdParty/shaderc
# ./utils/git-sync-deps
# cd ../../..

if [ ! -d "Engine/ThirdParty/glslang/External/spirv-tools" ]; then
  cd Engine/ThirdParty/glslang
  ./update_glslang_sources.py
  cd ../../..
fi

if [ $# -eq 0 ]; then
  echo "Please, specify the build mode (i.e. Debug or Release)"
  exit 1
else
  build_mode=$(echo ${1} | awk '{print toupper(substr($0,0,1))tolower(substr($0,2))}')
fi

if [ "${build_mode}" != "Debug" ] && [ "${build_mode}" != "Release" ]; then
  echo "Build mode must be either \"Debug\" or \"Release\"!"
  exit 1
fi

build_dir_name="build_${1}"

mkdir -p ${build_dir_name}
cd ${build_dir_name}
cmake .. -G "Ninja" -DCMAKE_EXPORT_COMPILE_COMMANDS=true -DCMAKE_BUILD_TYPE=${build_mode} ${2}
ninja
cd ..

cmake --install ${build_dir_name} --config "${build_mode}" --prefix "/Users/tralf/Dev/liger-engine/${build_dir_name}"
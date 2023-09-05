#!/bin/bash
set -e

mkdir -p .liger_log

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
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=${build_mode} ${2}
ninja
cd ..
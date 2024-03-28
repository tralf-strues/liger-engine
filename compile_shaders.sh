for shader in assets/.liger/shaders/*.{vert,frag,comp,geom}
do
  [ -f "$shader" ] || break

  directory=$(dirname ${shader})
  filename=$(basename -- "${shader}")
  binary_file="${directory}/spirv/${filename}.spv"
  flags="-g --target-env=vulkan1.2 -O"

  mkdir -p "${directory}/spirv/"

  echo "glslc ${flags} ${shader} -o ${binary_file}"
  glslc ${flags} ${shader} -o ${binary_file}
done
#version 450

#extension GL_GOOGLE_include_directive: enable
#extension GL_EXT_shader_explicit_arithmetic_types: enable

layout(location = 0) in f32vec3 a_color;

layout(location = 0) out f32vec4 out_color;

void main() {
  out_color = f32vec4(a_color, 1.0);
}

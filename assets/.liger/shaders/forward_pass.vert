#version 450

#extension GL_GOOGLE_include_directive: enable
#extension GL_EXT_shader_explicit_arithmetic_types: enable

layout(location = 0) out f32vec3 a_color;

const f32vec2 TRIANGLE_POSITIONS[3] = {
  f32vec2(0.0, -0.5),
  f32vec2(0.5, 0.5),
  f32vec2(-0.5, 0.5)
};

const f32vec3 TRIANGLE_COLORS[3] = {
  f32vec3(1.0, 0.0, 0.0),
  f32vec3(0.0, 1.0, 0.0),
  f32vec3(0.0, 0.0, 1.0)
};

void main() {
  a_color = TRIANGLE_COLORS[gl_VertexIndex];
  gl_Position = f32vec4(TRIANGLE_POSITIONS[gl_VertexIndex], 0.0, 1.0);
}
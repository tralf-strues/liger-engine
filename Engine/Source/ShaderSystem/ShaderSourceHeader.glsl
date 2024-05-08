R"=====(#version 450

#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable

/************************************************************************************************
 * Bindless resources setup
 ************************************************************************************************/
#define LIGER_DS 0

// #define LIGER_BINDING_UNIFORM_BUFFER  0
// #define LIGER_BINDING_STORAGE_BUFFER  1
// #define LIGER_BINDING_SAMPLED_TEXTURE 2
// #define LIGER_BINDING_STORAGE_TEXTURE 3
#define LIGER_BINDING_SAMPLED_TEXTURE 0
#define LIGER_BINDING_STORAGE_TEXTURE 1

// #define RegisterUniformBuffer(Name, Struct) \
//   layout(buffer_reference) uniform Name Struct global_uniform_buffers_##Name[]

// #define RegisterStorageBuffer(Layout, Access, Name, Struct) \
//   layout(Layout, set = LIGER_DS, binding = LIGER_BINDING_STORAGE_BUFFER) Access buffer Name Struct global_storage_buffers_##Name[]

#define RegisterUniformBuffer(Name, Struct) \
  layout(buffer_reference, scalar) readonly buffer Name Struct

#define RegisterStorageBuffer(Layout, Access, Name, Struct) \
  layout(buffer_reference, scalar) Access buffer Name Struct

// #define GetUniformBuffer(Name, Index) global_uniform_buffers_##Name[nonuniformEXT(Index)]
// #define GetStorageBuffer(Name, Index) global_storage_buffers_##Name[nonuniformEXT(Index)]

#define GetSampler2D(Index) global_samplers_2d[nonuniformEXT(Index)]
#define GetSamplerCube(Index) global_samplers_cube[nonuniformEXT(Index)]

// RegisterUniformBuffer(DummyUniform, { uint ignore; });
// RegisterStorageBuffer(std430, readonly, DummyBuffer, { uint ignore; });

layout(set = LIGER_DS, binding = LIGER_BINDING_SAMPLED_TEXTURE) uniform sampler2D   global_samplers_2d[];
// layout(set = LIGER_DS, binding = LIGER_BINDING_SAMPLED_TEXTURE) uniform samplerCube global_samplers_cube[];

layout(r32f, set = LIGER_DS, binding = LIGER_BINDING_STORAGE_TEXTURE) writeonly uniform image2D global_images_2d[];

/************************************************************************************************
 * Source
 ************************************************************************************************/
)====="
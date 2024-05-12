R"=====(#version 450

#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_EXT_nonuniform_qualifier : enable

/************************************************************************************************
 * Bindless resources setup
 ************************************************************************************************/
#define LIGER_DS 0

#define LIGER_BINDING_UNIFORM_BUFFER  0
#define LIGER_BINDING_STORAGE_BUFFER  1
#define LIGER_BINDING_SAMPLED_TEXTURE 2
#define LIGER_BINDING_STORAGE_TEXTURE 3

#define RegisterUniformBuffer(Name, Struct) \
  layout(set = LIGER_DS, binding = LIGER_BINDING_UNIFORM_BUFFER) uniform Name Struct global_uniform_buffers_##Name[]

#define RegisterStorageBuffer(Layout, Access, Name, Struct) \
  layout(Layout, set = LIGER_DS, binding = LIGER_BINDING_STORAGE_BUFFER) Access buffer Name Struct global_storage_buffers_##Name[]

#define GetUniformBuffer(Name, Index) global_uniform_buffers_##Name[nonuniformEXT(Index)]
#define GetStorageBuffer(Name, Index) global_storage_buffers_##Name[nonuniformEXT(Index)]

#define GetSampler2D(Index) global_samplers_2d[nonuniformEXT(Index)]
#define GetSamplerCube(Index) global_samplers_cube[nonuniformEXT(Index)]

RegisterUniformBuffer(DummyUniform, { uint ignore; });
RegisterStorageBuffer(std430, readonly, DummyBuffer, { uint ignore; });

layout(set = LIGER_DS, binding = LIGER_BINDING_SAMPLED_TEXTURE)       uniform sampler2D   global_samplers_2d[];
layout(set = LIGER_DS, binding = LIGER_BINDING_SAMPLED_TEXTURE)       uniform samplerCube global_samplers_cube[];

layout(set = LIGER_DS, binding = LIGER_BINDING_STORAGE_TEXTURE, r32f) uniform image2D     global_images_2d[];

/************************************************************************************************
 * Source
 ************************************************************************************************/
)====="
/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file test_rhi.cpp
 * @date 2024-02-11
 *
 * The MIT License (MIT)
 * Copyright (c) 2023 Nikita Mochalov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <fstream>
#include <iostream>
#include <liger/core/enum_reflection.hpp>
#include <liger/core/log/default_log.hpp>
#include <liger/core/platform/platform_layer.hpp>
#include <Liger-Engine/RHI/Instance.hpp>
#include <Liger-Engine/RHI/Shader_alignment.hpp>

struct ParticleSystem {
  /* Emission */
  SHADER_STRUCT_MEMBER(uint32_t)   max_particles          {2048};
  SHADER_STRUCT_MEMBER(float)      rate_over_time         {1.0f};  ///< How many particles are emmitted per second
  SHADER_STRUCT_MEMBER(float)      lifetime               {5.0f};

  SHADER_STRUCT_MEMBER(glm::vec3)  velocity_first         {0.0f, 0.0f, 0.0f};
  SHADER_STRUCT_MEMBER(glm::vec3)  velocity_second        {0.0f, 1.0f, 0.0f};

  /* Particle Shape */
  SHADER_STRUCT_MEMBER(glm::vec4)  color_start            {1.0f};
  SHADER_STRUCT_MEMBER(glm::vec4)  color_end              {1.0f};

  SHADER_STRUCT_MEMBER(float)      size_start             {1.0f};
  SHADER_STRUCT_MEMBER(float)      size_end               {1.0f};
};

struct Particle {
  SHADER_STRUCT_MEMBER(glm::vec3) position {0.0f};
  SHADER_STRUCT_MEMBER(glm::vec3) velocity {0.0f};
  SHADER_STRUCT_MEMBER(glm::vec4) color    {0.0f};
  SHADER_STRUCT_MEMBER(float)     size     {0.0f};
  SHADER_STRUCT_MEMBER(float)     time     {0.0f};
};

std::vector<uint32_t> LoadSpirvFile(std::string_view filename) {
  std::vector<uint32_t> output;

  std::ifstream file(filename.data(), std::ios::ate | std::ios::binary);
  assert(file.is_open());

  uint64_t file_size = file.tellg();
  output.resize(file_size / 4);

  file.seekg(0);
  file.read(reinterpret_cast<char*>(output.data()), file_size);

  file.close();

  return output;
}

bool g_running{true};
std::unique_ptr<liger::rhi::IGraphicsPipeline> g_forward_pipeline{nullptr};

bool OnWindowClose(const liger::WindowCloseEvent& event) {
  LIGER_LOG_INFO("Test RHI", "OnWindowClose: window = {0}", (void*)event.window);
  g_running = false;
  return true;
}

std::unique_ptr<liger::rhi::RenderGraph> DeclareGraph(liger::rhi::IDevice&                            device,
                                                      liger::rhi::RenderGraphBuilder&                 builder,
                                                      liger::rhi::RenderGraphBuilder::ResourceVersion rg_color) {
  liger::rhi::RenderGraph::DependentTextureInfo depth_info{};
  depth_info.extent.SetDependency(rg_color);
  depth_info.format          = liger::rhi::Format::kD32_SFLOAT;
  depth_info.type            = liger::rhi::TextureType::kTexture2D;
  depth_info.usage           = liger::rhi::DeviceResourceState::kDepthStencilTarget;
  depth_info.cube_compatible = false;
  depth_info.mip_levels      = 1;
  depth_info.samples         = 1;
  depth_info.name            = "Depth buffer";

  auto rg_depth = builder.DeclareTransientTexture(depth_info);

  builder.BeginRenderPass("Forward Pass");
  builder.AddColorTarget(rg_color, liger::rhi::AttachmentLoad::kClear, liger::rhi::AttachmentStore::kStore);
  builder.SetDepthStencil(rg_depth, liger::rhi::AttachmentLoad::kClear, liger::rhi::AttachmentStore::kDiscard);
  builder.EndRenderPass();

  return builder.Build(device, "Test RHI");
}

void SetupGraphJobs(liger::rhi::IDevice& device, liger::rhi::RenderGraph& graph, liger::rhi::Format color_format) {
  auto fwd_vert_spirv = LoadSpirvFile("assets/.liger/shaders/spirv/forward_pass.vert.spv");
  auto fwd_frag_spirv = LoadSpirvFile("assets/.liger/shaders/spirv/forward_pass.frag.spv");

  auto fwd_vs = device.CreateShaderModule(liger::rhi::IShaderModule::Source {
    .type          = liger::rhi::IShaderModule::Type::kVertex,
    .source_binary = fwd_vert_spirv
  });

  auto fwd_fs = device.CreateShaderModule(liger::rhi::IShaderModule::Source {
    .type          = liger::rhi::IShaderModule::Type::kFragment,
    .source_binary = fwd_frag_spirv
  });

  const liger::rhi::IShaderModule* fwd_modules[] = {fwd_vs.get(), fwd_fs.get()};

  liger::rhi::IGraphicsPipeline::Info forward_pipeline_info {};
  forward_pipeline_info.shader_modules                    = fwd_modules;
  forward_pipeline_info.attachments.render_target_formats = std::span(&color_format, 1);
  forward_pipeline_info.attachments.depth_stencil_format  = liger::rhi::Format::kD32_SFLOAT;

  g_forward_pipeline = device.CreatePipeline(forward_pipeline_info);

  graph.SetJob("Forward Pass", [&](liger::rhi::ICommandBuffer& cmds) {
    cmds.BindPipeline(g_forward_pipeline.get());
    cmds.Draw(3);
  });
}

int main() {
  auto rhi_instance = liger::rhi::IInstance::Create(liger::rhi::GraphicsAPI::kVulkan,
                                                    liger::rhi::IInstance::ValidationLevel::kExtensive);

  std::string str_devices_list;
  for (auto& device_info : rhi_instance->GetDeviceInfoList()) {
    str_devices_list += fmt::format(
        "    - [id={0}] \"{1}\", type={2}, engine_supported={3}, max_msaa={4}, max_sampler_anisotropy={5}\n",
        device_info.id, device_info.name, liger::EnumToString(device_info.type), device_info.engine_supported,
        device_info.properties.max_msaa_samples, device_info.properties.max_sampler_anisotropy);
  }

  LIGER_LOG_INFO("TestRHI", "Devices available:\n{0}", str_devices_list);

  auto device = rhi_instance->CreateDevice(rhi_instance->GetDeviceInfoList()[0].id, 2);

  /* Test Render Graph */
//   auto graph_builder = device->NewRenderGraphBuilder();

//   auto particle_buffer = graph_builder.DeclareTransientBuffer(liger::rhi::IBuffer::Info {
//     .size        = 4096,
//     .usage       = liger::rhi::DeviceResourceState::kStorageBuffer | liger::rhi::DeviceResourceState::kUniformBuffer,
//     .cpu_visible = false,
//     .name        = "Particle data"
//   });

//   auto color = graph_builder.DeclareTransientTexture(liger::rhi::ITexture::Info {
//     .format          = liger::rhi::Format::kR8G8B8A8_SRGB,
//     .type            = liger::rhi::TextureType::kTexture2D,
//     .usage           = liger::rhi::DeviceResourceState::kColorTarget,
//     .cube_compatible = false,
//     .extent          = {.x = 1920, .y = 1080, .z = 1},
//     .mip_levels      = 1,
//     .samples         = 1,
//     .name            = "Backbuffer",
//   });

//   auto depth = graph_builder.DeclareTransientTexture(liger::rhi::ITexture::Info {
//     .format          = liger::rhi::Format::kD32_SFLOAT,
//     .type            = liger::rhi::TextureType::kTexture2D,
//     .usage           = liger::rhi::DeviceResourceState::kDepthStencilTarget,
//     .cube_compatible = false,
//     .extent          = {.x = 1920, .y = 1080, .z = 1},
//     .mip_levels      = 1,
//     .samples         = 1,
//     .name            = "Depth buffer",
//   });

//   graph_builder.BeginCompute("Particle Update", true);
//   graph_builder.WriteBuffer(particle_buffer, liger::rhi::DeviceResourceState::kStorageBuffer);
//   graph_builder.EndCompute();

//   graph_builder.BeginRenderPass("Forward Pass");
//   auto next_color = graph_builder.AddColorTarget(color);
//   auto next_depth = graph_builder.SetDepthStencil(depth);
//   graph_builder.EndRenderPass();

//   graph_builder.BeginRenderPass("Forward Pass: particles");
//   graph_builder.AddColorTarget(next_color);
//   graph_builder.SetDepthStencil(next_depth);
//   graph_builder.ReadBuffer(particle_buffer, liger::rhi::DeviceResourceState::kStorageBuffer);
//   graph_builder.EndRenderPass();

  liger::EventDispatcher event_dispatcher;
  liger::PlatformLayer platform_layer(event_dispatcher);

  platform_layer.GetSink<liger::WindowCloseEvent>().Connect<&OnWindowClose>();

  auto window = platform_layer.CreateWindow(1280, 720, "Liger Test RHI");

  auto swapchain = device->CreateSwapchain(liger::rhi::ISwapchain::Info {
    .window   = window.get(),
    .min_size = 3,
    .vsync    = true,
    .usage    = liger::rhi::DeviceResourceState::kColorTarget,
    .name     = "Swapchain"
  });

  auto swapchain_textures = swapchain->GetTextures();

  auto graph_builder = device->NewRenderGraphBuilder();
  auto rg_color      = graph_builder.DeclareImportTexture(liger::rhi::DeviceResourceState::kUndefined,
                                                          liger::rhi::DeviceResourceState::kPresentTexture);

  auto graph = DeclareGraph(*device, graph_builder, rg_color);
  SetupGraphJobs(*device, *graph, swapchain_textures[0]->GetInfo().format);

  auto first_frame = true;

  while (g_running) {
    platform_layer.PollEvents();

    auto texture_idx = device->BeginFrame(*swapchain);
    if (!texture_idx) {
      swapchain->Recreate();
      swapchain_textures = swapchain->GetTextures();
      continue;
    }

    graph->ReimportTexture(rg_color, {.texture = swapchain_textures[*texture_idx]});
    device->ExecuteConsecutive(*graph);

    if (!device->EndFrame()) {
      swapchain->Recreate();
      swapchain_textures = swapchain->GetTextures();
      continue;
    }

    if (first_frame) {
      graph->DumpGraphviz(".liger_log/vulkan_render_graph.dot");
      first_frame = false;
    }
  }

  device->WaitIdle();

  g_forward_pipeline.reset();

  return 0;
}
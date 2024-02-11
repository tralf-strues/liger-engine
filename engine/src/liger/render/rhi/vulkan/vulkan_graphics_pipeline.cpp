/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file vulkan_graphics_pipeline.cpp
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

#include <liger/render/rhi/vulkan/vulkan_graphics_pipeline.hpp>

#include <liger/render/rhi/vulkan/vulkan_shader_module.hpp>

namespace liger::rhi {

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VkDevice vk_device) : vk_device_(vk_device) {}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline() {
  if (vk_pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(vk_device_, vk_pipeline_, nullptr);
    vk_pipeline_ = VK_NULL_HANDLE;
  }

  if (vk_layout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(vk_device_, vk_layout_, nullptr);
    vk_layout_ = VK_NULL_HANDLE;
  }
}

bool VulkanGraphicsPipeline::Init(const Info& info) {
  /* Pipeline layout */

  // TODO(tralf-strues): Descriptor set layout after bindless resources are added

  const bool push_constant_present = info.push_constant.size > 0;

  const VkPushConstantRange push_constant_range {
    .stageFlags = GetVulkanShaderStageFlags(info.push_constant.shader_types),
    .offset     = 0,
    .size       = info.push_constant.size
  };

  const VkPipelineLayoutCreateInfo layout_info {
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext                  = nullptr,
    .flags                  = 0,
    .setLayoutCount         = 0, // TODO(tralf-strues): Bindless resources!
    .pSetLayouts            = nullptr, // TODO(tralf-strues): Bindless resources!
    .pushConstantRangeCount = push_constant_present ? 1U : 0U,
    .pPushConstantRanges    = push_constant_present ? &push_constant_range : nullptr
  };

  VULKAN_CALL(vkCreatePipelineLayout(vk_device_, &layout_info, nullptr, &vk_layout_));


  /* Shader stages */
  std::vector<VkPipelineShaderStageCreateInfo> vk_stages_info{info.shader_modules.size()};
  for (uint32_t i = 0; i < info.shader_modules.size(); ++i) {
    auto shader_module = dynamic_cast<const VulkanShaderModule*>(info.shader_modules[i]);

    VkPipelineShaderStageCreateInfo vk_stage_create_info{};
    vk_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vk_stage_create_info.stage =
        static_cast<VkShaderStageFlagBits>(GetVulkanShaderStageFlags(shader_module->GetType()));
    vk_stage_create_info.module = shader_module->GetVulkanHandle();
    vk_stage_create_info.pName  = "main";
    vk_stage_create_info.pSpecializationInfo =
        nullptr;  // TODO(tralf-strues): Add specialization constants to Vulkan RHI

    vk_stages_info[i] = std::move(vk_stage_create_info);
  }

  /* Vertex input state */
  VkPipelineVertexInputStateCreateInfo vk_vertex_input_info{};
  std::vector<VkVertexInputBindingDescription> vk_binding_descriptions;
  std::vector<VkVertexInputAttributeDescription> vk_attribute_descriptions;

  if (!info.input_assembly.vertex_info.bindings.empty()) {
    for (const auto& binding_info : info.input_assembly.vertex_info.bindings) {
      VkVertexInputBindingDescription vk_binding_description{};
      vk_binding_description.binding   = binding_info.binding;
      vk_binding_description.stride    = binding_info.stride;
      vk_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      
      vk_binding_descriptions.emplace_back(std::move(vk_binding_description));

      for (const auto& attribute_info : binding_info.attributes) {
        VkVertexInputAttributeDescription vk_attribute_description{};
        vk_attribute_description.binding  = binding_info.binding;
        vk_attribute_description.location = attribute_info.location;
        vk_attribute_description.format   = GetVulkanFormat(attribute_info.format);
        vk_attribute_description.offset   = attribute_info.offset;

        vk_attribute_descriptions.emplace_back(std::move(vk_attribute_description));
      }
    }

    vk_vertex_input_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vk_vertex_input_info.vertexBindingDescriptionCount   = static_cast<uint32_t>(vk_binding_descriptions.size());
    vk_vertex_input_info.pVertexBindingDescriptions      = vk_binding_descriptions.data();
    vk_vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vk_attribute_descriptions.size());
    vk_vertex_input_info.pVertexAttributeDescriptions    = vk_attribute_descriptions.data();
  } else {
    vk_vertex_input_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vk_vertex_input_info.vertexBindingDescriptionCount   = 0;
    vk_vertex_input_info.pVertexBindingDescriptions      = nullptr;
    vk_vertex_input_info.vertexAttributeDescriptionCount = 0;
    vk_vertex_input_info.pVertexAttributeDescriptions    = nullptr;
  }

  /* Input assembly state */
  VkPipelineInputAssemblyStateCreateInfo vk_input_assembly_info{};
  vk_input_assembly_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  vk_input_assembly_info.topology               = GetVulkanPrimitiveTopology(info.input_assembly.topology);
  vk_input_assembly_info.primitiveRestartEnable = VK_FALSE;

  /* Dynamic states */
  const std::vector<VkDynamicState> vk_dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo vk_dynamic_state_info{};
  vk_dynamic_state_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  vk_dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(vk_dynamic_states.size());
  vk_dynamic_state_info.pDynamicStates    = vk_dynamic_states.data();

  /* Viewport and scissors */
  VkViewport vk_viewport{};
  vk_viewport.x        = 0.0f;
  vk_viewport.y        = 0.0f;
  vk_viewport.width    = 0.0f;
  vk_viewport.height   = 0.0f;
  vk_viewport.minDepth = 0.0f;
  vk_viewport.maxDepth = 1.0f;

  VkRect2D vk_scissor{};
  vk_scissor.offset = {0, 0};
  vk_scissor.extent = {0, 0};

  VkPipelineViewportStateCreateInfo vk_viewport_state_info{};
  vk_viewport_state_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  vk_viewport_state_info.viewportCount = 1;  // > 1 requires VkPhysicalDeviceFeatures::multiViewport to be true
  vk_viewport_state_info.scissorCount  = 1;  // > 1 requires VkPhysicalDeviceFeatures::multiViewport to be true
  vk_viewport_state_info.pViewports    = &vk_viewport;
  vk_viewport_state_info.pScissors     = &vk_scissor;

  /* Rasterization state */
  VkPipelineRasterizationStateCreateInfo vk_rasterizer_info{};
  vk_rasterizer_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  // Disable any output to the framebuffer
  vk_rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
  // LINE and POINT require ?some feature?
  vk_rasterizer_info.polygonMode             = GetVulkanPolygonMode(info.rasterization.polygon_mode);
  // > 1 requires VkPhysicalDeviceFeatures::wideLines
  vk_rasterizer_info.lineWidth               = 1.0f;
  vk_rasterizer_info.cullMode                = GetVulkanCullMode(info.rasterization.cull_mode);
  vk_rasterizer_info.frontFace               = GetVulkanFrontFace(info.rasterization.front_face);
  // Used for shadow mapping
  vk_rasterizer_info.depthBiasEnable         = VK_FALSE;
  vk_rasterizer_info.depthBiasConstantFactor = 0.0f;
  vk_rasterizer_info.depthBiasClamp          = 0.0f;
  vk_rasterizer_info.depthBiasSlopeFactor    = 0.0f;

  // Clamping fragments outside near/far planes instead of discarding (used for shadow mapping),
  // requires VkPhysicalDeviceFeatures::depthClamp to be true
  vk_rasterizer_info.depthClampEnable        = VK_FALSE;

  /* Multisampling (requires ?some feature?) */
  VkPipelineMultisampleStateCreateInfo vk_multisampling_info{};
  vk_multisampling_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  vk_multisampling_info.sampleShadingEnable   = VK_FALSE;
  vk_multisampling_info.rasterizationSamples  = static_cast<VkSampleCountFlagBits>(info.attachments.samples);
  vk_multisampling_info.minSampleShading      = 1.0f;
  vk_multisampling_info.pSampleMask           = nullptr;
  vk_multisampling_info.alphaToCoverageEnable = VK_FALSE;
  vk_multisampling_info.alphaToOneEnable      = VK_FALSE;

  /* Depth and stencil testing */
  VkPipelineDepthStencilStateCreateInfo vk_depth_stencil_info{};
  vk_depth_stencil_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  vk_depth_stencil_info.depthTestEnable       = static_cast<VkBool32>(info.depth_stencil_test.depth_test_enable);
  vk_depth_stencil_info.depthWriteEnable      = static_cast<VkBool32>(info.depth_stencil_test.depth_write_enable);
  vk_depth_stencil_info.depthCompareOp        = GetVulkanCompareOp(info.depth_stencil_test.depth_compare_operation);
  vk_depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
  vk_depth_stencil_info.minDepthBounds        = 0.0f;
  vk_depth_stencil_info.maxDepthBounds        = 1.0f;
  vk_depth_stencil_info.stencilTestEnable     = VK_FALSE; // TODO(tralf-strues): Stencil test
  vk_depth_stencil_info.front                 = {};
  vk_depth_stencil_info.back                  = {};

  /* Color blending */
  VkPipelineColorBlendAttachmentState vk_blend_attachment_state{};  // Created for each color attachment
  vk_blend_attachment_state.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  vk_blend_attachment_state.blendEnable         = static_cast<VkBool32>(info.blend.enable);
  vk_blend_attachment_state.srcColorBlendFactor = GetVulkanBlendFactor(info.blend.src_color_factor);
  vk_blend_attachment_state.dstColorBlendFactor = GetVulkanBlendFactor(info.blend.dst_color_factor);
  vk_blend_attachment_state.colorBlendOp        = GetVulkanBlendOp(info.blend.color_operation);
  vk_blend_attachment_state.srcAlphaBlendFactor = GetVulkanBlendFactor(info.blend.src_alpha_factor);
  vk_blend_attachment_state.dstAlphaBlendFactor = GetVulkanBlendFactor(info.blend.dst_alpha_factor);
  vk_blend_attachment_state.alphaBlendOp        = GetVulkanBlendOp(info.blend.alpha_operation);

  std::vector<VkPipelineColorBlendAttachmentState> vk_blend_attachment_states{
      info.attachments.render_target_formats.size(), vk_blend_attachment_state};

  VkPipelineColorBlendStateCreateInfo vk_color_blend_info{};
  vk_color_blend_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  vk_color_blend_info.logicOpEnable     = VK_FALSE;
  vk_color_blend_info.logicOp           = VK_LOGIC_OP_COPY;
  vk_color_blend_info.attachmentCount   = static_cast<uint32_t>(vk_blend_attachment_states.size());
  vk_color_blend_info.pAttachments      = vk_blend_attachment_states.data();
  vk_color_blend_info.blendConstants[0] = 0.0f;
  vk_color_blend_info.blendConstants[1] = 0.0f;
  vk_color_blend_info.blendConstants[2] = 0.0f;
  vk_color_blend_info.blendConstants[3] = 0.0f;

  /* Attachment Info */
  std::vector<VkFormat> vk_color_attachment_formats;
  vk_color_attachment_formats.reserve(info.attachments.render_target_formats.size());

  for (auto format : info.attachments.render_target_formats) {
    vk_color_attachment_formats.emplace_back(GetVulkanFormat(format));
  }

  const VkPipelineRenderingCreateInfo vk_pipeline_rendering {
    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .pNext                   = nullptr,
    .viewMask                = 0,
    .colorAttachmentCount    = static_cast<uint32_t>(vk_color_attachment_formats.size()),
    .pColorAttachmentFormats = vk_color_attachment_formats.data(),
    .depthAttachmentFormat   = GetVulkanFormat(info.attachments.depth_stencil_format),
    .stencilAttachmentFormat = GetVulkanFormat(info.attachments.depth_stencil_format)
  };

  /* Graphics pipeline */
  VkGraphicsPipelineCreateInfo vk_pipeline_info{};
  vk_pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  vk_pipeline_info.pNext               = &vk_pipeline_rendering;

  vk_pipeline_info.stageCount          = static_cast<uint32_t>(vk_stages_info.size());
  vk_pipeline_info.pStages             = vk_stages_info.data();

  vk_pipeline_info.pVertexInputState   = &vk_vertex_input_info;
  vk_pipeline_info.pInputAssemblyState = &vk_input_assembly_info;
  vk_pipeline_info.pViewportState      = &vk_viewport_state_info;
  vk_pipeline_info.pRasterizationState = &vk_rasterizer_info;
  vk_pipeline_info.pMultisampleState   = &vk_multisampling_info;
  vk_pipeline_info.pDepthStencilState  = &vk_depth_stencil_info;
  vk_pipeline_info.pColorBlendState    = &vk_color_blend_info;
  vk_pipeline_info.pDynamicState       = &vk_dynamic_state_info;

  vk_pipeline_info.layout              = vk_layout_;

  vk_pipeline_info.renderPass          = VK_NULL_HANDLE;
  vk_pipeline_info.subpass             = 0;

  // These are used only if VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is set
  vk_pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;
  vk_pipeline_info.basePipelineIndex   = -1;

  // TODO(tralf-strues): Add pipeline cache!
  VULKAN_CALL(vkCreateGraphicsPipelines(vk_device_, VK_NULL_HANDLE, 1, &vk_pipeline_info, nullptr, &vk_pipeline_));

  return true;
}

VkPipeline VulkanGraphicsPipeline::GetVulkanHandle() {
  return vk_pipeline_;
}

}  // namespace liger::rhi
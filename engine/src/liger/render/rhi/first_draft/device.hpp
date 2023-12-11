/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file render_device.hpp
 * @date 2023-09-26
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

#pragma once

#include <liger/core/platform/window.hpp>
#include <liger/render/rhi/buffer.hpp>
#include <liger/render/rhi/command_queue.hpp>
#include <liger/render/rhi/compute_pipeline.hpp>
#include <liger/render/rhi/device_info.hpp>
#include <liger/render/rhi/framebuffer.hpp>
#include <liger/render/rhi/graphics_pipeline.hpp>
#include <liger/render/rhi/render_pass.hpp>
#include <liger/render/rhi/sampler.hpp>
#include <liger/render/rhi/shader_module.hpp>
#include <liger/render/rhi/surface.hpp>
#include <liger/render/rhi/swapchain.hpp>
#include <liger/render/rhi/synchronization.hpp>
#include <liger/render/rhi/texture.hpp>

#include <span>

namespace liger::rhi {

class IDevice {
 public:
  virtual ~IDevice() = default;
  
  IDevice(const IDevice& other) = delete;
  IDevice& operator=(const IDevice& other) = delete;

  IDevice(IDevice&& other) = delete;
  IDevice& operator=(IDevice&& other) = delete;

  virtual const DeviceInfo& GetInfo() const = 0;

  /************************************************************************************************
   * SYNCHRONIZATION
   ************************************************************************************************/
  [[nodiscard]] virtual Fence CreateFence(bool device_only = true) = 0;

  virtual void WaitForFences(std::span<const Fence> fences) = 0;
  virtual void ResetFence(const Fence& fence) = 0;

  /************************************************************************************************
   * SWAPCHAIN
   ************************************************************************************************/
  [[nodiscard]] virtual Surface CreateSurface(Window* window) = 0;

  [[nodiscard]] virtual Swapchain CreateSwapchain(const Swapchain::Info& info) = 0;

  /**
   * @brief Get swapchain textures.
   * 
   * @note Textures are deleted automatically when the swapchain is deleted.
   * 
   * @param swapchain
   * @param textures
   */
  virtual void GetSwapchainTextures(const Swapchain& swapchain, std::span<Texture> textures) = 0;

  /**
   * @brief Get next texture from the swapchain.
   * 
   * @note The function does not wait for the acquiring to finish, it returns right away and sets the correct value to
   *       texture_idx. Synchronization primitives are provided for handling concurrent usages.
   * 
   * @param swapchain 
   * @param texture_idx 
   * @param signal_semaphore 
   * @param signal_fence
   * 
   * @return Whether swapchain is up to date and DOESN'T need recreating.
   */
  [[nodiscard]] virtual bool AcquireNextTexture(const Swapchain& swapchain,
                                                uint32& texture_idx,
                                                const Semaphore* signal_semaphore = nullptr,
                                                const Fence*     signal_fence     = nullptr) = 0;

  /**
   * @brief Present rendered window surface.
   * 
   * @param present_queue Must contain CommandTypes::kPresent.
   * @param swapchain
   * @param wait_semaphore
   * 
   * @return Whether swapchain is up to date and DOESN'T need recreating.
   */
  [[nodiscard]] virtual bool Present(const CommandQueue& present_queue,
                                     const Swapchain& swapchain,
                                     std::span<const Semaphore> wait_semaphores) = 0;

  /**
   * @brief Recreates the swapchain.
   * @note After recreating the swapchain, one should retrieve swapchain textures once more.
   * 
   * @param swapchain
   */
  virtual void RecreateSwapchain(Swapchain& swapchain) = 0;

  /************************************************************************************************
   * TEXTURE AND SAMPLER
   ************************************************************************************************/
  /**
   * @brief Create a texture and the default texture view (set to kTextureDefaultViewIdx).
   * 
   * @param info
   * @return Created texture.
   */
  [[nodiscard]] virtual Texture CreateTexture(const Texture::Info& info) = 0;

  /**
   * @brief Create a texture view.
   * 
   * @note Indices are sequential, the default view is always created and has index 0. Other views
   *       for the texture are assigned indices starting from 1 and incremented sequentially.
   * 
   * @param texture 
   * @param view
   * 
   * @return View idx, which can be used for bindings or different commands.
   */
  [[nodiscard]] virtual uint32 CreateTextureView(const Texture& texture, const TextureViewInfo& view) = 0;

  /**
   * @brief Get the binding index of the texture's view for accessing inside shaders.
   * 
   * @param texture
   * @param view_idx
   * 
   * @return View's binding index.
   */
  [[nodiscard]] virtual uint32 GetTextureViewBinding(const Texture& texture, uint32 view_idx = kTextureDefaultViewIdx) = 0;

  [[nodiscard]] virtual Sampler CreateSampler(const Sampler::Info& info) = 0;

  /************************************************************************************************
   * BUFFER
   ************************************************************************************************/
  [[nodiscard]] virtual Buffer CreateBuffer(const Buffer::Info& info) = 0;

  virtual void MapBuffer(Buffer& buffer, uint32 offset, uint32 size) = 0;

  /**
   * @brief Get the binding index of the buffer for accessing inside shaders.
   * 
   * @param buffer
   * 
   * @return Buffer's binding index.
   */
  [[nodiscard]] virtual uint32 GetBufferBinding(const Buffer& buffer) = 0;

  /************************************************************************************************
   * RENDER PASS
   ************************************************************************************************/
  [[nodiscard]] virtual RenderPass CreateRenderPass(const RenderPass::Info& info) = 0;

  [[nodiscard]] virtual Framebuffer CreateFramebuffer(const Framebuffer::Info& info) = 0;

  /************************************************************************************************
   * PIPELINES
   ************************************************************************************************/
  [[nodiscard]] virtual ShaderModule CreateShaderModule(const ShaderModuleBinary& binary) = 0;

  [[nodiscard]] virtual GraphicsPipeline CreateGraphicsPipeline(const GraphicsPipeline::Info& info) = 0;
  [[nodiscard]] virtual ComputePipeline CreateComputePipeline(const ComputePipeline::Info& info) = 0;

  /************************************************************************************************
   * COMMAND QUEUE / COMMAND LIST
   ************************************************************************************************/
  [[nodiscard]] virtual CommandQueue CreateCommandQueue(const CommandQueue::Info& info) = 0;

  virtual void CreateCommandBuffers(const CommandQueue& queue, bool temporary = false) = 0;
  virtual void DeleteCommandList(CommandListHandle command_list) = 0;

  struct SubmitDescription {
    uint32             list_count;
    CommandListHandle* lists;

    uint32             signal_semaphore_count;
    SemaphoreHandle*   signal_semaphores;

    uint32             wait_semaphore_count;
    SemaphoreHandle*   wait_semaphores;
    PipelineStages*    wait_stages;

    FenceHandle        signal_fence;
  };

  virtual void BeginCommandList(CommandListHandle command_list) = 0;
  virtual void EndCommandList(CommandListHandle command_list) = 0;
  virtual void SubmitCommandLists(CommandQueueHandle queue, const SubmitDescription& submit_description) = 0;

  virtual void ResetCommandList(CommandListHandle command_list) = 0;

  /************************************************************************************************
   * TRANSFER / BARRIER COMMANDS
   ************************************************************************************************/
  virtual void GenerateMipLevels(TextureHandle texture, TextureLayout final_layout) = 0;

  virtual void TransitionLayout(TextureHandle texture, TextureLayout old_layout, TextureLayout new_layout) = 0;

  virtual void SetBufferBarrier(BufferHandle buffer, MemoryAccessDependencies src_access,
                                MemoryAccessDependencies dst_access, PipelineStages src_stages,
                                PipelineStages dst_stages, uint32 offset, uint32 size) = 0;

  virtual void CopyBuffer(BufferHandle src_buffer, BufferHandle dst_buffer, uint32 size, uint32 src_offset = 0,
                          uint32 dst_offset = 0) = 0;

  /**
   * @brief Copy data from the buffer to the texture.
   * 
   * @param buffer 
   * @param texture 
   * @param width 
   * @param height
   * @param start_layer Start layer to write to (for regular 2D images it is always 0, for cube maps can be 0..5)
   * @param layer_count How many layers to copy
   * 
   * @warning Texture must be in either @ref{TextureLayout::kTransferDst} or @ref{TextureLayout::kGeneral} layouts.
   */
  virtual void CopyBufferToTexture(BufferHandle buffer, TextureHandle texture, uint32 width, uint32 height,
                                   uint32 start_layer = 0, uint32 layer_count = 1) = 0;

  /**
   * @brief Copy data from the texture to the buffer.
   * 
   * @param texture 
   * @param buffer 
   * @param width 
   * @param height
   * @param start_layer Layer to write to (for regular 2D images it is always 0, for cube maps can be from 0 to 5)
   * @param layer_count How many layers to copy
   * 
   * @warning Texture must be in either @ref{TextureLayout::kTransferSrc} or @ref{TextureLayout::kGeneral} layouts.
   */
  virtual void CopyTextureToBuffer(TextureHandle texture, BufferHandle buffer, uint32 width, uint32 height,
                                   uint32 start_layer = 0, uint32 layer_count = 1) = 0;

  /**
   * @brief Copy data from the src to the dst texture.
   * 
   * @param src_texture 
   * @param dst_texture 
   * @param width 
   * @param height 
   * 
   * @warning src_texture must be in either @ref{TextureLayout::kTransferSrc} or @ref{TextureLayout::kGeneral} layouts.
   * @warning dst_texture must be in either @ref{TextureLayout::kTransferDst} or @ref{TextureLayout::kGeneral} layouts.
   */
  virtual void CopyTexture(TextureHandle src_texture, TextureHandle dst_texture, uint32 width, uint32 height) = 0;

  
};

}  // namespace liger::rhi
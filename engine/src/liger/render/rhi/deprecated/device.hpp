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
#include <liger/render/rhi/descriptor.hpp>
#include <liger/render/rhi/device_info.hpp>
#include <liger/render/rhi/graphics_pipeline.hpp>
#include <liger/render/rhi/handle.hpp>
#include <liger/render/rhi/render_pass.hpp>
#include <liger/render/rhi/texture.hpp>

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
  [[nodiscard]] virtual FenceHandle CreateFence() = 0;
  virtual void DeleteFence(FenceHandle fence) = 0;

  virtual void WaitForFences(uint32 count, const FenceHandle* fences) = 0;
  virtual void ResetFence(FenceHandle fence) = 0;

  [[nodiscard]] virtual SemaphoreHandle CreateSemaphore() = 0;
  virtual void DeleteSemaphore(SemaphoreHandle semaphore) = 0;

  /************************************************************************************************
   * SWAPCHAIN
   ************************************************************************************************/
  [[nodiscard]] virtual WindowSurfaceHandle CreateWindowSurface(Window* window) = 0;
  virtual void DeleteWindowSurface(WindowSurfaceHandle window_surface) = 0;

  [[nodiscard]] virtual SwapchainHandle CreateSwapchain(WindowSurfaceHandle window_surface, TextureUsage usage) = 0;
  virtual void DeleteSwapchain(SwapchainHandle swapchain) = 0;

  /**
   * @brief Get swapchain textures.
   * 
   * @note Supposed to be called twice - first to retrieve the count and then textures themselves.
   * @note Textures are deleted automatically when the swapchain is deleted.
   * 
   * @param swapchain
   * @param textures_count Can be nullptr.
   * @param textures       Can be nullptr.
   */
  virtual void GetSwapchainTextures(SwapchainHandle swapchain, uint32* textures_count, TextureHandle* textures) = 0;

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
   * @return true If swapchain is up to date and DOESN'T need recreating.
   */
  [[nodiscard]] virtual bool AcquireNextTexture(SwapchainHandle swapchain, uint32* texture_idx,
                                                SemaphoreHandle signal_semaphore = kInvalidHandle,
                                                FenceHandle signal_fence = kInvalidHandle) = 0;

  /**
   * @brief Present rendered window surface.
   * 
   * @param swapchain
   * @param wait_semaphore
   * 
   * @return true If swapchain is up to date and DOESN'T need recreating.
   */
  [[nodiscard]] virtual bool Present(SwapchainHandle swapchain, SemaphoreHandle wait_semaphore) = 0;

  /**
   * @brief Recreates the swapchain.
   * 
   * @warning One must not delete the old swapchain! It is handled automatically by the function.
   * @note    After recreating the swapchain, one should retrieve swapchain textures once more.
   * 
   * @param swapchain Old swapchain handle.
   * 
   * @return SwapchainHandle New swapchain handle, can be the same as the old swapchain handle.
   */
  [[nodiscard]] virtual SwapchainHandle RecreateSwapchain(SwapchainHandle swapchain) = 0;

  /************************************************************************************************
   * TEXTURE AND SAMPLER
   ************************************************************************************************/
  [[nodiscard]] virtual TextureHandle CreateTexture(const TextureDescription& description) = 0;
  virtual void DeleteTexture(TextureHandle texture) = 0;

  virtual const TextureDescription& GetTextureDescription(TextureHandle texture) = 0;

  virtual uint32 CreateSubresource(TextureHandle texture, const TextureSubresourceDescription& description) = 0;
  virtual void DeleteSubresource(TextureHandle texture, uint32 subresource) = 0;

  [[nodiscard]] virtual SamplerHandle CreateSampler(const SamplerDescription& description) = 0;
  virtual void DeleteSampler(SamplerHandle sampler) = 0;

  virtual const SamplerDescription& GetSamplerDescription(SamplerHandle sampler) = 0;

  /************************************************************************************************
   * BUFFER
   ************************************************************************************************/
  /**
   * @brief Create a buffer with the specified size and usage.
   * 
   * @param size        Buffer's size in bytes.
   * @param usage       Bit mask specifying how the buffer can be used.
   * @param cpu_visible Whether buffer's memory is visible from CPU. Affects performance! Use it with caution!
   * @param map_data    If cpu_visible, then returns the mapped data pointer.
   *
   * @return BufferHandle
   */
  [[nodiscard]] virtual BufferHandle CreateBuffer(uint32 size, BufferUsage usage, bool cpu_visible,
                                                  void** map_data) = 0;

  BufferHandle CreateDynamicUniformBuffer(void** map_data, uint32 size) {
    return CreateBuffer(size, BufferUsageBit::kUniformBuffer, true, map_data);
  }

  BufferHandle CreateStaticUniformBuffer(uint32 size) {
    return CreateBuffer(size, BufferUsageBit::kUniformBuffer, false , nullptr);
  }

  template <typename T>
  BufferHandle CreateDynamicUniformBuffer(T** map_data, uint32 count = 1) {
    return CreateDynamicUniformBuffer(reinterpret_cast<void**>(map_data), count * sizeof(T));
  }

  template <typename T>
  BufferHandle CreateStaticUniformBuffer(uint32 count = 1) {
    return CreateStaticUniformBuffer(count * sizeof(T));
  }

  BufferHandle CreateDynamicStorageBuffer(void** map_data, uint32 size) {
    return CreateBuffer(size, BufferUsageBit::kStorageBuffer, true, map_data);
  }

  BufferHandle CreateStaticStorageBuffer(uint32 size) {
    return CreateBuffer(size, BufferUsageBit::kStorageBuffer, false , nullptr);
  }

  template <typename T>
  BufferHandle CreateDynamicStorageBuffer(T** map_data, uint32 count = 1) {
    return CreateDynamicStorageBuffer(reinterpret_cast<void**>(map_data), count * sizeof(T));
  }

  template <typename T>
  BufferHandle CreateStaticStorageBuffer(uint32 count = 1) {
    return CreateStaticStorageBuffer(count * sizeof(T));
  }

  virtual void DeleteBuffer(BufferHandle buffer) = 0;

    /**
   * @brief Update region of the buffer's memory.
   * 
   * @param buffer 
   * @param offset 
   * @param size 
   * @param data 
   */
  virtual void LoadBufferData(BufferHandle buffer, uint32 offset, uint32 size, const void* data) = 0;

  template <typename T>
  void LoadBufferData(BufferHandle buffer, uint32 offset_idx, uint32 count, const T* data) {
    LoadBufferData(buffer, offset_idx * sizeof(T), count * sizeof(T), reinterpret_cast<const void*>(data));
  }

  /**
   * @brief Invalidate dynamic buffer's memory.
   * @note General usage of dynamic buffers is
   *       1. InvalidateBufferMemory()
   *       2. Fill/Update the map_data
   *       3. FlushBufferMemory()
   * 
   * @param buffer
   * @param offset
   * @param size
   */
  virtual void InvalidateBufferMemory(BufferHandle buffer, uint32 offset, uint32 size) = 0;

  template <typename T>
  void InvalidateBufferMemory(BufferHandle buffer, uint32 offset_idx = 0, uint32 count = 1) {
    InvalidateBufferMemory(buffer, offset_idx * sizeof(T), count * sizeof(T));
  }

  /**
   * @brief Flush dynamic buffer's memory.
   * @note General usage of dynamic buffers is
   *       1. InvalidateBufferMemory()
   *       2. Fill/Update the map_data
   *       3. FlushBufferMemory()
   * 
   * @param buffer
   * @param offset
   * @param size
   */
  virtual void FlushBufferMemory(BufferHandle buffer, uint32 offset, uint32 size) = 0;

  template <typename T>
  void FlushBufferMemory(BufferHandle buffer, uint32 offset_idx = 0, uint32 count = 1) {
    FlushBufferMemory(buffer, offset_idx * sizeof(T), count * sizeof(T));
  }

  /************************************************************************************************
   * DESCRIPTOR SET
   ************************************************************************************************/
  virtual DescriptorSetLayoutHandle CreateDescriptorSetLayout(const DescriptorSetLayoutDescription& description) = 0;
  virtual void DeleteDescriptorSetLayout(DescriptorSetLayoutHandle layout) = 0;

  virtual DescriptorSetHandle CreateDescriptorSet(DescriptorSetLayoutHandle layout) = 0;
  virtual void DeleteDescriptorSet(DescriptorSetHandle descriptor_set) = 0;

  virtual void WriteDescriptorUniformBuffer(DescriptorSetHandle descriptor_set, uint32 binding_idx,
                                            BufferHandle uniform_buffer, uint32 offset, uint32 size) = 0;
  virtual void WriteDescriptorStorageBuffer(DescriptorSetHandle descriptor_set, uint32 binding_idx,
                                            BufferHandle storage_buffer, uint32 offset, uint32 size) = 0;

  virtual void WriteDescriptorSampler(DescriptorSetHandle descriptor_set, uint32 binding_idx, TextureHandle texture,
                                      SamplerHandle sampler) = 0;

  /************************************************************************************************
   * RENDER PASS
   ************************************************************************************************/
  virtual RenderPassHandle CreateRenderPass(const RenderPassDescription& description) = 0;
  virtual void DeleteRenderPass(RenderPassHandle render_pass) = 0;

  virtual FramebufferHandle CreateFramebuffer(const FramebufferDescription& description,
                                              RenderPassHandle compatible_render_pass) = 0;
  virtual void DeleteFramebuffer(FramebufferHandle framebuffer) = 0;

  /************************************************************************************************
   * PIPELINE
   ************************************************************************************************/
  virtual ShaderModuleHandle CreateShaderModule(ShaderModuleType type, uint32 size, const uint32* binary) = 0;
  virtual void DeleteShaderModule(ShaderModuleHandle shader_module) = 0;

  virtual PipelineHandle CreateGraphicsPipeline(const GraphicsPipelineDescription& description,
                                                RenderPassHandle compatible_render_pass) = 0;
  virtual PipelineHandle CreateComputePipeline(const ComputePipelineDescription& description) = 0;

  virtual void DeletePipeline(PipelineHandle pipeline) = 0;

  /************************************************************************************************
   * COMMAND QUEUE / COMMAND LIST
   ************************************************************************************************/
  virtual CommandQueueHandle CreateCommandQueue(CommandQueueUsage usage) = 0;
  virtual void DeleteCommandQueue(CommandQueueHandle queue) = 0;

  virtual CommandListHandle CreateCommandList(CommandQueueHandle queue, bool temporary = false) = 0;
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
/**
 * @author Nikita Mochalov (github.com/tralf-strues)
 * @file Device.hpp
 * @date 2023-12-10
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

#include <Liger-Engine/RHI/Buffer.hpp>
#include <Liger-Engine/RHI/Pipeline.hpp>
#include <Liger-Engine/RHI/RenderGraph.hpp>
#include <Liger-Engine/RHI/ShaderModule.hpp>
#include <Liger-Engine/RHI/Swapchain.hpp>

#include <string>

namespace liger::rhi {

/**
 * @brief Logical device, an interface for working with a physical device (e.g. GPU).
 */
class IDevice {
 public:
  /**
   * @brief Type of the device.
   */
  enum class Type : uint8_t {
    Undefined,
    IntegratedGPU,
    DiscreteGPU,
    VirtualGPU,
    CPU,
  };

  /**
   * @brief Properties, features and limits of the device.
   */
  struct Properties {
    uint8_t max_msaa_samples;
    float   max_sampler_anisotropy;
  };

  /**
   * @brief Device info, which can be used to identify the device needed.
   */
  struct Info {
    uint32_t    id;
    std::string name;
    Type        type;
    bool        engine_supported;
    Properties  properties;
  };

  using TransferCallback = std::function<void()>;

  struct DedicatedBufferTransfer {
    IBuffer*                   buffer;
    DeviceResourceState        final_state;
    std::unique_ptr<uint8_t[]> data;
    uint64_t                   size;
  };

  struct DedicatedTextureTransfer {
    ITexture*                  texture;
    DeviceResourceState        final_state;
    std::unique_ptr<uint8_t[]> data;
    uint64_t                   size;
    bool                       gen_mips{false};
    Filter                     gen_mips_filter{Filter::Linear};
  };

  struct DedicatedTransferRequest {
    std::list<DedicatedBufferTransfer>  buffer_transfers;
    std::list<DedicatedTextureTransfer> texture_transfers;
    TransferCallback                    callback;
  };

  virtual ~IDevice() = default;

  /**
   * @brief Get the device info, which is exactly the same as the corresponding info returned
   * by @ref IInstance::GetDeviceInfoList method.
   *
   * @return Device info.
   */
  virtual const Info& GetInfo() const = 0;

  /**
   * @brief Get the number of frames in flight the device is configured to work with.
   * @note This number is set upon creating a device via @ref IInstance::CreateDevice.
   * @return The number of frames in flight.
   */
  virtual uint32_t GetFramesInFlight() const = 0;

  /**
   * @brief Wait for any pending device work to finish.
   */
  virtual void WaitIdle() = 0;

  /**
   * @brief Begin a frame with the specified swapchain as the main target if it is valid.
   * @param swapchain
   * @return Index of the swapchain texture for this frame or std::nullopt if swapchain recreation is needed.
   */
  [[nodiscard]] virtual std::optional<uint32_t> BeginFrame(ISwapchain& swapchain) = 0;

  /**
   * @brief End the frame and present to screen (with the swapchain specified in @ref BeginFrame method).
   *
   * @return Whether successfully submitted the frame or false if swapchain recreation is needed.
   */
  [[nodiscard]] virtual bool EndFrame() = 0;

  /**
   * @brief Begin an offscreen frame, i.e. without rendering and presenting to screen.
   */
  virtual void BeginOffscreenFrame() = 0;

  /**
   * @brief End the offscreen frame.
   */
  virtual void EndOffscreenFrame() = 0;

  /**
   * @brief Get the current frame index in range 0...FIF-1.
   * @warning Calling this method outside of begin and end frame scope (either default or offscreen) can cause UB!
   * @return Current frame in flight index.
   */
  [[nodiscard]] virtual uint32_t CurrentFrame() const = 0;

  /**
   * @brief Get the current absolute frame index.
   * @warning Calling this method outside of begin and end frame scope (either default or offscreen) can cause UB!
   * @return Current absolute frame index.
   */
  [[nodiscard]] virtual uint64_t CurrentAbsoluteFrame() const = 0;

  /**
   * @brief Execute the render graph and synchronize it with previous (if any) render graphs during current frame.
   *
   * @warning Calling this method outside of begin and end frame scope (either default or offscreen) can cause UB!
   *
   * @param render_graph Render graph to execute, which must be created by this particular device.
   * @param context      Context data for render jobs to "communicate" with each other.
   */
  virtual void ExecuteConsecutive(RenderGraph& render_graph, Context& context) = 0;

  virtual void RequestDedicatedTransfer(DedicatedTransferRequest&& transfer) = 0;

  /**
   * @brief Create a render graph builder, the object for constructing a render graph.
   * @return Render graph builder.
   */
  [[nodiscard]] virtual RenderGraphBuilder NewRenderGraphBuilder(Context& context) = 0;

  [[nodiscard]] virtual std::unique_ptr<ISwapchain> CreateSwapchain(const ISwapchain::Info& info) = 0;
  [[nodiscard]] virtual std::unique_ptr<ITexture> CreateTexture(const ITexture::Info& info) = 0;
  [[nodiscard]] virtual std::unique_ptr<IBuffer> CreateBuffer(const IBuffer::Info& info) = 0;
  [[nodiscard]] virtual std::unique_ptr<IShaderModule> CreateShaderModule(const IShaderModule::Source& source) = 0;
  [[nodiscard]] virtual std::unique_ptr<IPipeline> CreatePipeline(const IPipeline::ComputeInfo& info) = 0;
  [[nodiscard]] virtual std::unique_ptr<IPipeline> CreatePipeline(const IPipeline::GraphicsInfo& info) = 0;
};

}  // namespace liger::rhi
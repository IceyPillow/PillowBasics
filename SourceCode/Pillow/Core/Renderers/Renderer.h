// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 2-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include <ranges>
#include <thread>
#include <barrier>
#include <atomic>
#include <vector>
#include <functional>
#include "Core/Auxiliaries.h"
#include "Core/Constants.h"
#include "Core/Resources/Texture.h"
#include "Core/Resources/Mesh.h"

using namespace Pillow::Graphics;
using namespace DirectX;

namespace Pillow::Graphics
{
   // Perceptual weightings for the importance of each channel.
   const XMVECTOR RGBLuminance = XMVectorSet(0.2125f / 0.7154f, 1, 0.0721f / 0.7154f, 1);
   const XMVECTOR RGBLuminanceInv = XMVectorSet(0.7154f / 0.2125f, 1, 0.7154f / 0.0721f, 1);

   extern int32_t RefreshRate;
   extern XMINT2 ScreenSize;
   class GenericRenderer;
   extern std::unique_ptr<GenericRenderer> Instance;

   typedef uint32_t ResHandle; // 28 bits for index, 4 bits for type

   constexpr ResHandle NullResHandle = 0;
   constexpr uint32_t ResourceTypeMask = 0xF << 28;

   enum class LightType : uint8_t
   {
      Directional,
      Point,
      Spot,
      AreaDisc,
      AreaSphere,
      AreaRectangle
   };

   // DIRECT3D12 VIEW TYPES
   // IN  DESCRIPTOR HEAP： RTV DSV CSV SRV UAV Sampler
   // OUT DESCRIPTOR HEAP： VBV IBV SOV
   //
   // DIRECT3D12 RESOURCE HEAP TYPES
   // Upload Default Readback Custom

   enum class ResourceType : uint32_t
   {
      None = 0,
      Mesh = 0X1 << 28,
      PiplelineState = 0X2 << 28,
      RenderTargetView = 0X3 << 28,
      DepthStencilVIew = 0X4 << 28,
      ShaderResourceView = 0X5 << 28,
      ConstantBufferView = 0X6 << 28,
      UnorderedAccessView = 0X7 << 28,
      ReadbackBuffer = 0X8U << 28,
   };

   // Those resources have no handles, client should refer to them by this enum type.
   enum class PiplelineBuffer : uint8_t
   {
      None,
      // Swap chain
      Backbuffer,
      // For half-resolution post-process effects
      PostProcessingHalf,
      PostProcessingHalfOld,
      // Lighting pass of the deferred pipeline
      LightBuffer,
      LightBufferOld,
      // G-buffer pass of the deferred pipeline
      Depth,
      DepthOld,
      MotionVector,
      MotionVectorOld,
      GBuffer1,
      GBuffer2,
      GBuffer3,
      Count = GBuffer3,
   };

   struct GenericRendererResourceDesc
   {
      ResourceType Type;
      union
      {
         //std::weak_ptr<GenericTextureInfo> TextureInfo;
         //MeshDesc Mesh;
         //TextureDesc Texture;
         //PipelineStateDesc PipelineState;
         //ConstantBufferDesc ConstantBuffer;
      };
   };

   // Designed for a modifiable deferred pipeline.
   struct GenericRendererCommand
   {
      enum class Type : uint8_t
      {
         None,
         // Clear commands
         ClearPiplelineBuffers,
         ClearRenderTargets,
         ClearDepthStencils,
         // Set commands
         SetPiplelineBuffers,
         SetRenderTargets,
         SetActiveCamera,
         SetViewport,
         SetLight,
         // Bind commands
         // Resource barriers are implicitly applied when executing bind commands.
         // We don't want GenericRenderer to expose explicit resource barriers. (CS term: encapsulation)
         BindPipelineState,
         BindShaderResourceViews,
         BindConstantBufferViews,
         BindUnorderedAccessViews,
         // Dispatch commands
         DispatchMesh,
         DispatchPostProcess,
         DispatchCompute
      };

      Type CmdType;
      uint8_t Flags8;
      int16_t Count;

      union UnionParams
      {
         XMFLOAT4X4 Matrix;
         struct
         {
            XMFLOAT4 Float4_1, Float4_2;
            uint32_t UIntArray8[8];
         };
      } Params;
   };
   static_assert(std::is_trivially_copyable_v<GenericRendererCommand>); //POD test

   class GenericPipelineConfig
   {
   public:
      enum class TopologyType : uint8_t
      {
         TriangleList, // vertex buffer + index buffer
         TriangleStrip // only vertex buffer
      };

   public:
      string ConfigName;
      std::vector<KeyValuePair> Macros;
      std::vector<string> VSTextures;
      std::vector<string> PSTextures;
      std::vector<string> ConstantBuffers;
      int32_t RenderTargetCount;
      TopologyType Topology;

      inline GenericPipelineConfig() : ConfigName("NullConfig") {/*dumb*/};

      // Example
      // ConfigName: SimpleShader@CheckOn@Quality=2
      GenericPipelineConfig(string name, const std::vector<KeyValuePair>& macros,
         const std::vector<string>& cbv, const std::vector<string>& vsTex, const std::vector<string>& psTex, int32_t rtNum, TopologyType topology);

      bool EqualTo(const GenericPipelineConfig& right) const;
   };

   class GenericRenderer
   {
      DeleteDefautedMethods(GenericRenderer)
         ReadonlyProperty(string, RendererName)
         ReadonlyProperty(int32_t, ThreadCount)

   public:
      virtual ~GenericRenderer() = 0;
      virtual uint64_t GetFrameIndex() = 0;
      ForceInline int32_t GetFrameArrayIdx() { return GetFrameIndex() % Constants::SwapChainSize; }
      inline virtual void ResourceRegister(ResHandle& handle, ResourceType type, const void* desc) {/*dumb*/};
      inline virtual void ResourceRelease(ResHandle handle) {/*dumb*/};
      void Launch();
      void Terminate();
      void Commit();

   protected:
      GenericRenderer(int32_t threadCount, string name);
      virtual void Worker(int32_t workerIndex) = 0;
      virtual void Pioneer() = 0;
      virtual void Assembler() = 0;

   private:
      void BaseWorker(std::stop_token token, int32_t workerIndex);
      friend static void BarrierCompletionAction() noexcept;
   };

#if defined(_WIN64)
   class D3D12Renderer final: public GenericRenderer
   {
      DeleteDefautedMethods(D3D12Renderer)

   public:
      D3D12Renderer(HWND windowHandle, int32_t threadCount);
      ~D3D12Renderer();
      uint64_t GetFrameIndex();

   private:
      void Worker(int32_t workerIndex);
      void Pioneer();
      void Assembler();
   };
#elif defined(__ANDROID__)
   //class GLES32Renderer : public GenericRenderer
   //{
   //   DeleteDefautedMethods(GLES32Renderer)
   //public:
   //   GLES32Renderer(HWND windowHandle);
   //   ~GLES32Renderer();
   //   int32_t CreateMesh() override;
   //   int32_t CreateTexture() override;
   //   int32_t CreatePiplelineState() override;
   //   int32_t CreateConstantBuffer() override;
   //   void ReleaseResource(int32_t handle) override;
   //   void CPUFrameBegin() override;
   //   void CPUFrameEnd() override;
   //};
#endif

   ForceInline void InitializeRenderer(int32_t threadCount, const void* parameter)
   {
      if (Instance) throw std::runtime_error("Renderer has already been initialized.");
#if defined(_WIN64)
      HWND hwnd = *(const HWND*)parameter;
      Instance = std::make_unique<Graphics::D3D12Renderer>(hwnd, threadCount);
#elif defined(__ANDROID__)
      //RendererInstance = std::make_unique<Pillow::GLES32Renderer>(Hwnd, 2);
#endif
   }

   ForceInline ResourceType GetResourceType(ResHandle handle) { return ResourceType(handle & ResourceTypeMask); }

   ForceInline bool CheckHandle(ResHandle handle) { return handle != 0; }

   // Create an empty command.
   ForceInline GenericRendererCommand CmdNone()
   {
      return GenericRendererCommand{/*empty*/};
   }

   // Clear 1~8 built-in pipeline buffers. 
   // depth, stencil: When clearing depth or stencil, they must be specified, otherwise they will be ignored.
   ForceInline GenericRendererCommand CmdClearPiplelineBuffers(PiplelineBuffer builtinBuffers[], int32_t count,
      const XMFLOAT4& color = Constants::CleanColor, float depth = Constants::FloatInfinity, uint8_t stencil = UINT8_MAX)
   {
      if (count > 8) throw std::runtime_error("Too many pipeline buffers to clear. Max is 8.");
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::ClearPiplelineBuffers;
      cmd.Count = count;
      for (int32_t i : std::views::iota(0, count))
      {
         cmd.Params.UIntArray8[i] = static_cast<uint32_t>(builtinBuffers[i]);
      }
      cmd.Params.Float4_1 = color;
      cmd.Params.Float4_2.x = depth;
      cmd.Flags8 = stencil;
      return cmd;
   }

   // Clear 1~8 render targets. (1 cubemap = 6 buffers)
   ForceInline GenericRendererCommand CmdClearRenderTargets(ResHandle handles[], int32_t count,
      const XMFLOAT4& color = Constants::CleanColor)
   {
      if (count > 8) throw std::runtime_error("Too many render targets to clear. Max is 8.");
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::ClearRenderTargets;
      cmd.Count = count;
      for (int32_t i : std::views::iota(0, count))
      {
         cmd.Params.UIntArray8[i] = handles[i];
      }
      cmd.Params.Float4_1 = color;
      return cmd;
   }

   // Clear 1~8 depth-stencil buffers. 
   // depth, stencil: When clearing depth or stencil, they must be specified, otherwise they will be ignored.
   ForceInline GenericRendererCommand CmdClearDepthStencils(ResHandle handles[], int32_t count,
      float depth = Constants::FloatInfinity, uint8_t stencil = UINT8_MAX)
   {
      if (count > 8) throw std::runtime_error("Too many depth-stencil buffers to clear. Max is 8.");
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::ClearDepthStencils;
      cmd.Count = count;
      for (int32_t i : std::views::iota(0, count))
      {
         cmd.Params.UIntArray8[i] = handles[i];
      }
      cmd.Params.Float4_1.x = depth;
      cmd.Flags8 = stencil;
      return cmd;
   }

   // Set 1~8 built-in pipeline buffers.
   ForceInline GenericRendererCommand CmdSetPipelineBuffers(ResHandle handles[], int32_t count)
   {
      if (count > 8) throw std::runtime_error("Too many pipeline buffers to set. Max is 8.");
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::SetPiplelineBuffers;
      cmd.Count = count;
      for (int32_t i : std::views::iota(0, count))
      {
         cmd.Params.UIntArray8[i] = handles[i];
      }
      return cmd;
   }

   //  Set 1~7 render targets and 1 depth buffer.
   ForceInline GenericRendererCommand CmdSetRenderTargets(ResHandle handles[], int32_t count, ResHandle depthHandle = NullResHandle)
   {
      if (count > 7) throw std::runtime_error("Too many render targets to set. Max is 7.");
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::SetRenderTargets;
      cmd.Count = count;
      for (int32_t i : std::views::iota(0, count))
      {
         cmd.Params.UIntArray8[i] = handles[i];
      }
      cmd.Params.UIntArray8[7] = depthHandle;
      return cmd;
   }

   // Set the active camera VP matrix.
   ForceInline GenericRendererCommand CmdSetActiveCamera(const XMFLOAT4X4& vpMatrix)
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::SetActiveCamera;
      cmd.Params.Matrix = vpMatrix;
      return cmd;
   }

   // Set the active viewport.
   ForceInline GenericRendererCommand CmdSetViewport(const XMFLOAT4& viewport)
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::SetViewport;
      cmd.Params.Float4_1 = viewport;
      return cmd;
   }

   // Set a light source.
   ForceInline GenericRendererCommand CmdSetLight(LightType type, const XMFLOAT4& quaternion,
      const XMFLOAT4& color,bool hasShadow, float intensity, float range, float size1, float size2, float size3)
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::SetLight;
      cmd.Flags8 = uint8_t(type) | (hasShadow ? 0X80 : 0);
      XMFLOAT4* vec1 = reinterpret_cast<XMFLOAT4*>(cmd.Params.Matrix.m);
      vec1[0] = quaternion;
      vec1[1] = color;
      vec1[2] = XMFLOAT4(intensity, range, 0, 0);
      vec1[3] = XMFLOAT4(size1, size2, size3, 0);
      return cmd;
   }

   // Set the active pipeline state object.
   ForceInline GenericRendererCommand CmdBindPipelineStates(ResHandle psoHandle)
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::BindPipelineState;
      cmd.Params.UIntArray8[0] = psoHandle;
      return cmd;
   }

   // Set 1~4 shader resources.
   ForceInline GenericRendererCommand CmdBindShaderResourceViews(int32_t rootParamIndex[], ResHandle handles[], int32_t count)
   {
      if (count > 4) throw std::runtime_error("Too many shader resource views to bind. Max is 4.");
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::BindShaderResourceViews;
      cmd.Count = count;
      for (int32_t i : std::views::iota(0, count))
      {
         cmd.Params.UIntArray8[i] = rootParamIndex[i];
         cmd.Params.UIntArray8[i + 4] = handles[i];
      }
      return cmd;
   }

   // Set 1~4 constant buffers.
   ForceInline GenericRendererCommand CmdBindConstantBufferViews(int32_t rootParamIndex[], ResHandle handles[], int32_t count)
   {
      if (count > 4) throw std::runtime_error("Too many constant buffer views to bind. Max is 4.");
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::BindConstantBufferViews;
      cmd.Count = count;
      for (int32_t i : std::views::iota(0, count))
      {
         cmd.Params.UIntArray8[i] = rootParamIndex[i];
         cmd.Params.UIntArray8[i + 4] = handles[i];
      }
      return cmd;
   }
   // Set 1~4 unordered access buffers.
   ForceInline GenericRendererCommand CmdBindUnorderedAccessViews(int32_t rootParamIndex[], ResHandle handles[], int32_t count)
   {
      if (count > 4) throw std::runtime_error("Too many unordered access views to bind. Max is 4.");
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::BindConstantBufferViews;
      cmd.Count = count;
      for (int32_t i : std::views::iota(0, count))
      {
         cmd.Params.UIntArray8[i] = rootParamIndex[i];
         cmd.Params.UIntArray8[i + 4] = handles[i];
      }
      return cmd;
   }

   // Dispatch an instanced (GPU Instancing) draw call.
   ForceInline GenericRendererCommand CmdDispatchMesh(ResHandle meshHandle, int32_t instanceCount)
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::DispatchMesh;
      cmd.Params.UIntArray8[0] = meshHandle;
      cmd.Params.UIntArray8[1] = instanceCount;
      return cmd;
   }

   // Dispatch a post-processing pass.
   ForceInline GenericRendererCommand CmdDispatchPostProcess(PiplelineBuffer from, PiplelineBuffer to)
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::DispatchPostProcess;
      cmd.Params.UIntArray8[0] = static_cast<uint32_t>(from);
      cmd.Params.UIntArray8[1] = static_cast<uint32_t>(to);
      return cmd;
   }

   // Dispatch a compute shader.
   ForceInline GenericRendererCommand CmdDispatchCompute()
   {
      /*dumb*/
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::DispatchCompute;
      return cmd;
   }
}

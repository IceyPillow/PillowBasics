// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include <cmath>
#include <ranges>
#include <vector>
#include <functional>
#include "Common.h"
#include "Resources/Texture.h"
#include "Resources/Mesh.h"

using namespace DirectX;
using namespace Pillow::Constants;
using namespace Pillow::Common;

namespace Pillow::Graphics
{
   const int32_t ReservedCommandCount = 20000; // 68B*20000 = nearly 1.3MB

   // Resource handle, index starts at 1.
   // 4-bit type + 28-bit index
   using ResHandle = uint32_t;
   const ResHandle ResHandleNULL = 0;
   const ResHandle ResIndexBits = 28;
   const ResHandle ResourceTypeMask = 0xF0000000;

   struct alignas(XMVECTOR) ObjectConstantBuffer
   {
      XMFLOAT3X4A MatrixModel;
      XMFLOAT3X4A MatrixModelInvTrans;
      XMFLOAT4A ColorObject;
      XMUINT4 TexIdx;
      // 16B
      uint32_t SamplerIdx;
      uint32_t InstanceNum;
      float Metallic;
      float Smoothness;
   };

   struct alignas(XMVECTOR) LightConstantBuffer
   {
      // 16B
      XMFLOAT3 PositionWorld;
      uint32_t TypeLight;
      // 16B
      XMFLOAT3 DirectionWorld;
      float Intensity;
      // 16B
      XMFLOAT3 ColorLight;
      float RangeMax;
      // 16B
      XMFLOAT4A ShapeLight;
   };

   struct alignas(XMVECTOR) CameraConstantBuffer
   {
      XMFLOAT3X4A MatrixView;
      XMFLOAT4X4A MatrixProjection;
      XMFLOAT4X4A MatrixViewProjection;
      XMFLOAT4X4A MatrixViewProjectionInv;
      XMFLOAT4A ViewportSizeAndRecip;
      // 16B
      XMFLOAT3 CameraPositionWorld;
      uint32_t FrameArrayIdx;
      // 16B
      float DistanceNear;
      float DistanceFar;
      float TimeDelta;
      float TimeLapse;
   };

   enum class LightType : uint8_t
   {
      Directional,
      Point,
      Spot,
      AreaDisc,
      AreaSphere,
      AreaRectangle
   };

   enum class PipelineBuffer : uint8_t
   {
      BackBuffer,
      Depth,
      MotionVector,
      // (TextureArray) General-usage render buffers.
      Buffer1,
      Buffer2,
      Buffer3,
      Buffer4,
      HalfBuffer1,
      HalfBuffer2,
      HalfBuffer3,
      HalfBuffer4,
      Count,
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
      Mesh = 1 << ResIndexBits,
      PiplelineState = 2 << ResIndexBits,
      RenderTargetView = 3 << ResIndexBits,
      DepthStencilVIew = 4 << ResIndexBits,
      ShaderResourceView = 5 << ResIndexBits,
      ConstantBufferView = 6 << ResIndexBits,
      UnorderedAccessView = 7 << ResIndexBits,
      ReadbackBuffer = 8u << ResIndexBits,
      Count = 8
   };

   // Those resources have no handles, client should refer to them by this enum type.
   enum class PiplelineBuffer : uint8_t
   {
      Backbuffer,
      Depth,
      MotionVector,
      // Geometry buffer for deferred shading.
      GBuffer1,
      GBuffer2,
      // Pixel Buffer for post-processing.
      PBuffer1,
      PBuffer2,
      PHalfBuffer1,
      PHalfBuffer2,
      Count
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

   // Programmers call it pipeline state, and artists call it material. They are essentially the same thing.
   // * Utilize a unified root signature to achieve a modern bindless architecture.
   class IPipelineState
   {
   public:
      // To draw a mass of grass, GPU instancing (with triangle input) is better than using geometry shader (with point input).
      // Hence, the point topology is removed.
      enum class TopologyType : uint8_t
      {
         TriangleList, // vertex buffer + index buffer
         TriangleStrip // only vertex buffer
      };

      enum class CullMode : uint8_t
      {
         KeepFrontPrimitives,
         KeepAllPrimitives
      };

      // How to handle the depth buffer.
      enum class DepthMode : uint8_t
      {
         ZTest_and_ZWrite, // Perform z-tests and write to the z-buffer.
         ZTest, // Only perform z-tests, don't modify the z-buffer.
         Disabled // Disable z-buffer, write to render targets forcely.
      };

      // How to blend and write colors in a render target.
      enum class BlendMode : uint8_t
      {
         OverWrite, // (Aka. Opaque) No blending, write to render targets forcely.
         Accumulation, // (Aka. Additive) Accumulate color values.
         AlphaBlend, // (Aka. Straight Alpha) Blend based on the alpha channel.
         AlphaToCoverage, // Use alpha channel as coverage for multisampling.
      };

      enum class ShaderType : uint16_t
      {
         // Generic computation
         ComputeShader = 0x001,
         // Mesh shading pipeline (not supported currently)
         AmplificationShader = 0x002,
         MeshShader = 0x004,
         // Geometry pipeline
         VertexShader = 0x008,
         HullShader = 0x010,
         DomainShader = 0x020,
         GeometryShader = 0x040,
         PixelShader = 0x080,
         Count = 8
      };

      static inline const std::string ShaderAcronyms[uint32_t(ShaderType::Count)] =
      {
         "CS",
         "AS",
         "MS",
         "VS",
         "HS",
         "DS",
         "GS",
         "PS"
      };

      // C++20 doesn't have reflections.
      static inline const std::string EntryPoints[uint32_t(ShaderType::Count)] =
      {
         "ComputeShader",
         "AmplificationShader",
         "MeshShader",
         "VertexShader",
         "HullShader",
         "DomainShader",
         "GeometryShader",
         "PixelShader"
      };

      struct Description
      {
         // VS+PS = 0x08 | 0x80 = 0x0088; CS = 0x0001, etc. (bit flags)
         uint16_t ShaderMask;
         TopologyType Topology;
         CullMode Cull;
         VertexType Vertex;
         DepthMode Depth;
         BlendMode Blend;
         uint8_t RTNum;
         TextureInfo::Format RT_Formats[8];
      };

   public:
      // Example: NameID = "HelloWorld@Stages=VS,PS@Depth=0@Blend=0@ASSERT_ON@Quality=2"
      const std::string NameID;
      const std::unique_ptr<std::vector<KeyValuePair>> Macros;
      const Description Desc;

      IPipelineState() = delete;
      IPipelineState(string originalName, std::vector<KeyValuePair>& macros, Description& desc);
      bool EqualTo(const IPipelineState& right) const;

      inline static uint16_t CreateShaderMask(std::initializer_list<ShaderType> shaderTypes)
      {
         uint16_t mask = 0;
         for (ShaderType type : shaderTypes)
         {
            mask |= uint16_t(type);
         }
         return mask;
      }

      inline static bool CheckShaderMask(uint16_t shaderMask, ShaderType type)
      {
         return (shaderMask & uint16_t(type)) != 0;
      }
   };

   // Generic renderer, abstract class (resembles an interface in C#).
   class IRenderer
   {
      DeleteDefautedMethods(IRenderer)
         ReadonlyProperty(string, RendererName)
         ReadonlyProperty(int32_t, ThreadCount)
         ReadonlyProperty(XMINT2, BackBufferSize)
         ReadonlyProperty(int32_t, RefreshRate)
         ReadonlyProperty(int32_t, VSyncBlanks)

   public:
      static void Initialize(uint32_t threadCount, XMINT2 backBufferSize, int32_t refreshRate, void* parameter);
      static IRenderer* GetInstance();

   public:
      virtual ~IRenderer() = 0;

      virtual uint64_t GetFrameIndex() = 0;

      uint32_t GetFrameArrayIdx()
      {
         return GetFrameIndex() % Constants::SwapChainSize;
      }

      float GetEstimatedMaxFrameRate()
      {
         if (f_VSyncBlanks != 0)
         {
            return std::ceilf(float(f_RefreshRate) / float(f_VSyncBlanks));
         }
         else
         {
            return 1e6f; // Represents an uncapped frame rate.
         }
      }

      void SetRenderBufferSize(XMINT2 size) { f_BackBufferSize = size; }
      void SetRefreshRate(int32_t rate) { f_RefreshRate = rate; }
      void SetVSyncBlanks(int32_t blanks) { f_VSyncBlanks = blanks; }

      virtual void ResourceRegister(ResHandle handle, ResourceType type, const void* desc) {/*dumb*/ }
      virtual void ResourceRelease(ResHandle handle) {/*dumb*/ }
      void Launch() { threadPool.Launch(); }
      void Terminate() { threadPool.Terminate(); }
      // ***CORE WORKLOAD***
      // Commit a frame, or wait for the last CPU renderer frame.
      void Commit() { threadPool.Commit(); }
      // Get the idle generic command list.
      // Client code should never invoke it; instead, uses CmdXXX() (see: CmdNone()) to record commands.
      std::vector<GenericRendererCommand>* GetIdleCmdList();

   protected:
      IRenderer(uint32_t threadCount, string name, XMINT2 renderBufferSize, int32_t refreshRate);
      virtual void Worker(uint32_t workerIndex) = 0;
      void BasePioneer();
      virtual void Pioneer() = 0;
      virtual void Assembler() = 0;
      // Get the busy generic command list, provided for a renderer backend.
      const std::vector<GenericRendererCommand>* GetBusyCmdList();

   private:
      using ActionT = void(IRenderer::*)();
      using WorkerT = void(IRenderer::*)(uint32_t);
      ThreadPool<IRenderer, WorkerT, ActionT> threadPool;
   };

#if defined(_WIN64)
   class D3D12Renderer final: public IRenderer
   {
      DeleteDefautedMethods(D3D12Renderer)

   public:
      D3D12Renderer(void* windowHandle, int32_t threadCount, XMINT2 backBufferSize, int32_t refreshRate);
      ~D3D12Renderer();
      uint64_t GetFrameIndex() override final;

   private:
      void Worker(uint32_t workerIndex) override final;
      void Pioneer() override final;
      void Assembler() override final;
   };
#elif defined(__ANDROID__)
   //class Vulkan12Renderer : public GenericRenderer
   //{
   //   DeleteDefautedMethods(Vulkan12Renderer)
   //public:
   //   Vulkan12Renderer(HWND windowHandle);
   //   ~Vulkan12Renderer();
   //};
#endif

   inline ResourceType GetResourceType(ResHandle handle) { return ResourceType(handle & ResourceTypeMask); }

   // ResourceIndex starts at 1.
   inline uint32_t GetResourceIndex(ResHandle handle) { return handle & ~ResourceTypeMask; }

   inline bool CheckHandle(ResHandle handle) { return GetResourceIndex(handle) != 0; }


   // Create an empty command.
   ForceInline void CmdNone()
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::None;
      IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   }

   // Clear 1~8 built-in pipeline buffers. 
   // depth, stencil: When clearing depth or stencil, they must be specified, otherwise they will be ignored.
   ForceInline void CmdClearPiplelineBuffers(PiplelineBuffer builtinBuffers[], int32_t count,
      const XMFLOAT4& color = Constants::CleanColor, float depth = Constants::FloatInfinity,
      uint8_t stencil = UINT8_MAX)
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
      IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   }

   // Clear 1~8 render targets. (1 cubemap = 6 buffers)
   ForceInline void CmdClearRenderTargets(ResHandle handles[], int32_t count,
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
      IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   }

   // Clear 1~8 depth-stencil buffers. 
   // depth, stencil: When clearing depth or stencil, they must be specified, otherwise they will be ignored.
   ForceInline void CmdClearDepthStencils(ResHandle handles[], int32_t count,
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
      IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   }

   // Set 1~8 built-in pipeline buffers.
   ForceInline void CmdSetPipelineBuffers(PiplelineBuffer builtinBuffers[], int32_t count)
   {
      if (count > 8) throw std::runtime_error("Too many pipeline buffers to set. Max is 8.");
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::SetPiplelineBuffers;
      cmd.Count = count;
      for (int32_t i : std::views::iota(0, count))
      {
         cmd.Params.UIntArray8[i] = (uint32_t)builtinBuffers[i];
      }
      IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   }

   //  Set 1~7 render targets and 1 depth buffer.
   ForceInline void CmdSetRenderTargets(ResHandle handles[], int32_t count, ResHandle depthHandle = ResHandleNULL)
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
      IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   }

   // Set the active camera VP matrix.
   ForceInline void CmdSetActiveCamera(const XMFLOAT4X4& vpMatrix)
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::SetActiveCamera;
      cmd.Params.Matrix = vpMatrix;
      IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   }

   // Set the active viewport.
   ForceInline void CmdSetViewport(const XMFLOAT4& viewport)
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::SetViewport;
      cmd.Params.Float4_1 = viewport;
      IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   }

   // Set a light source.
   ForceInline void CmdSetLight(LightType type, const XMFLOAT4& quaternion,
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
      IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   }

   // Set the active pipeline state object.
   ForceInline void CmdBindPipelineStates(ResHandle psoHandle)
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::BindPipelineState;
      cmd.Params.UIntArray8[0] = psoHandle;
      IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   }

   // Set 1~4 shader resources.
   ForceInline void CmdBindShaderResourceViews(int32_t rootParamIndex[], ResHandle handles[], int32_t count)
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
      IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   }

   // Set 1~4 constant buffers.
   ForceInline void CmdBindConstantBufferViews(int32_t rootParamIndex[], ResHandle handles[], int32_t count)
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
      IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   }

   // Set 1~4 unordered access buffers.
   ForceInline void CmdBindUnorderedAccessViews(int32_t rootParamIndex[], ResHandle handles[], int32_t count)
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
      IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   }

   // Dispatch an instanced (GPU Instancing) draw call.
   ForceInline void CmdDispatchMesh(ResHandle meshHandle, int32_t instanceCount)
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::DispatchMesh;
      cmd.Params.UIntArray8[0] = meshHandle;
      cmd.Params.UIntArray8[1] = instanceCount;
      IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   }

   // Dispatch a post-processing pass.
   ForceInline void CmdDispatchPostProcess(PiplelineBuffer from, PiplelineBuffer to)
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::DispatchPostProcess;
      cmd.Params.UIntArray8[0] = static_cast<uint32_t>(from);
      cmd.Params.UIntArray8[1] = static_cast<uint32_t>(to);
      IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   }

   // Dispatch a compute shader.
   ForceInline void CmdDispatchCompute()
   {
      /*dumb*/
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::DispatchCompute;
      IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   }
}

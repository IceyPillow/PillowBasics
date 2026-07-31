// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include <cmath>
#include <ranges>
#include <vector>
#include <functional>
#include "Common.h"

using namespace DirectX;
using namespace Pillow::Constants;
using namespace Pillow::Common;

// Types
namespace Pillow::Graphics
{
   constexpr int32_t ReservedCommandCount = 20000; // 68B*20000 = nearly 1.3MB

   // Resource handle, index starts at 1.
   // 4-bit type + 28-bit index
   using ResHandle = uint32_t;
   constexpr ResHandle NullHandle = 0;
   constexpr ResHandle ResIndexBits = 28;
   constexpr ResHandle ResourceTypeMask = 0xF0000000;

   // Forward declarations
   class TextureInfo;
   enum class VertexType : uint8_t;
   enum class TextureFormat : uint8_t;
   enum class GraphicsResourceType : uint8_t;
   GraphicsResourceType GetResourceType(ResHandle handle);
   ResHandle SetRecourceType(ResHandle handle, GraphicsResourceType type);
   ResHandle ClearResourceType(ResHandle handle);
   bool CheckHandle(ResHandle handle);

   struct alignas(XMVECTOR) ObjectConstantBuffer
   {
      XMFLOAT3X4A MatrixModel;
      XMFLOAT3X4A MatrixModelInvTrans;
      XMFLOAT4A Color;
      XMUINT4 TexIdxA;
      XMUINT4 TexIdxB;
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
      uint32_t Type;
      // 16B
      XMFLOAT3 DirectionWorld;
      float Intensity;
      // 16B
      XMFLOAT3 Color;
      float RangeMax;
      // 16B
      XMFLOAT4A Shape;
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
      // (TexArray) general buffers.
      Buffer1,
      Buffer2,
      Buffer3,
      Buffer4,
      // (TexArray) Half-resoulution general buffers.
      HalfBuffer1,
      HalfBuffer2,
      HalfBuffer3,
      HalfBuffer4,
      Count,
      NoneSwapChainResNum = 4 // Depth + Motion + BufferArray + HalfBufferArray
   };

   // Programmers call it pipeline state, and artists call it material. They are essentially the same thing.
   // * Utilize a unified root signature to achieve a modern bindless architecture.
   class PipelineInfo
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

      struct Configuration
      {
         // VS+PS = 0x08 | 0x80 = 0x0088; CS = 0x0001, etc. (bit flags)
         uint16_t ShaderMask;
         TopologyType Topology;
         CullMode Cull;
         VertexType Vertex;
         DepthMode Depth;
         BlendMode Blend;
         TextureFormat RT_Formats[8];
         mutable uint8_t RTNum;
      };

   public:
      // Example: NameID = "HelloWorld@Stages=VS,PS@Depth=0@Blend=0@ASSERT_ON@Quality=2"
      const std::string ID;
      const path ShortPath;
      const std::vector<KeyValuePair> Macros;
      const Configuration Config;

      PipelineInfo(path shortPath, std::vector<KeyValuePair>& macros, Configuration& config);

      bool EqualTo(const PipelineInfo& right) const;

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

   // DIRECT3D12 VIEW TYPES
   // IN  DESCRIPTOR HEAP： RTV DSV CSV SRV UAV Sampler
   // OUT DESCRIPTOR HEAP： VBV IBV SOV
   //
   // DIRECT3D12 RESOURCE HEAP TYPES
   // Upload Default Readback Custom

   enum class GraphicsResourceType : uint8_t
   {
      PiplelineState,
      Texture,
      VertexBuffer,
      IndexBuffer,
      StructArray,
      ConstBuffer,
      Count
   };

   struct GraphicsResourceDesc
   {
      GraphicsResourceType Type;
      union
      {
         PipelineInfo* PipeState;
         TextureInfo* TexInfo;
         struct VtxIdxBuffer
         {
            VertexType VtxType;
            uint32_t Count;
         } VtxIdxBuffer;
         struct StructAndCB
         {
            uint32_t ElementSize;
            uint32_t ElementCount;
         } StructAndCB;
      };
   };

   // Designed for a modifiable deferred pipeline.
   struct alignas(HalfCacheLine) GenericRendererCommand
   {
      enum class Type : uint8_t
      {
         None,
         // Clear commands
         ClearPiplelineBuffer,
         ClearRenderTarget,
         ClearDepthStencil,
         // Set commands
         SetPiplelineBuffers,
         SetRenderTargets,
         SetActiveCamera,
         SetViewport,
         SetLight,
         // Bind commands
         // Resource barriers are implicitly applied when executing bind commands.
         // We don't want GenericRenderer to expose explicit resource barriers. (CS term: Encapsulation)
         BindPipelineState,
         BindShaderResourceView,
         BindConstantBufferView,
         BindUnorderedAccessView,
         // Dispatch commands
         DispatchMesh,
         DispatchPostProcess,
         DispatchCompute,
         Count
      };
      // 16B
      union
      {
         XMFLOAT4 Float4;
         XMUINT4 Int4;
      };
      // 16B
      XMINT3 Int3;
      uint8_t Byte3[3];
      Type CmdType;
   };
   static_assert(std::is_trivially_copyable_v<GenericRendererCommand>); //POD test

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
      const static uint32_t MaxRescourceNum = UINT16_MAX;
      // Do not access it directly. Instead, use CmdXXX() to record commands (see: CmdNone()).
      static inline std::vector<GenericRendererCommand> cmdListIdle{};

      static void Initialize(uint32_t threadCount, XMINT2 backBufferSize, int32_t refreshRate, void* parameter);
      static IRenderer* GetInstance();

      virtual ~IRenderer() = 0;

      virtual uint64_t GetFrameIndex() = 0;
      uint32_t GetFrameArrayIdx() { return GetFrameIndex() % Constants::SwapChainSize; }
      float GetEstimatedMaxFrameRate() { return f_VSyncBlanks != 0 ? std::ceilf(float(f_RefreshRate) / f_VSyncBlanks) : 1e6f/*limitless frame rate*/; }

      void SetRenderBufferSize(XMINT2 size) { f_BackBufferSize = size; }
      void SetRefreshRate(int32_t rate) { f_RefreshRate = rate; }
      void SetVSyncBlanks(int32_t blanks) { f_VSyncBlanks = blanks; }

      virtual ResHandle ResourceCreate(const GraphicsResourceDesc& info);
      virtual void ResourceRelease(ResHandle handle);
      virtual void ResourceUpdate(ResHandle handle, const void* data, size_t dataSize) = 0;
      virtual void ResourceReadback(ResHandle handle, void* outData, size_t dataSize) = 0;

      void Launch() { threadPool.Launch(); }
      void Terminate() { threadPool.Terminate(); }
      // ***CORE WORKLOAD***
      // Commit a frame, or wait for the last CPU renderer frame.
      void Commit() { threadPool.Commit(); }

   protected:
      static inline std::vector<GenericRendererCommand> cmdListBusy{};

      IRenderer(uint32_t threadCount, string name, XMINT2 renderBufferSize, int32_t refreshRate);

      void BasePioneer();
      virtual void Worker(uint32_t workerIndex) = 0;
      virtual void Pioneer() = 0;
      virtual void Assembler() = 0;

   private:
      using ActionT = void(IRenderer::*)();
      using WorkerT = void(IRenderer::*)(uint32_t);
      ThreadPool<IRenderer, WorkerT, ActionT> threadPool;
      GenericHandlePool<uint16_t> resHandlePool{ "ResHandle Pool", MaxRescourceNum };
   };

#if defined(_WIN64)
   class D3D12Renderer final : public IRenderer
   {
      DeleteDefautedMethods(D3D12Renderer)

   public:
      D3D12Renderer(void* windowHandle, int32_t threadCount, XMINT2 backBufferSize, int32_t refreshRate);
      ~D3D12Renderer();
      uint64_t GetFrameIndex() override final;

      ResHandle ResourceCreate(const GraphicsResourceDesc& info) override final;
      void ResourceRelease(ResHandle handle) override final;
      void ResourceUpdate(ResHandle handle, const void* data, size_t dataSize) override final;
      void ResourceReadback(ResHandle handle, void* outData, size_t dataSize) override final;

   private:
      void Worker(uint32_t workerIndex) override final;
      void Pioneer() override final;
      void Assembler() override final;
   };
#endif

#if defined(_WIN64) || defined(__ANDROID__)
   class Vulkan12Renderer : public IRenderer
   {
      DeleteDefautedMethods(Vulkan12Renderer)

   public:
      virtual ~Vulkan12Renderer() {};
      Vulkan12Renderer(uint32_t) : IRenderer(2, "Vulkan12Renderer", {}, 60) {}
   };
#endif
}

// Functions
namespace Pillow::Graphics
{
   inline GraphicsResourceType GetResourceType(ResHandle handle)
   {
      return static_cast<GraphicsResourceType>((handle & ResourceTypeMask) >> ResIndexBits);
   }

   inline ResHandle SetRecourceType(ResHandle handle, GraphicsResourceType type)
   {
      return (handle & ~ResourceTypeMask) | (ResHandle(type) << ResIndexBits);
   }

   inline ResHandle ClearResourceType(ResHandle handle)
   {
      return handle & ~ResourceTypeMask;
   }

   inline bool CheckHandle(ResHandle handle)
   {
      return ClearResourceType(handle) != 0;
   }

   // Create an empty command.
   ForceInline void CmdNone()
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::None;
      IRenderer::cmdListIdle.push_back(cmd);
   }

   // Clear pipeline buffer. 
   // depth, stencil: When clearing depth or stencil, they must be specified, otherwise they will be ignored.
   ForceInline void CmdClearPiplelineBuffer(PipelineBuffer buffer,
      const XMFLOAT4& color = Constants::DefaultBackColor,
      float depth = Constants::FloatInfinity,
      uint8_t stencil = UINT8_MAX)
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::ClearPiplelineBuffer;
      cmd.Int3[0] = static_cast<uint8_t>(buffer);
      if(buffer == PipelineBuffer::Depth)
      {
         cmd.Float4[0] = depth;
         cmd.Int3[1] = stencil;
      }
      else
      {
         cmd.Float4 = color;
      }
      IRenderer::cmdListIdle.push_back(cmd);
   }

   // Clear render target.
   ForceInline void CmdClearRenderTarget(ResHandle handle, const XMFLOAT4& color = Constants::DefaultBackColor)
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::ClearRenderTarget;
      cmd.Int3[0] = handle;
      cmd.Float4 = color;
      IRenderer::cmdListIdle.push_back(cmd);
   }

   // Clear depth-stencil buffer. 
   ForceInline void CmdClearDepthStencil(ResHandle handle,
      float depth = Constants::FloatInfinity, uint8_t stencil = UINT8_MAX)
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::ClearDepthStencil;
      cmd.Int3[0] = handle;
      cmd.Float4[0] = depth;
      cmd.Int3[1] = stencil;
      IRenderer::cmdListIdle.push_back(cmd);
   }

   // Set pipeline buffers.
   ForceInline void CmdSetPipelineBuffers(std::vector<PipelineBuffer>& buffers, bool bSetDepth = false)
   {
      if (buffers.size() > 4) throw std::runtime_error("Maximum Render Target Num: 4");
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::SetPiplelineBuffers;
      uint32_t compactData = 0;
      for (uint32_t i = 0; i < buffers.size(); i++)
      {
         compactData |= static_cast<uint8_t>(buffers[i]) << (i * 8);
      }
      cmd.Int3[0] = compactData;
      cmd.Int3[1] = buffers.size();
      cmd.Int3[2] = bSetDepth ? 1 : 0;
      IRenderer::cmdListIdle.push_back(cmd);
   }

   //  Set render target and depth buffer.
   ForceInline void CmdSetRenderTargets(std::vector<ResHandle>& buffers, ResHandle depthBuffer = NullHandle)
   {
      if (buffers.size() > 4) throw std::runtime_error("Maximum Render Target Num: 4");
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::SetRenderTargets;
      for (uint32_t i = 0; i < buffers.size(); i++)
      {
         cmd.Int4[i] = buffers[i];
      }
      cmd.Int3[0] = buffers.size();
      cmd.Int3[1] = depthBuffer;
      IRenderer::cmdListIdle.push_back(cmd);
   }

   // Set the active camera.
   ForceInline void CmdSetActiveCamera(ResHandle cameraBuffer)
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::SetActiveCamera;
      cmd.Int3[0] = cameraBuffer;
      IRenderer::cmdListIdle.push_back(cmd);
   }

   // Set the active viewport.
   ForceInline void CmdSetViewport(const XMFLOAT4& viewport)
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::SetViewport;
      cmd.Float4 = viewport;
      IRenderer::cmdListIdle.push_back(cmd);
   }

   //// Set a light source.
   //ForceInline void CmdSetLight(LightType type, const XMFLOAT4& quaternion,
   //   const XMFLOAT4& color,bool hasShadow, float intensity, float range, float size1, float size2, float size3)
   //{
   //   GenericRendererCommand cmd;
   //   cmd.CmdType = GenericRendererCommand::Type::SetLight;
   //   cmd.Flags8 = uint8_t(type) | (hasShadow ? 0X80 : 0);
   //   XMFLOAT4* vec1 = reinterpret_cast<XMFLOAT4*>(cmd.Vector.Matrix.m);
   //   vec1[0] = quaternion;
   //   vec1[1] = color;
   //   vec1[2] = XMFLOAT4(intensity, range, 0, 0);
   //   vec1[3] = XMFLOAT4(size1, size2, size3, 0);
   //   IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   //}

   // Set the active pipeline state object.
   ForceInline void CmdBindPipelineState(ResHandle psoHandle)
   {
      GenericRendererCommand cmd;
      cmd.CmdType = GenericRendererCommand::Type::BindPipelineState;
      cmd.Int3[0] = psoHandle;
      IRenderer::cmdListIdle.push_back(cmd);
   }

   //// Set 1~4 shader resources.
   //ForceInline void CmdBindShaderResourceViews(int32_t rootParamIndex[], ResHandle handles[], int32_t count)
   //{
   //   if (count > 4) throw std::runtime_error("Too many shader resource views to bind. Max is 4.");
   //   GenericRendererCommand cmd;
   //   cmd.CmdType = GenericRendererCommand::Type::BindShaderResourceViews;
   //   cmd.Count = count;
   //   for (int32_t i : std::views::iota(0, count))
   //   {
   //      cmd.Vector.UIntArray8[i] = rootParamIndex[i];
   //      cmd.Vector.UIntArray8[i + 4] = handles[i];
   //   }
   //   IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   //}

   //// Set 1~4 constant buffers.
   //ForceInline void CmdBindConstantBufferViews(int32_t rootParamIndex[], ResHandle handles[], int32_t count)
   //{
   //   if (count > 4) throw std::runtime_error("Too many constant buffer views to bind. Max is 4.");
   //   GenericRendererCommand cmd;
   //   cmd.CmdType = GenericRendererCommand::Type::BindConstantBufferViews;
   //   cmd.Count = count;
   //   for (int32_t i : std::views::iota(0, count))
   //   {
   //      cmd.Vector.UIntArray8[i] = rootParamIndex[i];
   //      cmd.Vector.UIntArray8[i + 4] = handles[i];
   //   }
   //   IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   //}

   //// Set 1~4 unordered access buffers.
   //ForceInline void CmdBindUnorderedAccessViews(int32_t rootParamIndex[], ResHandle handles[], int32_t count)
   //{
   //   if (count > 4) throw std::runtime_error("Too many unordered access views to bind. Max is 4.");
   //   GenericRendererCommand cmd;
   //   cmd.CmdType = GenericRendererCommand::Type::BindConstantBufferViews;
   //   cmd.Count = count;
   //   for (int32_t i : std::views::iota(0, count))
   //   {
   //      cmd.Vector.UIntArray8[i] = rootParamIndex[i];
   //      cmd.Vector.UIntArray8[i + 4] = handles[i];
   //   }
   //   IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   //}

   //// Dispatch an instanced (GPU Instancing) draw call.
   //ForceInline void CmdDispatchMesh(ResHandle meshHandle, int32_t instanceCount)
   //{
   //   GenericRendererCommand cmd;
   //   cmd.CmdType = GenericRendererCommand::Type::DispatchMesh;
   //   cmd.Vector.UIntArray8[0] = meshHandle;
   //   cmd.Vector.UIntArray8[1] = instanceCount;
   //   IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   //}

   //// Dispatch a post-processing pass.
   //ForceInline void CmdDispatchPostProcess(PipelineBuffer from, PipelineBuffer to)
   //{
   //   GenericRendererCommand cmd;
   //   cmd.CmdType = GenericRendererCommand::Type::DispatchPostProcess;
   //   cmd.Vector.UIntArray8[0] = static_cast<uint32_t>(from);
   //   cmd.Vector.UIntArray8[1] = static_cast<uint32_t>(to);
   //   IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   //}

   //// Dispatch a compute shader.
   //ForceInline void CmdDispatchCompute()
   //{
   //   //dumb
   //   GenericRendererCommand cmd;
   //   cmd.CmdType = GenericRendererCommand::Type::DispatchCompute;
   //   IRenderer::GetInstance()->GetIdleCmdList()->push_back(cmd);
   //}
}

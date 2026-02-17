#pragma once
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

   typedef uint32_t ResourceHandle; // 28 bits for index, 4 bits for type

   constexpr ResourceHandle NullResourceHandle = 0;
   constexpr uint32_t ResourceTypeMask = 0xF << 28;

   // DIRECT3D12 VIEW TYPES
   // IN  DESCRIPTOR HEAP： CRV SRV UAV Sampler
   // OUT DESCRIPTOR HEAP： RTV DSV, VBV IBV SOV
   //
   // DIRECT3D12 RESOURCE HEAP TYPES
   // Upload Default Readback Custom

   enum class ResourceType : uint32_t
   {
      None = 0,
      Mesh = 0X1 << 28,
      PiplelineState = 0X2 << 28,
      ShaderResourceView = 0X3 << 28,
      ConstantBufferView = 0X4 << 28,
      UnorderedAccessView = 0X5 << 28,
      ReadbackBuffer = 0X6 << 28,
   };

   ForceInline ResourceType GetResourceType(ResourceHandle handle) { return ResourceType(handle & ResourceTypeMask); }

   ForceInline bool IsValidHandle(ResourceHandle handle) { return handle != 0; }

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
      enum class Type : uint8_t {
         None,
         // Clear commands
         ClearRenderTarget,
         ClearDepthStencil,
         // Set commands
         SetRenderTarget,
         SetDepthStencil,
         SetActiveCamera,
         SetViewport,
         // Bind commands
         // Resource barriers are implicitly applied when executing bind commands.
         // We don't want GenericRenderer exposes explicit resource barriers. (CS term: encapsulation)
         BindPipelineState,
         BindShaderResourceView,
         BindConstantBufferView,
         BindUnorderedAccessView,
         // Dispatch commands
         DispatchMesh,
         DispatchShadow,
         DispatchPostProcess,
         DispatchCompute
      };

      Type CmdType;

      uint8_t Flags8;
      uint8_t Index1;
      uint8_t Index2;

      union UnionParams
      {
         XMFLOAT4X4 Matrix;
         struct
         {
            ResourceHandle Handle[4];
            XMFLOAT4 Float4_1;
            XMFLOAT4 Float4_2;
            XMINT4 Int4;
         };
      } Params;
   };

   static_assert(std::is_trivially_copyable_v<GenericRendererCommand>); //POD test.

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
      inline virtual void ResourceRegister(ResourceHandle& handle, ResourceType type, const void* desc) {/*dumb*/};
      inline virtual void ResourceRelease(ResourceHandle handle) {/*dumb*/};
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
}

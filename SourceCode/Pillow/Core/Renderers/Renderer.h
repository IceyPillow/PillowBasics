#pragma once
#include <thread>
#include <barrier>
#include <atomic>
#include <vector>
#include <functional>
#include "../Auxiliaries.h"
#include "../Constants.h"
#include "../Texture.h"

namespace Pillow::Graphics
{
   typedef uint32_t ResourceHandle;

   enum class ResourceType : uint32_t
   {
      None = 0,
      Mesh = 1 << 28,
      Texture = 2 << 28,
      PiplelineState = 3 << 28,
      ConstantBuffer = 4 << 28,
   };

   ForceInline ResourceType GetResourceType(ResourceHandle handle)
   {
      return ResourceType(handle & (7 << 28));
   }

   ForceInline bool IsValidHandle(ResourceHandle handle)
   {
      return (handle & !(7 << 28)) != 0;
   }

   struct Drawcall
   {
      void* sth;
   };

   class GenericRenderer
   {
      DeleteDefautedMethods(GenericRenderer)
         ReadonlyProperty(std::string, RendererName)
         ReadonlyProperty(int32_t, ThreadCount)
   public:
      virtual ~GenericRenderer() = 0;
      virtual uint64_t GetFrameIndex() = 0;
      //ForceInline int32_t GetFrameArrayIndex() { return GetFrameIndex() % Constants::SwapChainSize; }
      virtual int32_t CreateMesh() = 0;
      virtual int32_t CreateTexture() = 0;
      virtual int32_t CreateConstantBuffer() = 0;
      virtual int32_t CreatePiplelineState() = 0;
      virtual void ReleaseResource(int32_t handle) = 0;
      void Launch();
      void Terminate();
      void Commit();

   protected:
      GenericRenderer(int32_t threadCount, std::string name);
      virtual void Worker(int32_t workerIndex) = 0;
      virtual void Assembler() = 0;

   private:
      void BaseWorker(int32_t workerIndex);
   };

#if defined(_WIN64)
   class D3D12Renderer final: public GenericRenderer
   {
      DeleteDefautedMethods(D3D12Renderer)

   public:
      D3D12Renderer(HWND windowHandle, int32_t threadCount);
      ~D3D12Renderer();
      uint64_t GetFrameIndex();
      int32_t CreateMesh();
      int32_t CreateTexture();
      int32_t CreatePiplelineState();
      int32_t CreateConstantBuffer();
      void ReleaseResource(int32_t handle);

   private:
      void Worker(int32_t workerIndex);
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

   extern std::unique_ptr<GenericRenderer> Instance;

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

#pragma once
#include <thread>
#include <barrier>
#include <atomic>
#include <vector>
#include <functional>
#include "../Auxiliaries.h"
#include "../AppConstants.h"
#include "../Texture.h"

namespace Pillow
{
   enum class GenericRendererResourceType : int32_t
   {
      Mesh = 1 << 28,
      Texture = 2 << 28,
      PiplelineState = 3 << 28,
      ConstantBuffer = 4 << 28,
   };

   ForceInline GenericRendererResourceType GetResType(int32_t handle)
   {
      return GenericRendererResourceType(handle & (7 << 28));
   }

   class GenericRenderer
   {
      DeleteDefautedMethods(GenericRenderer)
         ReadonlyProperty(std::string, RendererName)
         ReadonlyProperty(int32_t, ThreadCount)
   
   public:
      virtual ~GenericRenderer() = 0;
      virtual uint64_t GetFrameIndex() = 0;
      virtual int32_t CreateMesh() = 0;
      virtual int32_t CreateTexture() = 0;
      virtual int32_t CreateConstantBuffer() = 0;
      virtual int32_t CreatePiplelineState() = 0;
      virtual void ReleaseResource(int32_t handle) = 0;
      void RendererLaunch();
      void RendererTerminate();
      void FrameBegin();
      bool IsFrameEnd();

   protected:
      GenericRenderer(int32_t threadCount, std::string name);
      virtual void Worker(int32_t workerIndex) = 0;
   private:
      void BaseWorker(int32_t workerIndex);
   };

#if defined(_WIN64)
   class D3D12Renderer : public GenericRenderer
   {
      DeleteDefautedMethods(D3D12Renderer)
   
   public:
      D3D12Renderer(HWND windowHandle, int32_t threadCount);
      ~D3D12Renderer() override;
      uint64_t GetFrameIndex() override;
      int32_t CreateMesh() override;
      int32_t CreateTexture() override;
      int32_t CreatePiplelineState() override;
      int32_t CreateConstantBuffer() override;
      void ReleaseResource(int32_t handle) override;
   protected:
      void Worker(int32_t workerIndex) final;
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
}

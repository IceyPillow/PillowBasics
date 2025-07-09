// TODO: bundle cmd lists
#if defined(_WIN64)
#include "Renderer.h"
#include <memory>
#include <vector>
#include <comdef.h>
#include <queue>
#include <wrl.h> // import Component Object Model Pointer
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

using namespace Pillow;
using Microsoft::WRL::ComPtr;

// __LINE__ in an inline function doesn't show the line number of the caller, thus choose a macro.
#define CheckHResult(hr)\
{\
   if (FAILED(hr))\
   {\
      std::string errorMsg;\
      errorMsg = errorMsg + "File:" + __FILE__ + ", Line:" + std::to_string(__LINE__);\
      errorMsg = errorMsg + "\nError: " + Wstring2String(_com_error(hr).ErrorMessage());\
      throw std::exception(errorMsg.c_str());\
   }\
}

typedef IDXGIFactory5 IFactory;                  // Has CheckFeatureSupport()
typedef ID3D12Device4 IDevice;                   // Has CreateCommandList1()
typedef ID3D12GraphicsCommandList2 ICommandList; // Has WriteBufferImmediate()
typedef IDXGISwapChain1 ISwapChain;              // Has SetBackgroundColor() 
typedef ID3D12Resource IResource;                // The original one is fine

// An anonymous namespace has internal linkage (accessable in local translation unit)
// Static variables
namespace
{
   const int32_t CBAlignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
   const int32_t BCPixelBlock = 16;

   const DXGI_FORMAT NativeTexFmt[int32_t(GenericTexFmt::Count)]
   {
      DXGI_FORMAT_R8G8B8A8_UNORM,
      DXGI_FORMAT_R8G8_SNORM,
      DXGI_FORMAT_R8_UNORM,
   };
   const DXGI_FORMAT NativeBCTexFmt[int32_t(GenericTexFmt::Count)]
   {
      DXGI_FORMAT_BC3_UNORM,
      DXGI_FORMAT_BC5_SNORM,
      DXGI_FORMAT_BC4_UNORM,
   };
#define DEFAULT_LAYOUT \
0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
   const D3D12_INPUT_ELEMENT_DESC _BasicVertex[3]
   {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, DEFAULT_LAYOUT },
      { "TEXCOORD", 0, DXGI_FORMAT_R8G8_UINT, DEFAULT_LAYOUT },
      { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, DEFAULT_LAYOUT }
   };
   const D3D12_INPUT_ELEMENT_DESC _StaticVertex[5]
   {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, DEFAULT_LAYOUT },
      { "TEXCOORD", 0, DXGI_FORMAT_R8G8_UINT, DEFAULT_LAYOUT },
      { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, DEFAULT_LAYOUT },
      { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, DEFAULT_LAYOUT },
      { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, DEFAULT_LAYOUT }
   };
   const D3D12_INPUT_ELEMENT_DESC _SkeletalVertex[5]
   {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, DEFAULT_LAYOUT },
      { "TEXCOORD", 0, DXGI_FORMAT_R8G8B8A8_UINT, DEFAULT_LAYOUT },
      { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, DEFAULT_LAYOUT },
      { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, DEFAULT_LAYOUT },
      { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, DEFAULT_LAYOUT }
   };
   const D3D12_INPUT_LAYOUT_DESC InputLayoutBasic{ _BasicVertex , 3};
   const D3D12_INPUT_LAYOUT_DESC InputLayoutStatic{ _StaticVertex, 5 };
   const D3D12_INPUT_LAYOUT_DESC InputLayoutSkeletal{ _SkeletalVertex, 5 };

   class FenceSync;
   class DescriptorHeapManager;
   class lateReleaseManager;
   class UnitedBuffer;
   std::unique_ptr<FenceSync> fenceSync;
   std::unique_ptr<DescriptorHeapManager> descriptorMgr;
   std::unique_ptr<lateReleaseManager> lateReleaseMgr;
   ComPtr<IFactory> factory;
   ComPtr<IDevice> device;
   ComPtr<ID3D12CommandQueue> cmdQueue;
   std::vector<ComPtr<ICommandList>> cmdLists;
   std::vector<ID3D12CommandList*> _cmdLists; // A copy of cmdLists, prepared for ExecuteCommandLists()
   std::vector<ComPtr<ID3D12CommandAllocator>> cmdAllocators;
   ComPtr<ISwapChain> swapChain;

   uint16_t tempRTVs[Constants::SwapChainSize] = { 0 }; // Temporary RTVs for swapchain buffers
   ComPtr<IResource> backbuffers[Constants::SwapChainSize]{};

   HWND hwnd;
   int32_t threads;
   bool allowTearing;
   XMINT2 backbufferSize;
   int32_t verticalBlanks{ 1 };
}

// Types
namespace
{
   ForceInline void ApplyBarrier(ComPtr<ICommandList>& cmdList, ComPtr<IResource>& resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

   // Fence synchronization wrapper
   class FenceSync
   {
      ReadonlyProperty(uint64_t, FrameIndex)

   public:
      FenceSync(ComPtr<IDevice>& device, ComPtr<ID3D12CommandQueue>& commandQueue)
      {
         this->commandQueue = commandQueue;
         syncEventHandle = CreateEventEx(nullptr, L"D3D12Renderer Fence Event", 0, EVENT_ALL_ACCESS);
         if (syncEventHandle == 0) throw std::exception("Failed to create fence sync event handle.");
         CheckHResult(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
      }

      ~FenceSync()
      {
         CloseHandle(syncEventHandle);
      }

      uint64_t GetTargetFence() { return _FrameIndex + 1; }
      uint64_t GetCompletedFence() { return fence->GetCompletedValue(); }
      int32_t GetFrameArrayIdx() { return _FrameIndex % Constants::SwapChainSize; }

      // Get the next frame.
      // ***WARNING***
      // Invoke this AFTER ExecuteCommandList() in one frame.
      void NextFrame()
      {
         _FrameIndex++;
         commandQueue->Signal(fence.Get(), _FrameIndex);
         uint64_t minFence = (_FrameIndex < Constants::SwapChainSize) ? 0 : (_FrameIndex - Constants::SwapChainSize + 1);
         Synchronize(minFence);
      }

      // Get all GPU's work done.
      // ***WARNING***
      // Invoke this BEFORE entering worker threads (to avoid resource references existing in uncommitted cmd lists), or AFTER NextFrame() in one frame.
      void FlushQueue()
      {
         uint64_t minFence = _FrameIndex;
         Synchronize(minFence);
      }

   private:
      void Synchronize(uint64_t targetFence)
      {
         // Make sure the GPU arrives at the targetFence.
         if (fence->GetCompletedValue() < targetFence)
         {
            fence->SetEventOnCompletion(targetFence, syncEventHandle);
            WaitForSingleObjectEx(syncEventHandle, INFINITE, true);
         }
      }

   private:
      HANDLE syncEventHandle;
      ComPtr<ID3D12Fence> fence;
      ComPtr<ID3D12CommandQueue> commandQueue;
   };

   class lateReleaseManager
   {
   public:
      lateReleaseManager() {};

      // Enqueue an element that will be released after current frame.
      void Enqueue(std::unique_ptr<UnitedBuffer>&& buffer)
      {
         Item item{ std::move(buffer), fenceSync->GetTargetFence() };
         releaseQueue.push(std::move(item));
      }

      void ReleaseGarbage()
      {
         uint64_t completedFence = fenceSync->GetCompletedFence();
         if (completedFence == 0) return;
         while (!releaseQueue.empty())
         {
            Item& item = releaseQueue.front();
            // FIFO indicates that if one element dequeued is incomplete, so are the remnants.
            if (item.targetFence > completedFence) break;
            item.buffer.reset();
            releaseQueue.pop();
         }
      }

   private:
      struct Item
      {
         std::unique_ptr<UnitedBuffer> buffer;
         uint64_t targetFence;
      };

      std::queue<Item> releaseQueue;
   };

   enum class ViewType : uint8_t
   {
      // Stored in srvUavDescHeap.
      CBV,
      SRV,
      UAV,
      // Stored in rtvDescHeap.
      RTV,
      // stored in dsvDescHeap.
      DSV
   };

   class DescriptorHeapManager
   {
   public:
      DescriptorHeapManager(ComPtr<IDevice>& device) :
         csuSize(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
         rtvSize(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)),
         dsvSize(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV))
      {
         csuFreePool.reserve(MaxCsuCount);
         rtvFreePool.reserve(MaxRtvCount);
         dsvFreePool.reserve(MaxDsvCount);
         for (uint16_t i = MaxCsuCount; i > 0; i--)
         {
            csuFreePool.push_back(i | uint16_t(InnerFlag::CSU) << 14);
            if (i <= MaxRtvCount) rtvFreePool.push_back(i | uint16_t(InnerFlag::RTV) << 14);
            if (i <= MaxDsvCount) dsvFreePool.push_back(i | uint16_t(InnerFlag::DSV) << 14);
         }

         D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc
         {
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            (UINT)MaxCsuCount,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
         };
         CheckHResult(device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&csuDescHeap)));
         descHeapDesc =
         {
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            (UINT)MaxRtvCount,
         };
         CheckHResult(device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&rtvDescHeap)));
         descHeapDesc =
         {
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            (UINT)MaxDsvCount
         };
         CheckHResult(device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&dsvDescHeap)));
         csuCpuHandle0 = csuDescHeap->GetCPUDescriptorHandleForHeapStart();
         csuGpuHandle0 = csuDescHeap->GetGPUDescriptorHandleForHeapStart();
         rtvCpuHandle0 = rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
         dsvCpuHandle0 = dsvDescHeap->GetCPUDescriptorHandleForHeapStart();
      }

      ForceInline void BindSrvHeap(ComPtr<ICommandList>& cmd)
      {
         cmd->SetDescriptorHeaps(1, csuDescHeap.GetAddressOf());
      }

      ForceInline D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint16_t handle)
      {
         D3D12_CPU_DESCRIPTOR_HANDLE result{};
         auto flag = GetInnerFlag(handle);
         handle &= 0x3FFF; // Clear the flag bits
         switch (flag)
         {
         case InnerFlag::CSU:
            result.ptr = csuCpuHandle0.ptr + csuSize * handle;
            break;
         case InnerFlag::RTV:
            result.ptr = rtvCpuHandle0.ptr + rtvSize * handle;
            break;
         case InnerFlag::DSV:
            result.ptr = dsvCpuHandle0.ptr + dsvSize * handle;
            break;
         }
         return result;
      }

      ForceInline D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint16_t handle)
      {
         D3D12_GPU_DESCRIPTOR_HANDLE result{};
         auto flag = GetInnerFlag(handle);
         handle = RemoveFlag(handle);
         switch (flag)
         {
         case InnerFlag::CSU:
            result.ptr = csuGpuHandle0.ptr + csuSize * handle;
            break;
         default:
            throw std::exception("GPU handle is not supported for RTV and DSV.");
         }
         return result;
      }

      uint16_t CreateView(ComPtr<IDevice>& device, ComPtr<IResource>& res, void* viewDesc, ViewType type)
      {
         uint16_t handle{};
         auto GetHandle = [&](std::vector<uint16_t>& freePool, const char* name)
            {
               if (freePool.empty())
                  throw std::exception((std::string(name) + ": This descriptor heap is full.").c_str());
               handle = freePool.back();
               freePool.pop_back();
            };
         switch (type)
         {
         case ViewType::CBV:
            GetHandle(csuFreePool, "CSV_SRV_UAV");
            device->CreateConstantBufferView((D3D12_CONSTANT_BUFFER_VIEW_DESC*)viewDesc, GetCPUHandle(handle));
            break;
         case ViewType::SRV:
            GetHandle(csuFreePool, "CSV_SRV_UAV");
            device->CreateShaderResourceView(res.Get(), (D3D12_SHADER_RESOURCE_VIEW_DESC*)viewDesc, GetCPUHandle(handle));
            break;
         case ViewType::UAV:
            GetHandle(csuFreePool, "CSV_SRV_UAV");
            device->CreateUnorderedAccessView(res.Get(), nullptr, (D3D12_UNORDERED_ACCESS_VIEW_DESC*)viewDesc, GetCPUHandle(handle));
            break;
         case ViewType::RTV:
            GetHandle(rtvFreePool, "RTV");
            device->CreateRenderTargetView(res.Get(), (D3D12_RENDER_TARGET_VIEW_DESC*)viewDesc, GetCPUHandle(handle));
            break;
         case ViewType::DSV:
            GetHandle(dsvFreePool, "DSV");
            device->CreateDepthStencilView(res.Get(), (D3D12_DEPTH_STENCIL_VIEW_DESC*)viewDesc, GetCPUHandle(handle));
         }
#ifdef PILLOW_DEBUG
         //LogSystem(L"ViewHandle=" + std::to_wstring(handle) + L" Index=" + std::to_wstring(RemoveFlag(handle)));
#endif
         return handle;
      }

      void ReleaseView(uint16_t handle)
      {
         auto ReleaseHandle = [&handle](std::vector<uint16_t>& freePool)
            {
#ifdef PILLOW_DEBUG
               bool found = std::find(freePool.begin(), freePool.end(), handle) != freePool.end();
               if (found) throw std::exception("Invalid index.");
#endif
               freePool.push_back(handle);
            };
         auto flag = GetInnerFlag(handle);
         switch (flag)
         {
         case InnerFlag::CSU:
            ReleaseHandle(csuFreePool);
            break;
         case InnerFlag::RTV:
            ReleaseHandle(rtvFreePool);
            break;
         case InnerFlag::DSV:
            ReleaseHandle(dsvFreePool);
            break;
         }
      }

   private:
      enum struct InnerFlag : uint32_t
      {
         CSU = 0,
         RTV = 1,
         DSV = 2
      };

      ForceInline InnerFlag GetInnerFlag(uint16_t handle)
      {
         return InnerFlag(handle >> 14);
      }

      ForceInline uint16_t RemoveFlag(uint16_t handle)
      {
         return handle & 0x3FFF; // Clear the flag bits
      }

   private:
      const uint32_t FlagBits = 2;
      const uint32_t HandleMaxNum = (1 << (16 - FlagBits)); // value=16384

      const int32_t MaxCsuCount = 4096;
      const int32_t MaxRtvCount = 64;
      const int32_t MaxDsvCount = 16;

      const int32_t csuSize, rtvSize, dsvSize;
      ComPtr<ID3D12DescriptorHeap> csuDescHeap;
      ComPtr<ID3D12DescriptorHeap> rtvDescHeap;
      ComPtr<ID3D12DescriptorHeap> dsvDescHeap;
      std::vector<uint16_t> csuFreePool;
      std::vector<uint16_t> rtvFreePool;
      std::vector<uint16_t> dsvFreePool;
      D3D12_CPU_DESCRIPTOR_HANDLE csuCpuHandle0;
      D3D12_GPU_DESCRIPTOR_HANDLE csuGpuHandle0;
      // RTV and DSV don't have gpu handles.
      D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle0;
      D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle0;
   };

   // A superior wrapper for D3D12 resources of all types.
   class UnitedBuffer
   {
      DeleteDefautedMethods(UnitedBuffer)

   public:
      // Use none-scoped enumerations for convenience.
      enum HeapType : uint8_t
      {
         Upload = D3D12_HEAP_TYPE_UPLOAD,
         TextureUpload = D3D12_HEAP_TYPE_CUSTOM,
         Readback = D3D12_HEAP_TYPE_READBACK,
         Default = D3D12_HEAP_TYPE_DEFAULT
      };

      enum DataType : uint8_t
      {
         Texture,
         ConstBuffer,
         VertexOrIdxBuffer
      };

      // For texture arrays, create only a minimal number of mid buffers for uploading, which saves a lot of memory.
      static const int32_t MaxMidPoolSize = 4;

      const HeapType _HeapType;
      const DataType _DataType;
      const GenericTextureInfo TexInfo;
      const int32_t ElementCount;
      const int32_t RawElementSize;
      const int32_t AlignedElementSize;
      const int32_t TotalSize;
      const bool KeepMidPool;

      UnitedBuffer(HeapType heapType, DataType dataType, int32_t _rawElementSize, int32_t count, bool keepMiddlePool = false):
         UnitedBuffer(heapType, dataType, _rawElementSize, count, keepMiddlePool, GenericTextureInfo{})
      {
         bool wrongUseCheck = dataType == Texture;
         if (wrongUseCheck) throw std::runtime_error("Wrong constructor usage.");
      }

      UnitedBuffer(HeapType heapType, DataType dataType, const GenericTextureInfo& texInfo, bool keepMiddlePool = false):
         UnitedBuffer(heapType, dataType, 0, 0, keepMiddlePool, texInfo)
      {
         bool wrongUseCheck = dataType != Texture;
         wrongUseCheck |= dataType == Texture && (heapType == Upload || heapType == TextureUpload);
         if (wrongUseCheck) throw std::runtime_error("Wrong constructor usage.");
      }

      ~UnitedBuffer() = default;

      uint64_t GetGPUAddress(int index = 0) { return pointerGPU + index * RawElementSize; };

      // The destination data should align with 64 bytes(the cache line size).
      void ReadBack(std::unique_ptr<CacheLine[]>& destination, int32_t destinationSize = 0)
      {
         if (_HeapType != HeapType::Readback) throw std::exception("Cannot use ReadBack() with non-readback buffers.");
         if (!destination) destination = CreateAlignedMemory(TotalSize);
         else if (destinationSize < TotalSize) throw std::exception("Destination buffer is too small.");
         if (_DataType == DataType::Texture)
         {
            int32_t rowPitch = TexInfo.GetWidth() * TexInfo.GetPixelSize();
            int32_t depthPitch = TexInfo.GetMipZeroSize();
            heap->ReadFromSubresource(destination.get(), rowPitch, depthPitch, 0, nullptr);
         }
         else memcpy(destination.get(), pointerCPU, TotalSize);
      }

      void WriteNumericData(const uint8_t* rawData, int indexOffset = 0, int _elementCount = 1)
      {
         if (_DataType == DataType::Texture) throw new std::runtime_error("Cannot use WriteNumericData() with textures.");
         if (indexOffset + _elementCount > ElementCount) throw std::exception("Out of Range");
         if (_HeapType == HeapType::Default)
         {
            RegisterGPUCopy();
            middlePool[0]->WriteNumericData(rawData, indexOffset, _elementCount);
            return;
         }
         // Write to the middle buffer
         if (_DataType == DataType::VertexOrIdxBuffer)
         {
            memcpy(pointerCPU + indexOffset * RawElementSize, rawData, _elementCount * RawElementSize);
         }
         else if (_DataType == DataType::ConstBuffer)
         {
            int32_t alignedSize = GetAlignedSize(RawElementSize, CBAlignment);
            for (int32_t i = 0; i < _elementCount; i++)
            {
               int32_t destOffset = (indexOffset + i) * alignedSize;
               int32_t srcOffset = (indexOffset + i) * RawElementSize;
               memcpy(pointerCPU + destOffset, rawData + srcOffset, RawElementSize);
            }
         }
      }

      // 1.D3D12 texture subresource indexing: SubRes[PlaneIdx][ArrayIdx][MipIdx]
      // Normally, planar formats are not used to store RGBA data.
      // 
      // 2.ABOUT THE FOOTPRINT: In Direct3D 12 terminology, footprint describes the memory layouts of D3D12 resources.
      // In detail, the size of a texture row should be aligned(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT), which makes memory allocation more sophisticated.
      // But there are ways to avoid touching the footprints.
      // For instance, we can use ID3D12Resource::WriteToSubresource to copy unaligned data into a custom upload heap,
      // then use ID3DCommandList::CopyTextureRegion to copy it into a default buffer while ignoring the footprints.
      void WriteTexture(const uint8_t* rawTexture, const GenericTextureInfo& texInfo, int32_t arrayIndex = 0)
      {
         if (_DataType != DataType::Texture) throw std::runtime_error("Cannot use WriteTexture() with numeric data.");
         if(middleTargets.size() == MaxMidPoolSize)  throw std::runtime_error("The middle pool is exhausted.");
         if (_HeapType == HeapType::Default)
         {
            RegisterGPUCopy();
            if (std::find(middleTargets.begin(), middleTargets.end(), arrayIndex) != middleTargets.end())
               throw std::runtime_error("Write to a same texture twice in one frame.");
            middlePool[middleTargets.size()]->WriteTexture(rawTexture, texInfo, arrayIndex);
            middleTargets.push_back(arrayIndex);
            return;
         }
         // Write to the middle buffer
         for (int32_t mip = 0; mip < texInfo.GetMipCount(); mip++)
         {
            int32_t width = texInfo.GetWidth() >> mip;
            int32_t rowPitch = width * texInfo.GetPixelSize();
            int32_t depthPitch = width * rowPitch;
            heap->WriteToSubresource(arrayIndex * texInfo.GetMipCount() + mip, nullptr, rawTexture, rowPitch, depthPitch);
            rawTexture += depthPitch;
         }
      }

      static void GPUCopy(ComPtr<ICommandList>& cmdList)
      {
         if (DirtyPool.empty()) return;
         while (!DirtyPool.empty())
         {
            UnitedBuffer& buffer = *DirtyPool.back();
            DirtyPool.pop_back();
            if (buffer.middlePool.size() == 1)
            {
               ApplyBarrier(cmdList, buffer.middlePool[0]->heap, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE);
               ApplyBarrier(cmdList, buffer.heap, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
               cmdList->CopyResource(buffer.heap.Get(), buffer.middlePool[0]->heap.Get()); // GPU Copy
               ApplyBarrier(cmdList, buffer.middlePool[0]->heap, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ);
               ApplyBarrier(cmdList, buffer.heap, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
            }
            else // Texture array
            {
               while (buffer.middleTargets.size())
               {
                  // Preparation
                  int32_t target = buffer.middleTargets.back();
                  buffer.middleTargets.pop_back();
                  int32_t midIdx = buffer.middleTargets.size();
                  D3D12_TEXTURE_COPY_LOCATION src{ buffer.middlePool[midIdx]->heap.Get(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, 0 };
                  D3D12_TEXTURE_COPY_LOCATION dst{ buffer.heap.Get(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, 0 };
                  // GPU copy
                  ApplyBarrier(cmdList, buffer.middlePool[midIdx]->heap, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE);
                  ApplyBarrier(cmdList, buffer.heap, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
                  for (int mip = 0; mip < buffer.TexInfo.GetMipCount(); mip++)
                  {
                     src.SubresourceIndex = mip;
                     dst.SubresourceIndex = target * buffer.TexInfo.GetMipCount() + mip;
                     cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, NULL);
                  }
                  ApplyBarrier(cmdList, buffer.middlePool[midIdx]->heap, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ);
                  ApplyBarrier(cmdList, buffer.heap, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
               }
            }

         }
      }

   private:
      const D3D12_RESOURCE_DESC DefaultResDesc
      {
         D3D12_RESOURCE_DIMENSION_BUFFER, 0, uint64_t(TotalSize), 1, 1, 1, DXGI_FORMAT_UNKNOWN,
         DXGI_SAMPLE_DESC{1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE
      };

      inline static std::vector<UnitedBuffer*> DirtyPool{};
      std::vector<std::unique_ptr<UnitedBuffer>> middlePool;
      std::vector<int8_t> middleTargets;
      ComPtr<IResource> heap{};
      uint64_t pointerGPU{};
      uint8_t* pointerCPU{};

      UnitedBuffer(HeapType heapType, DataType dataType, int32_t _rawElementSize, int32_t count, bool keepMiddlePool, const GenericTextureInfo& texInfo) :
         _HeapType(heapType),
         _DataType(dataType),
         TexInfo(texInfo),
         ElementCount(count),
         RawElementSize(_rawElementSize),
         AlignedElementSize(GetAlignedSize(_rawElementSize, dataType == ConstBuffer ? CBAlignment : 1)),
         TotalSize(GetAlignedSize(_rawElementSize, dataType == ConstBuffer ? CBAlignment : 1)* count),
         KeepMidPool(keepMiddlePool)
      {
         bool isUpload = heapType == Upload || heapType == TextureUpload;
         bool isRdBack = heapType == Readback;
         if (isRdBack && dataType == Texture && texInfo.GetMipCount() != 1)
            throw std::runtime_error("Texture readback buffers don't support mipmaps. It's a restriction of the Pillow Basics design.");

         // Write-combining disables the CPU cache and enables the write-combining buffer. It's suitable for CPU-write-only actions.
         auto pageType = isUpload ? D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE : (isRdBack ? D3D12_CPU_PAGE_PROPERTY_WRITE_BACK : D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE);
         // Level 0 memory pool = CPU main memory
         auto memPool = (isRdBack || isUpload) ? D3D12_MEMORY_POOL_L0 : D3D12_MEMORY_POOL_L1;
         D3D12_HEAP_PROPERTIES heapProperties{ D3D12_HEAP_TYPE(heapType), pageType, memPool };
         D3D12_RESOURCE_DESC resourceDesc = DefaultResDesc;
         if (dataType == Texture)
         {
            int32_t fmt = int32_t(texInfo.GetFormat());
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resourceDesc.Width = texInfo.GetWidth();
            resourceDesc.Height = texInfo.GetWidth();
            resourceDesc.DepthOrArraySize = uint16_t(texInfo.GetArrayCount());
            resourceDesc.MipLevels = uint16_t(texInfo.GetMipCount());
            resourceDesc.Format = texInfo.GetCompressionMode() == CompressionMode::None ? NativeTexFmt[fmt] : NativeBCTexFmt[fmt];
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
         }
         auto flags = D3D12_HEAP_FLAG_NONE;
         auto state = heapType == Readback ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;
         CheckHResult(device->CreateCommittedResource(&heapProperties, flags, &resourceDesc, state, nullptr, IID_PPV_ARGS(&heap)));
         GetCPUGPUPointers();
         CreateMiddleBuffers();
      }

      void CreateMiddleBuffers()
      {
         if (_HeapType != Default) return;
         if (!middlePool.empty()) throw std::runtime_error("The middle buffer has been created.");
         HeapType heapType = _DataType == Texture ? TextureUpload : Upload;
         int32_t count = _DataType == Texture ? MaxMidPoolSize : 1;
         middlePool.reserve(count);
         middleTargets.reserve(count);
         for (int i = 0; i < count; i++)
         {
            auto ptr = std::unique_ptr<UnitedBuffer>(new UnitedBuffer(heapType, _DataType, ElementCount, RawElementSize, KeepMidPool, TexInfo));
            middlePool.push_back(std::move(ptr));
         }
      }

      void GetCPUGPUPointers()
      {
         if (_HeapType != Default)
         {
            D3D12_RANGE range{ 0, 0 };
            // CPU read is only needed by readback buffers.
            CheckHResult(heap->Map(0, _HeapType == Readback ? nullptr : &range, (void**)(&pointerCPU)));
         }
         if (_DataType != Texture)
         {
            pointerGPU = heap->GetGPUVirtualAddress();
         }
      }

      void RegisterGPUCopy()
      {
         if (middlePool.empty()) throw std::runtime_error("The middle buffer of current default buffer died.");
         DirtyPool.push_back(this);
         // Release the mid pool.
         if (KeepMidPool) return;
         while (middlePool.size())
         {
            lateReleaseMgr->Enqueue(std::move(middlePool.back()));
            middlePool.pop_back();
         }
      }
   };

   class PipelineStateManager
   {

   };
}

// Static functions
namespace
{
   ForceInline void PlaceBCIndex(uint8_t* destination, uint32_t value, uint32_t index)
   {
      uint32_t bitCount = index * 3;
      uint32_t byteOffset = bitCount / 8;
      uint32_t bitOffset = bitCount % 8;
      destination[byteOffset] = (destination[byteOffset] & ~(0x7 << bitOffset)) | value << bitOffset;
   }

   void OptimizeAlpha(float* pX, float* pY, const float* block, uint32_t cSteps)
   {
      static constexpr float pC6[] = { 1, 4.f / 5.f, 3.f / 5.f, 2.f / 5.f, 1.f / 5.f, 0 };
      static constexpr float pD6[] = { pC6[5], pC6[4], pC6[3], pC6[2], pC6[1], pC6[0] };
      static constexpr float pC8[] = { 1, 6.f / 7.f, 5.f / 7.f, 4.f / 7.f, 3.f / 7.f, 2.f / 7.f, 1.f / 7.f, 0 };
      static constexpr float pD8[] = { pC8[7], pC8[6], pC8[5], pC8[4], pC8[3], pC8[2], pC8[1], pC8[0] };
      const float* pC = (6 == cSteps) ? pC6 : pC8;
      const float* pD = (6 == cSteps) ? pD6 : pD8;
      constexpr float MAX_VALUE = 1.0f;
      constexpr float MIN_VALUE = 0.0f;
      // Find Min and Max points, as starting point
      float fX = MAX_VALUE;
      float fY = MIN_VALUE;
      if (8 == cSteps)
      {
         for (size_t iPoint = 0; iPoint < BCPixelBlock; iPoint++)
         {
            if (block[iPoint] < fX) fX = block[iPoint];
            if (block[iPoint] > fY) fY = block[iPoint];
         }
      }
      else
      {
         for (size_t iPoint = 0; iPoint < BCPixelBlock; iPoint++)
         {
            if (block[iPoint] < fX && block[iPoint] > MIN_VALUE) fX = block[iPoint];
            if (block[iPoint] > fY && block[iPoint] < MAX_VALUE) fY = block[iPoint];
         }
         if (fX == fY) fY = MAX_VALUE;
      }
      // Use Newton's Method to find local minima of sum-of-squares error.
      const auto fSteps = static_cast<float>(cSteps - 1);
      for (size_t iIteration = 0; iIteration < 8; iIteration++)
      {
         if ((fY - fX) < (1.0f / 256.0f)) break;
         float const fScale = fSteps / (fY - fX);
         // Calculate new steps
         float pSteps[8];
         for (size_t iStep = 0; iStep < cSteps; iStep++)
            pSteps[iStep] = pC[iStep] * fX + pD[iStep] * fY;
         if (6 == cSteps)
         {
            pSteps[6] = MIN_VALUE;
            pSteps[7] = MAX_VALUE;
         }
         // Evaluate function, and derivatives
         float dX = 0.0f;
         float dY = 0.0f;
         float d2X = 0.0f;
         float d2Y = 0.0f;
         for (size_t iPoint = 0; iPoint < BCPixelBlock; iPoint++)
         {
            const float fDot = (block[iPoint] - fX) * fScale;
            uint32_t iStep;
            if (fDot <= 0.0f)
            {
               // D3DX10 / D3DX11 didn't take into account the proper minimum value for the bRange (BC4S/BC5S) case
               iStep = ((6 == cSteps) && (block[iPoint] <= (fX + MIN_VALUE) * 0.5f)) ? 6u : 0u;
            }
            else if (fDot >= fSteps)
            {
               iStep = ((6 == cSteps) && (block[iPoint] >= (fY + MAX_VALUE) * 0.5f)) ? 7u : (cSteps - 1);
            }
            else
            {
               iStep = uint32_t(fDot + 0.5f);
            }
            if (iStep < cSteps)
            {
               // D3DX had this computation backwards (pPoints[iPoint] - pSteps[iStep])
               // this fix improves RMS of the alpha component
               const float fDiff = pSteps[iStep] - block[iPoint];

               dX += pC[iStep] * fDiff;
               d2X += pC[iStep] * pC[iStep];

               dY += pD[iStep] * fDiff;
               d2Y += pD[iStep] * pD[iStep];
            }
         }
         // Move endpoints
         if (d2X > 0.0f) fX -= dX / d2X;
         if (d2Y > 0.0f) fY -= dY / d2Y;
         if (fX > fY) std::swap(fX, fY);
         if ((dX * dX < (1.0f / 64.0f)) && (dY * dY < (1.0f / 64.0f))) break;
      }
      *pX = (fX < MIN_VALUE) ? MIN_VALUE : (fX > MAX_VALUE) ? MAX_VALUE : fX;
      *pY = (fY < MIN_VALUE) ? MIN_VALUE : (fY > MAX_VALUE) ? MAX_VALUE : fY;
   }

   void EncodeBC3RGBA(bool RGBDithering)
   {

   }

   // The block should be located in 4 rows.
   void EncodeBC4Alpha(const float* block, uint8_t* destination)
   {
      // Step 1: Find end points.
      bool bUsing4BlockCodec = false;
      for (size_t i = 1; i < BCPixelBlock; ++i)
      {
         //  If there are boundary values in input texels, should use 4 interpolated color values to guarantee
         //  the exact code of the boundary values.
         if (block[i] == 0 || block[i] == 1)
         {
            bUsing4BlockCodec = true;
            break;
         }
      }
      // Using Optimize
      float fStart, fEnd;
      if (bUsing4BlockCodec)
      {
         // 4 interpolated color values
         OptimizeAlpha(&fStart, &fEnd, block, 6);
         destination[0] = ColorFloat2UInt8(fStart);
         destination[1] = ColorFloat2UInt8(fEnd);
      }
      else
      {
         // 6 interpolated color values
         OptimizeAlpha(&fStart, &fEnd, block, 8);
         destination[0] = ColorFloat2UInt8(fEnd);
         destination[1] = ColorFloat2UInt8(fStart);
      }
      // Step2: Compute indices, which follows the below mapping:
      // 0-C0, 1-C1, 2-Interpolation1, ..., 5-Interpolation4, 6-Interpolation5/0.0f, 7-Interpolation6/1.0f
      for (size_t i = 0; i < BCPixelBlock; ++i)
      {
         uint32_t value;
         if (bUsing4BlockCodec)
         {
            if (block[i] == 0) value = 6;
            else if (block[i] == 1) value = 7;
            if (fStart > 0 && fStart > block[i]) value = (fStart - block[i]) / fStart <= 0.5f ? 6 : 0;
            if (fEnd < 1 && fEnd < block[i]) value = (block[i] - fEnd) / (1 - fEnd) <= 0.5f ? 1 : 7;
            else
            {
               value = uint32_t(5.f * (block[i] - fStart) / (fEnd - fStart) + 0.5f);
            }
         }
         else
         {
            value = uint32_t(7.f * (block[i] - fEnd) / (fStart - fEnd) + 0.5f);
            if (value == fEnd) value = 0;
            else if (value == fStart) value = 1;
            else value += 1;
         }
         PlaceBCIndex(destination, value, i);
      }
   }

   void EncodeBC5Normal()
   {

   }

   ForceInline void ApplyBarrier(ComPtr<ICommandList>& cmdList, ComPtr<IResource>& resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
   {
      D3D12_RESOURCE_BARRIER barrier
      {
         D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
         D3D12_RESOURCE_BARRIER_FLAG_NONE,
         D3D12_RESOURCE_TRANSITION_BARRIER { resource.Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, before, after }
      };
      cmdList->ResourceBarrier(1, &barrier);
   }

   // Return true if the client size doesn't change.
   ForceInline bool GetClientSize()
   {
      RECT rect{};
      GetClientRect(hwnd, &rect);
      auto oldValue = backbufferSize;
      backbufferSize = XMINT2{ rect.right, rect.bottom };
      return oldValue == backbufferSize;
   }

   void CreateBase()
   {
      // Factory
      uint32_t factoryFlags = 0;
#ifdef PILLOW_DEBUG
      factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
      ComPtr<ID3D12Debug3> debugController;
      CheckHResult(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
      debugController->EnableDebugLayer();
#endif
      CheckHResult(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)));
      BOOL winBool = 0;
      factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &winBool, sizeof(winBool));
      allowTearing = (winBool == TRUE);
      // Device
      try
      {
         CheckHResult(D3D12CreateDevice(nullptr, Constants::DX12FeatureLevel, IID_PPV_ARGS(&device))); // Default adapter
      }
      catch (...)
      {
         ComPtr<IDXGIAdapter> Warp;
         CheckHResult(factory->EnumWarpAdapter(IID_PPV_ARGS(&Warp)));
         CheckHResult(D3D12CreateDevice(Warp.Get(), Constants::DX12FeatureLevel, IID_PPV_ARGS(&device)));
      }
      // Queue
      D3D12_COMMAND_QUEUE_DESC queueDesc{ D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0 };
      CheckHResult(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue)));
      // Fence
      fenceSync = std::make_unique<FenceSync>(device, cmdQueue);
      // Swapchain
      DXGI_SWAP_CHAIN_DESC1 swapChainDesc
      {
         0,0, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, false, DXGI_SAMPLE_DESC{1, 0}/*no obselete MSAA*/,
         DXGI_USAGE_RENDER_TARGET_OUTPUT, Constants::SwapChainSize, DXGI_SCALING_NONE,
         DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL/*need to access previous frame buffers*/, DXGI_ALPHA_MODE_IGNORE,
         uint32_t(allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING/*allow to disable V-Sync*/ : 0)
      };
      CheckHResult(factory->CreateSwapChainForHwnd(cmdQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, swapChain.GetAddressOf()));
      DXGI_RGBA color{ 0.f, 0.f, 0.f, 1.f };
      swapChain->SetBackgroundColor(&color);
      // Command Allocators & Lists
      int32_t count = Constants::SwapChainSize * threads;
      cmdAllocators.reserve(count);
      for (int i = 0; i < count; i++)
      {
         ComPtr<ID3D12CommandAllocator> temp;
         CheckHResult(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&temp)));
         cmdAllocators.push_back(std::move(temp));
      }
      // CreateCommandList1 closes the cmd list automatically.
      cmdLists.reserve(threads);
      _cmdLists.reserve(threads);
      for (int i = 0; i < threads; i++)
      {
         ComPtr<ICommandList> temp;
         CheckHResult(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&temp)));
         _cmdLists.push_back(temp.Get());
         cmdLists.push_back(std::move(temp));
      }
      // Others
      lateReleaseMgr = std::make_unique<lateReleaseManager>();
   }

   void CreateHeapsAndPSOs()
   {
      // Build all descriptor heaps.
      descriptorMgr = std::make_unique<DescriptorHeapManager>(device);

      // Create constant buffer and pass cbv.

   }

   void CreateFrames()
   {
      D3D12_RENDER_TARGET_VIEW_DESC rtvDesc
      {
         DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RTV_DIMENSION_TEXTURE2D
      };
      rtvDesc.Texture2D = { 0,0 };
      for (int i = 0; i < Constants::SwapChainSize; i++)
      {
         // After resizing the swapchain, the frame array index may not be euqal to the active backbuffer index.
         // So, we should associate the first buffer of the resized swapchain to the current frame array index.
         // e.g. frameIdx = 8, frameArrayIdx = 2, in this case, backbuffers[2] should refer to swapChain->GetBuffer(0).
         int32_t offset = (fenceSync->GetFrameIndex() + i) % Constants::SwapChainSize;
         CheckHResult(swapChain->GetBuffer(i, IID_PPV_ARGS(&backbuffers[offset])));
         tempRTVs[offset] = descriptorMgr->CreateView(device, backbuffers[offset], &rtvDesc, ViewType::RTV);
      }
   }

   void TryResizingSwapchain()
   {
      // Interval check.
      constexpr double MinInterval = 1.0 / 60.0;
      static double interval = 0;
      interval += GlobalDeltaTime;
      if (interval < MinInterval) return;
      interval = 0;
      if (GetClientSize()) return;
      // Resize the swapchain.
      fenceSync->FlushQueue();
      for (int i = 0; i < Constants::SwapChainSize; i++)
      {
         backbuffers[i].Reset();
         descriptorMgr->ReleaseView(tempRTVs[i]);
      }
      CheckHResult(swapChain->ResizeBuffers(Constants::SwapChainSize, 0, 0, DXGI_FORMAT_R8G8B8A8_UNORM,
         allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING/*allow to disable V-Sync*/ : 0));
      CreateFrames();
   }

   void BlockCompressionEncode()
   {

   }

   void RendererTestZone()
   {
      // footprint
      D3D12_RESOURCE_DESC resourceDesc
      {
         D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, 512, 512, 1, 10, DXGI_FORMAT_R8G8B8A8_UNORM,
         DXGI_SAMPLE_DESC{1, 0}, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE
      };
      D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint[10];
      uint32_t rows[10];
      uint64_t rowSize[10];
      uint64_t totalSize;
      device->GetCopyableFootprints(&resourceDesc, 0, 10, 0, footprint, rows, rowSize, &totalSize);
   }
}

D3D12Renderer::D3D12Renderer(HWND windowHandle, int32_t threadCount) : GenericRenderer(threadCount, "D3D12Renderer")
{
   SingletonCheck();
   hwnd = windowHandle;
   threads = threadCount;
   GetClientSize();
   CreateBase();
   CreateHeapsAndPSOs();
   CreateFrames();
   RendererTestZone();
}

D3D12Renderer::~D3D12Renderer()
{
}

uint64_t D3D12Renderer::GetFrameIndex()
{
   return fenceSync->GetFrameIndex();
}

void D3D12Renderer::ReleaseResource(uint32_t handle)
{
}

void D3D12Renderer::Worker(int32_t workerIndex)
{
   int32_t frameIdx = fenceSync->GetFrameArrayIdx();
   ComPtr<ICommandList>& cmdList = cmdLists[workerIndex];
   ID3D12CommandAllocator* allocator = cmdAllocators[frameIdx * threads + workerIndex].Get();
   CheckHResult(allocator->Reset());
   CheckHResult(cmdList->Reset(allocator, nullptr));
   if (workerIndex == 0) UnitedBuffer::GPUCopy(cmdList); // Copy all dirty buffers to default heaps.
   // Do actual work.
   if (workerIndex == 0)
   {
      ApplyBarrier(cmdList, backbuffers[frameIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
      XMFLOAT4 color{ 0.5f + 0.5f * XMScalarCos(2 * GlobalLastingTime), 0.5f + 0.5f * XMScalarCos(2 * GlobalLastingTime + 2),0.5f + 0.5f * XMScalarCos(2 * GlobalLastingTime + 4),0 };
      cmdList->ClearRenderTargetView(descriptorMgr->GetCPUHandle(tempRTVs[frameIdx]), (float*)(&color), 0, nullptr);
      ApplyBarrier(cmdList, backbuffers[frameIdx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
   }
   CheckHResult(cmdList->Close());
}

void Pillow::Graphics::D3D12Renderer::Pioneer()
{
   TryResizingSwapchain();
}

void D3D12Renderer::Assembler()
{
   lateReleaseMgr->ReleaseGarbage(); // Place it here, so it works not in the main thread.
   cmdQueue->ExecuteCommandLists(threads, _cmdLists.data());
   CheckHResult(swapChain->Present(verticalBlanks, (allowTearing && verticalBlanks == 0) ? DXGI_PRESENT_ALLOW_TEARING : 0));
   fenceSync->NextFrame();
}
#endif
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

using namespace Pillow;
using Microsoft::WRL::ComPtr;

typedef IDXGIFactory5 IFactory;                  // Has CheckFeatureSupport()
typedef ID3D12Device4 IDevice;                   // Has CreateCommandList1()
typedef ID3D12GraphicsCommandList2 ICommandList; // Has WriteBufferImmediate()
typedef IDXGISwapChain1 ISwapChain;              // Has SetBackgroundColor() 
typedef ID3D12Resource IResource;                // The original one is fine

// An anonymous namespace has internal linkage (accessable in local translation unit)

// Static variables
namespace
{
   class FenceSync;
   class DescriptorHeapManager;
   class lateReleaseManager;
   class GeneralBuffer;

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
   // TODO: add a BC encoder
   const DXGI_FORMAT Generic2DxgiFormat[int32_t(GenericTextureFormat::Count)]
   {
      DXGI_FORMAT_R8G8B8A8_UNORM, //BC3
      DXGI_FORMAT_R8G8_UNORM, //BC5
      DXGI_FORMAT_R8_UNORM, //BC4
   };

   ForceInline D3D12_RESOURCE_BARRIER CreateBarrier(ComPtr<IResource>& resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

   // Fence synchronization wrapper
   class FenceSync
   {
      ReadonlyProperty(uint64_t, FrameIndex)
         ReadonlyProperty(uint64_t, CompletedFence)

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
      // Invoke this BEFORE entering worker threads or after NextFrame() in one frame.
      void FlushQueue()
      {
         uint64_t minFence = _FrameIndex;
         Synchronize(minFence);
      }

   private:
      void Synchronize(uint64_t targetFence)
      {
         // Make sure the GPU arrives at the targetFence.
         _CompletedFence = fence->GetCompletedValue();
         if (_CompletedFence < targetFence)
         {
            fence->SetEventOnCompletion(targetFence, syncEventHandle);
            WaitForSingleObjectEx(syncEventHandle, INFINITE, true);
            _CompletedFence = targetFence;
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
      lateReleaseManager(){};

      // Enqueue an element that will be released after current frame.
      void Enqueue(std::unique_ptr<GeneralBuffer>&& buffer)
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
         std::unique_ptr<GeneralBuffer> buffer;
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
         LogSystem(L"ViewHandle=" + std::to_wstring(handle) + L" Index=" + std::to_wstring(RemoveFlag(handle)));
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
      const uint32_t FlagBits = 2;
      enum struct InnerFlag : uint32_t { CSU = 0, RTV = 1, DSV = 2 };
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

      ForceInline InnerFlag GetInnerFlag(uint16_t handle)
      {
         return InnerFlag(handle >> 14);
      }

      ForceInline uint16_t RemoveFlag(uint16_t handle)
      {
         return handle & 0x3FFF; // Clear the flag bits
      }
   };

   enum class BufferDataType
   {
      Texture,
      ConstantBuffer,
      VertexOrIndexBuffer
   };

   enum class BufferHeapType
   {
      Upload = D3D12_HEAP_TYPE_UPLOAD,
      Default = D3D12_HEAP_TYPE_DEFAULT
   };

   class GeneralBuffer
   {
      DeleteDefautedMethods(GeneralBuffer)
   public:
      const BufferHeapType HeapType;
      const BufferDataType DataType;
      const GenericTextureInfo TexInfo;
      const int32_t ElementCount;
      const int32_t RawElementSize;
      const int32_t TotalSize;
      const bool isConstantData;

      // The element size is 4 for an element of an R8G8B8A8 texture.
      GeneralBuffer(BufferHeapType heapType, BufferDataType dataType, int32_t count, int32_t _rawElementSize, bool isConstantData = true, const GenericTextureInfo& texInfo = {}) :
         HeapType(heapType),
         DataType(dataType),
         TexInfo(texInfo),
         ElementCount(count),
         RawElementSize(_rawElementSize),
         TotalSize(GetAlignedElementSize(dataType, _rawElementSize)* count),
         isConstantData(isConstantData)
      {
         if (heapType == BufferHeapType::Default)
         {
            middleBuffer = std::make_unique<GeneralBuffer>(BufferHeapType::Upload, dataType, count, _rawElementSize, isConstantData, texInfo);
         }

         bool isUpload = heapType == BufferHeapType::Upload;
         D3D12_RESOURCE_DESC resourceDesc
         {
            D3D12_RESOURCE_DIMENSION_BUFFER, 0, (uint64_t)TotalSize, 1, 1, 1, DXGI_FORMAT_UNKNOWN,
            DXGI_SAMPLE_DESC{1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE
         };
         D3D12_HEAP_PROPERTIES heapProps
         {
            D3D12_HEAP_TYPE(heapType),
            isUpload ? D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE : D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE,
            isUpload ? D3D12_MEMORY_POOL_L0 : D3D12_MEMORY_POOL_L1,
            0, 0
         };
         if (dataType == BufferDataType::Texture)
         {
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resourceDesc.Width = resourceDesc.Height = (uint32_t)texInfo.GetWidth();
            resourceDesc.DepthOrArraySize = (uint16_t)texInfo.GetArraySliceCount();
            resourceDesc.MipLevels = (uint16_t)texInfo.GetMipSliceCount();
            resourceDesc.Format = Generic2DxgiFormat[(int32_t)texInfo.GetFormat()];
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            heapProps.Type = isUpload ? D3D12_HEAP_TYPE_CUSTOM : heapProps.Type;
         }
         CheckHResult(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
            &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&heap)));
         gpuPointer = heap->GetGPUVirtualAddress();
         D3D12_RANGE range{ 0, 0 };
         CheckHResult(heap->Map(0, &range, (void**)(&cpuPointer)))
      }

      ~GeneralBuffer() = default;

      uint64_t GetGPUAddress(int index = 0) { return gpuPointer + index * RawElementSize; };

      void Write(const char* data, int indexOffset = 0, int _elementCount = 1)
      {
         if (DataType == BufferDataType::Texture) throw new std::exception("Cannot use Write() with texture buffers.");
         if (indexOffset + _elementCount > ElementCount) throw std::exception("Out of Range");
         if (HeapType == BufferHeapType::Default)
         {
            if (isConstantData) lateReleaseMgr->Enqueue(std::move(middleBuffer));
            middleBuffer->Write(data, indexOffset, _elementCount);
            //......
         }
         else
         {
            if (DataType == BufferDataType::VertexOrIndexBuffer)
            {
               memcpy((char*)cpuPointer + indexOffset * RawElementSize, data, _elementCount * RawElementSize);
            }
            else if (DataType == BufferDataType::ConstantBuffer)
            {
               for (int i = 0; i < _elementCount; i++)
               {
                  int32_t destOffset = (indexOffset + i) * GetAlignedElementSize(DataType, RawElementSize);
                  int32_t srcOffset = (indexOffset + i) * RawElementSize;
                  memcpy(cpuPointer + destOffset, data + srcOffset, RawElementSize);
               }
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
      void WriteTexture(const char* data, const GenericTextureInfo& texInfo, int32_t arrayIndex = 0)
      {
         if (DataType != BufferDataType::Texture) throw std::exception("Cannot use WriteTexture() with non-texture buffers.");
         if (HeapType == BufferHeapType::Default)
         {
            if (!middleBuffer) throw std::runtime_error("Constant default buffer cannot be written twice.");
            if (isConstantData) lateReleaseMgr->Enqueue(std::move(middleBuffer));
            middleBuffer->WriteTexture(data, texInfo, arrayIndex);
            dirtyBuffer.push_back(this);
         }
         else
         {
            for (int mip = 0; mip < texInfo.GetMipSliceCount(); mip++)
            {
               int32_t rowPixels = texInfo.GetWidth() >> mip;
               int32_t rowPitch = rowPixels * texInfo.GetPixelSize();
               int32_t depthPitch = rowPixels * rowPitch;
               heap->WriteToSubresource(arrayIndex * texInfo.GetMipSliceCount(), nullptr, data, rowPitch, depthPitch);
               data += depthPitch;
            }
         }
      }

      static void CmdList_CopyAllDefaultBuffers(ComPtr<ICommandList>& cmdList)
      {
         if (dirtyBuffer.empty()) return;
         while (!dirtyBuffer.empty())
         {
            GeneralBuffer& buffer = *dirtyBuffer.back();
            dirtyBuffer.pop_back();
            //if (buffer.HeapType != BufferHeapType::Default) throw std::exception("Invalid buffer type.");
            // Before barrier
            auto barrier = CreateBarrier(buffer.middleBuffer->heap, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE);
            cmdList->ResourceBarrier(1, &barrier);
            barrier = CreateBarrier(buffer.heap, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
            cmdList->ResourceBarrier(1, &barrier);
            // GPU Copy
            if (buffer.DataType == BufferDataType::Texture)
            {
               for (uint32_t i = 0; i < buffer.TexInfo.GetMipSliceCount(); i++)
               {
                  D3D12_TEXTURE_COPY_LOCATION sourceLoc{ buffer.middleBuffer->heap.Get(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, i };
                  D3D12_TEXTURE_COPY_LOCATION destinationLoc{ buffer.heap.Get(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, i };
                  cmdList->CopyTextureRegion(&destinationLoc, 0, 0, 0, &sourceLoc, nullptr);
               }
            }
            else
            {
               //cmdList->CopyBufferRegion(buffer.heap.Get(), 0, buffer.middleBuffer->heap.Get(), buffer.middleBuffer->TotalSize);
               cmdList->CopyResource(buffer.heap.Get(), buffer.middleBuffer->heap.Get());
            }
            // After barrier
            barrier = CreateBarrier(buffer.heap, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
            cmdList->ResourceBarrier(1, &barrier);
         }
      }

   private:
      // TODO: Middle Buffer Pool Optimization
      // static std::vector<std::unique_ptr<GeneralBuffer>> middleBufferPool;
      inline static std::vector<GeneralBuffer*> dirtyBuffer{};

      std::unique_ptr<GeneralBuffer> middleBuffer{};
      ComPtr<IResource> heap{};
      uint64_t gpuPointer{};
      char* cpuPointer{};

      static int32_t GetAlignedElementSize(BufferDataType type, int32_t _rawSize)
      {
         return type == BufferDataType::ConstantBuffer ? (_rawSize + 255) & ~255 : _rawSize;
      }
   };
}

// Static functions
namespace
{
   ForceInline D3D12_RESOURCE_BARRIER CreateBarrier(ComPtr<IResource>& resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
   {
      D3D12_RESOURCE_BARRIER barrier
      {
         D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
         D3D12_RESOURCE_BARRIER_FLAG_NONE,
         D3D12_RESOURCE_TRANSITION_BARRIER { resource.Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, before, after }
      };
      return barrier;
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
   ICommandList* cmdList = cmdLists[workerIndex].Get();
   ID3D12CommandAllocator* allocator = cmdAllocators[frameIdx * threads + workerIndex].Get();
   CheckHResult(allocator->Reset());
   CheckHResult(cmdList->Reset(allocator, nullptr));
   // Do actual work.
   if (workerIndex == 0)
   {
      auto barrier = CreateBarrier(backbuffers[frameIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
      cmdList->ResourceBarrier(1, &barrier);
      
      using namespace DirectX;
      XMFLOAT4 color{ 0.5f + 0.5f * XMScalarCos(2 * lastingTime), 0.5f + 0.5f * XMScalarCos(2 * lastingTime + 2),0.5f + 0.5f * XMScalarCos(2 * lastingTime + 4),0 };
      cmdList->ClearRenderTargetView(descriptorMgr->GetCPUHandle(tempRTVs[frameIdx]), (float*)(&color), 0, nullptr);
      
      barrier = CreateBarrier(backbuffers[frameIdx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
      cmdList->ResourceBarrier(1, &barrier);
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
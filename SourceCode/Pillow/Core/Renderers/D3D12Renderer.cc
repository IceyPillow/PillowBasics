#if defined(_WIN64)
#include "Renderer.h"
#include <memory>
#include <vector>
#include <comdef.h>
#include <queue>
#include <wrl.h> // import Component Object Model Pointer
#include <d3d12.h>
#include <dxgi1_6.h>

using namespace Pillow;
using namespace Pillow::Graphics;
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

// An anonymous namespace has internal linkage (accessable in local translation unit)
namespace
{
   class FenceSync;
   class DescriptorHeapManager;
   class DelayReleaseManager;
   class GeneralBuffer;

   std::unique_ptr<FenceSync> fence;
   std::unique_ptr<DescriptorHeapManager> heapMgr;
   std::unique_ptr<DelayReleaseManager> delayReleaseMgr;
   ComPtr<IDXGIFactory6> dxgiFactory;
   ComPtr<ID3D12Device6> d3d12Device;
   ComPtr<ID3D12CommandQueue> cmdQueue;
   ComPtr<ID3D12GraphicsCommandList5> cmdList;
   ComPtr<ID3D12CommandAllocator> cmdAllocator[Constants::SwapChainSize];
   ComPtr<IDXGISwapChain1> swapChain;
   HWND Hwnd;

   const DXGI_FORMAT Generic2DxgiFormat[(int32_t)GenericTextureFormat::Count]
   {
      DXGI_FORMAT_R10G10B10A2_UNORM,
      DXGI_FORMAT_B8G8R8A8_UNORM,
      DXGI_FORMAT_R8_UNORM,
      DXGI_FORMAT_R32_FLOAT
   };

   D3D12_RESOURCE_BARRIER CreateBarrier(const ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
   {
      D3D12_RESOURCE_BARRIER barrier
      {
         D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
         D3D12_RESOURCE_BARRIER_FLAG_NONE,
         D3D12_RESOURCE_TRANSITION_BARRIER { resource.Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, before, after }
      };
      return barrier;
   }

   // Fence synchronization wrapper
   class FenceSync
   {
      ReadonlyProperty(int32_t, FrameIdx)
         ReadonlyProperty(uint64_t, CompletedFence)
         ReadonlyProperty(uint64_t, TargetFence)

   public:
      FenceSync(ComPtr<ID3D12Device6>& device, ComPtr<ID3D12CommandQueue>& commandQueue)
      {
         this->commandQueue = commandQueue;
         syncEventHandle = CreateEventEx(nullptr, L"FencSync", 0, EVENT_ALL_ACCESS);
         CheckHResult(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
         _TargetFence = 1;
      }

      ~FenceSync()
      {
         CloseHandle(syncEventHandle);
      }

      // Advance the frame index, wait only if no available frame. Invoke this AFTER ExecuteCommandList()
      // flushQueue: Frame index unchanged if set this TRUE.
      void Synchronize(bool flushQueue)
      {
         commandQueue->Signal(fence.Get(), _TargetFence);
         frameEndTimestamps[_FrameIdx] = _TargetFence++;
         // Cycle through the circular frame resource array.
         if (!flushQueue) _FrameIdx = (_FrameIdx + 1) % Constants::SwapChainSize;
         int64_t endTimestamp = frameEndTimestamps[_FrameIdx];
         // Has the GPU finished processing the commands of the current frame resource?
         // If not, wait until the GPU has completed commands up to this fence point.
         if (endTimestamp > 0 && fence->GetCompletedValue() < endTimestamp)
         {
            fence->SetEventOnCompletion(endTimestamp, syncEventHandle);
            WaitForSingleObjectEx(syncEventHandle, INFINITE, true);
         }
         _CompletedFence = endTimestamp;
      }

   private:
      ComPtr<ID3D12Fence1> fence;
      HANDLE syncEventHandle;
      ComPtr<ID3D12CommandQueue> commandQueue;
      int64_t frameEndTimestamps[Constants::SwapChainSize]{};
   };

   class DelayReleaseManager
   {
   public:
      DelayReleaseManager() {};

      // Enqueue an element that will be released after current frame.
      void Enqueue(std::unique_ptr<GeneralBuffer>&& buffer)
      {
         Item item{ std::move(buffer), fence->GetTargetFence() };
         releaseQueue.push(std::move(item));
      }

      void Update()
      {
         uint64_t completedFence = fence->GetCompletedFence();
         if (completedFence == 0) return;
         while (!releaseQueue.empty())
         {
            Item& item = releaseQueue.front();
            // FIFO means if one element dequeued is uncompleted, remains also.
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

   enum class ViewType
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
      DescriptorHeapManager(ComPtr<ID3D12Device6>& device) :
         csuSize(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
         rtvSize(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)),
         dsvSize(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV))
      {
         csuFreePool.reserve(MaxCsuCount);
         rtvFreePool.reserve(MaxRtvCount);
         dsvFreePool.reserve(MaxDsvCount);
         for (int32_t i = MaxCsuCount; i >= 0; i--) csuFreePool.push_back(i);
         for (int32_t i = MaxRtvCount; i >= 0; i--) rtvFreePool.push_back(i);
         for (int32_t i = MaxDsvCount; i >= 0; i--) dsvFreePool.push_back(i);

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

      void BindSrvHeap(ComPtr<ID3D12GraphicsCommandList5>& cmd)
      {
         cmd->SetDescriptorHeaps(1, csuDescHeap.GetAddressOf());
      }

      D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(int32_t idx, ViewType type)
      {
         auto CheckIdx = [&idx](int32_t max) {if (idx < 0 || idx >= max) throw std::exception("Out of Range"); };
         D3D12_CPU_DESCRIPTOR_HANDLE result{};
         switch (type)
         {
         case ViewType::CBV:
         case ViewType::SRV:
         case ViewType::UAV:
            CheckIdx(MaxCsuCount);
            result.ptr = csuCpuHandle0.ptr + csuSize * idx;
            break;
         case ViewType::RTV:
            CheckIdx(MaxRtvCount);
            result.ptr = rtvCpuHandle0.ptr + rtvSize * idx;
            break;
         case ViewType::DSV:
            CheckIdx(MaxDsvCount);
            result.ptr = dsvCpuHandle0.ptr + dsvSize * idx;
            break;
         }
         return result;
      }

      D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(int32_t idx, ViewType type)
      {
         auto CheckIdx = [&idx](int32_t max) {if (idx < 0 || idx >= max) throw std::exception("Out of Range"); };
         D3D12_GPU_DESCRIPTOR_HANDLE result{};
         switch (type)
         {
         case ViewType::CBV:
         case ViewType::SRV:
         case ViewType::UAV:
            CheckIdx(MaxCsuCount);
            result.ptr = csuGpuHandle0.ptr + csuSize * idx;
            break;
         default:
            throw std::exception("GPU handle is not supported for RTV and DSV.");
         }
         return result;
      }

      int32_t CreateView(ComPtr<ID3D12Device6>& device, ComPtr<ID3D12Resource>& res, void* viewDesc, ViewType type)
      {
         int32_t idx{};
         auto CheckAndPop = [&idx](std::vector<int32_t>& vector, const char* name) {
            if (vector.empty())
               throw std::exception((std::string(name) + ": A descriptor heap is full.").c_str());
            idx = vector.back();
            vector.pop_back();
            };
         switch (type)
         {
         case ViewType::CBV:
            CheckAndPop(csuFreePool, "CSV_SRV_UAV");
            device->CreateConstantBufferView((D3D12_CONSTANT_BUFFER_VIEW_DESC*)viewDesc, GetCPUHandle(idx, type));
            break;
         case ViewType::SRV:
            CheckAndPop(csuFreePool, "CSV_SRV_UAV");
            device->CreateShaderResourceView(res.Get(), (D3D12_SHADER_RESOURCE_VIEW_DESC*)viewDesc, GetCPUHandle(idx, type));
            break;
         case ViewType::UAV:
            CheckAndPop(csuFreePool, "CSV_SRV_UAV");
            device->CreateUnorderedAccessView(res.Get(), nullptr, (D3D12_UNORDERED_ACCESS_VIEW_DESC*)viewDesc, GetCPUHandle(idx, type));
            break;
         case ViewType::RTV:
            CheckAndPop(rtvFreePool, "RTV");
            device->CreateRenderTargetView(res.Get(), (D3D12_RENDER_TARGET_VIEW_DESC*)viewDesc, GetCPUHandle(idx, type));
            break;
         case ViewType::DSV:
            CheckAndPop(dsvFreePool, "DSV");
            device->CreateDepthStencilView(res.Get(), (D3D12_DEPTH_STENCIL_VIEW_DESC*)viewDesc, GetCPUHandle(idx, type));
         }
      }

      void ReleaseView(int32_t idx, ViewType type)
      {
         auto CheckAndPush = [&idx](std::vector<int32_t>& vector) {
#ifdef _DEBUG
            bool found = std::find(vector.begin(), vector.end(), idx) != vector.end();
            if (found) throw std::exception("Invalid index.");
#endif
            vector.push_back(idx);
            };
         switch (type)
         {
         case ViewType::CBV:
         case ViewType::SRV:
         case ViewType::UAV:
            CheckAndPush(csuFreePool);
            break;
         case ViewType::RTV:
            CheckAndPush(rtvFreePool);
            break;
         case ViewType::DSV:
            CheckAndPush(dsvFreePool);
            break;
         }
      }

   private:
      const int MaxCsuCount = 1024;
      const int MaxRtvCount = 16;
      const int MaxDsvCount = 1;
      const int32_t csuSize, rtvSize, dsvSize;
      ComPtr<ID3D12DescriptorHeap> csuDescHeap;
      ComPtr<ID3D12DescriptorHeap> rtvDescHeap;
      ComPtr<ID3D12DescriptorHeap> dsvDescHeap;
      std::vector<int32_t> csuFreePool;
      std::vector<int32_t> rtvFreePool;
      std::vector<int32_t> dsvFreePool;
      D3D12_CPU_DESCRIPTOR_HANDLE csuCpuHandle0;
      D3D12_GPU_DESCRIPTOR_HANDLE csuGpuHandle0;
      // RTV and DSV don't have gpu handles.
      D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle0;
      D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle0;
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
         TotalSize(GetAlignedElementSize(dataType, _rawElementSize) * count),
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
         CheckHResult(d3d12Device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
            &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&heap)));
         gpuPointer = heap->GetGPUVirtualAddress();
         D3D12_RANGE range {0, 0};
         CheckHResult(heap->Map(0, &range, (void**)(&cpuPointer)))
      }

      ~GeneralBuffer() = default;

      uint64_t GetGPUAddress(int index = 0) { return gpuPointer + index * RawElementSize; };

      void Write(const char* data, int indexOffset = 0, int _elementCount = 1)
      {
         if (indexOffset + _elementCount > ElementCount) throw std::exception("Out of Range");
         if (DataType == BufferDataType::Texture) throw new std::exception("Cannot use Write() with texture buffers.");
         if (HeapType == BufferHeapType::Default)
         {
            if (isConstantData) delayReleaseMgr->Enqueue(std::move(middleBuffer));
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

      // D3D12 texture subresource indexing: SubRes[PlaneIdx][ArrayIdx][MipIdx]
      // Typically planar formats are not used to store RGBA data.
      void WriteTexture(const char* data, const GenericTextureInfo& texInfo, int32_t arrayIndex = 0)
      {
         if (DataType != BufferDataType::Texture) throw std::exception("Cannot use WriteTexture() with non-texture buffers.");
         if (HeapType == BufferHeapType::Default)
         {
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

      static void CmdList_CopyAllDefaultBuffers()
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
      static std::vector<GeneralBuffer*> dirtyBuffer;

      std::unique_ptr<GeneralBuffer> middleBuffer{};
      ComPtr<ID3D12Resource> heap{};
      uint64_t gpuPointer{};
      char* cpuPointer{};

      static int32_t GetAlignedElementSize(BufferDataType type, int32_t _rawSize)
      {
         return type == BufferDataType::ConstantBuffer ? (_rawSize + 255) & ~255 : _rawSize;
      }
   };

   std::vector<GeneralBuffer*> GeneralBuffer::dirtyBuffer;

   void CreateD3D12Infrastructure()
   {
      // Device
#ifdef _DEBUG
      ComPtr<ID3D12Debug3> debugController;
      CheckHResult(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
      debugController->EnableDebugLayer();
#endif
      CheckHResult(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory)));
      try
      {
         //// default adapter
         CheckHResult(D3D12CreateDevice(nullptr, Constants::DX12FeatureLevel, IID_PPV_ARGS(&d3d12Device)));
      }
      catch (...)
      {
         ComPtr<IDXGIAdapter> Warp;
         CheckHResult(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&Warp)));
         CheckHResult(D3D12CreateDevice(Warp.Get(), Constants::DX12FeatureLevel, IID_PPV_ARGS(&d3d12Device)));
      }
      // Queue
      D3D12_COMMAND_QUEUE_DESC queueDesc{ D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0 };
      CheckHResult(d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue)));
      // Fence
      fence = std::make_unique<FenceSync>(d3d12Device, cmdQueue);
      // Swapchain
      DXGI_SWAP_CHAIN_DESC1 swapChainDesc
      {
         0,0, DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM, false, DXGI_SAMPLE_DESC{1, 0}/*no obselete MSAA*/,
         DXGI_USAGE_RENDER_TARGET_OUTPUT, Constants::SwapChainSize, DXGI_SCALING_NONE,
         DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL/*need to access previous frame buffers*/, DXGI_ALPHA_MODE_IGNORE,
         DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING/*allow to disable V-Sync*/ | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
      };
      CheckHResult(dxgiFactory->CreateSwapChainForHwnd(cmdQueue.Get(), Hwnd, &swapChainDesc, nullptr, nullptr, swapChain.GetAddressOf()));
      // Command Allocators & Lists
      for (int i = 0; i < Constants::SwapChainSize; i++)
      {
         CheckHResult(d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator[i])));
      }
      // Command List: need to close it before resetting.
      CheckHResult(d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator[0].Get(), nullptr, IID_PPV_ARGS(&cmdList)));
      cmdList->Close();
   }

   void CreateHeapsAndPSOs()
   {
      // Build all descriptor heaps.
      heapMgr = std::make_unique<DescriptorHeapManager>(d3d12Device);

      // Create constant buffer and pass cbv.

   }

   void CreateFrames()
   {
      //
   }


   void func(int&& a)
   {
      ;
   }

   void func(const int a)
   {
      ;
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
      d3d12Device->GetCopyableFootprints(&resourceDesc, 0, 10, 0, footprint, rows, rowSize, &totalSize);
      ;

      // clock
      //double time = Clock::GetPrecisionInMilliseconds();
      ;
   }
}

D3D12Renderer::D3D12Renderer(HWND windowHandle, int32_t threadCount) : GenericRenderer(threadCount, "D3D12Renderer")
{
   SingletonCheck();
   ::Hwnd = windowHandle;
   CreateD3D12Infrastructure();
   CreateHeapsAndPSOs();
   CreateFrames();
   RendererTestZone();
}

D3D12Renderer::~D3D12Renderer()
{
}
uint64_t D3D12Renderer::GetFrameIndex()
{
   return fence->GetFrameIdx();
}
int32_t D3D12Renderer::CreateMesh()
{
   return 0;
}
int32_t D3D12Renderer::CreateTexture()
{
   return 0;
}
int32_t D3D12Renderer::CreatePiplelineState()
{
   return 0;
}
int32_t D3D12Renderer::CreateConstantBuffer()
{
   return 0;
}
void D3D12Renderer::ReleaseResource(int32_t handle)
{
}
void D3D12Renderer::Worker(int32_t workerIndex)
{
}
void D3D12Renderer::Assembler()
{
}
#endif
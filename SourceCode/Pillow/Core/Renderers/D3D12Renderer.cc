// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 2-Clause License. Do not remove, obscure, or alter this notice.
// TODO: bundle cmd lists
#if defined(_WIN64)
#define NOMINMAX
#include "Renderer.h"
//#include <d3d12.h> // Deprecated
#include "DX12Agility-1.618/d3d12.h" // To avoid header order issues
#include "DX12Agility-1.618/d3dx12/d3dx12.h"
//#include <d3dcompiler.h> // Deprecated
#include "dxc_feb2026/d3d12shader.h"
#include "dxc_feb2026/dxcapi.h"
#include <dxgi1_6.h>
#include <comdef.h>
#include <wrl.h> // Import Component Object Model Pointer
#undef NOMINMAX
#include <shared_mutex>
#include <memory>
#include <vector>
#include <queue>
#include <deque>
#include <array>
#include <ranges>
#include <algorithm>
#include <format>
#include <span>
#include <fstream>
#include <filesystem>
#include <exception>
#include "utfcpp-4.0.6/utf8.h"

using namespace Pillow;
using Microsoft::WRL::ComPtr;

// DirectX 12 Agility SDK 1.618.5 (618), released on 2025.05.12
// Versions: https://devblogs.microsoft.com/directx/directx12agility
// Tutorial: https://devblogs.microsoft.com/directx/gettingstarted-dx12agility
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 618; }
// Avoid the mismatch between D3D12SDKLayers (the debug layer) and D3D12Core.dll.
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

typedef uint32_t DescriptorHandle; // Inner descriptor handle

typedef IDXGIFactory5 IFactory;     // Has CheckFeatureSupport()
typedef IDXGISwapChain1 ISwapChain; // Has SetBackgroundColor()

typedef ID3D12Device4 IDevice;                   // Has CreateCommandList1()
typedef ID3D12GraphicsCommandList2 ICommandList; // Has WriteBufferImmediate()
typedef ID3D12Resource IResource;                // The original one is fine

typedef CD3DX12_PIPELINE_STATE_STREAM2 PIPELINE_STATE_STREAM; // Supports mesh shader.

// An anonymous namespace has internal linkage (accessable in local translation unit)
// Static variables
namespace
{
   // 11_0 feature level in DX12 can support GPU down to GeForce 400 series!
   // 2026.3.8
   // Update to 11_1. Resource binding tier 3 is required; The Texture2D array is essential to a bindless design.
   const D3D_FEATURE_LEVEL Direct3DFeatureLevel = D3D_FEATURE_LEVEL_11_1;
   // XeSS 2 requires shader model 6.4.
   const D3D_SHADER_MODEL MinShaderModelLevel = D3D_SHADER_MODEL_6_4;
   const int32_t AlignmentTextureRow = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
   const int32_t AlignmentTextureSubres = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
   const int32_t AlignmentConstBuffer = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
   const int32_t BCBlockLength = 16; // 4 * 4
   const int32_t BC1BlockSize = 8; // C0(2B) C1(2B) Indices(16*2bits = 4B) RGB, 1:6 zip rate
   const int32_t BC4BlockSize = 8; // C0(1B) C1(1B) Indices(16*3bits = 6B) A, 1:2 zip rate
   const int32_t BC3BlockSize = BC1BlockSize + BC4BlockSize; // RGBA, 1:4 zip rate
   const int32_t BC5BlockSize = BC4BlockSize * 2; // AA, 1:2 zip rate

   constexpr int32_t PiplelineBufferNum = (int32_t)PiplelineBuffer::Count;
   constexpr int32_t piplelineBufferArrayNum = (int32_t)Constants::SwapChainSize * PiplelineBufferNum;
   
   // TODO: BC7, mode 6 and mode 7
   const DXGI_FORMAT NativeTexFmt[int32_t(TextureInfo::Format::Count)]
   {
      DXGI_FORMAT_R8_UNORM,
      DXGI_FORMAT_R8G8_SNORM,
      DXGI_FORMAT_R8G8B8A8_UNORM,
      DXGI_FORMAT_R8G8B8A8_UNORM,
   };
   const DXGI_FORMAT NativeBCTexFmt[int32_t(TextureInfo::Format::Count)]
   {
      DXGI_FORMAT_BC4_UNORM,
      DXGI_FORMAT_BC5_UNORM,
      DXGI_FORMAT_BC1_UNORM,
      DXGI_FORMAT_BC3_UNORM,
   };
   constexpr uint32_t NativeBCBlockSize[int32_t(TextureInfo::Format::Count)]
   {
      BC4BlockSize,
      BC5BlockSize,
      BC1BlockSize,
      BC3BlockSize
   };

#define DEFAULT_INPUT_LAYOUT \
0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0
   const D3D12_INPUT_ELEMENT_DESC BasicVtx[3]
   {
      { "position", 0, DXGI_FORMAT_R32G32B32_FLOAT, DEFAULT_INPUT_LAYOUT },
      { "texIdx", 0, DXGI_FORMAT_R8G8_UINT, DEFAULT_INPUT_LAYOUT },
      { "uv01", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, DEFAULT_INPUT_LAYOUT }
   };
   const D3D12_INPUT_ELEMENT_DESC StaticVtx[5]
   {
      { "position", 0, DXGI_FORMAT_R32G32B32_FLOAT, DEFAULT_INPUT_LAYOUT },
      { "texIdx", 0, DXGI_FORMAT_R8G8_UINT, DEFAULT_INPUT_LAYOUT },
      { "uv01", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, DEFAULT_INPUT_LAYOUT },
      { "normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, DEFAULT_INPUT_LAYOUT },
      { "tangent", 0, DXGI_FORMAT_R32G32B32_FLOAT, DEFAULT_INPUT_LAYOUT }
   };
   const D3D12_INPUT_ELEMENT_DESC SkeletalVtx[5]
   {
      { "position", 0, DXGI_FORMAT_R32G32B32_FLOAT, DEFAULT_INPUT_LAYOUT },
      { "texIdx_boneIdx", 0, DXGI_FORMAT_R8G8B8A8_UINT, DEFAULT_INPUT_LAYOUT },
      { "uv01", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, DEFAULT_INPUT_LAYOUT },
      { "normal_boneWeight0", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, DEFAULT_INPUT_LAYOUT },
      { "tangent_boneWeight1", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, DEFAULT_INPUT_LAYOUT }
   };
   const D3D12_INPUT_LAYOUT_DESC InputLayoutBasic{ BasicVtx , 3};
   const D3D12_INPUT_LAYOUT_DESC InputLayoutStatic{ StaticVtx, 5 };
   const D3D12_INPUT_LAYOUT_DESC InputLayoutSkeletal{ SkeletalVtx, 5 };

#define TEX_CLAMP D3D12_TEXTURE_ADDRESS_MODE_CLAMP
#define TEX_WRAP D3D12_TEXTURE_ADDRESS_MODE_WRAP
#define MAX_MIPS 16
#define SMAPLER_DESC(filter, addressMode, cmpFunc, maxLOD, registerNum) \
{filter, addressMode, addressMode, addressMode, 0, Constants::AnisotropyLevel, cmpFunc, \
D3D12_STATIC_BORDER_COLOR(0), 0, maxLOD, registerNum, 0, D3D12_SHADER_VISIBILITY_ALL}
   const D3D12_STATIC_SAMPLER_DESC StaticSamplers[7]
   {
      SMAPLER_DESC(D3D12_FILTER_MIN_MAG_MIP_POINT, TEX_CLAMP, D3D12_COMPARISON_FUNC(0), 0, 0),         // Point-Clamp (Post-process)
      SMAPLER_DESC(D3D12_FILTER_MIN_MAG_MIP_POINT, TEX_WRAP, D3D12_COMPARISON_FUNC(0), 0, 1),          // Point-Wrap (Retro rendering)
      SMAPLER_DESC(D3D12_FILTER_MIN_MAG_MIP_LINEAR, TEX_CLAMP, D3D12_COMPARISON_FUNC(0), MAX_MIPS, 2), // Trilinear-Clamp (Post-process / UI)
      SMAPLER_DESC(D3D12_FILTER_MIN_MAG_MIP_LINEAR, TEX_WRAP, D3D12_COMPARISON_FUNC(0), MAX_MIPS, 3),  // Trilinear-Wrap (Post-process / UI)
      SMAPLER_DESC(D3D12_FILTER_ANISOTROPIC, TEX_CLAMP, D3D12_COMPARISON_FUNC(0), MAX_MIPS, 4),        // Anisotropic-Clamp (Mesh)
      SMAPLER_DESC(D3D12_FILTER_ANISOTROPIC, TEX_WRAP, D3D12_COMPARISON_FUNC(0), MAX_MIPS, 5),         // Anisotropic-Wrap (Mesh)
      SMAPLER_DESC(D3D12_FILTER_MIN_MAG_MIP_LINEAR, TEX_CLAMP, D3D12_COMPARISON_FUNC_GREATER_EQUAL, 0, 6) // LessEqual-PCF-Comparison (Shadow)
   };

   class FenceSync;
   class PipelineStateManager;
   class DescriptorHeapManeger;
   class LateReleaseManager;
   class UnionBuffer;

   // DXGI
   ComPtr<IFactory> factory;
   ComPtr<ISwapChain> swapChain;

   // D3D12
   ComPtr<IDevice> device;
   ComPtr<ID3D12CommandQueue> cmdQueue;
   std::vector<ComPtr<ICommandList>> cmdLists;
   std::vector<ID3D12CommandList*> cmdListsRaw; // A copy of cmdLists, used for ExecuteCommandLists()
   std::vector<ComPtr<ID3D12CommandAllocator>> cmdAllocators;
   ComPtr<IResource> backBuffers[Constants::SwapChainSize]{};

   // Utility Wrapper
   std::unique_ptr<FenceSync> fenceSync;
   std::unique_ptr<PipelineStateManager> psoMgr;
   std::unique_ptr<DescriptorHeapManeger> descriptorMgr;
   std::unique_ptr<LateReleaseManager> lateReleaseMgr;

   // Parameters
   std::array<DescriptorHandle, piplelineBufferArrayNum> piplelineRTVs;
   std::array<DescriptorHandle, piplelineBufferArrayNum> piplelineSRVs;
   HWND hwnd;
   D3D12Renderer* rendererInstance;
   int32_t workerThreadCount;
   bool bDeviceSupportTearing;
   XMINT2 currentBackBufferSize;
}

// Types
namespace
{
   void Check_HRESULT(HRESULT hResult);
   void ApplyBarrier(ComPtr<ICommandList>& cmdList, ComPtr<IResource>& resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
   uint32_t GetAlignMipmapSize(const TextureInfo& texInfo, uint32_t mipLevel);
   uint32_t GetAlignTextureArraySliceSize(const TextureInfo& texInfo);
   uint32_t GetPiplelineBufferIndex(PiplelineBuffer type, uint32_t frameIdx);

   // Fence synchronization wrapper
   class FenceSync
   {
      ReadonlyProperty(uint64_t, FrameIndex)

   public:
      FenceSync(ComPtr<IDevice>& device, ComPtr<ID3D12CommandQueue>& commandQueue)
      {
         this->cmdQueue = commandQueue.Get();
         syncEventHandle = CreateEventEx(nullptr, L"D3D12Renderer Fence Event", 0, EVENT_ALL_ACCESS);
         if (syncEventHandle == 0) throw std::exception("Failed to create fence sync event handle.");
         Check_HRESULT(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
      }

      ~FenceSync()
      {
         CloseHandle(syncEventHandle);
      }

      uint64_t GetTargetFence() { return f_FrameIndex + 1; }
      uint64_t GetCompletedFence() { return fence->GetCompletedValue(); }
      int32_t GetFrameArrayIdx() { return f_FrameIndex % Constants::SwapChainSize; }

      // Get the next frame.
      // ***WARNING***
      // Invoke this AFTER ExecuteCommandLists() in one frame.
      void NextFrame()
      {
         f_FrameIndex++;
         cmdQueue->Signal(fence.Get(), f_FrameIndex);
         uint64_t minFence = (f_FrameIndex < Constants::SwapChainSize) ? 0 : (f_FrameIndex - Constants::SwapChainSize + 1);
         Synchronize(minFence);
      }

      // Get all GPU's work done.
      // ***WARNING***
      // To make suere GPU doesn't access any resource, invoke this BEFORE entering worker threads or right AFTER NextFrame().
      void FlushQueue()
      {
         uint64_t minFence = f_FrameIndex;
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
      ID3D12CommandQueue* cmdQueue; // Use it, not own it.
   };

   class LateReleaseManager
   {
   public:
      LateReleaseManager()
      {
         SingletonCheck();
      }

      // Enqueue an element that will be released after current frame.
      void Enqueue(std::unique_ptr<UnionBuffer>&& buffer)
      {
         // Construt an item in the queue directly
         releaseQueue.emplace(std::move(buffer), fenceSync->GetTargetFence());
      }

      void GarbageCollect()
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
         std::unique_ptr<UnionBuffer> buffer;
         uint64_t targetFence;
      };

      std::queue<Item> releaseQueue;
   };

   class DescriptorHeapManeger
   {
   public:
      enum class Type : uint8_t
      {
         // Stored in srvUavDescHeap.
         CBV,
         SRV,
         UAV,
         // Stored in rtvDescHeap.
         RTV,
         // stored in dsvDescHeap.
         DSV,
         Count
      };

   public:
      DescriptorHeapManeger(ComPtr<IDevice>& device) :
         csuSize(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
         rtvSize(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)),
         dsvSize(device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV))
      {
         SingletonCheck();
         std::unique_lock lock(mutex);
         csuFreePool.reserve(MaxHeapCapcity);
         rtvFreePool.reserve(MaxHeapCapcity);
         dsvFreePool.reserve(MaxHeapCapcity);
         // Warning: DescriptorHandle is uint32_t, be aware of an ill-defined for-loop!
         for (DescriptorHandle idx = MaxHeapCapcity; idx > 0; idx--)
         {
            csuFreePool.push_back(idx);
            rtvFreePool.push_back(idx);
            dsvFreePool.push_back(idx);
         }

         D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc
         {
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            MaxHeapCapcity,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
         };
         Check_HRESULT(device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&csuDescHeap)));
         descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
         descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
         Check_HRESULT(device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&rtvDescHeap)));
         descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
         Check_HRESULT(device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&dsvDescHeap)));
         csuCpuHandle0 = csuDescHeap->GetCPUDescriptorHandleForHeapStart();
         csuGpuHandle0 = csuDescHeap->GetGPUDescriptorHandleForHeapStart();
         rtvCpuHandle0 = rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
         dsvCpuHandle0 = dsvDescHeap->GetCPUDescriptorHandleForHeapStart();
      }

      // Only the CSU descriptor heap is shader-visible, so only it can be bound to command lists.
      void BindCSUDescriptorHeap(ComPtr<ICommandList>& cmd)
      {
         cmd->SetDescriptorHeaps(1, csuDescHeap.GetAddressOf());
      }

      ForceInline D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(DescriptorHandle handle)
      {
         D3D12_CPU_DESCRIPTOR_HANDLE result{};
         Type type = GetType(handle);
         handle = ClearType(handle);
         switch (type)
         {
         case Type::CBV:
         case Type::SRV:
         case Type::UAV:
            result.ptr = csuCpuHandle0.ptr + csuSize * handle;
            break;
         case Type::RTV:
            result.ptr = rtvCpuHandle0.ptr + rtvSize * handle;
            break;
         case Type::DSV:
            result.ptr = dsvCpuHandle0.ptr + dsvSize * handle;
            break;
         }
         return result;
      }

      ForceInline D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(DescriptorHandle handle)
      {
         D3D12_GPU_DESCRIPTOR_HANDLE result{};
         Type type = GetType(handle);
         handle = ClearType(handle);
         switch (type)
         {
         case Type::CBV:
         case Type::SRV:
         case Type::UAV:
            result.ptr = csuGpuHandle0.ptr + csuSize * handle;
            break;
         default:
            throw std::exception("GPU handle is not supported for RTV and DSV.");
         }
         return result;
      }

      DescriptorHandle CreateDescirptor(ComPtr<IDevice>& device, ComPtr<IResource>& res, void* viewDesc, Type type)
      {
         std::unique_lock lock(mutex);
         DescriptorHandle handle{};
         auto GetHandle = [&](std::vector<DescriptorHandle>& freePool, const char* name)
            {
               if (freePool.empty())
                  throw std::exception(std::format("Descriptor heap[{}] is full, try to alter MaxHeapCapcity.", name).c_str());
               handle = SetType(freePool.back(), type);
               freePool.pop_back();
            };
         switch (type)
         {
         case Type::CBV:
            GetHandle(csuFreePool, "CSV_SRV_UAV");
            device->CreateConstantBufferView((D3D12_CONSTANT_BUFFER_VIEW_DESC*)viewDesc, GetCPUHandle(handle));
            break;
         case Type::SRV:
            GetHandle(csuFreePool, "CSV_SRV_UAV");
            device->CreateShaderResourceView(res.Get(), (D3D12_SHADER_RESOURCE_VIEW_DESC*)viewDesc, GetCPUHandle(handle));
            break;
         case Type::UAV:
            GetHandle(csuFreePool, "CSV_SRV_UAV");
            device->CreateUnorderedAccessView(res.Get(), nullptr, (D3D12_UNORDERED_ACCESS_VIEW_DESC*)viewDesc, GetCPUHandle(handle));
            break;
         case Type::RTV:
            GetHandle(rtvFreePool, "RTV");
            device->CreateRenderTargetView(res.Get(), (D3D12_RENDER_TARGET_VIEW_DESC*)viewDesc, GetCPUHandle(handle));
            break;
         case Type::DSV:
            GetHandle(dsvFreePool, "DSV");
            device->CreateDepthStencilView(res.Get(), (D3D12_DEPTH_STENCIL_VIEW_DESC*)viewDesc, GetCPUHandle(handle));
         }
#ifdef PILLOW_DEBUG
         //LogSystem(L"ViewHandle=" + std::to_wstring(handle) + L" Index=" + std::to_wstring(RemoveFlag(handle)));
#endif
         return handle;
      }

      void ReleaseDescriptor(DescriptorHandle handle)
      {
         std::unique_lock lock(mutex);
         auto ReleaseHandle = [&](std::vector<DescriptorHandle>& freePool)
            {
#ifdef PILLOW_DEBUG
               //bool found = std::find(freePool.begin(), freePool.end(), handle) != freePool.end();
               //if (found) throw std::exception("Invalid index.");
#endif
               freePool.push_back(handle);
            };
         auto Type = GetType(handle);
         handle = ClearType(handle);
         switch (Type)
         {
         case Type::CBV:
         case Type::SRV:
         case Type::UAV:
            ReleaseHandle(csuFreePool);
            break;
         case Type::RTV:
            ReleaseHandle(rtvFreePool);
            break;
         case Type::DSV:
            ReleaseHandle(dsvFreePool);
            break;
         }
      }

   private:
      static Type GetType(DescriptorHandle handle)
      {
         return Type(handle >> IndexBits);
      }

      static DescriptorHandle ClearType(DescriptorHandle handle)
      {
         return handle & ((1 << IndexBits) - 1);
      }

      static DescriptorHandle SetType(DescriptorHandle handle, Type type)
      {
         return ClearType(handle) | (DescriptorHandle(type) << IndexBits);
      }

   private:
      const static uint32_t FlagBits = 3;
      const static uint32_t IndexBits = sizeof(DescriptorHandle) * 8 - FlagBits;
      const static uint32_t HandleMaxNum = 1 << IndexBits;
      const static uint32_t MaxHeapCapcity = 1 << 16;
      static_assert(MaxHeapCapcity <= HandleMaxNum, "MaxHeapCapcity exceeds the max allowed number of handles.");

      mutable std::shared_mutex mutex;
      const int32_t csuSize, rtvSize, dsvSize;
      ComPtr<ID3D12DescriptorHeap> csuDescHeap;
      ComPtr<ID3D12DescriptorHeap> rtvDescHeap;
      ComPtr<ID3D12DescriptorHeap> dsvDescHeap;
      std::vector<DescriptorHandle> csuFreePool;
      std::vector<DescriptorHandle> rtvFreePool;
      std::vector<DescriptorHandle> dsvFreePool;
      D3D12_CPU_DESCRIPTOR_HANDLE csuCpuHandle0;
      D3D12_GPU_DESCRIPTOR_HANDLE csuGpuHandle0;
      // RTV and DSV don't have gpu handles.
      D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuHandle0;
      D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle0;
   };

   // A comprehensive wrapper class for all resources.
   // It represents the core memory management in D3D12.
   class UnionBuffer
   {
      DeleteDefautedMethods(UnionBuffer)

   public:
      enum class HeapType : uint8_t
      {
         Default = D3D12_HEAP_TYPE_DEFAULT,
         Upload = D3D12_HEAP_TYPE_UPLOAD,
         ReadBack = D3D12_HEAP_TYPE_READBACK,
      };

      enum class DataType : uint8_t
      {
         Texture,
         ConstantBuffer, // Store simple constants in system memory.
         StructuredBuffer, // Used as SRV or UAV. e.g., store skeletal matrices; store compute shaders' output.
         VertexIndexBuffer,
      };

      // The maximum number of updated subresources in one resource each frame.
      // For texture arrays, create a minimal number of mid buffers for uploading, which saves a lot of memory.
      static const uint32_t MidBufferSlotNumMax = 4;

      const HeapType Heap_Type;
      const DataType Data_Type;
      const bool KeepMidBuffer;
      const uint32_t MidBufferSlotNum;
      const uint32_t ElementRawSize;
      const uint32_t ElementCount;
      const uint32_t ElementAlignSize;
      const std::unique_ptr<TextureInfo> TexInfo;
      ReadonlyProperty(uint64_t, ReadyFence)

   public:
      // 7 Pre-Defined Resource Types
      // No  NAME                      DATA_TYPE          HEAP_TYPE  WRITE_FUNC       READ_FUNC
      // 1   Texture                   Texture            Default    WriteOneTexture  x
      // 2   TextureReadBack           Texture            ReadBack   x                ReadBackResources
      // 3   ConstantBuffer            ConstantBuffer     Upload     WriteStructs     x
      // 4   StructuredBuffer          StructuredBuffer   Upload     WriteStructs     x
      // 5   StructuredBufferReadBack  StructuredBuffer   ReadBack   x                ReadBackResources
      // 6   VertexIndexBuffer         VertexIndexBuffer  Default    WriteStructs     x
      // 7   DynamicVertexIndexBuffer  VertexIndexBuffer  Upload     WriteStructs     x

      // Default Heap + Texture2D
      static UnionBuffer Create1_Texture(const TextureInfo& textureInfo, bool keepMidBuffer = false)
      {
         return UnionBuffer(HeapType::Default, DataType::Texture, 0, 0, 0, keepMidBuffer, &textureInfo);
      }

      // Read-Back Heap + Texture2D
      static UnionBuffer Create2_TextureReadBack(const TextureInfo& textureInfo, bool keepMidBuffer = false)
      {
         if (textureInfo.MipCount != 1) throw std::runtime_error("Read back textures with multiple mips are not supported currently.");
         if (textureInfo.CompressionType != TextureInfo::ZipType::None) throw std::runtime_error("Read-back textures cannot be compressed.");
         return UnionBuffer(HeapType::ReadBack, DataType::Texture, 0, 0, 0, keepMidBuffer, &textureInfo);
      }

      // Upload Heap + Buffer
      static UnionBuffer Create3_ConstantBuffer(int32_t elementSize, int32_t elementCount)
      {
         return UnionBuffer(HeapType::Upload, DataType::ConstantBuffer, elementSize, elementCount, GetAlignSize(elementSize, AlignmentConstBuffer));
      }

      // Upload Heap + Buffer
      // Upload heap is enough for a small structured buffer (<= 64KB).
      static UnionBuffer Create4_StructuredBuffer(int32_t structSize, int32_t structCount)
      {
         return UnionBuffer(HeapType::Upload, DataType::StructuredBuffer, structSize, structCount, structSize);
      }

      // Read-Back Heap + Buffer
      static UnionBuffer Create5_StructuredBufferReadBack(int32_t structSize, int32_t structCount)
      {
         return UnionBuffer(HeapType::ReadBack, DataType::StructuredBuffer, structSize, structCount, structSize);
      }

      // Default Heap + Buffer
      // ordinary meshes, can only update once.
      static UnionBuffer Create6_VertexIndexBuffer(int32_t elementSize, int32_t elementCount)
      {
         return UnionBuffer(HeapType::Default, DataType::VertexIndexBuffer, elementSize, elementCount, elementSize, false);
      }

      // Upload Heap + Buffer
      // For dynamic meshes (typically have vertex animations) in a small memory size.
      static UnionBuffer Create7_DynamicVertexIndexBuffer(int32_t elementSize, int32_t elementCount, bool keepMidBuffer = false)
      {
         return UnionBuffer(HeapType::Upload, DataType::VertexIndexBuffer, elementSize, elementCount, elementSize, keepMidBuffer);
      }

   public:
      // Pillow: Not supported; it's about virtual textures.
      void UpdateTileMappings()
      {
         throw std::runtime_error("UpdateTileMappings() is not implemented yet.");
         device->CreateReservedResource(nullptr, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resource));
         cmdQueue->UpdateTileMappings(nullptr, 0, nullptr, nullptr, nullptr, 0, nullptr, nullptr,nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
      }

      // Pillow: Not supported; it's not useful.
      void SetResidencyPriority()
      {
         throw std::runtime_error("SetResidencyPriority() is not implemented yet.");
         D3D12_RESIDENCY_PRIORITY priority = D3D12_RESIDENCY_PRIORITY_NORMAL;
         device->SetResidencyPriority(1, nullptr, &priority);
      }

      void Evict()
      {
         device->Evict(1, (ID3D12Pageable**)resource.GetAddressOf());
         if (midBuffer)midBuffer->Evict();
      }

      void MakeResident()
      {
         device->MakeResident(1, (ID3D12Pageable**)resource.GetAddressOf());
         if (midBuffer)midBuffer->MakeResident();
      }

      uint64_t GetGPUAddress(int index = 0) { return pointerGPU + index * ElementAlignSize; };

      uint32_t GetMidBufferFreeSlotNum()
      {
         uint32_t capacity = 1;
         if (Heap_Type != HeapType::Default) throw std::runtime_error("Middle buffer is only for default buffers.");
         if (midTargets != nullptr)
         {
            capacity = MidBufferSlotNum - midTargets->size();
            if (capacity < 0) throw std::runtime_error("GetMidBufferFreeSlotNum cannot return negative values.");
         }
         return capacity;
      }

      void WriteStructs(const uint8_t* src, uint32_t srcSize, uint32_t elementCount = 1, uint32_t dstElementIdx = 0)
      {
         if (Data_Type == DataType::Texture) throw std::runtime_error("WriteStructs() is for structs data.");
         if (Heap_Type == HeapType::ReadBack) throw std::runtime_error("Cannot write data into a read-back buffer.");
         if (dstElementIdx + elementCount > ElementCount) throw std::runtime_error("The index range exceeds the resource limit.");
         // VIB + Default Heap
         if (Heap_Type == HeapType::Default && Data_Type == DataType::VertexIndexBuffer)
         {
            if (ElementAlignSize * ElementCount != srcSize) throw std::runtime_error("Source data size isn't euqal to the buffer size.");
            RegisterGPUCopy();
            memcpy(midBuffer->pointerCPU, src, srcSize);
         }
         // Upload heaps
         else if (Heap_Type == HeapType::Upload && Data_Type == DataType::ConstantBuffer)
         {
            if (ElementRawSize * elementCount > srcSize) throw std::runtime_error("Source data size is too small.");
            for (uint32_t i = 0; i < elementCount; i++)
            {
               uint32_t dstOffset = (dstElementIdx + i) * ElementAlignSize;
               uint32_t srcOffset = i * ElementRawSize;
               memcpy(pointerCPU + dstOffset, src + srcOffset, ElementRawSize);
            }
         }
         else if (Heap_Type == HeapType::Upload)
         {
            if (ElementRawSize * elementCount > srcSize) throw std::runtime_error("Source data size is too small.");
            if (ElementRawSize != ElementAlignSize) throw std::runtime_error("A upload heap with none-constant-buffer data cannot have special alignment.");
            uint32_t dstOffset = dstElementIdx  * ElementRawSize;
            memcpy(pointerCPU + dstOffset, src, elementCount * ElementRawSize);
         }
         else
         {
            throw std::runtime_error("WriteStructs() doesn't support this operation.");
         }
      }

      // 1. srcArraySlice should be aligned properly for D3D12_RESOURCE_DIMENSION_BUFFER!
      // 
      // 2. D3D12 texture subresource indexing: SubRes[PlaneIdx][ArrayIdx][MipIdx]
      // Planar formats are not used to store RGBA data.
      // 
      // 3. ABOUT THE FOOTPRINT: In Direct3D 12 terminology, footprint describes the memory layouts of D3D12 resources.
      // In detail, the size of a texture row should be aligned(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT), making memory allocation sophisticated.
      // But there is a way to ignore the footprints.
      // Use ID3D12Resource::WriteToSubresource to copy unaligned data into a customized upload heap,
      // then use ID3DCommandList::CopyTextureRegion to let GPU copy it into a default buffer.
      void WriteOneTexture(const uint8_t* src, uint32_t srcSize, uint32_t dstArrayIdx = 0)
      {
         if (Data_Type != DataType::Texture) throw std::runtime_error("WriteOneTexture() is for textures.");
         if (Heap_Type != HeapType::Default) throw std::runtime_error("Target texture should be in a default heap.");
         uint32_t arraySliceSize = GetAlignTextureArraySliceSize(*TexInfo);
         if (srcSize != arraySliceSize) throw std::runtime_error("Source data size doesn't match the target size.");
         RegisterGPUCopy(dstArrayIdx);
         uint32_t midIdx = 0;
         if (midTargets) midIdx = midTargets->size() - 1;
         if (midIdx < 0) throw std::runtime_error("Index cannot cannot be less than 0.");
         memcpy(midBuffer->pointerCPU + midIdx * arraySliceSize, src, arraySliceSize);
      }

      void ReadBackResources(uint8_t* dst, uint32_t dstBufferSize, uint32_t count = 1, uint32_t srcArrayIdx = 0)
      {
         if (Heap_Type != HeapType::ReadBack) throw std::runtime_error("ReadBackOneResource() is for read-back buffers.");
         if (dst == nullptr) throw std::runtime_error("Destination buffer is null.");
         if (Data_Type == DataType::Texture)
         {
            uint32_t mip0Size = TexInfo->GetMipmapSize(0);
            if (dstBufferSize < mip0Size * count) throw std::runtime_error("Destination buffer is too small.");
            uint32_t rowPitch = TexInfo->Width * GetPixelSize(*TexInfo);
            for (uint32_t i = 0; i < count; i++)
            {
               resource->ReadFromSubresource(dst + i* mip0Size, rowPitch, 0, srcArrayIdx + i, nullptr);
            }
         }
         else if (Data_Type == DataType::StructuredBuffer)
         {
            if (dstBufferSize < ElementRawSize * count) throw std::runtime_error("Destination buffer is too small.");
            if (ElementRawSize != ElementAlignSize) throw std::runtime_error("A upload heap with none-constant-buffer data cannot have special alignment.");
            memcpy(dst, pointerCPU + srcArrayIdx * ElementRawSize, count * ElementRawSize);
         }
         else
         {
            throw std::runtime_error("ReadBackOneResource() only supports for textures and structured buffers.");
         }
      }

      // Let GPU copy data to all default heaps.
      static void GPUCopyAll(ComPtr<ICommandList>& cmdList)
      {
         while (DirtyPool.empty() == false)
         {
            UnionBuffer& buffer = *DirtyPool.back();
            DirtyPool.pop_back();
            buffer.f_ReadyFence = fenceSync->GetTargetFence();
            // Fill the cmd list.
            if(buffer.Data_Type == DataType::VertexIndexBuffer)
            {
               ApplyBarrier(cmdList, buffer.midBuffer->resource, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE);
               ApplyBarrier(cmdList, buffer.resource, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
               cmdList->CopyResource(buffer.resource.Get(), buffer.midBuffer->resource.Get()); // GPU Copy
               ApplyBarrier(cmdList, buffer.midBuffer->resource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ);
               ApplyBarrier(cmdList, buffer.resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
            }
            else if (buffer.Data_Type == DataType::Texture)
            {
               if (buffer.TexInfo == nullptr) throw std::runtime_error("Texture(Array)'s TexInfo cannot be null.");
               const uint32_t mipNum = buffer.TexInfo->MipCount;
               if (buffer.MidBufferSlotNum == 1)
               {
                  // Preparation
                  D3D12_TEXTURE_COPY_LOCATION src{ buffer.midBuffer->resource.Get(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, 0 };
                  D3D12_TEXTURE_COPY_LOCATION dst{ buffer.resource.Get(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, 0 };
                  // GPU copy
                  ApplyBarrier(cmdList, buffer.midBuffer->resource, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE);
                  ApplyBarrier(cmdList, buffer.resource, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
                  for (uint32_t mip = 0; mip < mipNum; mip++)
                  {
                     src.SubresourceIndex = mip;
                     dst.SubresourceIndex = mip;
                     cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, NULL);
                  }
                  ApplyBarrier(cmdList, buffer.midBuffer->resource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ);
                  ApplyBarrier(cmdList, buffer.resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
               }
               else
               {
                  if (buffer.midTargets == nullptr) throw std::runtime_error("TextureArray's midTargets cannot be null.");
                  uint32_t midTargetCount = buffer.midTargets->size();
                  for (uint32_t srcArrayIdx = 0; srcArrayIdx < midTargetCount; srcArrayIdx++)
                  {
                     // Preparation
                     uint32_t dstArrayIdx = buffer.midTargets->front();
                     buffer.midTargets->pop_front();
                     D3D12_TEXTURE_COPY_LOCATION src{ buffer.midBuffer->resource.Get(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, 0 };
                     D3D12_TEXTURE_COPY_LOCATION dst{ buffer.resource.Get(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, 0 };
                     // GPU copy
                     ApplyBarrier(cmdList, buffer.midBuffer->resource, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE);
                     ApplyBarrier(cmdList, buffer.resource, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
                     for (uint32_t mip = 0; mip < mipNum; mip++)
                     {
                        src.SubresourceIndex = srcArrayIdx * mipNum + mip;
                        dst.SubresourceIndex = dstArrayIdx * mipNum + mip;
                        cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, NULL);
                     }
                     ApplyBarrier(cmdList, buffer.midBuffer->resource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ);
                     ApplyBarrier(cmdList, buffer.resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
                  }
                  if (!buffer.midTargets->empty()) throw std::runtime_error("midTargets should be empty.");
               }
            }
            else
            {
               throw std::runtime_error("GPUCopy() only supports textures and vertex/index buffers.");
            }
         }
      }

   private:
      inline static std::vector<UnionBuffer*> DirtyPool{};

      // Used to copy data into default heaps.
      std::unique_ptr<UnionBuffer> midBuffer;
      // Used for texture arrays; stores the target array indices.
      // Enqueue in the back; dequeue in the front.
      std::unique_ptr<std::deque<uint32_t>> midTargets;
      ComPtr<IResource> resource{};
      uint64_t pointerGPU{};
      uint8_t* pointerCPU{};

      UnionBuffer(HeapType heapType, DataType dataType, int32_t eleRawSize, int32_t eleNum, int32_t eleAlignSize, bool keepMid = false, const TextureInfo* texInfo = nullptr) :
         Heap_Type(heapType),
         Data_Type(dataType),
         KeepMidBuffer(keepMid),
         MidBufferSlotNum(texInfo ? std::min(MidBufferSlotNumMax, uint32_t(texInfo->ArrayCount))  : 1),
         ElementRawSize(eleRawSize),
         ElementCount(eleNum),
         ElementAlignSize(eleAlignSize),
         TexInfo(texInfo ? std::make_unique<TextureInfo>(*texInfo) : nullptr),
         f_ReadyFence(0) // For default heaps.
      {
         constexpr D3D12_RESOURCE_DESC defaultResDesc
         {
            D3D12_RESOURCE_DIMENSION_BUFFER, 0, 1, 1, 1, 1, DXGI_FORMAT_UNKNOWN,
            DXGI_SAMPLE_DESC{1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE
         };
         // Write-combining disables the CPU cache and enables the write-combining buffer. It's suitable for CPU-write-only actions.
         D3D12_CPU_PAGE_PROPERTY pageType = D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
         if (heapType == HeapType::Upload) pageType = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
         else if (heapType == HeapType::ReadBack) pageType = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
         // L0 memory pool = CPU memory
         auto memPool = (heapType == HeapType::Default) ? D3D12_MEMORY_POOL_L1 : D3D12_MEMORY_POOL_L0;
         D3D12_HEAP_PROPERTIES heapProperties{ D3D12_HEAP_TYPE(heapType), pageType, memPool, 0, 0};
         D3D12_RESOURCE_DESC resDesc = defaultResDesc;
         if (dataType == DataType::Texture)
         {
            if (texInfo == nullptr) throw std::runtime_error("Texture resource's TexInfo cannot be null.");
            if (heapType == HeapType::Default)
            {
               auto fmt = uint32_t(texInfo->TexFormat);
               resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
               resDesc.Width = texInfo->Width;
               resDesc.Height = texInfo->Height;
               resDesc.DepthOrArraySize = uint16_t(texInfo->ArrayCount);
               resDesc.MipLevels = uint16_t(texInfo->MipCount);
               resDesc.Format = (texInfo->CompressionType == TextureInfo::ZipType::None) ? NativeTexFmt[fmt] : NativeBCTexFmt[fmt];
               resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            }
            // Upload heaps and write-back heaps.
            else
            {
               resDesc.Width = GetAlignTextureArraySliceSize(*texInfo) * texInfo->ArrayCount;
            }

         }
         // None-texture buffers.
         else
         {
            resDesc.Width = ElementAlignSize * ElementCount;
         }
         auto heapFlags = D3D12_HEAP_FLAG_NONE;
         auto resStates = (heapType == HeapType::ReadBack) ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;
         Check_HRESULT(device->CreateCommittedResource(&heapProperties, heapFlags, &resDesc, resStates, nullptr, IID_PPV_ARGS(&resource)));

         GetCPUGPUPointers();
         CreateMidBuffer();
      }

      void GetCPUGPUPointers()
      {
         if (Heap_Type != HeapType::Default)
         {
            D3D12_RANGE emptyRange{ 0, 0 };
            // CPU read is only needed by readback buffers.
            Check_HRESULT(resource->Map(0, Heap_Type == HeapType::ReadBack ? nullptr : &emptyRange, (void**)(&pointerCPU)));
         }
         if (Data_Type != DataType::Texture)
         {
            //This method is only useful for buffer resources, it will return zero for all texture resources.
            pointerGPU = resource->GetGPUVirtualAddress();
         }
      }

      // Only a default heap buffer can create a mid buffer.
      void CreateMidBuffer()
      {
         if (Heap_Type != HeapType::Default) return;
         if (midBuffer != nullptr) throw std::runtime_error("The middle buffer has been created.");
         std::unique_ptr<TextureInfo> midTexInfo = nullptr;
         if (Data_Type == DataType::Texture)
         {
            if (MidBufferSlotNum > 1)
            {
               midTexInfo = std::make_unique<TextureInfo>(TextureInfo::DefineTextureArray(*TexInfo, MidBufferSlotNum));
               midTargets = std::make_unique<std::deque<uint32_t>>();
            }
         }
         midBuffer = std::unique_ptr<UnionBuffer>(new UnionBuffer(HeapType::Upload, Data_Type, ElementRawSize,
            ElementCount, ElementAlignSize, KeepMidBuffer, midTexInfo != nullptr ? midTexInfo.get() : TexInfo.get()));
      }

      void RegisterGPUCopy(uint32_t targetArrayIndex = 0)
      {
         // Push into Dirtypool
         if (!midBuffer) throw std::runtime_error("Mid buffer died.");
         if (fenceSync->GetCompletedFence() < f_ReadyFence) throw std::runtime_error("The previous GPU copy hasn't completed.");
         // Ensure each buffer is only pushed once for each frame.
         if (std::ranges::find(DirtyPool, this) == DirtyPool.end())
         {
            DirtyPool.push_back(this);
         }
         else if (MidBufferSlotNum == 1)
         {
            throw std::runtime_error("Only a texture array supports writing multiple times per frame.");
         }
         // Update midTargets
         if (midTargets)
         {
            if (midTargets->size() == MidBufferSlotNum) throw std::runtime_error("Middle buffer is full for this frame.");
            if (std::ranges::find(*midTargets, targetArrayIndex) == midTargets->end())
            {
               midTargets->push_back(targetArrayIndex);
            }
            else
            {
               throw std::runtime_error("Cannot write to the same texture twice a frame.");
            }

         }
         // Release the mid pool.
         if (KeepMidBuffer) return;
         lateReleaseMgr->Enqueue(std::move(midBuffer));
         if (midBuffer) throw std::runtime_error("Mid buffer should be released.");
      }
   };

   class HLSLInclude : public ID3DInclude
   {
      ReadonlyProperty(std::filesystem::path, LocalPath)

   public:
      HLSLInclude(std::filesystem::path location)
      {
         f_LocalPath = location.parent_path();
      }

      HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
      {
         std::filesystem::path location = GetResourcePath("Shaders");
         location = location / pFileName;
         // If the root dir doesn't own the file, use the local dir.
         // Ignore D3D_INCLUDE_TYPE, which makes things complicated.
         if (!std::filesystem::exists(location))
         {
            location = f_LocalPath;
            location /= pFileName;
            if (!std::filesystem::exists(location)) return E_FAIL;
         }
         std::ifstream file(location, std::ios::binary | std::ios::ate);
         if (!file.is_open()) return E_FAIL;
         uint32_t size = uint32_t(file.tellg());
         file.seekg(0, std::ios::beg);
         buffer = std::make_unique<char[]>(size);
         if (!file.read(buffer.get(), size)) return E_FAIL;
         file.close();
         *ppData = buffer.get();
         *pBytes = size;
         return S_OK;
      }

      HRESULT Close(LPCVOID pData)
      {
         if (pData != buffer.get()) throw std::runtime_error("HLSLInclude cannot be closed safely.");
         buffer.reset();
         return S_OK;
      }

   private:
      // Modern C++ memory management.
      std::unique_ptr<char[]> buffer;
   };

   class PipelineStateManager
   {

   };
}

// Static functions
namespace
{
   // IntelliSense doesn't work well with macros, use a function instead.
   void Check_HRESULT(HRESULT hResult)
   {
      if (SUCCEEDED(hResult)) return;
      string msg = "D3D12Renderer Error\n";
      std::wstring systemMsg = _com_error(hResult).ErrorMessage();
      utf8::utf16to8(systemMsg.begin(), systemMsg.end(), std::back_inserter(msg));
      throw std::runtime_error(msg);
   }

   void ApplyBarrier(ComPtr<ICommandList>& cmdList, ComPtr<IResource>& resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
   {
      D3D12_RESOURCE_BARRIER barrier
      {
         D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
         D3D12_RESOURCE_BARRIER_FLAG_NONE,
         D3D12_RESOURCE_TRANSITION_BARRIER { resource.Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, before, after}
      };
      cmdList->ResourceBarrier(1, &barrier);
   }

   // Must align the subresource size with D3D12_TEXTURE_DATA_PITCH_ALIGNMENT.
   uint32_t GetAlignMipmapSize(const TextureInfo& texInfo, uint32_t mipLevel)
   {
      uint32_t size{};
      uint32_t w = std::max(1, texInfo.Width >> mipLevel);
      uint32_t h = std::max(1, texInfo.Height >> mipLevel);
      if (texInfo.CompressionType == TextureInfo::ZipType::None)
      {
         uint32_t pixel = GetPixelSize(texInfo);
         if (pixel == 3) pixel = 4; // DXGI donesn't support R8G8B8, use R8G8B8A8 instead. 
         uint32_t rowPitch = GetAlignSize(w * pixel, AlignmentTextureRow);
         size = rowPitch * h;
      }
      else
      {
         w = (w + 3) / 4;
         h = (h + 3) / 4;
         uint32_t block = NativeBCBlockSize[uint32_t(texInfo.TexFormat)];
         uint32_t rowPitch = GetAlignSize(w * block, AlignmentTextureRow);
         size = rowPitch * h;
      }
      return size;
   }

   // Get the aligned size of one texture (both rows and subresources are aligned).
   uint32_t GetAlignTextureArraySliceSize(const TextureInfo& texInfo)
   {
      uint32_t mips = texInfo.MipCount;
      uint32_t size = 0;
      for (uint32_t i = 0; i < mips; i++)
      {
         // Align the last mipmap as well, in case of a texture array.
         size += GetAlignSize(GetAlignMipmapSize(texInfo, i), AlignmentTextureSubres);
      }
      return size;
   }

   uint32_t GetPiplelineBufferIndex(PiplelineBuffer type, uint32_t frameIdx)
   {
      return frameIdx * PiplelineBufferNum + uint32_t(type);
   }

   ForceInline void PlaceBCIndex(uint8_t* destination, uint32_t value, uint32_t pixelIndex)
   {
      uint32_t bitCount = pixelIndex * 3;
      uint32_t byteOffset = bitCount / 8;
      uint32_t bitOffset = bitCount % 8;
      destination[byteOffset] = (destination[byteOffset] & ~(0x7 << bitOffset)) | value << bitOffset;
   }

   void XM_CALLCONV OptimizeRGB(XMVECTOR& color0, XMVECTOR& color1, const XMVECTOR* block)
   {
      const uint32_t steps = 4;
      constexpr float fEpsilon = (0.25f / 64.f) * (0.25f / 64.f);
      static constexpr float pC[] = { 1, 2.f / 3.f, 1.f / 3.f, 0 };
      static constexpr float pD[] = { pC[3], pC[2], pC[1], pC[0] };
      // Find Min and Max points, as starting point
      XMVECTOR c0 = RGBLuminance;
      XMVECTOR c1 = XMVectorZero();
      for (int32_t i = 0; i < BCBlockLength; i++)
      {
         XMVECTOR select = XMVectorLess(block[i], c0);
         c0 = XMVectorSelect(c0, block[i], select);
         select = XMVectorGreater(block[i], c1);
         c1 = XMVectorSelect(c1, block[i], select);
      }
      // Diagonal axis
      const XMVECTOR AB = XMVectorSubtract(c1, c0);
      const float fAB = XMVectorGetX(XMVector3Dot(AB, AB));
      // Single color block.. no need to root-find
      if (fAB < FLT_MIN)
      {
         color0 = c0;
         color1 = c1;
         return;
      }
      // Try all four axis directions, to determine which diagonal best fits data
      XMVECTOR dir = XMVectorScale(AB, 1.f / fAB);
      const XMVECTOR Mid = XMVectorLerp(c0, c1, 0.5f);
      XMVECTOR fDir = XMVectorZero();
      for (int32_t i = 0; i < BCBlockLength; i++)
      {
         XMVECTOR pt = XMVectorMultiply(XMVectorSubtract(block[i], Mid), dir);
         //XMVectorSetW(pt, 0);
         XMFLOAT3A _pt;
         XMStoreFloat3A(&_pt, pt);
         XMVECTOR f = XMVectorReplicate(_pt.x);
         f = XMVectorAdd(f, XMVectorSet(_pt.y, _pt.y, -_pt.y, -_pt.y));
         f = XMVectorAdd(f, XMVectorSet(_pt.z, -_pt.z, _pt.z, -_pt.z));
         fDir = XMVectorMultiply(f, f);
      }     
      XMFLOAT4A _fDir {};
      XMStoreFloat4A(&_fDir, fDir);
      float fDirMax = _fDir[0];
      int32_t  iDirMax = 0;
      for (size_t i = 1; i < 4; i++)
      {
         if (_fDir[i] <= fDirMax) continue;
         fDirMax = _fDir[i];
         iDirMax = i;
      }
      if (iDirMax & 2)
      {
         const XMVECTOR select = XMVectorSelectControl(0, 1, 0, 0);
         XMVECTOR temp = c0;
         c0 = XMVectorSelect(c0, c1, select);
         c1 = XMVectorSelect(c1, temp, select);
      }
      if (iDirMax & 1)
      {
         const XMVECTOR select = XMVectorSelectControl(0, 0, 1, 0);
         XMVECTOR temp = c0;
         c0 = XMVectorSelect(c0, c1, select);
         c1 = XMVectorSelect(c1, temp, select);
      }
      // Two color block.. no need to root-find
      if (fAB < 1.f / 4096.f)
      {
         color0 = c0;
         color1 = c1;
         return;
      }
      // Use Newton's Method to find local minima of sum-of-squares error.
      const float fSteps = steps - 1;
      for (int32_t i = 0; i < 8; i++)
      {
         // Calculate new steps
         XMVECTOR pSteps[4];
         for (size_t iStep = 0; iStep < steps; iStep++)
         {
            pSteps[iStep] = XMVectorAdd(XMVectorScale(c0, pC[iStep]), XMVectorScale(c1, pD[iStep]));
         }
         // Calculate color direction
         dir = XMVectorSubtract(c1, c0);
         const float fLen = XMVectorGetX(XMVector3Dot(dir, dir));
         if (fLen < (1.0f / 4096.0f)) break;
         dir = XMVectorScale(dir, fSteps / fLen);
         // Evaluate function, and derivatives
         float d2X = 0;
         float d2Y = 0;
         XMVECTOR dX = XMVectorZero();
         XMVECTOR dY = XMVectorZero();
         for (int32_t i = 0; i < BCBlockLength; i++)
         {
            const float fDot = XMVectorGetX(XMVector3Dot(XMVectorSubtract(block[i], c0), dir));
            uint32_t iStep;
            if (fDot <= 0) iStep = 0;
            else if (fDot >= fSteps) iStep = steps - 1;
            else iStep = fDot + 0.5f;
            XMVECTOR diff = XMVectorSubtract(pSteps[iStep], block[i]);
            const float fC = pC[iStep] * (1.f / 8.f);
            const float fD = pD[iStep] * (1.f / 8.f);
            d2X += fC * pC[iStep];
            dX = XMVectorAdd(dX, XMVectorScale(diff, fC));
            d2Y += fD * pD[iStep];
            dY = XMVectorAdd(dY, XMVectorScale(diff, fD));
         }
         // Move endpoints
         if (d2X > 0) c0 = XMVectorAdd(c0, XMVectorScale(dX, -1 / d2X));
         if (d2Y > 0) c1 = XMVectorAdd(c1, XMVectorScale(dY, -1 / d2Y));
         XMVECTOR cmp1 = XMVectorLess(XMVectorMultiply(dX, dX), XMVectorReplicate(fEpsilon));
         XMVECTOR cmp2 = XMVectorLess(XMVectorMultiply(dY, dY), XMVectorReplicate(fEpsilon));
         XMVECTOR cmp = XMVectorAndInt(cmp1, cmp2);
         cmp = XMVectorAndInt(XMVectorAndInt(cmp, XMVectorSplatY(cmp)), XMVectorSplatZ(cmp));
         if (XMVectorGetIntX(cmp)) break;
      }
      color0 = c0;
      color1 = c1;
   }

   void OptimizeAlpha(float& colorMin, float& colorMax, const float* block, uint32_t steps)
   {
      static constexpr float pC6[] = { 1, 4.f / 5.f, 3.f / 5.f, 2.f / 5.f, 1.f / 5.f, 0 };
      static constexpr float pD6[] = { pC6[5], pC6[4], pC6[3], pC6[2], pC6[1], pC6[0] };
      static constexpr float pC8[] = { 1, 6.f / 7.f, 5.f / 7.f, 4.f / 7.f, 3.f / 7.f, 2.f / 7.f, 1.f / 7.f, 0 };
      static constexpr float pD8[] = { pC8[7], pC8[6], pC8[5], pC8[4], pC8[3], pC8[2], pC8[1], pC8[0] };
      const float* pC = (6 == steps) ? pC6 : pC8;
      const float* pD = (6 == steps) ? pD6 : pD8;
      // Find Min and Max points, as starting point
      float _min = 1;
      float _max = 0;
      for (size_t i = 0; i < BCBlockLength; i++)
      {
         if (block[i] < _min) _min = block[i];
         if (block[i] > _max) _max = block[i];
      }
      if (steps == 6 && _min == _max) _max = 1;
      // Use Newton's Method to find local minima of sum-of-squares error.
      const float fSteps = steps - 1;
      for (size_t i = 0; i < 8; i++)
      {
         if ((_max - _min) < (1.0f / 256.0f)) break;
         float const fScale = fSteps / (_max - _min);
         // Calculate new steps
         float pSteps[8];
         for (size_t iStep = 0; iStep < steps; iStep++)
            pSteps[iStep] = pC[iStep] * _min + pD[iStep] * _max;
         if (steps == 6)
         {
            pSteps[6] = 0;
            pSteps[7] = 1;
         }
         // Evaluate function, and derivatives
         float dX = 0.0f;
         float dY = 0.0f;
         float d2X = 0.0f;
         float d2Y = 0.0f;
         for (int32_t iPoint = 0; iPoint < BCBlockLength; iPoint++)
         {
            const float fDot = (block[iPoint] - _min) * fScale;
            uint32_t iStep;
            if (fDot == 0.0f)
            {
               iStep = (steps == 6 && block[iPoint] <= _min * 0.5f) ? 6u : 0u;
            }
            else if (fDot >= fSteps)
            {
               iStep = (steps == 6 && block[iPoint] >= (_max + 1) * 0.5f) ? 7u : (steps - 1);
            }
            else
            {
               iStep = fDot + 0.5f;
            }
            if (iStep < steps)
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
         if (d2X > 0.0f) _min -= dX / d2X;
         if (d2Y > 0.0f) _max -= dY / d2Y;
         if (_min > _max) std::swap(_min, _max);
         if (dX * dX < 1.f / 64.f && dY * dY < 1.f / 64.f) break;
      }
      colorMin = std::clamp(_min, 0.f, 1.f);
      colorMax = std::clamp(_max, 0.f, 1.f);
   }

   void EncodeBC1RGB(const XMFLOAT4A* blockRGB, uint8_t* destination, bool RGBDithering)
   {
      const uint32_t uSteps = 4;
      // Quantize block to R56B5, using Floyd Stienberg error diffusion. This
      // increases the chance that colors will map directly to the quantized
      // axis endpoints.
      XMVECTOR colors[BCBlockLength];
      XMVECTOR errors[BCBlockLength];
      if (RGBDithering) for (int32_t i = 0; i < BCBlockLength; i++) errors[i] = XMVectorZero();
      for (int32_t i = 0; i < BCBlockLength; i++)
      {
         XMVECTOR c = XMLoadFloat4A(&blockRGB[i]);
         if (RGBDithering) c = XMVectorAdd(c, errors[i]);
         const XMVECTOR v2 = XMVectorSet(31.f, 63.f, 31.f, 0);
         const XMVECTOR v3 = XMVectorReplicate(0.5f);
         const XMVECTOR factor = XMVectorSet(1 / 31.f, 1 / 63.f, 1 / 31.f, 0);
         colors[i] = XMVectorMultiply(XMVectorFloor(XMVectorMultiplyAdd(c, v2, v3)), factor);
         colors[i] = XMVectorMultiply(colors[i], RGBLuminance);
         if (!RGBDithering) continue;
         XMVECTOR diff = XMVectorSubtract(c, colors[i]);
         if (3 != (i & 3))
         {
            const XMVECTOR factor = XMVectorReplicate(7.f / 16.f);
            errors[i + 1] = XMVectorMultiplyAdd(diff, factor, errors[i + 1]);
         }
         if (i < 12)
         {
            const XMVECTOR factor = XMVectorReplicate(5.f / 16.f);
            errors[i + 4] = XMVectorMultiplyAdd(diff, factor, errors[i + 4]);
            if (i & 3)
            {
               const XMVECTOR factor = XMVectorReplicate(3.f / 16.f);
               errors[i + 3] = XMVectorMultiplyAdd(diff, factor, errors[i + 3]);
            }
            if (3 != (i & 3))
            {
               const XMVECTOR factor = XMVectorReplicate(1 / 16.f);
               errors[i + 5] = XMVectorMultiplyAdd(diff, factor, errors[i + 5]);
            }
         }
      }
      // Perform 6D root finding function to find two endpoints of color axis.
      // Then quantize and sort the endpoints depending on mode.
      XMVECTOR ColorA, ColorB, ColorC, ColorD;
      OptimizeRGB(ColorA, ColorB, colors);
      ColorC = XMVectorMultiply(ColorA, RGBLuminanceInv);
      ColorD = XMVectorMultiply(ColorB, RGBLuminanceInv);
      const uint16_t wColorA = EncodeRGB565(ColorC);
      const uint16_t wColorB = EncodeRGB565(ColorD);
      if (wColorA == wColorB)
      {
         reinterpret_cast<uint16_t*>(destination)[0] = wColorA;
         reinterpret_cast<uint16_t*>(destination)[1] = wColorA;
         reinterpret_cast<uint32_t*>(destination)[1] = 0x0;
         return;
      }
      ColorC = DecodeRGB565(wColorA);
      ColorD = DecodeRGB565(wColorB);
      ColorA = XMVectorMultiply(ColorC, RGBLuminance);
      ColorB = XMVectorMultiply(ColorD, RGBLuminance);
      // Calculate color steps
      XMVECTOR Step[4];
      reinterpret_cast<uint16_t*>(destination)[0] = wColorB;
      reinterpret_cast<uint16_t*>(destination)[1] = wColorA;
      Step[0] = ColorB;
      Step[1] = ColorA;
      static const int32_t pSteps[] = { 0, 2, 3, 1 };
      Step[2] = XMVectorLerp(Step[0], Step[1], 1 / 3.f);
      Step[3] = XMVectorLerp(Step[0], Step[1], 2 / 3.f);
      // Calculate color direction
      XMVECTOR Dir;
      Dir = Step[1] - Step[0];
      const float fSteps = uSteps - 1;
      const float fScale = (wColorA != wColorB) ? (fSteps / XMVectorGetX(XMVector3Dot(Dir, Dir))) : 0;
      Dir = XMVectorScale(Dir, fScale);
      // Encode colors, 2 bits per pixel
      uint32_t encodedIndices = 0;
      if (RGBDithering) for (int32_t i = 0; i < BCBlockLength; i++) errors[i] = XMVectorZero();
      for (int32_t i = 0; i < BCBlockLength; i++)
      {
         XMVECTOR c = XMLoadFloat4A(&blockRGB[i]);
         c = XMVectorMultiply(c, RGBLuminance);
         if (RGBDithering) c = XMVectorAdd(c, errors[i]);
         const float fDot = XMVectorGetX(XMVector3Dot(XMVectorSubtract(c, Step[0]), Dir));
         uint32_t iStep;
         if (fDot <= 0.0f) iStep = 0;
         else if (fDot >= fSteps) iStep = 1;
         else iStep = pSteps[uint32_t(fDot + 0.5f)];
         encodedIndices = (iStep << 30) | (encodedIndices >> 2);
         if (!RGBDithering) continue;
         XMVECTOR diff = XMVectorSubtract(c, Step[iStep]);
         if (3 != (i & 3))
         {
            const XMVECTOR factor = XMVectorReplicate(7.f / 16.f);
            errors[i + 1] = XMVectorMultiplyAdd(diff, factor, errors[i + 1]);
         }
         if (i < 12)
         {
            const XMVECTOR factor = XMVectorReplicate(5.f / 16.f);
            errors[i + 4] = XMVectorMultiplyAdd(diff, factor, errors[i + 4]);
            if (i & 3)
            {
               const XMVECTOR factor = XMVectorReplicate(3.f / 16.f);
               errors[i + 3] = XMVectorMultiplyAdd(diff, factor, errors[i + 3]);
            }
            if (3 != (i & 3))
            {
               const XMVECTOR factor = XMVectorReplicate(1.f / 16.f);
               errors[i + 5] = XMVectorMultiplyAdd(diff, factor, errors[i + 5]);
            }
         }
      }
      reinterpret_cast<uint32_t*>(destination)[1] = encodedIndices;
   }

   void EncodeBC3RGBA(const XMFLOAT4A* blockRGB, const float* blockA, uint8_t* destination, bool RGBDithering)
   {
      void EncodeBC4Alpha(const float*, uint8_t*);
      EncodeBC4Alpha(blockA, destination);
      EncodeBC1RGB(blockRGB, destination + BC4BlockSize, RGBDithering);
   }

   void EncodeBC4Alpha(const float* block, uint8_t* destination)
   {
      // Step 1: Find end points.
      bool bUsing4BlockCodec = false;
      for (size_t i = 0; i < BCBlockLength; ++i)
      {
         //  If there are boundary values in input texels, should use 4 interpolated color values to guarantee
         //  the exact code of the boundary values.
         if (block[i] == 0 || block[i] == 1)
         {
            bUsing4BlockCodec = true;
            break;
         }
      }
      float min, max;
      OptimizeAlpha(min, max, block, bUsing4BlockCodec ? 6 : 8);
      ColorFloat2Byte(destination[0], bUsing4BlockCodec ? min : max);
      ColorFloat2Byte(destination[1], bUsing4BlockCodec ? max : min);
      // Step 2: Compute indices, which follows the below mapping:
      // 0:C0, 1:C1, 2:Interpolation1, ..., 5:Interpolation4, 6:Interpolation5/0.0f, 7:Interpolation6/1.0f
      for (size_t i = 0; i < BCBlockLength; i++)
      {
         uint32_t value;
         if (bUsing4BlockCodec)
         {
            if (block[i] == 0) value = 6;
            else if (block[i] == 1) value = 7;
            else if (block[i] < min) value = (min - block[i]) / min <= 0.5f ? 6 : 0;
            else if (block[i] > max) value = (block[i] - max) / (1 - max) <= 0.5f ? 1 : 7;
            else
            {
               value = 5.f * (block[i] - min) / (max - min) + 0.5f;
               if (value == 0) value = 0;
               else if (value == 5) value = 1;
               else value += 1;
            }
         }
         else
         {
            value = 7.f * (block[i] - max) / (min - max) + 0.5f;
            if (value == 0) value = 0;
            else if (value == 7) value = 1;
            else value += 1;
         }
         PlaceBCIndex(destination + 2, value, i); // +2: Point it to the index block
      }
   }

   void EncodeBC5Normal(const float* blockRed, const float* blockGreen, uint8_t* destination)
   {
      EncodeBC4Alpha(blockRed, destination);
      EncodeBC4Alpha(blockGreen, destination + BC4BlockSize);
   }

   void CheckDriverFeatures()
   {
      //device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &deviceFeatures, sizeof(deviceFeatures));
   }

   void CreateBase()
   {
      // Factory
      uint32_t factoryFlags = 0;
#ifdef PILLOW_DEBUG
      factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
      ComPtr<ID3D12Debug3> debugController;
      Check_HRESULT(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
      debugController->EnableDebugLayer();
#endif
      Check_HRESULT(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)));
      BOOL winBool = 0;
      factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &winBool, sizeof(winBool));
      bDeviceSupportTearing = (winBool == TRUE);
      // Device
      try
      {
         Check_HRESULT(D3D12CreateDevice(nullptr, Direct3DFeatureLevel, IID_PPV_ARGS(&device))); // Default adapter
      }
      catch (...)
      {
         ComPtr<IDXGIAdapter> Warp;
         Check_HRESULT(factory->EnumWarpAdapter(IID_PPV_ARGS(&Warp)));
         Check_HRESULT(D3D12CreateDevice(Warp.Get(), Direct3DFeatureLevel, IID_PPV_ARGS(&device)));
      }
      // Queue
      D3D12_COMMAND_QUEUE_DESC queueDesc{ D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0 };
      Check_HRESULT(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue)));
      // Fence
      fenceSync = std::make_unique<FenceSync>(device, cmdQueue);
      // Swapchain
      DXGI_SWAP_CHAIN_DESC1 swapChainDesc
      {
         0,0, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM, false, DXGI_SAMPLE_DESC{1, 0}/*no obselete MSAA*/,
         DXGI_USAGE_RENDER_TARGET_OUTPUT, Constants::SwapChainSize, DXGI_SCALING_NONE,
         DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL/*need to access previous frame buffers*/, DXGI_ALPHA_MODE_IGNORE,
         uint32_t(bDeviceSupportTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING/*allow to disable V-Sync*/ : 0)
      };
      Check_HRESULT(factory->CreateSwapChainForHwnd(cmdQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, swapChain.GetAddressOf()));
      DXGI_RGBA color{ 0.f, 0.f, 0.f, 1.f };
      swapChain->SetBackgroundColor(&color);
      // Command Allocators & Lists
      int32_t count = Constants::SwapChainSize * workerThreadCount;
      cmdAllocators.reserve(count);
      for (int i = 0; i < count; i++)
      {
         ComPtr<ID3D12CommandAllocator> temp;
         Check_HRESULT(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&temp)));
         cmdAllocators.push_back(std::move(temp));
      }
      // CreateCommandList1 closes the cmd list automatically.
      cmdLists.reserve(workerThreadCount);
      cmdListsRaw.reserve(workerThreadCount);
      for (int i = 0; i < workerThreadCount; i++)
      {
         ComPtr<ICommandList> temp;
         Check_HRESULT(device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&temp)));
         cmdListsRaw.push_back(temp.Get());
         cmdLists.emplace_back(std::move(temp));
      }
   }

   void CreateManagers()
   {
      // Build all descriptor heaps.
      lateReleaseMgr = std::make_unique<LateReleaseManager>();
      descriptorMgr = std::make_unique<DescriptorHeapManeger>(device);
      psoMgr = std::make_unique<PipelineStateManager>();
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
         int32_t frameArrayIdx = (fenceSync->GetFrameIndex() + i) % Constants::SwapChainSize;
         Check_HRESULT(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[frameArrayIdx])));
         piplelineRTVs[GetPiplelineBufferIndex(PiplelineBuffer::Backbuffer, frameArrayIdx)] =
            descriptorMgr->CreateDescirptor(device, backBuffers[frameArrayIdx], &rtvDesc, DescriptorHeapManeger::Type::RTV);
      }
   }

   void TryResizeSwapChain()
   {
      if (currentBackBufferSize == IRenderer::GetInstance()->GetBackBufferSize()) return;
      currentBackBufferSize = IRenderer::GetInstance()->GetBackBufferSize();
      // Resize the swapchain.
      fenceSync->FlushQueue();
      for (int32_t i = 0; i < Constants::SwapChainSize; i++)
      {
         backBuffers[i].Reset();
         descriptorMgr->ReleaseDescriptor(piplelineRTVs[GetPiplelineBufferIndex(PiplelineBuffer::Backbuffer, i)]);
      }
      Check_HRESULT(swapChain->ResizeBuffers(Constants::SwapChainSize, 0, 0, DXGI_FORMAT_R8G8B8A8_UNORM,
         bDeviceSupportTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING/*allow to disable V-Sync*/ : 0));
      CreateFrames();
   }

   void BlockCompressionEncode()
   {

   }

   void TEMP_RendererTestZone()
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

   // Like a view in C++20, std::span doesn't own the data, so it's cheap to pass around.
   void TranslateCommands_RHI_IR_Mixed(int32_t workerIndex, std::span<const GenericRendererCommand> subSpan)
   {
      int32_t frameIdx = fenceSync->GetFrameArrayIdx();
      ComPtr<ICommandList>& cmdList = cmdLists[workerIndex];
      for (const GenericRendererCommand& cmd : subSpan)
      {
         if (cmd.CmdType == GenericRendererCommand::Type::ClearPiplelineBuffers)
         {
            if (cmd.Count == 1 && cmd.Params.UIntArray8[0] == (int32_t)PiplelineBuffer::Backbuffer)
            {
               DescriptorHandle handle = piplelineRTVs[GetPiplelineBufferIndex(PiplelineBuffer::Backbuffer, frameIdx)];
               cmdList->ClearRenderTargetView(descriptorMgr->GetCPUHandle(handle), (float*)(&cmd.Params.Float4_1), 0, nullptr);
            }
         }
      }
   }
}

D3D12Renderer::D3D12Renderer(void* windowHandle, int32_t threadCount, XMINT2 backBufferSize, int32_t refreshRate) :
   IRenderer(threadCount, "D3D12Renderer", backBufferSize, refreshRate)
{
   SingletonCheck();
   hwnd = HWND(windowHandle);
   rendererInstance = this;
   workerThreadCount = threadCount;
   currentBackBufferSize = backBufferSize;
   CreateBase();
   CreateManagers();
   CreateFrames();
   TEMP_RendererTestZone();
   CheckDriverFeatures();
}

D3D12Renderer::~D3D12Renderer()
{
}

uint64_t D3D12Renderer::GetFrameIndex()
{
   return fenceSync->GetFrameIndex();
}

void D3D12Renderer::Worker(int32_t workerIndex)
{
   int32_t frameIdx = fenceSync->GetFrameArrayIdx();
   ComPtr<ICommandList>& cmdList = cmdLists[workerIndex];
   ID3D12CommandAllocator* allocator = cmdAllocators[frameIdx * workerThreadCount + workerIndex].Get();
   Check_HRESULT(allocator->Reset());
   Check_HRESULT(cmdList->Reset(allocator, nullptr));

   // Copy all dirty buffers to default heaps.
   if (workerIndex == 0) UnionBuffer::GPUCopyAll(cmdList);

   // Do actual work.
   if (workerIndex == 0)
   {
      ApplyBarrier(cmdList, backBuffers[frameIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
   }
   const auto mixedCmdList = GetBusyCmdList();
   int32_t cmdCount = mixedCmdList->size();
   int32_t cmdSlice = cmdCount / workerThreadCount;
   // Split the command list into sublists for each worker.
   std::span<const GenericRendererCommand> bigSpan(*mixedCmdList);
   std::span<const GenericRendererCommand> subSpan;
   if (cmdSlice == 0 && workerIndex == 0)
   {
      subSpan = bigSpan;
   }
   else if (cmdSlice == 0 && workerIndex != 0)
   {
      subSpan = {};
   }
   else
   {
      subSpan = bigSpan.subspan(workerIndex * cmdSlice, std::min((workerIndex + 1) * cmdSlice, cmdCount));
   }
   TranslateCommands_RHI_IR_Mixed(workerIndex, subSpan);
   // ...
   if (workerIndex == workerThreadCount - 1)
   {
      ApplyBarrier(cmdList, backBuffers[frameIdx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
   }
   Check_HRESULT(cmdList->Close());
}

void Pillow::Graphics::D3D12Renderer::Pioneer()
{
   TryResizeSwapChain();
}

void D3D12Renderer::Assembler()
{
   lateReleaseMgr->GarbageCollect(); // Place it here, so it works not in the main thread.
   cmdQueue->ExecuteCommandLists(cmdListsRaw.size(), cmdListsRaw.data());
   Check_HRESULT(swapChain->Present(f_VSyncBlanks, (bDeviceSupportTearing && f_VSyncBlanks == 0) ? DXGI_PRESENT_ALLOW_TEARING : 0));
   fenceSync->NextFrame();
}
#endif
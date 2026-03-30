// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#include "Renderer.h"
#include <ranges>
#include <algorithm>

using namespace Pillow;
using namespace Pillow::Graphics;
using namespace DirectX;

namespace
{
   std::unique_ptr<IRenderer> rendererInstance;
   std::vector<GenericRendererCommand> cmdListIdle;
   std::vector<GenericRendererCommand> cmdListBusy;
   std::vector<std::jthread> workers; // jthread from C++20
   std::optional<std::barrier<void(*)() noexcept>> frameBarrier;
   std::atomic<bool> signalCompute;
   std::stop_source signalTerminate;


   // Example: NameID = "HelloWorld@Stages=VS,PS@Depth=0@Blend=0@ASSERT_ON@Quality=2"
   std::string MakePipelineStateID(const string& originalName, const std::vector<KeyValuePair>& macros, const IPipelineState::Description& desc)
   {
      const char separator = ',';
      const char prefixMacro = '@';
      const char prefixValue = '=';
      string name = originalName;
      name += "@Stages=";
      using ShaderType = IPipelineState::ShaderType;
      for (uint16_t i = 1; i <= uint16_t(ShaderType::Count); i++)
      {
         if (i != 1)
         {
            name += separator;
         }
         if (IPipelineState::CheckShaderMask(desc.ShaderMask, ShaderType(i)))
         {
            name += IPipelineState::ShaderAcronyms[i];
         }
      }
      name += std::format("@Depth={}@Blend={}", int32_t(desc.Depth), int32_t(desc.Blend));
      for (const auto& pair : macros)
      {
         name += prefixMacro + pair.GetKey();
         if (!pair.EmptyValue())
         {
            name += prefixValue + pair.GetValueRaw();
         }
      }
      return name;
   }
}

IPipelineState::IPipelineState(string originalName, std::vector<KeyValuePair>& macros, Description& desc) :
   NameID(MakePipelineStateID(originalName, macros, desc)),
   Macros(macros.empty() ? nullptr : std::make_unique<std::vector<KeyValuePair>>(macros)),
   Desc(desc)
{
   // Empty body
}

bool IPipelineState::EqualTo(const IPipelineState& right) const
{
   return this->NameID == right.NameID;
}

static void Pillow::Graphics::BarrierCompletionAction() noexcept
{
   if(rendererInstance) rendererInstance->Assembler();
   signalCompute.store(false, std::memory_order::release);
}

void IRenderer::Initialize(int32_t threadCount, XMINT2 renderBufferSize, int32_t refreshRate, void* parameter)
{
   if (rendererInstance) throw std::runtime_error("Renderer has already been initialized.");
#if defined(_WIN64)
   rendererInstance = std::make_unique<Graphics::D3D12Renderer>(parameter, threadCount, renderBufferSize, refreshRate);
#elif defined(__ANDROID__)
   //RendererInstance = std::make_unique<Pillow::Vulkan12Renderer>(Hwnd, 2);
#endif
}

IRenderer* IRenderer::GetInstance()
{
#ifdef PILLOW_DEBUG
   if (!rendererInstance) throw std::runtime_error("Renderer instance is not initialized.");
#endif
   return rendererInstance.get();
}

void IRenderer::Terminate()
{
   if (!rendererInstance) return;
   signalTerminate.request_stop();
   for (auto& thread : workers)
   {
      if (thread.joinable()) thread.join();
   }
   rendererInstance.reset();
}

IRenderer::IRenderer(int32_t threadCount, std::string name, XMINT2 backBufferSize, int32_t refreshRate) :
   f_RendererName(name),
   f_ThreadCount(threadCount),
   f_BackBufferSize(backBufferSize),
   f_RefreshRate(refreshRate),
   f_VSyncBlanks(1)
{
   cmdListIdle.reserve(ReservedCommandCount);
   cmdListBusy.reserve(ReservedCommandCount);
   workers.reserve(threadCount);
   frameBarrier.emplace(threadCount, BarrierCompletionAction);
   signalCompute.store(false);
}

IRenderer::~IRenderer()
{
   cmdListIdle.clear();
   cmdListIdle.shrink_to_fit();
   cmdListBusy.clear();
   cmdListBusy.shrink_to_fit();
   workers.clear();
   workers.shrink_to_fit();
   frameBarrier.reset();
}

void IRenderer::Launch()
{
   for (int32_t i = 0; i < f_ThreadCount; i++)
   {
      workers.emplace_back(std::jthread(&IRenderer::BaseWorker, this, signalTerminate.get_token(), i));
   }
}

void IRenderer::CommitOrWait()
{
   while (signalCompute.load(std::memory_order::acquire)) std::this_thread::yield();
   cmdListIdle.swap(cmdListBusy);
   cmdListIdle.clear();
   // Pre-process before the worker threads.
   this->Pioneer();
   // Start the worker threads.
   signalCompute.store(true, std::memory_order::release); // Activates all workers.
}

std::vector<GenericRendererCommand>* IRenderer::GetIdleCmdList()
{
   if (!rendererInstance) throw std::runtime_error("Renderer instance is not initialized.");
   return &cmdListIdle;
}

const std::vector<GenericRendererCommand>* IRenderer::GetBusyCmdList()
{
   if (!rendererInstance) throw std::runtime_error("Renderer instance is not initialized.");
   return &cmdListBusy;
}

void IRenderer::BaseWorker(std::stop_token token, int32_t workerIndex)
{
   while(true)
   {
      while (!signalCompute.load(std::memory_order::acquire))
      {
         if (token.stop_requested()) return;
         std::this_thread::yield();
      }
      // ***CORE WORKLOAD*** Translate generic graphics commands.
      this->Worker(workerIndex);
      frameBarrier->arrive_and_wait();
   }
}
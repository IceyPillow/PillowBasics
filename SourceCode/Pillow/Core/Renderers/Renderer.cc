// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 2-Clause License. Do not remove, obscure, or alter this notice.
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

   ForceInline std::vector<KeyValuePair> Sort(const std::vector<KeyValuePair>& macros)
   {
      std::vector<KeyValuePair> result = macros;
      std::sort(result.begin(), result.end());
      return result;
   }
}

GenericPipelineConfig::GenericPipelineConfig(string name, const std::vector<KeyValuePair>& macros,
   const std::vector<string>& cbv, const std::vector<string>& vsTex, const std::vector<string>& psTex, int32_t rtNum, TopologyType topology) :
   Macros(Sort(macros)),
   VSTextures(vsTex),
   PSTextures(psTex),
   ConstantBuffers(cbv),
   RenderTargetCount(rtNum),
   Topology(topology)
{
   const char prefixMacro = '@';
   const char prefixValue = '=';
   for (const auto& pair : Macros)
   {
      name += "@" + pair.GetKey();
      if (!pair.IsKeyOnly())
      {
         name += "=" + pair.GetValueRaw();
      }
   }
   ConfigName = name;
   //auto view = std::ranges::split_view(name, _char0) | std::ranges::views::drop(1);
   //_Macros.reserve(std::ranges::distance(view));
   //for (auto&& _macro : view)
   //{
   //   // Uses std::string_view to avoid a string copy.
   //   auto subView = std::ranges::split_view(std::string_view(_macro.begin(), _macro.end()), _char1);
   //   auto iterator = subView.begin();
   //   ShaderMacro macro;
   //   macro.Name = string((*iterator).begin(), (*iterator).end());
   //   iterator++;
   //   if (iterator != subView.end())
   //   {
   //      macro.Value = string((*iterator).begin(), (*iterator).end());
   //   }
   //   _Macros.push_back(std::move(macro));
   //}
}

bool GenericPipelineConfig::EqualTo(const GenericPipelineConfig& right) const
{
   return this->ConfigName == right.ConfigName;
}

static void Pillow::Graphics::BarrierCompletionAction() noexcept
{
   if(rendererInstance) rendererInstance->Assembler();
   signalCompute.store(false, std::memory_order::release);
}

void IRenderer::Initialize(int32_t threadCount, XMINT2 renderBufferSize, int32_t refreshRate, const void* parameter)
{
   if (rendererInstance) throw std::runtime_error("Renderer has already been initialized.");
#if defined(_WIN64)
   HWND hwnd = *static_cast<const HWND*>(parameter);
   rendererInstance = std::make_unique<Graphics::D3D12Renderer>(hwnd, threadCount, renderBufferSize, refreshRate);
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
   if (!rendererInstance) throw std::runtime_error("Renderer instance is not initialized.");
   signalTerminate.request_stop();
   for (auto& thread : workers)
   {
      if (thread.joinable()) thread.join();
   }
   rendererInstance.reset();
}

IRenderer::IRenderer(int32_t threadCount, std::string name) :
   f_RendererName(name),
   f_ThreadCount(threadCount),
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
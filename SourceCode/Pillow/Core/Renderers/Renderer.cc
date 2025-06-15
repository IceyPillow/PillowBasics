#include "Renderer.h"

using namespace Pillow;
using namespace Pillow::Graphics;

std::unique_ptr<GenericRenderer> Pillow::Graphics::Instance{};

namespace
{
   // Asynchronous vs synchronous multithreading rendering
   // 
   // If the CPU job comprises two parts: Tt(tick computation time) and Tg(graphics compuation time)
   // 
   // An async method with one buffer generates a higher frame rate but with a longer delay.
   // The maximum span of a CPU frame is 2*Tg (Tt < Tg) or Tt + Tg (Tt >= Tg), if we don't take into account the GPU.
   // 
   // On the other hand, a sync method generates a lower framerate, but provides a better delay.
   // The maximum span is Tt + Tg, if we don't take into account the GPU.
   //
   // We choose the first method for a better performance.

   std::vector<Drawcall> cachedDrawcalls;
   std::vector<Drawcall> submittedDrawcalls;

   std::vector<std::thread> workers;
   std::optional<std::barrier<void(*)() noexcept>> frameBarrier;
   std::atomic<bool> signal_IsActive;
   std::atomic<bool> signal_IsComputing;
}

static void Pillow::Graphics::BarrierCompletionAction() noexcept
{
   if(Instance) Instance->Assembler();
   signal_IsComputing.store(false, std::memory_order::release);
}

GenericRenderer::GenericRenderer(int32_t threadCount, std::string name) :
   _RendererName(name),
   _ThreadCount(threadCount)
{
   workers.reserve(threadCount);
   frameBarrier.emplace(threadCount, BarrierCompletionAction);
   signal_IsActive.store(true);
   signal_IsComputing.store(false);
}

GenericRenderer::~GenericRenderer()
{
   workers.clear();
   frameBarrier.reset();
}

void GenericRenderer::Launch()
{
   for (int32_t i = 0; i < _ThreadCount; i++)
   {
      workers.emplace_back(std::thread(&GenericRenderer::BaseWorker, this, i));
   }
}

void GenericRenderer::Terminate()
{
   signal_IsActive.store(false, std::memory_order::release);
   for (auto& thread : workers)
   {
      if (thread.joinable()) thread.join();
   }
}

void GenericRenderer::Commit()
{
   while (signal_IsComputing.load(std::memory_order::acquire)) std::this_thread::yield();
   this->Pioneer();
   signal_IsComputing.store(true, std::memory_order::release);
}

//#include <Windows.h>
//#include <format>
void GenericRenderer::BaseWorker(int32_t workerIndex)
{
   while(true)
   {
      while (!signal_IsComputing.load(std::memory_order::acquire))
      {
         if (!signal_IsActive.load(std::memory_order::acquire)) return;
         std::this_thread::yield();
      }
      //OutputDebugString(std::format(L"Frame={} Worker={}\n", this->GetFrameIndex(), workerIndex).c_str());
      this->Worker(workerIndex);
      frameBarrier->arrive_and_wait();
   }
}
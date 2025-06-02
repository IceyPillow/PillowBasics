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
   // An full async method with triple buffering could generate a higher frame rate but with a worse delay.
   // The maximum delay of a CPU frame from the beginning to the end is 3Tg (Tt < Tg) or Tt + Tg (Tt > Tg)
   // 
   // On the other hand, a full sync method may generate a lower framerate, but provides a better delay.
   // The maximum delay always is Tt + Tg.
   //
   // A sync method seems to be better for normal games.
   //
   // But we can mix them up to go beyond.
   // 
   // Use a single buffer between the tick thread pool and the renderer thread pool.
   // In this case, the maximum delay is Tg - Tt, which is acceptable.

   std::vector<Drawcall> cachedDrawcalls;

   std::vector<std::thread> workers;
   void BarrierCompletionAction() noexcept;
   std::optional<std::barrier<void(*)() noexcept>> frameBarrier;
   std::atomic<bool> signal_IsActive;
   std::atomic<bool> signal_IsComputing;

   void BarrierCompletionAction() noexcept
   {
      signal_IsComputing.store(false, std::memory_order::release);
   }
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
      if (workerIndex == 0) this->Assembler();
   }
}
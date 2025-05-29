#include "Renderer.h"

std::unique_ptr<Pillow::GenericRenderer> Pillow::RendererInstance{};

namespace
{
   std::vector<std::thread> workers;
   void BarrierCompletionAction() noexcept;
   std::optional<std::barrier<void(*)() noexcept>> frameBarrier;
   std::atomic<bool> signalActive;
   std::atomic<bool> signalSync;

   void BarrierCompletionAction() noexcept
   {
      signalSync.store(false, std::memory_order::release);
   }
}

Pillow::GenericRenderer::GenericRenderer(int32_t threadCount, std::string name) :
   _RendererName(name),
   _ThreadCount(threadCount)
{
   workers.reserve(threadCount);
   frameBarrier.emplace(threadCount, BarrierCompletionAction);
   signalActive.store(true);
   signalSync.store(false);
}

Pillow::GenericRenderer::~GenericRenderer()
{
   workers.clear();
   frameBarrier.reset();
}

void Pillow::GenericRenderer::RendererLaunch()
{
   for (int32_t i = 0; i < _ThreadCount; i++)
   {
      workers.emplace_back(std::thread(&GenericRenderer::BaseWorker, this, i));
   }
}

void Pillow::GenericRenderer::RendererTerminate()
{
   signalActive.store(false);
   for (auto& thread : workers)
   {
      if (thread.joinable()) thread.join();
   }
}

void Pillow::GenericRenderer::FrameBegin()
{
   signalSync.store(true, std::memory_order::release);
}

bool Pillow::GenericRenderer::IsFrameEnd()
{
   return !signalSync.load(std::memory_order::acquire);
}

void Pillow::GenericRenderer::BaseWorker(int32_t workerIndex)
{
   while (signalActive.load())
   {
      while (!signalSync.load(std::memory_order::acquire)) std::this_thread::yield();
      this->Worker(workerIndex);
      frameBarrier->arrive_and_wait();
   }
}
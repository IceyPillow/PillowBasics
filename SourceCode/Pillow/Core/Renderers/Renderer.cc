#include "Renderer.h"

using namespace Pillow;
using namespace Pillow::Graphics;

std::unique_ptr<GenericRenderer> Pillow::Graphics::Instance{};

namespace
{
   std::vector<std::thread> workers;
   std::vector<GenericGraphicsCommand> genericCommands;
   void BarrierCompletionAction() noexcept;
   std::optional<std::barrier<void(*)() noexcept>> frameBarrier;
   std::atomic<bool> signalActive;
   std::atomic<bool> signalSync;

   void BarrierCompletionAction() noexcept
   {
      signalSync.store(false, std::memory_order::release);
   }
}

GenericRenderer::GenericRenderer(int32_t threadCount, std::string name) :
   _RendererName(name),
   _ThreadCount(threadCount)
{
   workers.reserve(threadCount);
   frameBarrier.emplace(threadCount, BarrierCompletionAction);
   signalActive.store(true);
   signalSync.store(false);
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
   signalActive.store(false);
   //BuildCommandsBegin();
   for (auto& thread : workers)
   {
      if (thread.joinable()) thread.join();
   }
}

void GenericRenderer::BuildCommands()
{
   signalSync.store(true, std::memory_order::release);
}

bool GenericRenderer::IsFrameEnd()
{
   return !signalSync.load(std::memory_order::acquire);
}

void GenericRenderer::BaseWorker(int32_t workerIndex)
{
   while (signalActive.load())
   {
      while (!signalSync.load(std::memory_order::acquire)) std::this_thread::yield();
      this->Worker(workerIndex);
      frameBarrier->arrive_and_wait();
   }
}
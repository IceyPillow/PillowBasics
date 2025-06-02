#include "Constants.h"

using namespace Pillow;

int32_t Pillow::Constants::ThreadNumRenderer{};
int32_t Pillow::Constants::ThreadNumPhysics{};
int32_t Pillow::Constants::ThreadNumTick{};

void Constants::SetThreadNumbers()
{
   if (ThreadNumRenderer != 0) throw std::runtime_error("Thread numbers have already been set.");
   int32_t threadNum = std::thread::hardware_concurrency();
   ThreadNumRenderer = std::clamp(threadNum / 4, 1, 4);
   ThreadNumTick = ThreadNumPhysics = std::clamp(threadNum / 4, 1, 8);
}
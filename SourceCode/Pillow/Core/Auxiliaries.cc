#include "Auxiliaries.h"
#include <codecvt>

using namespace Pillow;
using namespace std::chrono;

double Pillow::GlobalDeltaTime{ 0 }, Pillow::GlobalLastingTime{ 0 };

namespace
{
   GameClock globalGameClock;
}

string Pillow::GetResourcePath(const string& name)
{
   using namespace std::filesystem;
   static std::filesystem::path resourceRootPath;
   if (resourceRootPath.empty())
   {
      path currentPath = current_path();
      do
      {
         path searchPath = currentPath / path("Resources");
         if (exists(searchPath))
         {
            resourceRootPath = searchPath;
            break;
         }
         currentPath = currentPath.parent_path();
      } while (currentPath != currentPath.root_path());
      if (resourceRootPath.empty()) throw std::exception("\"Resources\" folder does not exist.");
   }
   string result;
#if defined(_WIN64)
   std::wstring _result = resourceRootPath / name;
   utf8::utf16to8(_result.begin(), _result.end(), std::back_inserter(result));
#elif defined(__ANDROID__)
#endif
   return result;
}

void Pillow::LogSystem(const string& text)
{
#if defined(_WIN64)
   std::wstring _text;
   utf8::utf8to16(text.begin(), text.end(), std::back_inserter(_text));
   OutputDebugString(_text.c_str());
   OutputDebugString(L"\n");
#elif defined(__ANDROID__)
#endif
}

void Pillow::LogGame(const string& text)
{
   //
}

void GameClock::Start()
{
   startPoint = steady_clock::now();
   lastPoint = startPoint;
}

void GameClock::GetTime(double& deltaTimeInSeconds, double& lastingTimeInSeconds)
{
   auto currentPoint = steady_clock::now();
   deltaTimeInSeconds = duration_cast<duration<double, std::ratio<1>>>(currentPoint - lastPoint).count();
   lastingTimeInSeconds = duration_cast<duration<double, std::ratio<1>>>(currentPoint - startPoint).count();
   lastPoint = currentPoint;
}

double GameClock::GetPrecisionMilliseconds()
{
   const int32_t test_rounds = 5;
   auto last = steady_clock::now();
   steady_clock::duration precision = steady_clock::duration::max();
   for (size_t i = 0; i < test_rounds; ++i) {
      auto next = steady_clock::now();
      while (next == last) next = steady_clock::now();
      auto interval = next - last;
      precision = std::min(precision, interval);
      last = next;
   }
   return duration_cast<duration<double, std::milli>>(precision).count();
}

void Pillow::GlobalClockStart()
{
   globalGameClock.Start();
}

void Pillow::GlobalClockUpdate()
{
   globalGameClock.GetTime(GlobalDeltaTime, GlobalLastingTime);
}
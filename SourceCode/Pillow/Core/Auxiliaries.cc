#include "Auxiliaries.h"
#include <codecvt>

using namespace Pillow;
using namespace std::chrono;


std::string Pillow::Wstring2String(const std::wstring& wstr)
{
   static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
   return converter.to_bytes(wstr);
}

std::wstring Pillow::String2Wstring(const std::string& str)
{
   static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
   return converter.from_bytes(str);
}

std::wstring Pillow::GetResourcePath(const std::wstring& name)
{
   using namespace std::filesystem;
   static std::filesystem::path resourceRootPath;
   if (resourceRootPath.empty())
   {
      path currentPath = current_path();
      do
      {
         path searchPath = currentPath / path(L"Resources");
         if (exists(searchPath))
         {
            resourceRootPath = searchPath;
            break;
         }
         currentPath = currentPath.parent_path();
      } while (currentPath != currentPath.root_path());
      if (resourceRootPath.empty()) throw std::exception("\"Resources\" folder does not exist.");
   }
   return resourceRootPath / path(name);
}

void Pillow::LogSystem(const std::wstring& text)
{
#if defined(_WIN64)
   OutputDebugString(text.c_str());
   OutputDebugString(L"\n");
#elif defined(__ANDROID__)
#endif
}

void Pillow::LogGame(const std::wstring& text)
{
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


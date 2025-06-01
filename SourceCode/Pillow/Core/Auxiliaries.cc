#include "Auxiliaries.h"
#include <codecvt>
#include <chrono>

using namespace std::chrono;

namespace
{
   /*
   * std::chrono::steady_clock
   * 
   * [https://en.cppreference.com/w/cpp/chrono/steady_clock]
   * 
   * [Member types]
   * rep: a number type used to count ticks. e.g.: int64_t, double
   * period: a std::ratio type representing the tick size in seconds. e.g.: ratio<1, 1000> means 1 millisecond.
   * duration: a type representing ranges of ticks.
   * time_point: a type representing time points.
   */

   steady_clock::time_point startPoint;
   steady_clock::time_point lastPoint;
}

namespace Pillow
{
   std::string Wstring2String(const std::wstring& wstr)
   {
      static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
      return converter.to_bytes(wstr);
   }

   std::wstring String2Wstring(const std::string& str)
   {
      static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
      return converter.from_bytes(str);
   }

   std::wstring GetResourcePath(const std::wstring& name)
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
         if(resourceRootPath.empty()) throw std::exception("\"Resources\" folder does not exist.");
      }
      return resourceRootPath / path(name);
   }

   void Clock::Start()
   {
      startPoint = steady_clock::now();
      lastPoint = startPoint;
   }

   void Clock::GetFrameTime(double& deltaTimeInSeconds, double& lastingTimeInSeconds)
   {
      auto currentPoint = steady_clock::now();
      deltaTimeInSeconds = duration_cast<duration<double, std::ratio<1>>>(currentPoint - lastPoint).count();
      lastingTimeInSeconds = duration_cast<duration<double, std::ratio<1>>>(currentPoint - startPoint).count();
      lastPoint = currentPoint;
   }

   double Clock::GetPrecisionMilliseconds()
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
}

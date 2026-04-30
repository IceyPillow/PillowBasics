// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#include "Common.h"
#include <stdexcept>
#include <thread>
#include <regex>
#include <ranges>

using namespace Pillow::Common;
using namespace std::chrono;

namespace
{
   KeyValuePair::Type GetValueType(const string& value, bool isStringValue)
   {
      KeyValuePair::Type type;
      if (value.empty() || isStringValue)
      {
         type = KeyValuePair::Type::String;
      }
      else if (std::regex_match(value, std::regex(R"(^\-?\d+$)")))
      {
         type = KeyValuePair::Type::Integer;
      }
      else if (std::regex_match(value, std::regex(R"(^\-?\d+\.\d+$)")))
      {
         type = KeyValuePair::Type::Float;
      }
      else if (std::regex_match(value, std::regex(R"(^\-?\d+\.\d+(,\-?\d+\.\d+){1,3}$)")))
      {
         type = KeyValuePair::Type::Float4;
      }
      else
      {
         type = KeyValuePair::Type::String;
      }
      return type;
   }
}

namespace Pillow::Hidden
{
   GameClock GlobalClock{};
}

namespace Pillow::Constants
{
   int32_t ThreadNumRenderer{};
   int32_t ThreadNumPhysics{};
   int32_t ThreadNumTick{};

   void SetThreadNumbers()
   {
      if (ThreadNumRenderer != 0) throw std::runtime_error("Thread numbers have already been set.");
      int32_t threadNum = std::thread::hardware_concurrency();
      ThreadNumRenderer = std::clamp(threadNum / 4, 1, MaxThreadNumRenderer);
      ThreadNumTick = ThreadNumPhysics = std::clamp(threadNum / 4, 1, MaxThreadNumOther);
   }
}

namespace Pillow::Common
{
   KeyValuePair::KeyValuePair(string key, string value, bool isStringValue) :
      f_Key(std::regex_replace(key, std::regex("\\s"), "")),
      f_ValueRaw(std::regex_replace(value, std::regex("\\s"), ""))
   {
      f_Type = GetValueType(f_ValueRaw, isStringValue);
   }

   void KeyValuePair::SetValue(string value, bool bPureString)
   {
      f_ValueRaw = std::regex_replace(value, std::regex("\\s"), "");
      f_Type = GetValueType(f_ValueRaw, bPureString);
   }

   XMFLOAT4A KeyValuePair::GetFloat4Aligned() const
   {
      if (f_Type != Type::Float4) throw std::runtime_error("Value type is not Float4.");
      auto view = std::ranges::split_view(f_ValueRaw, ',');
      XMFLOAT4A result{};
      int32_t i = 0;
      for (auto it = view.begin(); it != view.end(); it++, i++)
      {
         string temp((*it).begin(), (*it).end());
         result[i] = std::stof(temp);
      }
      return result;
   }

   std::strong_ordering KeyValuePair::operator<=>(const KeyValuePair& right) const
   {
      if (this->f_Type == Type::Integer && right.f_Type == Type::Integer)
      {
         return this->GetInteger() <=> right.GetInteger();
      }
      if (this->f_Type == Type::Float && right.f_Type == Type::Float)
      {
         return std::strong_order(this->GetFloat(), right.GetFloat());
      }
      return this->f_ValueRaw <=> right.f_ValueRaw;
   }

   bool KeyValuePair::operator==(const KeyValuePair& right) const
   {
      return f_ValueRaw == right.f_ValueRaw;
   }

   path GetCurrentWorkingDirectory()
   {
      return std::filesystem::current_path();
   }

   path GetResourceRelativePath(const path& shortPath)
   {
      const path rootFolder = "Resources";
      static path rootRelativePath;
      {
         static std::mutex mutex;
         std::unique_lock lock(mutex);
         if (rootRelativePath.empty())
         {
            path candidatePath = GetCurrentWorkingDirectory();
            while(true)
            {
               candidatePath /= rootFolder;

               if (exists(candidatePath))
               {
                  rootRelativePath = std::filesystem::relative(candidatePath);
                  break;
               }
               candidatePath = candidatePath / ".." / "..";
               candidatePath = candidatePath.lexically_normal();
               if (candidatePath == candidatePath.root_path()) break;
            }
            if (rootRelativePath.empty()) throw std::exception("Resources folder does not exist.");
         }
      }
      if (shortPath.is_absolute())
         throw std::exception("The path should be relative to the root resources folder.");
      return exists(shortPath) ? shortPath : rootRelativePath / shortPath;
   }

   string GetU8StringfromPath(const path& dir)
   {
      std::string result;
#if defined(_WIN64)
      utf8::utf16to8(dir.native().begin(), dir.native().end(), std::back_inserter(result));
#elif defined(__ANDROID__)
      static_assert(std::is_same_v<path::string_type, std::string>, "The path::string_type test failed.");
      result = dir.native();
#endif
      return result;
   }

   void LogSystem(const string& text)
   {
#if defined(_WIN64)
      std::wstring _text;
      utf8::utf8to16(text.begin(), text.end(), std::back_inserter(_text));
      //OutputDebugString(_text.c_str());
      //OutputDebugString(L"\n");
#elif defined(__ANDROID__)
#endif
   }

   void LogGame(const string& text)
   {
      //
   }

   void GameClock::Restart()
   {
      startPoint = steady_clock::now();
      lastPoint = startPoint;
      f_DeltaTime = 0;
      f_LastingTime = 0;
   }

   void GameClock::Tick()
   {
      auto currentPoint = steady_clock::now();
      f_DeltaTime = duration_cast<duration<double, std::ratio<1>>>(currentPoint - lastPoint).count();
      f_LastingTime = duration_cast<duration<double, std::ratio<1>>>(currentPoint - startPoint).count();
      lastPoint = currentPoint;
   }

   bool GameClock::CheckSlice(float sliceSize)
   {
      auto currentPoint = steady_clock::now();
      float deltaTime = duration_cast<duration<double, std::ratio<1>>>(currentPoint - lastPoint).count();
      bool bSliced = deltaTime > sliceSize;
      if (bSliced)
      {
         f_DeltaTime = deltaTime;
         f_LastingTime = duration_cast<duration<double, std::ratio<1>>>(currentPoint - startPoint).count();
         lastPoint = currentPoint;
      }
      return bSliced;

   }

   double GameClock::GetPrecisionMilliseconds()
   {
      auto precision = steady_clock::duration::max();
      const int32_t sampleCount = 4;
      for (int32_t i : std::views::iota(0, sampleCount)) {
         auto last = steady_clock::now();
         auto next = last;
         while (next == last) next = steady_clock::now();
         auto interval = next - last;
         precision = std::min(precision, interval);
      }
      return duration_cast<duration<double, std::milli>>(precision).count();
   }

   double GetFrameDeltaTime()
   {
      return Hidden::GlobalClock.GetDeltaTime();
   }

   double GetLapseTimeSinceLaunch()
   {
      return Hidden::GlobalClock.GetLastingTime();
   }
}
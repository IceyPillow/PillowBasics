// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 2-Clause License. Do not remove, obscure, or alter this notice.
#include "Auxiliaries.h"
#include <regex>
#include <ranges>

using namespace Pillow;
using namespace std::chrono;

namespace
{
   GameClock globalGameClock;
}

KeyValuePair::KeyValuePair(string key, string value, bool isStringValue) :
   f_Key(std::regex_replace(key, std::regex("\\s"), "")),
   f_ValueRaw(std::regex_replace(value, std::regex("\\s"), ""))
{
   if (value.empty() || isStringValue)
   {
      f_Type = ValueType::String;
   }
   else if (std::regex_match(value, std::regex(R"(^\-?\d+$)")))
   {
      f_Type = ValueType::Integer;
   }
   else if (std::regex_match(value, std::regex(R"(^\-?\d+\.\d+$)")))
   {
      f_Type = ValueType::Float;
   }
   else if (std::regex_match(value, std::regex(R"(^\-?\d+\.\d+(,\-?\d+\.\d+){1,3}$)")))
   {
      f_Type = ValueType::Float4;
   }
   else
   {
      f_Type = ValueType::String;
   }
}

XMFLOAT4A Pillow::KeyValuePair::GetFloat4Aligned()
{

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

bool KeyValuePair::operator==(const KeyValuePair& right) const
{
   if (this->f_Type == ValueType::Integer)
   {
      return this->GetInteger() == right.GetInteger();
   }
   if (this->f_Type == ValueType::Float)
   {
      return this->GetFloat() == right.GetFloat();
   }
   return this->f_Key == right.f_Key && this->f_ValueRaw == right.f_ValueRaw;
}

bool Pillow::KeyValuePair::operator>(const KeyValuePair& right) const
{
   if (this->f_Type == ValueType::Integer)
   {
      return this->GetInteger() > right.GetInteger();
   }
   if (this->f_Type == ValueType::Float)
   {
      return this->GetFloat() > right.GetFloat();
   }
   return this->f_Key > right.f_Key;
}

bool Pillow::KeyValuePair::operator<(const KeyValuePair& right) const
{
   return this->operator>(right);
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

bool Pillow::GameClock::CheckSlice(float sliceSize)
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
#pragma once
#include <map>
#include <typeinfo>
#include <type_traits>
#include <exception>
#include <shared_mutex>
#include <string>
#include <filesystem>
#include <locale>
#include <chrono>

#if defined(_WIN64)
#elif defined(__ANDROID__)
#endif

#if defined(_MSC_VER)
#define ForceInline __forceinline
#elif defined(__GNUC__) | defined(__clang__)
#define ForceInline __attribute__((always_inline))
#endif

// Known issue: VS applies wrong formats for consecutive "PropertyReadonly" macros.
#define ReadonlyProperty(type, name) \
protected: type _##name{}; \
public: ForceInline type Get##name() const { return _##name; }

#define SingletonCheck() \
static decltype(this) instance = nullptr; \
if(instance) throw std::exception("A singleton class cannot be created twice."); \
instance = this;

#define DeleteDefautedMethods(type) \
public: \
type() = delete; \
type(const type&) = delete; \
type(type&&) = delete; \
type& operator=(const type&) = delete; \
type& operator=(type&&) = delete;

namespace Pillow
{
   std::string Wstring2String(const std::wstring& wstr);
   std::wstring String2Wstring(const std::string& str);
   std::wstring GetResourcePath(const std::wstring& name);

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
   class GameClock
   {

   public:
      void Start();
      void GetTime(double& deltaTimeInSeconds, double& lastingTimeInSeconds);
      double GetPrecisionMilliseconds();

   private:
      std::chrono::steady_clock::time_point startPoint{};
      std::chrono::steady_clock::time_point lastPoint{};
   };
}
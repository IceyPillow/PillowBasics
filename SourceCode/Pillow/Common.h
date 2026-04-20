// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include <bit>
#include <limits>
#include <map>
#include <string>
#include <ranges>
#include <chrono>
#include <exception>
#include <algorithm>
#include <filesystem>
#include <shared_mutex>
#include "utfcpp-4.0.6/utf8.h"
#include "DirectXMath-apr2025/DirectXMath.h"

// Macro template
#if defined(_WIN64)
#elif defined(__ANDROID__)
#endif

#if defined(_MSC_VER)
#define ForceInline __forceinline
#elif defined(__GNUC__) | defined(__clang__)
#define ForceInline __attribute__((always_inline))
#endif

// A known issue: VS applies wrong formats for consecutive "PropertyReadonly" macros.
// _Aaa is reserved for the compiler in C++, which is unfriendly to a C# coder.
// "f_" means field; it's from C#.
#define ReadonlyProperty(type, name) \
protected: type f_##name{}; \
public: ForceInline type Get##name() const { return f_##name; }

#define SingletonCheck() \
static decltype(this) hidden_instance = nullptr; \
if(hidden_instance) throw std::exception("A singleton class cannot be created twice."); \
hidden_instance = this;

#define DeleteDefautedMethods(type) \
public: \
type() = delete; \
type(const type&) = delete; \
type(type&&) = delete; \
type& operator=(const type&) = delete; \
type& operator=(type&&) = delete;

namespace Pillow::Constants
{
   // Best anisotropy level value considering both performance and quality.
   const int32_t AnisotropyLevel = 4;
   const int32_t SwapChainSize = 3;

   const DirectX::XMFLOAT4A CleanColor{ 0.2f, 0.21f, 0.2f, 0.0f };
   // Perceptual weightings for the importance of each channel.
   const DirectX::XMFLOAT4A RGBLuminance{ 0.2125f / 0.7154f, 1, 0.0721f / 0.7154f, 1.f };
   const DirectX::XMFLOAT4A RGBLuminanceInv{ 0.7154f / 0.2125f, 1, 0.7154f / 0.0721f, 1.f };

   // 1 Unit = 1 km

   const int32_t MaxStaticRenderItems = 1 << 10;
   const int32_t MaxUIRenderItems = 1 << 8;

   const int32_t MaxThreadNumRenderer = 4, MaxThreadNumOther = 8;
   extern int32_t ThreadNumRenderer, ThreadNumPhysics, ThreadNumTick;
   constexpr float FloatInfinity = std::bit_cast<float>(std::uint32_t(0X7F800000U));
   constexpr float FloatNegativeInfinity = -FloatInfinity;
   static_assert(sizeof(float) == sizeof(std::uint32_t));
   static_assert(std::numeric_limits<float>::is_iec559); // IEEE-754

   constexpr float FrameTime5FPS = 0.2f;
   constexpr float FrameTime10FPS = 0.1f;
   constexpr float FrameTime15FPS = 1.f / 15.f;
   constexpr float FrameTime30FPS = 1.f / 30.f;
   constexpr float FrameTime60FPS = 1.f / 60.f;
   constexpr float FrameTime120FPS = 1.f / 120.f;

   void SetThreadNumbers();
}

namespace Pillow::Common
{
   // I. The Mystery of Character Set
   // ASCII char set:   char = code point(a number)  = encoding(actual binaries)
   // Unicode char set: char = code point(a number) != encoding(actual binaries)
   // Unicode has several encoding methods. The most famous ones are UTF-8, UTF-16, and UTF-32
   // Only UTF-32 is completely equal to the code points of Unicode.
   // 
   // II. Win and UTF-8
   // On Win, it's bad to invoke the system's interfaces with UTF-8.
   // Cause UTF-8 is optional(Region->Change System locale->Beta: Use Unicode UTF-8...),
   // which means the client's machines may not support it.
   // For example, MessageBoxA supports UTF-8 only when clients enable the above option(off 
   // by default), but MessageBoxW always supports UTF-16.
   // 
   // III. The Naughty Wide Character
   // whcar_t represents UTF-16 on Win, However, represents UTF-32 on Android.
   // 
   // IV. The String Convention of Pillow Basics
   // Only use UTF-8 strings in Pillow Basics for coherence.
   using std::string;
   using std::filesystem::path;
   using namespace DirectX;

   struct alignas(64) CacheLine
   {
      uint8_t padding[64]{}; // 64 bytes cache line padding
   };

   // The alignment must be a power of two.
   ForceInline uint32_t GetAlignSize(uint32_t size, uint32_t alignment)
   {
      return (size + alignment - 1) & ~(alignment - 1);
   }

   class KeyValuePair
   {
   public:
      enum struct Type : uint8_t
      {
         String,
         Integer,
         Float,
         Float4
      };

      ReadonlyProperty(string, Key)
         ReadonlyProperty(string, ValueRaw)
         ReadonlyProperty(Type, Type)

   public:
      // bPureString: True if using quick initialization.
      KeyValuePair(string key, string value, bool bPureString = false);
      void SetValue(string value, bool bPureString = false);
      
      bool EmptyValue() const { return f_ValueRaw.empty(); }
      
      int32_t GetInteger() const { return std::stoi(f_ValueRaw); }
      
      float GetFloat() const { return std::stof(f_ValueRaw); }
      
      XMFLOAT4A GetFloat4Aligned() const;

      // Three-way comparison, C++20
      std::strong_ordering operator<=>(const KeyValuePair& right) const;
      bool operator==(const KeyValuePair& right) const;
   };

   inline bool CheckUTF8(const string& str) { return utf8::is_valid(str.begin(), str.end()); }
   inline bool CheckUTF16_HighSurrogate(char16_t uc) { return (uc >= 0xD800 && uc <= 0xDBFF); }
   inline bool CheckUTF16_LowSurrogate(char16_t uc) { return (uc >= 0xDC00 && uc <= 0xDFFF); }
   inline bool CheckUTF16_SingleUnit(char16_t uc) { return (uc & 0xF800) != 0xD800; }

   path GetResourcePath(const path& name);
   void LogSystem(const string& text);
   void LogGame(const string& text);

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
      ReadonlyProperty(double, DeltaTime)
         ReadonlyProperty(double, LastingTime)
   public:
      inline GameClock() { Restart(); }
      void Restart();
      // Updates inner data. Call it every frame.
      void Tick();
      // A code candy for time-slicing algorithm.
      bool CheckSlice(float sliceSize);
      static double GetPrecisionMilliseconds();

   private:
      std::chrono::steady_clock::time_point startPoint;
      std::chrono::steady_clock::time_point lastPoint;
   };

   // Check the global clock.
   double GetFrameDeltaTime();
   // Check the global clock.
   double GetLapseTimeSinceLaunch();

   template<std::unsigned_integral ANY_UINT>
   class GenericHandlePool
   {
   public:
      static const ANY_UINT NullHandle = 0;
      const string Name;
      const ANY_UINT MaxIndex;

      GenericHandlePool(const string& name, ANY_UINT maxIndex) :
         Name(name),
         MaxIndex(maxIndex)
      {
         const uint32_t initialPoolSize = 256;
         FreePool.reserve(initialPoolSize);
      }

      ANY_UINT Acquire()
      {
         if (FreePool.empty() == false)
         {
            ANY_UINT handle = FreePool.back();
            FreePool.pop_back();
            return handle;
         }
         else
         {
            if (head > MaxIndex || head == 0)
               throw std::exception(std::format("Handle pool overflowed. PoolName={}", Name).c_str());
            return head++;
         }
      }

      ANY_UINT Release(ANY_UINT handle)
      {
         if (handle == NullHandle || handle >= head)
         {
            string error = std::format("Invalid handle. PoolName={}, Handle={}", Name, handle);
            throw std::exception(error.c_str());
         }
#ifdef PILLOW_DEBUG
         bool found = std::find(FreePool.begin(), FreePool.end(), handle) != FreePool.end();
         if (found)
         {
            string error = std::format("Cannot release an unbound handle. PoolName={}, Handle={}", Name, handle);
            throw std::exception(error.c_str());
         }
#endif
         FreePool.push_back(handle);
         return NullHandle;
      }

   private:
      ANY_UINT head = 1;
      std::vector<ANY_UINT> FreePool;
   };
}
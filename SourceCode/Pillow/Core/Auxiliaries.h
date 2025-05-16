#pragma once
#include <map>
#include <typeinfo>
#include <type_traits>
#include <exception>
#include <shared_mutex>
#include <string>
#include <filesystem>
#include <locale>

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
   /// <summary>
   /// Compare the types of a and b. If b is nullptr, compare a with T.<para/>
   /// T must be a polymorphic class.
   /// </summary>
   //template<typename T>
   //bool IsSameType(const void* a, const void* b = nullptr)
   //{
   //   static_assert(std::is_polymorphic<T>::value, "T must be a polymorphic class.");
   //   if (!a) return false;
   //   T* ptrA = dynamic_cast<T*>(a);
   //   if (!ptrA) return false;
   //   if (b)
   //   {
   //      T* ptrB = dynamic_cast<T*>(b);
   //      return ptrB ? typeid(*ptrA) == typeid(*ptrB) : false;
   //   }
   //   else
   //   {
   //      return typeid(*ptrA) == typeid(T);
   //   }
   //}

   class GameClock
   {
      DeleteDefautedMethods(GameClock)
   public:
      static void Start();
      static void GetTimeDataPerFrame(double& deltaTimeInSeconds, double& lastingTimeInSeconds);
      static double GetPrecisionInMilliseconds();
   };
}
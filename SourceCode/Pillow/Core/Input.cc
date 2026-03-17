// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 2-Clause License. Do not remove, obscure, or alter this notice.
#include "Input.h"
#include "Renderers/Renderer.h"
#include "utfcpp-4.0.6/utf8.h"
#include<map>
#if defined(_WIN64)
#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#include <winuser.h>
#include <hidusage.h>
#include <xinput.h>
#include <imm.h>
#elif defined(__ANDROID__)
#endif

using namespace Pillow;
using namespace Pillow::Input;

namespace
{
   enum class State : char
   {
      Released,
      Pressed,
      Down,
      Up
   };
}

namespace Pillow::Input
{

   void InputPreTick()
   {
      // Update key states.
      for (auto& keyState : keyStates)
      {
         if (keyState.second == State::Up)
         {
            keyState.second = State::Released;
         }
         else if (keyState.second == State::Down)
         {
            keyState.second = State::Pressed;
         }
      }
#if defined(_WIN64)
      miceOffset = { 0,0 };
      wheelOffset = 0;
#elif defined(__ANDROID__)
#endif
   }

   void ToggleInputMethod(bool bEnable)
   {
#if defined(_WIN64)
      HIMC immContext = ImmGetContext(hwnd);
      ImmSetOpenStatus(immContext, bEnable);
      ImmReleaseContext(hwnd, immContext);
#elif defined(__ANDROID__)
#endif
   }

   void SetInputMethodPosition(XMINT2 position)
   {
#if defined(_WIN64)
      HIMC immContext = ImmGetContext(hwnd);
      COMPOSITIONFORM form;
      form.dwStyle = CFS_POINT;
      form.ptCurrentPos.x = position.x;
      form.ptCurrentPos.y = position.y;
      ImmSetCompositionWindow(immContext, &form);
      ImmReleaseContext(hwnd, immContext);
#elif defined(__ANDROID__)
#endif
   }

   void AddChar16(char16_t character)
   {
      inputBuffer += character;
   }

   bool GetKey(GenricKey key)
   {
      return keyStates[key] == State::Pressed || keyStates[key] == State::Down;
   }

   bool GetKeyDown(GenricKey key)
   {
      return keyStates[key] == State::Down;
   }

   bool GetKeyUp(GenricKey key)
   {
      return keyStates[key] == State::Up;
   }

   std::u32string GetInputStringAndClear()
   {
      std::u32string result{};
      std::string temp{};
      bool bFullU16String = IS_LOW_SURROGATE(inputBuffer.back());
      utf8::utf16to8(inputBuffer.begin(), inputBuffer.end() - (bFullU16String ? 0 : 1), std::back_inserter(temp));
      result = utf8::utf8to32(temp);
      inputBuffer = bFullU16String ? u"" : inputBuffer.substr(inputBuffer.size() - 1);
      return result;
   }
}
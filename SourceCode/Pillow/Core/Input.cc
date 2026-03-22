// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 2-Clause License. Do not remove, obscure, or alter this notice.
#include "Input.h"
#include "Renderers/Renderer.h"
#include "utfcpp-4.0.6/utf8.h"
#include<map>
#include<array>
#if defined(_WIN64)
#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
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

#if defined(_WIN64)
   const char TriggerThreshold = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

   const std::map<uint16_t, GenricKey> KeyMap =
   {
      // Mouse Buttons
      {VK_LBUTTON, GenricKey::MiceLeft},
      {VK_MBUTTON, GenricKey::MiceMiddle},
      {VK_RBUTTON, GenricKey::MiceRight},
      {VK_XBUTTON1, GenricKey::MiceSide0},
      {VK_XBUTTON2, GenricKey::MiceSide1},
      // GamePad Buttons (XInput)
      // GamePad Triggers are analog buttons, so we need to check their values.
      {XINPUT_GAMEPAD_A, GenricKey::PadA},
      {XINPUT_GAMEPAD_B, GenricKey::PadB},
      {XINPUT_GAMEPAD_X, GenricKey::PadX},
      {XINPUT_GAMEPAD_Y, GenricKey::PadY},
      {XINPUT_GAMEPAD_DPAD_UP, GenricKey::PadUp},
      {XINPUT_GAMEPAD_DPAD_DOWN, GenricKey::PadDown},
      {XINPUT_GAMEPAD_DPAD_LEFT, GenricKey::PadLeft},
      {XINPUT_GAMEPAD_DPAD_RIGHT, GenricKey::PadRight},
      // LT and RT are linear triggers, must be treated separately.
      {XINPUT_GAMEPAD_LEFT_SHOULDER, GenricKey::PadLB},
      {XINPUT_GAMEPAD_RIGHT_SHOULDER, GenricKey::PadRB},
      {XINPUT_GAMEPAD_BACK, GenricKey::PadReturn},
      {XINPUT_GAMEPAD_START, GenricKey::PadMenu},
      {XINPUT_GAMEPAD_LEFT_THUMB, GenricKey::StickLeft},
      {XINPUT_GAMEPAD_RIGHT_THUMB, GenricKey::StickRight},
      // Keyboard Letters
      {'A', GenricKey::A},
      {'B', GenricKey::B},
      {'C', GenricKey::C},
      {'D', GenricKey::D},
      {'E', GenricKey::E},
      {'F', GenricKey::F},
      {'G', GenricKey::G},
      {'H', GenricKey::H},
      {'I', GenricKey::I},
      {'J', GenricKey::J},
      {'K', GenricKey::K},
      {'L', GenricKey::L},
      {'M', GenricKey::M},
      {'N', GenricKey::N},
      {'O', GenricKey::O},
      {'P', GenricKey::P},
      {'Q', GenricKey::Q},
      {'R', GenricKey::R},
      {'S', GenricKey::S},
      {'T', GenricKey::T},
      {'U', GenricKey::U},
      {'V', GenricKey::V},
      {'W', GenricKey::W},
      {'X', GenricKey::X},
      {'Y', GenricKey::Y},
      {'Z', GenricKey::Z},
      // Keyboard Numbers
      {'0', GenricKey::Num0},
      {'1', GenricKey::Num1},
      {'2', GenricKey::Num2},
      {'3', GenricKey::Num3},
      {'4', GenricKey::Num4},
      {'5', GenricKey::Num5},
      {'6', GenricKey::Num6},
      {'7', GenricKey::Num7},
      {'8', GenricKey::Num8},
      {'9', GenricKey::Num9},
      // Function Keys
      {VK_F1, GenricKey::F1},
      {VK_F2, GenricKey::F2},
      {VK_F3, GenricKey::F3},
      {VK_F4, GenricKey::F4},
      {VK_F5, GenricKey::F5},
      {VK_F6, GenricKey::F6},
      {VK_F7, GenricKey::F7},
      {VK_F8, GenricKey::F8},
      {VK_F9, GenricKey::F9},
      {VK_F10, GenricKey::F10},
      {VK_F11, GenricKey::F11},
      {VK_F12, GenricKey::F12},
      // Symbols
      {VK_OEM_3, GenricKey::Backtick},        // `~ key
      {VK_OEM_MINUS, GenricKey::Minus},       // -_ key
      {VK_OEM_PLUS, GenricKey::Equals},       // =+ key
      {VK_OEM_4, GenricKey::BracketLeft},     // [{ key
      {VK_OEM_6, GenricKey::BracketRight},    // ]} key
      {VK_OEM_5, GenricKey::Backslash},       // \| key
      {VK_OEM_1, GenricKey::Semicolon},       // ;: key
      {VK_OEM_7, GenricKey::Quote},           // '" key
      {VK_OEM_COMMA, GenricKey::Comma},       // ,< key
      {VK_OEM_PERIOD, GenricKey::Period},     // .> key
      {VK_OEM_2, GenricKey::Slash},           // /? key
      // Control Keys
      {VK_ESCAPE, GenricKey::Esc},
      {VK_TAB, GenricKey::Tab},
      {VK_CAPITAL, GenricKey::CapsLock},
      {VK_SHIFT, GenricKey::Shift},           // Covers both left and right Shift
      {VK_CONTROL, GenricKey::Ctrl},          // Covers both left and right Ctrl
      {VK_MENU, GenricKey::Alt},              // Covers both left and right Alt
      {VK_SPACE, GenricKey::Space},
      {VK_BACK, GenricKey::Backspace},
      {VK_RETURN, GenricKey::Enter},
      // Arrow Keys
      {VK_UP, GenricKey::ArrowUp},
      {VK_DOWN, GenricKey::ArrowDown},
      {VK_LEFT, GenricKey::ArrowLeft},
      {VK_RIGHT, GenricKey::ArrowRight}
   };

   HWND hwnd{};

   std::u16string inputBuffer{};
   XMFLOAT2 micePos{};
   XMFLOAT2 miceOffset{};
   float wheelOffset{};

#elif defined(__ANDROID__)
   //...
#endif

   std::map<GenricKey, State> genericKeyStates{};
}

namespace Pillow::Input
{

   void InputPreTick()
   {
      // Update key states.
      for (auto& keyState : genericKeyStates)
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
      return genericKeyStates[key] == State::Pressed || genericKeyStates[key] == State::Down;
   }

   bool GetKeyDown(GenricKey key)
   {
      return genericKeyStates[key] == State::Down;
   }

   bool GetKeyUp(GenricKey key)
   {
      return genericKeyStates[key] == State::Up;
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
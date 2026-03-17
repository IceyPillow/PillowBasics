// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 2-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include <string>
#include "DirectXMath-apr2025/DirectXMath.h"

using namespace DirectX;

namespace Pillow::Input
{
   // Not all keys are valid on a specific platform.
   // e.g. A mice is not intended to be supported on Android.
   enum class GenricKey : char
   {
      // Screen touch
      Touch0, Touch1, Touch2, Touch3, Touch4, Touch5,
      // Mice
      MiceLeft, MiceMiddle, MiceRight, MiceSide0, MiceSide1,
      // GamePad
      PadX, PadY, PadA, PadB,
      PadUp, PadDown, PadLeft, PadRight,
      PadLB, PadLT, PadRB, PadRT,
      PadReturn, PadMenu,
      StickLeft, StickRight,
      // Keyboard Keys
      // Letters
      A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
      // Numbers
      Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
      // Function Keys
      F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
      // Symbols
      Backtick, Minus, Equals, BracketLeft, BracketRight, Backslash,
      Semicolon, Quote, Comma, Period, Slash,
      // Control Keys
      Esc, Tab, CapsLock, Shift, Ctrl, Alt, Space, Backspace, Enter,
      // Arrow Keys
      ArrowUp, ArrowDown, ArrowLeft, ArrowRight,
      Count
   };

   // C++26 provides reflections, not C++20.
   inline const char* const GenricKeyName[] =
   {
      "Touch0", "Touch1", "Touch2", "Touch3", "Touch4", "Touch5",
      "MiceLeft", "MiceMiddle", "MiceRight", "MiceSide0", "MiceSide1",
      "PadX", "PadY", "PadA", "PadB",
      "PadUp", "PadDown", "PadLeft", "PadRight",
      "PadLB", "PadLT", "PadRB", "PadRT",
      "PadReturn", "PadMenu",
      "StickLeft", "StickRight",
      "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
      "Num0", "Num1", "Num2", "Num3", "Num4", "Num5", "Num6", "Num7", "Num8", "Num9",
      "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
      "Backtick", "Minus", "Equals", "BracketLeft", "BracketRight", "Backslash",
      "Semicolon", "Quote", "Comma", "Period", "Slash",
      "Esc", "Tab", "CapsLock", "Shift", "Ctrl", "Alt", "Space", "Backspace", "Enter",
      "ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight",
   };

   void InputInitialize(void* params);
   void InputClose();
   // Update once per frame; invoke it before processing any message!
   void InputPreTick();
   // Process a single message.
   void InputCallback(const void* messages);
   
   void ToggleInputMethod(bool bEnable);
   void SetInputMethodPosition(XMINT2 position);
   void AddChar16(char16_t character);

   bool GetKey(GenricKey key);
   bool GetKeyDown(GenricKey key);
   bool GetKeyUp(GenricKey key);
   std::u32string GetInputStringAndClear();

#if defined(_WIN64)
   XMFLOAT2 GetMicePos();
   XMFLOAT2 GetMiceOffset();
   float GetWheelOffset();
#elif defined(__ANDROID__)
   XMFLOAT2 GetTouchPos(uint32_t index);
   XMFLOAT2 GetTouchOffset(uint32_t index);
#endif
}
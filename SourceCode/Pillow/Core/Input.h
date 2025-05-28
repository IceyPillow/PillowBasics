#pragma once
#include "DirectXMath-apr2025/DirectXMath.h"

namespace Pillow::Input
{
   enum class GenricButton : char
   {
      // Mice
      MiceMiddle, MiceLeft, MiceRight, MiceSide0, MiceSide1,
      // GamePad
      PadX, PadY, PadA, PadB,
      PadUp, PadDown, PadLeft, PadRight,
      PadLB, PadLT, PadRB, PadRT,
      PadReturn, PadMenu,
      StickLeft, StickRight,
      Count
   };

   const char* GenricButtonName[(int32_t)GenricButton::Count] =
   {
      // Mice
      "MiceMiddle", "MiceLeft", "MiceRight", "MiceSide0", "MiceSide1",
      // GamePad
      "PadX", "PadY", "PadA", "PadB",
      "PadUp", "PadDown", "PadLeft", "PadRight",
      "PadLB", "PadLT", "PadRB", "PadRT",
      "PadReturn", "PadMenu",
      "StickLeft", "StickRight"
   };

   enum class GenricKey : char
   {
      // Letters
      A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
      // Numbers
      Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
      // Function Keys
      F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
      // Symbols
      Backtick, Minus, Euqals, BracketLeft, BracketRight, Backslash, Semicolon, Quote, Comma, Period, Slash,
      // Control Keys
      Esc, Tab, CapsLock, Shift, Ctrl, Alt, Space, Backspace, Enter,
      // Arrow Keys
      ArrowUp, ArrowDown, ArrowLeft, ArrowRight,
      Count
   };
   const char* GenricKeyName[(int32_t)GenricKey::Count] =
   {
      // Letters
      "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
      "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
      // Numbers
      "Num0", "Num1", "Num2", "Num3", "Num4", "Num5", "Num6", "Num7", "Num8", "Num9",
      // Function Keys
      "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
      // Symbols
      "Backtick", "Minus", "Equals", "BracketLeft", "BracketRight", "Backslash",
      "Semicolon", "Quote", "Comma", "Period", "Slash",
      // Control Keys
      "Esc", "Tab", "CapsLock", "Shift", "Ctrl", "Alt", "Space", "Backspace", "Enter",
      // Arrow Keys
      "ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight"
   };

   using DirectX::XMFLOAT2;
//#if defined(_WIN64)
//   XMFLOAT2 GetMousePos();
//   XMFLOAT2 GetMouseOffset();
//   float GetWheelOffset();
//#elif defined(__ANDROID__)
//#endif


   void InputInitialize();

   void InputCallback();

   bool GetKey(int32_t keyCode);
   bool GetKeyDown(int32_t keyCode);
   bool GetKeyUp(int32_t keyCode);
   bool GetButton(GenricButton button);
   bool GetButtonDown(GenricButton button);
   bool GetButtonUp(GenricButton button);
}
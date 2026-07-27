// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
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
#include <Xinput.h>
#include <imm.h>
#elif defined(__ANDROID__)
#endif

using namespace Pillow;
using namespace Pillow::Common;
using namespace Pillow::Input;

namespace Pillow::Hidden
{
   // A positive value indicates rotating upward.
   float WheelAccumulation{};
}

// Static variables & types
namespace
{
   constexpr int32_t StringBufferMaxSize = 128;

#if defined(_WIN64)
   constexpr int32_t RawInputBufferMinNum = 128;

   constexpr int32_t MiceKeyNum = 5;

   constexpr uint16_t MiceKeys[MiceKeyNum] =
   {
      VK_LBUTTON,
      VK_MBUTTON,
      VK_RBUTTON,
      VK_XBUTTON1,
      VK_XBUTTON2
   };

   constexpr GenericKey MiceGKeys[MiceKeyNum] =
   {
      GenericKey::MiceLeft,
      GenericKey::MiceMiddle,
      GenericKey::MiceRight,
      GenericKey::MiceSide0,
      GenericKey::MiceSide1
   };

   constexpr int32_t GamepadKeyNum = 14;

   constexpr uint16_t GamepadKeys[GamepadKeyNum] =
   {
      XINPUT_GAMEPAD_A,
      XINPUT_GAMEPAD_B,
      XINPUT_GAMEPAD_X,
      XINPUT_GAMEPAD_Y,
      XINPUT_GAMEPAD_DPAD_UP,
      XINPUT_GAMEPAD_DPAD_DOWN,
      XINPUT_GAMEPAD_DPAD_LEFT,
      XINPUT_GAMEPAD_DPAD_RIGHT,
      XINPUT_GAMEPAD_LEFT_SHOULDER,
      XINPUT_GAMEPAD_RIGHT_SHOULDER,
      XINPUT_GAMEPAD_BACK,
      XINPUT_GAMEPAD_START,
      XINPUT_GAMEPAD_LEFT_THUMB,
      XINPUT_GAMEPAD_RIGHT_THUMB
   };

   constexpr GenericKey GamepadGKeys[GamepadKeyNum] =
   {
      GenericKey::PadA,
      GenericKey::PadB,
      GenericKey::PadX,
      GenericKey::PadY,
      GenericKey::PadUp,
      GenericKey::PadDown,
      GenericKey::PadLeft,
      GenericKey::PadRight,
      GenericKey::PadLB,
      GenericKey::PadRB,
      GenericKey::PadReturn,
      GenericKey::PadMenu,
      GenericKey::StickLeft,
      GenericKey::StickRight
   };

   const std::map<uint16_t, GenericKey> KeyMap =
   {
      // Mouse Buttons
      {VK_LBUTTON, GenericKey::MiceLeft},
      {VK_MBUTTON, GenericKey::MiceMiddle},
      {VK_RBUTTON, GenericKey::MiceRight},
      {VK_XBUTTON1, GenericKey::MiceSide0},
      {VK_XBUTTON2, GenericKey::MiceSide1},
      // GamePad Buttons (XInput)
      // GamePad Triggers are analog buttons, so we need to check their values.
      {XINPUT_GAMEPAD_A, GenericKey::PadA},
      {XINPUT_GAMEPAD_B, GenericKey::PadB},
      {XINPUT_GAMEPAD_X, GenericKey::PadX},
      {XINPUT_GAMEPAD_Y, GenericKey::PadY},
      {XINPUT_GAMEPAD_DPAD_UP, GenericKey::PadUp},
      {XINPUT_GAMEPAD_DPAD_DOWN, GenericKey::PadDown},
      {XINPUT_GAMEPAD_DPAD_LEFT, GenericKey::PadLeft},
      {XINPUT_GAMEPAD_DPAD_RIGHT, GenericKey::PadRight},
      // LT and RT are linear triggers, must be treated separately.
      {XINPUT_GAMEPAD_LEFT_SHOULDER, GenericKey::PadLB},
      {XINPUT_GAMEPAD_RIGHT_SHOULDER, GenericKey::PadRB},
      {XINPUT_GAMEPAD_BACK, GenericKey::PadReturn},
      {XINPUT_GAMEPAD_START, GenericKey::PadMenu},
      {XINPUT_GAMEPAD_LEFT_THUMB, GenericKey::StickLeft},
      {XINPUT_GAMEPAD_RIGHT_THUMB, GenericKey::StickRight},
      // Keyboard Letters
      {'A', GenericKey::A},
      {'B', GenericKey::B},
      {'C', GenericKey::C},
      {'D', GenericKey::D},
      {'E', GenericKey::E},
      {'F', GenericKey::F},
      {'G', GenericKey::G},
      {'H', GenericKey::H},
      {'I', GenericKey::I},
      {'J', GenericKey::J},
      {'K', GenericKey::K},
      {'L', GenericKey::L},
      {'M', GenericKey::M},
      {'N', GenericKey::N},
      {'O', GenericKey::O},
      {'P', GenericKey::P},
      {'Q', GenericKey::Q},
      {'R', GenericKey::R},
      {'S', GenericKey::S},
      {'T', GenericKey::T},
      {'U', GenericKey::U},
      {'V', GenericKey::V},
      {'W', GenericKey::W},
      {'X', GenericKey::X},
      {'Y', GenericKey::Y},
      {'Z', GenericKey::Z},
      // Keyboard Numbers
      {'0', GenericKey::Num0},
      {'1', GenericKey::Num1},
      {'2', GenericKey::Num2},
      {'3', GenericKey::Num3},
      {'4', GenericKey::Num4},
      {'5', GenericKey::Num5},
      {'6', GenericKey::Num6},
      {'7', GenericKey::Num7},
      {'8', GenericKey::Num8},
      {'9', GenericKey::Num9},
      // Function Keys
      {VK_F1, GenericKey::F1},
      {VK_F2, GenericKey::F2},
      {VK_F3, GenericKey::F3},
      {VK_F4, GenericKey::F4},
      {VK_F5, GenericKey::F5},
      {VK_F6, GenericKey::F6},
      {VK_F7, GenericKey::F7},
      {VK_F8, GenericKey::F8},
      {VK_F9, GenericKey::F9},
      {VK_F10, GenericKey::F10},
      {VK_F11, GenericKey::F11},
      {VK_F12, GenericKey::F12},
      // Symbols
      {VK_OEM_3, GenericKey::Backtick},        // `~ key
      {VK_OEM_MINUS, GenericKey::Minus},       // -_ key
      {VK_OEM_PLUS, GenericKey::Equals},       // =+ key
      {VK_OEM_4, GenericKey::BracketLeft},     // [{ key
      {VK_OEM_6, GenericKey::BracketRight},    // ]} key
      {VK_OEM_5, GenericKey::Backslash},       // \| key
      {VK_OEM_1, GenericKey::Semicolon},       // ;: key
      {VK_OEM_7, GenericKey::Quote},           // '" key
      {VK_OEM_COMMA, GenericKey::Comma},       // ,< key
      {VK_OEM_PERIOD, GenericKey::Period},     // .> key
      {VK_OEM_2, GenericKey::Slash},           // /? key
      // Control Keys
      {VK_ESCAPE, GenericKey::Esc},
      {VK_TAB, GenericKey::Tab},
      {VK_CAPITAL, GenericKey::CapsLock},
      {VK_SHIFT, GenericKey::Shift},           // Covers both left and right Shift
      {VK_CONTROL, GenericKey::Ctrl},          // Covers both left and right Ctrl
      {VK_MENU, GenericKey::Alt},              // Covers both left and right Alt
      {VK_SPACE, GenericKey::Space},
      {VK_BACK, GenericKey::Backspace},
      {VK_RETURN, GenericKey::Enter},
      // Arrow Keys
      {VK_UP, GenericKey::ArrowUp},
      {VK_DOWN, GenericKey::ArrowDown},
      {VK_LEFT, GenericKey::ArrowLeft},
      {VK_RIGHT, GenericKey::ArrowRight}
   };

   HWND hwnd{};
   std::vector<RAWINPUT> rawMessages;
   // In screen coordiantes.
   XMFLOAT2A cursorPos{};
   XMFLOAT2A cursorOffset{};
   float wheelOffset{};
   bool bCursorHidden = false;

   // Weighted moving average.
   XMFLOAT2A InputFilter(const XMFLOAT2A& newValue, const XMFLOAT2A& prevValue)
   {
      XMFLOAT2A result;
      const float tau = Constants::FrameTime60FPS;
      float aplha = 1 - std::exp(GetFrameDeltaTime() / -tau);
      result.x = newValue.x * aplha + prevValue.x * (1 - aplha);
      result.y = newValue.y * aplha + prevValue.y * (1 - aplha);
      return result;
   }

   // Weighted moving average (SIMD).
   XMFLOAT4A XM_CALLCONV InputFilter(FXMVECTOR newVec, const XMFLOAT4A& prevValue)
   {
      XMFLOAT4A result;
      const float tau = Constants::FrameTime60FPS;
      float aplha = 1 - std::exp(GetFrameDeltaTime() / -tau);
      XMVECTOR prevVec = XMLoadFloat4A(&prevValue);
      XMVECTOR resVec = XMVectorAdd(XMVectorScale(newVec, aplha), XMVectorScale(prevVec, 1 - aplha));
      XMStoreFloat4A(&result, resVec);
      return result;
   }

   void UpdateCursor()
   {
      static GameClock localClock;
      RECT clientRect{};
      GetClientRect(hwnd, &clientRect);
      // Get cursor screen position.
      POINT point{};
      GetCursorPos(&point);
      XMFLOAT2A newPos{};
      newPos.x = static_cast<float>(point.x);
      newPos.y = static_cast<float>(clientRect.bottom - point.y);
      XMFLOAT2A newCursorOffset{};
      newCursorOffset.x = newPos.x - cursorPos.x;
      newCursorOffset.y = newPos.y - cursorPos.y;
      cursorPos = newPos;
      cursorOffset = InputFilter(newCursorOffset, cursorOffset);
      if (bCursorHidden == false) return;
      // Reposition the cursor.
      POINT center = { clientRect.right / 2, clientRect.bottom / 2 };
      ClientToScreen(hwnd, &center);
      SetCursorPos(center.x, center.y);
      cursorPos = { float(center.x), float(clientRect.bottom - center.y) };
      // Constrain the cursor in the client area when the form is ACTIVE, and release it otherwise.
      if (localClock.CheckSlice(Constants::FrameTime30FPS) == false) return;
      if (GetForegroundWindow() == hwnd && IsIconic(hwnd) == false)
      {
         MapWindowPoints(hwnd, nullptr, reinterpret_cast<POINT*>(&clientRect), 2);
         ClipCursor(&clientRect);
      }
      else
      {
         ClipCursor(nullptr);
      }
   }

   // Simulate the macro NEXTRAWINPUTBLOCK()
   void GetNextRawMsg(RAWINPUT** rawMsg)
   {
      uint32_t msgAlignedSize = GetAlignSize((*rawMsg)->header.dwSize, sizeof(uint64_t));
      *rawMsg = reinterpret_cast<RAWINPUT*>(reinterpret_cast<uint8_t*>(*rawMsg) + msgAlignedSize);
   }

#elif defined(__ANDROID__)
   //...
#endif

   std::u32string stringBuffer{};
   std::map<GenericKey, KeyState> genericKeyStates{};
   XMFLOAT4A gameStick{};
}

namespace Pillow::Input
{
   void InputInitialize(void* params)
   {
      stringBuffer.reserve(StringBufferMaxSize);
      rawMessages.reserve(RawInputBufferMinNum);
      for (int32_t i = 0; i < (int32_t)GenericKey::Count; i++)
      {
         genericKeyStates.emplace((GenericKey)i, KeyState::Released);
      }
#if defined(_WIN64)
      POINT point{};
      GetCursorPos(&point);
      cursorPos.x = static_cast<float>(point.x);
      cursorPos.y = static_cast<float>(point.y);
      cursorOffset = {0, 0};

      hwnd = reinterpret_cast<HWND>(params);
      const int32_t deviceNum = 1;
      RAWINPUTDEVICE devices[deviceNum]
      {
         //{HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE, 0, 0}, // Mice
         {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD, 0, 0}, // Keyboard
      };
      RegisterRawInputDevices(devices, deviceNum, sizeof(RAWINPUTDEVICE));
#elif defined(__ANDROID__)
#endif
   }

   void InputClose()
   {
#if defined(_WIN64)
      const int32_t deviceNum = 1;
      RAWINPUTDEVICE devices[deviceNum]
      {
         //{HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE, RIDEV_REMOVE, 0}, // Mice
         {HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD, RIDEV_REMOVE, 0}, // Keyboard
      };
      RegisterRawInputDevices(devices, deviceNum, sizeof(RAWINPUTDEVICE));
#elif defined(__ANDROID__)
#endif
   }

   void InputTick()
   {
      // 1. Update key states.
      for (auto& keyState : genericKeyStates)
      {
         if (keyState.second == KeyState::Up)
         {
            keyState.second = KeyState::Released;
         }
         else if (keyState.second == KeyState::Down)
         {
            keyState.second = KeyState::Pressed;
         }
      }

      // 2. Process input data.
#if defined(_WIN64)
      // 2.1 Mice
      UpdateCursor();
      wheelOffset = Hidden::WheelAccumulation;
      Hidden::WheelAccumulation = 0;
      for (int32_t i = 0; i < MiceKeyNum; i++)
      {
         uint16_t vKey = MiceKeys[i];
         GenericKey Key = MiceGKeys[i];
         bool bPressed = GetAsyncKeyState(vKey) & 0x8000;
         if (bPressed && genericKeyStates[Key] != KeyState::Pressed)
         {
            genericKeyStates[Key] = KeyState::Down;
         }
         else if (!bPressed && genericKeyStates[Key] != KeyState::Released)
         {
            genericKeyStates[Key] = KeyState::Up;
         }
      }
      // 2.2 Keyboard
      uint32_t bufferSize, msgNum;
      while (true)
      {
         bufferSize = rawMessages.capacity() * sizeof(RAWINPUT);
         msgNum = GetRawInputBuffer(rawMessages.data(), &bufferSize, sizeof(RAWINPUTHEADER));
         const uint32_t errorNum = 0xffffffff; // (uint32_t)-1
         // Enlarge the buffer if it's too small.
         if (msgNum == errorNum && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
         {
            rawMessages.reserve(rawMessages.capacity() * 2);
            continue;
         }
         // Otherwise, throw an error.
         else if(msgNum == errorNum)
         {
            throw std::runtime_error(std::format("GetRawInputBuffer failed, error code:{}", GetLastError()));
         }
         break;
      }
      RAWINPUT* rawMsg = rawMessages.data();
      for (int32_t i = 0; i < msgNum; i++, GetNextRawMsg(&rawMsg))
      {
         if (rawMsg->header.dwType == RIM_TYPEKEYBOARD)
         {
            // RAWIKEYBOARD::Flags: RI_KEY_E0 and RI_KEY_E1 indicate extended keys;
            // to simplify the code, don't distinguish them.
            uint16_t vKey = rawMsg->data.keyboard.VKey;
            uint16_t msg = rawMsg->data.keyboard.Message;
            if (KeyMap.contains(vKey) == false) continue;
            GenericKey key = KeyMap.at(vKey);
            // Add an extra condition to avoid repeating the KEYDOWN event.
            if (msg == WM_KEYDOWN && genericKeyStates[key] != KeyState::Pressed)
            {
               genericKeyStates[key] = KeyState::Down;
            }
            else if (msg == WM_KEYUP && genericKeyStates[key] != KeyState::Released)
            {
               genericKeyStates[key] = KeyState::Up;
            }
         }
      }
      // 2.3 Gamepad (XInput)
      XINPUT_STATE state{};
      XINPUT_GAMEPAD& gamepad = state.Gamepad;
      uint32_t dwResult = XInputGetState(0, &state);
      if (dwResult == ERROR_SUCCESS)
      {
         for (int32_t i = 0; i < GamepadKeyNum; i++)
         {
            uint16_t vKey = GamepadKeys[i];
            GenericKey key = GamepadGKeys[i];
            bool bPressed = (gamepad.wButtons & vKey);
            if (bPressed && genericKeyStates[key] != KeyState::Pressed)
            {
               genericKeyStates[key] = KeyState::Down;
            }
            else if (!bPressed && genericKeyStates[key] != KeyState::Released)
            {
               genericKeyStates[key] = KeyState::Up;
            }
         }
         bool bLeftTrigger = gamepad.bLeftTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
         if (bLeftTrigger && genericKeyStates[GenericKey::PadLT] != KeyState::Pressed)
         {
            genericKeyStates[GenericKey::PadLT] = KeyState::Down;
         }
         else if (!bLeftTrigger && genericKeyStates[GenericKey::PadLT] != KeyState::Released)
         {
            genericKeyStates[GenericKey::PadLT] = KeyState::Up;
         }
         bool bRightTrigger = gamepad.bRightTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
         if (bRightTrigger && genericKeyStates[GenericKey::PadRT] != KeyState::Pressed)
         {
            genericKeyStates[GenericKey::PadRT] = KeyState::Down;
         }
         else if (!bRightTrigger && genericKeyStates[GenericKey::PadRT] != KeyState::Released)
         {
            genericKeyStates[GenericKey::PadRT] = KeyState::Up;
         }
         // Dead zone.
         constexpr float maxOffset = -float(INT16_MIN);
         constexpr float deadL = float(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
         constexpr float deadR = float(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
         constexpr float denL = 1.f / (maxOffset - deadL);
         constexpr float denR = 1.f / (maxOffset - deadR);
         XMVECTOR stk0 = XMVectorSet(float(gamepad.sThumbLX), float(gamepad.sThumbLY), float(gamepad.sThumbRX), float(gamepad.sThumbRY));
         XMVECTOR stk1 = XMVectorSwizzle(stk0, 2, 3, 0, 1);
         XMVECTOR dir = XMVectorSelect(XMVector2NormalizeEst(stk0), XMVector2NormalizeEst(stk1), XMVectorSet(0, 0, 1, 1));
         XMVECTOR len = XMVectorSelect(XMVector2Length(stk0), XMVector2Length(stk1), XMVectorSet(0, 0, 1, 1));
         len = XMVectorSubtract(len, XMVectorSet(deadL, deadL, deadR, deadR));
         len = XMVectorMultiply(len, XMVectorSet(denL, denL, denR, denR));
         len = XMVectorSaturate(len);
         XMVECTOR newGameStick = XMVectorMultiply(dir, len);
         gameStick = InputFilter(newGameStick, gameStick);
      }
      else // Gamepad is not connected
      {
         for (int32_t i = 0; i < GamepadKeyNum; i++)
         {
            genericKeyStates[GamepadGKeys[i]] = KeyState::Released;
         }
         genericKeyStates[GenericKey::PadLT] = KeyState::Released;
         genericKeyStates[GenericKey::PadRT] = KeyState::Released;
         gameStick = {};
      }
#elif defined(__ANDROID__)
#endif
   }

   void SetCursorMode(bool bHidden)
   {
#if defined(_WIN64)
      bCursorHidden = bHidden;
      if (bCursorHidden)
      {
         while (ShowCursor(false) >= 0);

      }
      else
      {
         while (ShowCursor(true) < 0);
         ClipCursor(nullptr);
      }
#elif defined(__ANDROID__)
#endif
   }

   void SetVibration(uint16_t leftMotor, uint16_t rightMotor)
   {
#if defined(_WIN64)
      XINPUT_VIBRATION vibration{};
      vibration.wLeftMotorSpeed = leftMotor;
      vibration.wRightMotorSpeed = rightMotor;
      XInputSetState(0, &vibration); // Only support the first controller for now.
#endif
   }

   void SetInputMethod(bool bEnable)
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

   void AddChar32(char32_t character)
   {
      // Truncate the input buffer if too long, to avoid memory overflow.
      if (stringBuffer.size() >= StringBufferMaxSize)
      {
         stringBuffer.clear();
      }
      stringBuffer += character;
   }

   bool GetCursorMode()
   {
      return bCursorHidden;
   }

   bool GetKey(GenericKey key)
   {
      return genericKeyStates[key] == KeyState::Pressed || genericKeyStates[key] == KeyState::Down;
   }

   bool GetKeyDown(GenericKey key)
   {
      return genericKeyStates[key] == KeyState::Down;
   }

   bool GetKeyUp(GenericKey key)
   {
      return genericKeyStates[key] == KeyState::Up;
   }

   KeyState GetKeyState(GenericKey key)
   {
      return genericKeyStates[key];
   }

   XMFLOAT4A GetNormalizedSticks()
   {
      return gameStick;
   }

   std::u32string ConsumeInputString()
   {
      std::u32string str = stringBuffer;
      stringBuffer.clear();
      return str;
   }

#if defined(_WIN64)
   XMFLOAT2A GetMicePos()
   {
      return cursorPos;
   }

   XMFLOAT2A GetMiceOffset()
   {
      return cursorOffset;
   }

   float GetWheelOffset()
   {
      return wheelOffset;
   }

#elif defined(__ANDROID__)
   XMFLOAT2 GetTouchPos(uint32_t index)
   {
      return XMFLOAT2();
   }

   XMFLOAT2 GetTouchOffset(uint32_t index)
   {
      return XMFLOAT2();
   }

#endif
}
// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#include <iostream>
#include <thread>
#include <string_view>
#include "DirectXMath-apr2025/DirectXMath.h"
#include "utfcpp-4.0.6/utf8.h"
#include "Constants.h"
#include "Renderers/Renderer.h"
#include "Input.h"
#include "UI.h"
#include "Auxiliaries.h"
#if defined(_WIN64)
#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#elif defined(__ANDROID__)
#endif

extern void TempCode();

namespace Pillow::Hidden
{
   extern Pillow::GameClock GlobalClock;
   extern float WheelAccumulation;
}

// Static definitions. External code cannot access those contents.
namespace
{
   using namespace Pillow;
   using namespace Pillow::Input;
   using namespace Pillow::Graphics;
   using namespace Pillow::Constants;
   using namespace DirectX; // DXMath

   void EngineLaunch();
   void EngineTick();
   // Deconstruct the renderer; if it's nullptr, do nothing.
   void EngineTerminate();

   XMINT2 screenSize{};
   int32_t refreshRate{};

#if defined(_WIN64)
   HWND hwnd;
   uint64_t timerHandle;
   bool isFullscreen = false;
   XMINT2 screenOrigin;
   const XMINT2 minClientSize{ 400, 300 };
   XMINT2 clientSize = minClientSize;
   XMINT2 minWindowSize; // The border makes the window size bigger than the client size.

   void Disable_DPI_AutoScale();
   void CreateGameWindow(HINSTANCE hInstance, int show);
   void GameMessageLoop();
   LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
   void CALLBACK TimerEvent(HWND hwnd, UINT arg1, UINT_PTR arg2, DWORD arg3);
   void GetMonitorParams();
   void GetMinWindowSize();
   void UpdateClientSize();
   void SetWindowMode(bool fullScreen, bool allowResizing = true);

   void Disable_DPI_AutoScale()
   {
      SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
   }

   void CreateGameWindow(HINSTANCE hInstance, int nShowCmd)
   {
      GetMonitorParams();
      GetMinWindowSize();
      int32_t posX = screenOrigin.x + (screenSize.x - minWindowSize.x) / 2;
      int32_t posY = screenOrigin.y + (screenSize.y - minWindowSize.y) / 2;
      // 1 Register Window
      const wchar_t* className = L"PillowBasics";
      WNDCLASS windowSettings{};
      windowSettings.style = CS_HREDRAW | CS_VREDRAW;
      windowSettings.lpfnWndProc = WndProc;
      windowSettings.cbClsExtra = 0;
      windowSettings.cbWndExtra = 0;
      windowSettings.hInstance = hInstance;
      windowSettings.hIcon = LoadIcon(0, IDI_APPLICATION);
      windowSettings.hCursor = LoadCursor(0, IDC_ARROW);
      windowSettings.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
      windowSettings.lpszMenuName = 0;
      windowSettings.lpszClassName = className;
      if (!RegisterClass(&windowSettings))
      {
         MessageBoxA(0, "RegisterClass FAILED", 0, MB_OK);
         exit(EXIT_FAILURE);
      }
      // 2 Create and show window
      hwnd = CreateWindow(className, L"DefaultTitle", WS_OVERLAPPEDWINDOW, posX, posY, minWindowSize.x, minWindowSize.y, 0, 0, hInstance, 0);
      if (hwnd == 0)
      {
         MessageBoxA(0, "CreateWindow FAILED", 0, MB_OK);
         exit(EXIT_FAILURE);
      }
      // 3 Display Window
      ShowWindow(hwnd, nShowCmd);
      UpdateWindow(hwnd); // Update the window before initializing the game engine.
   }

   void GameMessageLoop()
   {
      try
      {
         EngineLaunch();
#ifdef PILLOW_DEBUG
         TempCode();
#endif
         // Frame-based message loop.
         MSG message{};
         while (message.message != WM_QUIT)
         {
            // 1 Process pending messages.
            while (PeekMessage(&message, 0, 0, WM_INPUT - 1, PM_REMOVE))
            {
               DispatchMessage(&message);
            }
            while (PeekMessage(&message, 0, WM_INPUT + 1, 0xFFFF, PM_REMOVE))
            {
               // Generate WM_CHAR messages.
               // (WM_INPUT = 0x00FF, WM_CHAR = 0x0102)
               TranslateMessage(&message);
               DispatchMessage(&message);
            }
            // 2 Update the engine ticks.
            EngineTick();
         }
      }
      catch (std::exception& e)
      {
         EngineTerminate();
         MessageBoxA(hwnd, e.what(), 0, MB_OK);
         exit(EXIT_FAILURE);
      }
   }

   // Recieve message （callback from system）
   LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
   {
      static std::u8string u8string;
      static std::u16string u16string;
      static std::u32string u32string;
      switch (msg)
      {
      case WM_MOUSEWHEEL:
      {
         // (scroll up = positive)
         Pillow::Hidden::WheelAccumulation = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
         break;
      }
      // Use RawInput to process keyboard input.
      case WM_KEYDOWN:
      case WM_SYSKEYDOWN:
      {
         //if (wParam == VK_F4)
         //{
         //   // Disable mouse acceleration.
         //   int32_t mouseParams[3];
         //   SystemParametersInfo(SPI_GETMOUSE, 0, mouseParams, 0);
         //   if(mouseParams[2] == 1)
         //   {
         //      mouseParams[2] = 0;
         //      SystemParametersInfo(SPI_SETMOUSE, 0, mouseParams, SPIF_SENDCHANGE);
         //   }
         //}
         break;
      }
      case WM_CHAR:
      {
         char16_t CharU16 = static_cast<char16_t>(wParam);
         u16string += CharU16;
         if (CheckUTF16_SingleUnit(CharU16) || CheckUTF16_LowSurrogate(CharU16))
         {
            utf8::utf16to8(u16string.begin(), u16string.end(), std::back_inserter(u8string));
            utf8::utf8to32(u8string.begin(), u8string.end(), std::back_inserter(u32string));
            //if (u32string.length() > 1) throw std::runtime_error("Only can one UTF-32 char one time!");
            AddChar32(u32string.at(0));
            u8string.clear();
            u16string.clear();
            u32string.clear();
         }
         break;
      }
      case WM_GETMINMAXINFO:
      {
         auto& info = *(MINMAXINFO*)lParam;
         info.ptMinTrackSize.x = minWindowSize.x;
         info.ptMinTrackSize.y = minWindowSize.y;
         break;
      }
      case WM_ENTERSIZEMOVE:
      {
         // 1. When users resize or move the form, the program will be trapped in a modal loop,
         // which will stop our message loop and causing the engine not to run.
         // So create a timer to run the engine.
         // 2. USER_TIMER_MINIMUM: Let the renderer determines the resizing-check interval,
         // which provides a much higher framerate.
         timerHandle = SetTimer(hwnd, 1, USER_TIMER_MINIMUM, TimerEvent);
         break;
      }
      case WM_EXITSIZEMOVE:
      {
         if (timerHandle != 0)
         {
            KillTimer(hwnd, timerHandle); // Stop the timer
            timerHandle = 0;
         }
         break;
      }
      case WM_DESTROY:// End message loop
      {
         PostQuitMessage(0);
         return 0;
      }
      }
      // Default procedure
      return DefWindowProc(hwnd, msg, wParam, lParam);
   }

   void TimerEvent(HWND hwnd, UINT arg1, UINT_PTR arg2, DWORD arg3)
   {
      EngineTick();
   }

   void GetMonitorParams()
   {
      HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
      MONITORINFOEX info = { sizeof(MONITORINFOEX) };
      if (GetMonitorInfo(monitor, &info) == FALSE)
      {
         MessageBoxA(0, "GetMonitorInfo FAILED", 0, MB_OK);
         exit(EXIT_FAILURE);
      }
      screenOrigin = XMINT2{ info.rcMonitor.left, info.rcMonitor.top };
      DEVMODE devMode{ 0 };
      devMode.dmSize = sizeof(DEVMODE);
      if (EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devMode) == FALSE)
      {
         MessageBoxA(0, "EnumDisplaySettings FAILED", 0, MB_OK);
         exit(EXIT_FAILURE);
      }
      screenSize = XMINT2{ int32_t(devMode.dmPelsWidth), int32_t(devMode.dmPelsHeight) };
      refreshRate = int32_t(devMode.dmDisplayFrequency);
   }

   void GetMinWindowSize()
   {
      RECT rect{ 0,0,minClientSize.x,minClientSize.y };
      AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
      minWindowSize = XMINT2{ rect.right - rect.left, rect.bottom - rect.top };
   }

  void UpdateClientSize()
  {
     RECT rect{};
     GetClientRect(hwnd, &rect);
     clientSize = XMINT2{ rect.right, rect.bottom };
  }

   void SetWindowMode(bool fullScreen, bool allowResizing)
   {
      static RECT lastRect{};
      const uint32_t flags = SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW;
      if (fullScreen && !isFullscreen) // To fullscreen
      {
         GetMonitorParams();
         GetWindowRect(hwnd, &lastRect);
         SetWindowLongPtr(hwnd, GWL_STYLE, WS_OVERLAPPED);
         SetWindowPos(hwnd, 0, screenOrigin.x, screenOrigin.y, screenSize.x, screenSize.y, flags);
      }
      else if (!fullScreen && isFullscreen) // To a window
      {
         SetWindowLongPtr(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW & (allowResizing ? UINT32_MAX : !WS_THICKFRAME));
         SetWindowPos(hwnd, 0, lastRect.left, lastRect.top, lastRect.right - lastRect.left, lastRect.bottom - lastRect.top, flags);
      }
      isFullscreen = fullScreen;
   }
#elif defined(__ANDROID__)
#endif
}

#if defined(_WIN64)
// Program Entry Point
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
   Disable_DPI_AutoScale();
   CreateGameWindow(hInstance, nShowCmd);
   GameMessageLoop();
   return EXIT_SUCCESS;
}
#elif defined(__ANDROID__)
#endif

namespace
{
   void GameTick();

   void EngineLaunch()
   {
      SetThreadNumbers();
      InputInitialize(hwnd);
#if defined(_WIN64)
      IRenderer::Initialize(Constants::ThreadNumRenderer, clientSize, refreshRate, (void*)hwnd);
#elif defined(__ANDROID__)
      //...
#endif
      IRenderer::GetInstance()->Launch();
      Hidden::GlobalClock.Restart();
      return;
   }

   void EngineTick()
   {
      InputTick();
      Hidden::GlobalClock.Tick();
      auto renderer = IRenderer::GetInstance();
      if (GetKeyDown(GenericKey::F11))
      {
         SetWindowMode(!isFullscreen);
         //// Hide the cursor.
         //if (isFullscreen)
         //{
         //   SetCursorMode(true);
         //}
         //else
         //{
         //   SetCursorMode(false);
         //}
      }
#ifdef _WIN64
      static GameClock localClock;
      // To trigger the swap-chain resizing.
      if (localClock.CheckSlice(Constants::FrameTime60FPS))
      {
         // Refresh rate is acquired only once when the game startsm because GetMonitorParams() costs a lot.
         UpdateClientSize();
         renderer->SetRenderBufferSize(clientSize);
         //renderer->SetRefreshRate(refreshRate);
      }
#endif
      TempUISubSystem_April2025::UITick();
      GameTick();
      renderer->CommitOrWait();
      //Pillow::Input::Update();
   }

   void EngineTerminate()
   {
      IRenderer::Terminate();
      InputClose();
   }

   // Game logic goes here.
   void GameTick()
   {
      auto renderer = IRenderer::GetInstance();
      // Clear the main render target.
      XMVECTOR _color = XMVectorReplicate(GetLapseTimeSinceLaunch());
      _color = XMVectorAdd(_color, XMVectorSet(0, XM_PI * 0.66f, XM_PI * 1.33f, 0));
      _color = XMVectorMultiplyAdd(XMVectorSin(_color), XMVectorReplicate(0.5f), XMVectorReplicate(0.5f));
      XMFLOAT4 color;
      XMStoreFloat4(&color, _color);
      PiplelineBuffer buffers[1] = { PiplelineBuffer::Backbuffer };
      CmdClearPiplelineBuffers(buffers, 1, color);
   }
}
#if defined(_WIN64)
#include <Windows.h>
#include <WinUser.h>
#elif defined(__ANDROID__)
#endif
#include "DirectXMath-apr2025/DirectXMath.h"
#include "Core/Renderers/Renderer.h"
#include "Core/Input.h"

using namespace Pillow;

void TempCode();

bool isFullScreen = false;
int32_t RefreshRate{};
int32_t ScreenSize[2]{};
int32_t ScreenOrigin[2]{};

#if defined(_WIN64)
void CreateGameWindow(HINSTANCE hInstance, int show);
void GameMessageLoop();
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
   static HWND Hwnd;
   const int32_t MinimumSize[2]{ 600, 400 };
}

// Program Entry Point
static int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
   CreateGameWindow(hInstance, nShowCmd);
   GameMessageLoop();
   return 0;
}

void GetMonitorParams()
{
   HMONITOR monitor = MonitorFromWindow(Hwnd, MONITOR_DEFAULTTONEAREST);
   MONITORINFOEX info = {sizeof(MONITORINFOEX)};
   if (!GetMonitorInfo(monitor, &info))
   {
      MessageBoxA(0, "GetMonitorInfo FAILED", 0, MB_OK);
      exit(EXIT_FAILURE);
   }
   ScreenOrigin[0] = info.rcMonitor.left;
   ScreenOrigin[1] = info.rcMonitor.top;
   DEVMODE devMode{0};
   devMode.dmSize = sizeof(DEVMODE);
   if (!EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devMode))
   {
      MessageBoxA(0, "EnumDisplaySettings FAILED", 0, MB_OK);
      exit(EXIT_FAILURE);
   }
   RefreshRate = devMode.dmDisplayFrequency;
   ScreenSize[0] = devMode.dmPelsWidth;
   ScreenSize[1] = devMode.dmPelsHeight;
}

void SetWindowMode(bool fullScreen, bool allowResizing = true)
{
   static uint32_t posAndSize[4]{};
   const uint32_t flags = SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW;
   if (fullScreen && !isFullScreen)
   {
      GetMonitorParams();
      RECT rect{};
      GetWindowRect(Hwnd, &rect);
      posAndSize[0] = rect.left;
      posAndSize[1] = rect.top;
      posAndSize[2] = rect.right - rect.left;
      posAndSize[3] = rect.bottom - rect.top;
      uint32_t style = WS_OVERLAPPED;
      uint32_t a = SetWindowLongPtr(Hwnd, GWL_STYLE, style);
      SetWindowPos(Hwnd, 0, ScreenOrigin[0], ScreenOrigin[1], ScreenSize[0], ScreenSize[1], flags);
   }
   else if(!fullScreen && isFullScreen)
   {
      uint32_t style = WS_OVERLAPPEDWINDOW & (allowResizing ? UINT32_MAX : !WS_THICKFRAME);
      SetWindowLongPtr(Hwnd, GWL_STYLE, style);
      SetWindowPos(Hwnd, 0, posAndSize[0], posAndSize[1], posAndSize[2], posAndSize[3], flags);
   }
   isFullScreen = fullScreen;
}

void CreateGameWindow(HINSTANCE hInstance, int nShowCmd)
{
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
   Hwnd = CreateWindow(className, L"DefaultTitle", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
      CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);
   if (Hwnd == 0)
   {
      MessageBoxA(0, "CreateWindow FAILED", 0, MB_OK);
      exit(EXIT_FAILURE);
   }
   // 3 Display Window
   ShowWindow(Hwnd, nShowCmd);
   UpdateWindow(Hwnd); // Update the window before initializing the game engine.
}


void GameMessageLoop()
{
   try
   {
      GetMonitorParams();
#ifdef PILLOW_DEBUG
      TempCode();
#endif
      MSG message{};
      while (message.message != WM_QUIT)
      {
         if (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
         {
            TranslateMessage(&message);
            DispatchMessage(&message);
         }
         else
         {
            // Game update
         }
      }
   }
   catch (std::exception& e)
   {
      MessageBoxA(Hwnd, e.what(), 0, MB_OK);
   }
}

// Recieve message （callback from system）
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   {
   case WM_INPUT:
      // Process raw input here if needed
      Pillow::Input::InputCallback((const void*)lParam);
      return 0;
      break;
   case WM_GETMINMAXINFO:
   {
      auto& info = *(MINMAXINFO*)lParam;
      info.ptMinTrackSize.x = MinimumSize[0];
      info.ptMinTrackSize.y = MinimumSize[1];
      break;
   }
   case WM_KEYDOWN:
      if (wParam == VK_F11)
      {
         SetWindowMode(!isFullScreen, true); // Toggle fullscreen mode
      }
      break;
   case WM_ENTERSIZEMOVE:
      /*
      * When users resize or move the form, the program will be trapped in a modal loop,
      * which will stop our message loop, causing the engine not to run.
      * Therefore, create a timer to run the engine.
      */
      break;
   case WM_EXITSIZEMOVE:
      break;
   case WM_DESTROY:// End message loop
      PostQuitMessage(0);
      return 0;
   }
   // Default procedure
   return DefWindowProc(hWnd, msg, wParam, lParam);
}

//#include "DirectXMath-apr2025/DirectXMath.h"
//#include "DirectXMath-apr2025/DirectXMathSSE4.h"
//#include "Core/Auxiliaries.h"
//#include "Core/Texture.h"

//#include "OpenAL-1.24.3/al.h"
//#include "OpenAL-1.24.3/alc.h"
#include "PhysX-4.1/PxPhysicsAPI.h"
#include <iostream>
using namespace physx;

// 简单的错误回调类
class SimpleErrorCallback : public PxErrorCallback {
public:
   void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) override {
      std::cerr << "PhysX Error: " << message << " in " << file << " at line " << line << std::endl;
   }
};

void TempCode()
{
   //SetWindowMode(true);
   //D3D12Renderer renderer(windowHandle, 2);
   //
   //
   //using namespace DirectX;
   //XMVECTOR v = XMVectorSet(1, 1, 1, 1);
   //XMVECTOR v2 = XMVector4Dot(v, v);
   //float result;
   //XMStoreFloat(&result, v2);
   //bool SSE4Check = SSE4::XMVerifySSE4Support();
   //LoadTexture(L"Textures\\SRGBInterpolationExample.png");
  
   //ALCdevice* device = alcOpenDevice(NULL);
   //ALCcontext* context = alcCreateContext(device, NULL);
   //alcMakeContextCurrent(context);

   //// Genrate sin wave
   //#define SAMPLE_RATE 44100
   //#define FREQUENCY 440.0f
   //#define DURATION 6.0f
   //int samples = (int)(SAMPLE_RATE * DURATION);
   //short* bufferData = (short*)malloc(samples * sizeof(short));
   //for (int i = 0; i < samples; ++i) {
   //   bufferData[i] = (short)(32760.0f * sinf(2.0f * 3.14 * FREQUENCY * i / SAMPLE_RATE));
   //}

   //// Create buffer
   //ALuint buffer;
   //alGenBuffers(1, &buffer);
   //alBufferData(buffer, AL_FORMAT_MONO16, bufferData, samples * sizeof(short), SAMPLE_RATE);

   //// Create audio source
   //ALuint source;
   //alGenSources(1, &source);
   //alSourcei(source, AL_BUFFER, buffer);

   //// Play audio
   //alSourcePlay(source);
   //printf("播放 440 Hz 正弦波...\n");

   //// wait to finish
   //ALint state;
   //do {
   //   alGetSourcei(source, AL_SOURCE_STATE, &state);
   //} while (state == AL_PLAYING);

   //// Release
   //alDeleteSources(1, &source);
   //alDeleteBuffers(1, &buffer);
   //free(bufferData);
   //alcMakeContextCurrent(NULL);
   //alcDestroyContext(context);
   //alcCloseDevice(device);

   //static PxDefaultAllocator allocator;
   //static SimpleErrorCallback errorCallback;
   //PxFoundation* foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
   //if (!foundation) {
   //   std::cerr << "Failed to create PhysX Foundation!" << std::endl;
   //   //return 1;
   //}

   //// Initialize PhysX SDK
   //PxPhysics* physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale());
   //if (!physics) {
   //   std::cerr << "Failed to create PhysX SDK!" << std::endl;
   //   foundation->release();
   //   //return 1;
   //}

   //// Create a simple scene
   //PxSceneDesc sceneDesc(physics->getTolerancesScale());
   //sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
   //PxDefaultCpuDispatcher* dispatcher = PxDefaultCpuDispatcherCreate(1);
   //sceneDesc.cpuDispatcher = dispatcher;
   //sceneDesc.filterShader = PxDefaultSimulationFilterShader;

   //PxScene* scene = physics->createScene(sceneDesc);
   //if (!scene) {
   //   std::cerr << "Failed to create PhysX Scene!" << std::endl;
   //   dispatcher->release();
   //   physics->release();
   //   foundation->release();
   //   //return 1;
   //}

   //// Output
   //std::cout << "PhysX initialized successfully! Scene created." << std::endl;

   //// Release
   //scene->release();
   //dispatcher->release();
   //physics->release();
   //foundation->release();
}
#elif defined(__ANDROID__)
#endif
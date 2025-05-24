#include <Windows.h>
#include "Core/Renderers/Renderer.h"

using namespace Pillow;

static HWND windowHandle;
void TestZone();
int GameMessageLoop();
bool CreateGameWindow(HINSTANCE hInstance, int show);
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Program Entry Point
static int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
   return CreateGameWindow(hInstance, nShowCmd) ? GameMessageLoop() : 0;
}

bool CreateGameWindow(HINSTANCE hInstance, int nShowCmd)
{
   // 1 Register Window
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
   windowSettings.lpszClassName = L"BasicWndClass";
   if (!RegisterClass(&windowSettings))
   {
      MessageBox(0, L"RegisterClass FAILED", 0, 0);
      return false;
   }

   // 2 Create and show window
   windowHandle = CreateWindow(
      L"BasicWndCLass",
      L"Win32BasicTitle",
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      0,
      0,
      hInstance,
      0);
   if (windowHandle == 0)
   {
      MessageBox(0, L"CreateWindow FAILED", 0, 0);
      return false;
   }

   // 3 Display Window
   ShowWindow(windowHandle, nShowCmd);
   UpdateWindow(windowHandle); // Update the window before initializing the game engine.
   return true;
}


int GameMessageLoop()
{
   TestZone();

   MSG msg{};
   while (msg.message != WM_QUIT)
   {
      if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
      else
      {
         // Game update
      }
   }
   return (int)msg.wParam;
}

// Recieve message （callback from system）
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   {
   case WM_LBUTTONDOWN:
      MessageBox(0, L"Hello, World", L"Hello", MB_OK);
      return 0;
   case WM_KEYDOWN:
      if (wParam == VK_ESCAPE)
      {
         DestroyWindow(windowHandle);
         return 0;
      }
      else if (wParam == VK_F11)
      {
         // Fullscreen
      }
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

void TestZone()
{
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

   static PxDefaultAllocator allocator;
   static SimpleErrorCallback errorCallback;
   PxFoundation* foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
   if (!foundation) {
      std::cerr << "Failed to create PhysX Foundation!" << std::endl;
      //return 1;
   }

   // Initialize PhysX SDK
   PxPhysics* physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale());
   if (!physics) {
      std::cerr << "Failed to create PhysX SDK!" << std::endl;
      foundation->release();
      //return 1;
   }

   // Create a simple scene
   PxSceneDesc sceneDesc(physics->getTolerancesScale());
   sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
   PxDefaultCpuDispatcher* dispatcher = PxDefaultCpuDispatcherCreate(1);
   sceneDesc.cpuDispatcher = dispatcher;
   sceneDesc.filterShader = PxDefaultSimulationFilterShader;

   PxScene* scene = physics->createScene(sceneDesc);
   if (!scene) {
      std::cerr << "Failed to create PhysX Scene!" << std::endl;
      dispatcher->release();
      physics->release();
      foundation->release();
      //return 1;
   }

   // Output
   std::cout << "PhysX initialized successfully! Scene created." << std::endl;

   // Release
   scene->release();
   dispatcher->release();
   physics->release();
   foundation->release();
}
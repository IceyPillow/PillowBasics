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
//
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
}
#include "Renderers/Renderer.h"
#include "World.h"

using namespace Pillow::Graphics;
using namespace Pillow::Common;
using namespace Pillow::World;

namespace Gameplay
{
   IRenderer* renderer;
   Camera mainCamera;

   void BaseBegin()
   {
      renderer = IRenderer::GetInstance();
      mainCamera.Position = { 3, 3, -3, 1};
      XMStoreFloat4A(&mainCamera.Quaternion, XMQuaternionRotationRollPitchYaw(XM_PIDIV4, -XM_PIDIV4, 0));
      XMINT2 screenSize = renderer->GetBackBufferSize();      
      mainCamera.Config = { XM_PIDIV4, float(screenSize.x) / float(screenSize.y), 0.1f, 100.0f };
   }

   void BaseEnd()
   {
   }

   // Game logic goes here.
   void BaseTick()
   {
      double deltaTime = GetFrameDeltaTime();
      double lapseTime = GetLapseTimeSinceLaunch();

      // ...
      XMINT2 screenSize = renderer->GetBackBufferSize();
      mainCamera.Config.AspectRatio = float(screenSize.x) / float(screenSize.y);

      // Clear the main render target.
      XMVECTOR _color = XMVectorReplicate(lapseTime);
      _color = XMVectorAdd(_color, XMVectorSet(0, XM_PI * 0.66f, XM_PI * 1.33f, 0));
      _color = XMVectorMultiplyAdd(XMVectorSin(_color), XMVectorReplicate(0.5f), XMVectorReplicate(0.5f));
      XMFLOAT4 color;
      XMStoreFloat4(&color, _color);
      PiplelineBuffer buffers[1] = { PiplelineBuffer::Backbuffer };
      CmdClearPiplelineBuffers(buffers, 1, color);
   }
}
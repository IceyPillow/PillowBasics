#include "Renderers/Renderer.h"
#include "World.h"

using namespace Pillow;
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
   }

   void BaseEnd()
   {
   }

   // Game logic goes here.
   void BaseTick()
   {
      double deltaTime = GetFrameDeltaTime();
      double lapseTime = GetLapseTimeSinceLaunch();
      float safeLapseTime = (float)std::fmod(lapseTime, Constants::TimeLapseMax);
      uint64_t frameIdx = renderer->GetFrameIndex();
      uint32_t safeFrameIdx = (uint32_t)(frameIdx % Constants::FrameIdxMax);

      // Camera.

      // Clear the main render target.
      XMVECTOR _color = XMVectorReplicate(lapseTime);
      _color = XMVectorAdd(_color, XMVectorSet(0, XM_PI * 0.66f, XM_PI * 1.33f, 0));
      _color = XMVectorMultiplyAdd(XMVectorSin(_color), XMVectorReplicate(0.5f), XMVectorReplicate(0.5f));
      XMFLOAT4 color;
      XMStoreFloat4(&color, _color);
      PipelineBuffer buffers[1] = { PipelineBuffer::BackBuffer };
      CmdClearPiplelineBuffers(buffers, 1, color);
   }
}
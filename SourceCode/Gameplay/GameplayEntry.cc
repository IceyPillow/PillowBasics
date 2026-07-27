// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#include "Renderers/Renderer.h"
#include "World.h"
#include "Resources/Mesh.h"

using namespace Pillow;
using namespace Pillow::Graphics;
using namespace Pillow::Common;
using namespace Pillow::World;

namespace Gameplay
{
   IRenderer* renderer;
   Camera mainCamera;
   std::unique_ptr<StandardMesh> Bunny;

   void BaseBegin()
   {
      renderer = IRenderer::GetInstance();
      mainCamera.Trans.Position = { 3, 3, -3, 1};
      XMStoreFloat4A(&mainCamera.Trans.Quaternion, XMQuaternionRotationRollPitchYaw(XM_PIDIV4, -XM_PIDIV4, 0));
      //Bunny = std::make_unique<StandardMesh>(GetResourcePath(L"Models\\bunny2.gltf"), VertexType::Standard);


   }

   void BaseEnd()
   {
   }

   // Game logic goes here.
   void BaseTick()
   {
      // Parameters.
      double deltaTime = GetFrameDeltaTime();
      double lapseTime = GetLapseTimeSinceLaunch();
      float safeLapseTime = (float)std::fmod(lapseTime, Constants::TimeLapseMax);
      uint64_t frameIdx = renderer->GetFrameIndex();
      uint32_t safeFrameIdx = (uint32_t)(frameIdx % Constants::FrameIdxMax);

      // Clear the main render target.
      XMVECTOR _color = XMVectorReplicate(lapseTime);
      _color = XMVectorAdd(_color, XMVectorSet(0, XM_PI * 0.66f, XM_PI * 1.33f, 0));
      _color = XMVectorMultiplyAdd(XMVectorSin(_color), XMVectorReplicate(0.5f), XMVectorReplicate(0.5f));
      XMFLOAT4 color;
      XMStoreFloat4(&color, _color);
      PipelineBuffer buffers[1] = { PipelineBuffer::BackBuffer };
      CmdClearPiplelineBuffers(buffers, 1, color);

      //Clear render targets.
      //PipelineBuffer buffers1[4] = { PipelineBuffer::Depth, PipelineBuffer::BackBuffer, PipelineBuffer::Buffer1, PipelineBuffer::Buffer2 };
      //CmdClearPiplelineBuffers(buffers1, 4, DefaultBackColor);

      // Draw a bunny.
      //PipelineBuffer buffers2[3] = { PipelineBuffer::Depth, PipelineBuffer::Buffer1, PipelineBuffer::Buffer2 };
      //CmdSetPipelineBuffers(buffers2, 3);
      //CmdSetActiveCamera();
      
      CmdDispatchMesh(0, 1);
   }
}
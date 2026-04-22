// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include <vector>
#include "DirectXMath-apr2025/DirectXMath.h"
#include "Renderers/Renderer.h"

using namespace DirectX;
using namespace Pillow::Graphics;

namespace Pillow::World
{
   using ComponentEvent = void(*)(float deltaTime);
   class IGameObject;

   // Multi-threaded manager for game objects.
   class GameObjectMTManager
   {

   };

   class IGameObject
   {
   public:
      XMFLOAT4A Position;
      XMFLOAT4A Quaternion;
      std::vector<ComponentEvent> ticks{};
   };

   class Camera : public IGameObject
   {
   public:
      struct Configuration
      {
         float VerticalFOV = XM_PIDIV4;
         float AspectRatio = 16.0f / 9.0f;
         float Width = 16.0f;
         float Height = 9.0f;
         float NearZ = 0.1f;
         float FarZ = 100.0f;
      } Config;

      void UpdateConstBuffer(uint32_t frameIdx, float deltaTime, float timeLapse, XMINT2 viewportSize);

      const CameraConstantBuffer& GetConstBuffer() const { return ConstBuffer; }

   private:
      CameraConstantBuffer ConstBuffer{};
   };
}
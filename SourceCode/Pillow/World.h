// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include <vector>
#include "DirectXMath-apr2025/DirectXMath.h"
#include "Renderers/Renderer.h"

using namespace DirectX;

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
         float VerticalFOV;
         float AspectRatio;
         float NearZ;
         float FarZ;
      } Config;

      void Write(Graphics::PassConstantBuffer& passCB);
   };
}
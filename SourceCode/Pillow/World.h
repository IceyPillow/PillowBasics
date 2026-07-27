// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include <vector>
#include "DirectXMath-jun2026/DirectXMath.h"
#include "Renderers/Renderer.h"

using namespace DirectX;
using namespace Pillow::Graphics;

namespace Pillow::World
{
   //using ComponentEvent = void(*)(float deltaTime);

   struct alignas(CacheLine) Transform
   {
      XMFLOAT4A Position;
      XMFLOAT4A Quaternion;
      XMFLOAT4A Scale;
   };

   class IGameObject
   {
   public:
      Transform Trans;

      //std::vector<ComponentEvent> ticks{};

      virtual void Tick(double deltaTime, double timeLapse) = 0;
   };

   // Multi-threaded manager for game objects.
   class GameObjectMTManager
   {

   };

   class Camera : public IGameObject
   {
   public:
      struct CameraConfig
      {
         float VerticalFOV = XM_PIDIV4;
         float Width = 16.0f;
         float Height = 9.0f;
         float NearZ = 0.1f;
         float FarZ = 100.0f;
      } Config;

      const CameraConstantBuffer& GetConstBuffer() const { return ConstBuffer; }

      void Tick(double deltaTime, double timeLapse) override final;

   private:
      CameraConstantBuffer ConstBuffer{};
   };
}
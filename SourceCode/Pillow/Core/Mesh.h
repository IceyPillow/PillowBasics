#pragma once
#include <vector>
#include "Auxiliaries.h"
#include "Constants.h"
#include "Texture.h"

using namespace Pillow::Graphics;
using namespace DirectX;

namespace Pillow::Graphics
{
   struct alignas(XMFLOAT4A) BasicVertex
   {
      XMFLOAT3 position;
      uint8_t texIdx[2];
      uint16_t padding0;
      XMFLOAT4 uv01;
   };

   // C++ inheritance makes a base type be aligned to itself,
   // which wastes a block of memory between the base type and the child type.
   struct alignas(XMFLOAT4A) StaticVertex
   {
      XMFLOAT3 position;
      uint8_t texIdx[2];
      uint16_t padding0;
      XMFLOAT4 uv01;
      XMFLOAT4 normal;
      XMFLOAT4 tangent;
   };

   struct alignas(XMFLOAT4A) SkeletalVertex
   {
      XMFLOAT3 position;
      uint8_t texIdx_boneIdx[4];
      XMFLOAT4 uv01;
      XMFLOAT4 normal_boneWeight0;
      XMFLOAT4 tangent_boneWeight1;
   };

   class BasicMesh
   {

   };

   class StaticMesh
   {

   };

   class SkeletalMesh
   {

   };

   std::unique_ptr<StaticMesh> CreateCube(float xHalf = 0.5f, float yHalf = 0.5f, float zHalf = 0.5f);
   std::unique_ptr<StaticMesh> CreateSphere(float radius = 0.5f);
}
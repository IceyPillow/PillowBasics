#pragma once
#include <vector>
#include "Auxiliaries.h"
#include "Constants.h"
#include "Texture.h"

using namespace Pillow::Graphics;
using namespace DirectX;

namespace Pillow::Graphics
{
   struct alignas(CacheLine) BasicVertex
   {
      XMFLOAT3 position;
      uint8_t texIdx[2];
      XMFLOAT4 uv01;
   };

   // C++ inheritance makes a base type be aligned to itself,
   // which wastes a block of memory between the base type and the child type.
   struct alignas(CacheLine) StaticVertex
   {
      XMFLOAT3 position;
      uint8_t texIdx[2];
      XMFLOAT4 uv01;
      XMFLOAT3 normal;
      XMFLOAT3 tangent;
   };

   struct alignas(CacheLine) SkeletalVertex
   {
      XMFLOAT3 position;
      uint8_t texIdx_boneIndx[4];
      XMFLOAT4 uv01;
      XMFLOAT3 normal_boneWeight0;
      XMFLOAT3 tangent_boneWeight1;
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
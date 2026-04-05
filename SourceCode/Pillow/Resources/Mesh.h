// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include <vector>
#include "Auxiliaries.h"
#include "Constants.h"
#include "Texture.h"

using namespace Pillow::Graphics;
using namespace DirectX;

namespace Pillow::Graphics
{
   enum class VertexType : uint8_t
   {
      Unknown,
      Basic,
      Standard
   };

   struct alignas(64) BasicVertex
   {
      XMFLOAT4A Position;
      XMUINT4 TexIdxA;
      XMUINT4 TexIdxB;
      XMFLOAT4A UV01;
   };

   struct alignas(64) StandardVertex : BasicVertex
   {
      XMFLOAT4A Normal;
      XMFLOAT4A Tangent;
      XMUINT4 BoneIdx;
      XMFLOAT4A BoneWeights;
   };

   constexpr int32_t VertexSize[2]
   {
      sizeof(BasicVertex),
      sizeof(StandardVertex),
   };

   const uint32_t VtxMemberNum[2]{ 4, 8 };

   struct alignas(16) BoneData
   {
      XMFLOAT3X4A MatrixPalette;
   };

   class BasicMesh
   {

   };

   class StandardMesh : BasicMesh
   {

   };

   class GenericMeshInfo
   {

   };

   std::unique_ptr<StandardMesh> CreateCube(float xHalf = 0.5f, float yHalf = 0.5f, float zHalf = 0.5f);
   std::unique_ptr<StandardMesh> CreateSphere(float radius = 0.5f);
}
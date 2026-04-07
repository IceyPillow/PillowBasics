// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include <vector>
#include "Auxiliaries.h"
#include "Constants.h"
#include "Texture.h"
#include "DirectXMath-apr2025/DirectXPackedVector.h"

using namespace Pillow::Graphics;
using namespace DirectX;
using namespace DirectX::PackedVector;

namespace Pillow::Graphics
{
   enum class VertexType : uint8_t
   {
      UI,
      Standard,
      Count
   };

   struct alignas(32) UIVertex
   {
      union
      {
         XMFLOAT4A Position;
         struct
         {
            XMFLOAT3 padding;
            uint32_t Unknown;
         };
      };
      uint16_t TexIdx[4];
      XMHALF4 UV01;
   };

   struct alignas(CacheLine) StandardVertex
   {
      // 16B
      union
      {
         XMFLOAT4A Position;
         struct
         {
            XMFLOAT3 padding1;
            uint16_t Bone1;
            uint16_t Bone2;
         };
      };
      // 16B
      uint16_t TexIdxA[4];
      uint16_t TexIdxB[4];
      // 16B
      XMHALF4 UV01;
      uint16_t BoneWeight_Bone3[4];
      // 16B
      XMHALF4 Normal;
      XMHALF4 Tangent;
   };

   constexpr int32_t VertexSize[uint32_t(VertexType::Count)]
   {
      sizeof(UIVertex),
      sizeof(StandardVertex),
   };

   const uint32_t VtxMemberNum[uint32_t(VertexType::Count)]
   {
      4,
      9
   };

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
// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include <vector>
#include "Auxiliaries.h"
#include "Constants.h"
#include "Texture.h"
#include "DirectXMath-apr2025/DirectXPackedVector.h"

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

   class StandardMesh
   {
   public:
      const uint32_t VtxNum;
      const VertexType VtxType;
      const bool SkeletonEnabled;

      StandardMesh(VertexType vtxType, uint32_t vtxNum, bool skeletonEnabled = false) :
         VtxType(vtxType),
         VtxNum(vtxNum),
         SkeletonEnabled(skeletonEnabled)
      {
         if (vtxNum < 1) throw std::runtime_error("Vertex number must be at least 1.");
         uint32_t num = (vtxNum * VertexSize[uint32_t(vtxType)] + 32) / sizeof(CacheLine);
         vtx = std::make_unique<CacheLine[]>(num);
      }

      UIVertex* GetUIVertices()
      {
         if (VtxType != VertexType::UI) throw std::runtime_error("Vertex type mismatch.");
         return reinterpret_cast<UIVertex*>(vtx.get());
      }

      StandardVertex* GetStandardVertices()
      {
         if (VtxType != VertexType::Standard) throw std::runtime_error("Vertex type mismatch.");
         return reinterpret_cast<StandardVertex*>(vtx.get());
      }

   private:
      std::unique_ptr<CacheLine[]> vtx;
   };

   class GenericMeshInfo
   {

   };

   std::unique_ptr<StandardMesh> CreateCube(float xHalf = 0.5f, float yHalf = 0.5f, float zHalf = 0.5f);
   std::unique_ptr<StandardMesh> CreateSphere(float radius = 0.5f);
}
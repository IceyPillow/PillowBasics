// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include <vector>
#include "Common.h"
#include "Texture.h"
#include "DirectXMath-apr2025/DirectXPackedVector.h"
#include "cgltf-1.15/cgltf.h"

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Pillow::Common;

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

   class StandardMesh
   {
   public:
      const uint32_t VtxNum;
      const uint32_t IdxNum;
      const VertexType VtxType;
      const bool SkeletonEnabled;

      StandardMesh() = delete;

      StandardMesh(const path _path, VertexType vtxType,
         uint32_t vtxNum, uint32_t idxNum, bool skeletonEnabled = false) :
         VtxNum(vtxNum),
         IdxNum(idxNum),
         VtxType(vtxType),
         SkeletonEnabled(skeletonEnabled)
      {
         if (vtxNum < 1) throw std::runtime_error("Vertex number must be at least 1.");
         uint32_t vtxBufferSize = vtxNum * VertexSize[uint32_t(vtxType)];
         uint32_t idxBufferSize = idxNum * sizeof(uint32_t);
         vtx = std::make_unique<uint8_t[]>(vtxBufferSize);
         idx = std::make_unique<uint8_t[]>(idxBufferSize);
         string dir = GetResourcePathUTF8(_path);
         // Load the model
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
      std::unique_ptr<uint8_t[]> vtx;
      std::unique_ptr<uint8_t[]> idx;
      ResHandle resVtx = ResHandleNull;
      ResHandle resIdx = ResHandleNull;
   };

   std::unique_ptr<StandardMesh> CreateCube(float xHalf = 0.5f, float yHalf = 0.5f, float zHalf = 0.5f);
   std::unique_ptr<StandardMesh> CreateSphere(float radius = 0.5f);
}
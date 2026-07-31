// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include <vector>
#include "Common.h"
#include "DirectXMath-jun2026/DirectXPackedVector.h"
#include "Renderers/Renderer.h"

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Pillow::Common;
using namespace Pillow::Graphics;

// TBN Space:
// To locate the texcoord origin at the bottom-left corner, we assume that the Z axis points inwards.

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
            XMFLOAT3 Padding;
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
            XMFLOAT3 Padding1;
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

   constexpr uint32_t VtxMemberNum[uint32_t(VertexType::Count)]
   {
      4,
      9
   };

   class StandardMesh
   {
   public:
      static inline const string DefaultQuadID = "DefaultQuad";
      static inline const string DefaultCubeID = "DefaultCube";
      static inline const string DefaultSphereID = "DefaultSphere";
      static inline const string DefaultCylinderID = "DefaultCylinder";
      static inline const string DefaultCapsuleID = "DefaultCapsule";

      static void InitializeDefaultMeshes();
      static StandardMesh* TryGetMesh(const string& id);
      static StandardMesh* CreateEmptyStaticMesh(const string& id,
         uint32_t VtxNum, uint32_t IdxNum, VertexType vtxType = VertexType::Standard);
      // Load one meshe from a .gltf file.
      // Strict Mode: One mesh in a file, no transform.
      string LoadGltfStaticStrict(const path shortPath_ID);

   public:
      ReadonlyProperty(uint32_t, VtxNum)
      ReadonlyProperty(uint32_t, IdxNum)

   public:
      const VertexType VtxType;
      const bool SkeletonEnabled;
      const path RelativePath;
      const string UniqueName;

      bool FromGltf();
      void HotReload();
      uint32_t GetPrimitiveCount() const;
      uint32_t GetPrimitiveIdxOffset(uint32_t primIdx) const;
      uint32_t GetPrimitiveIdxCount(uint32_t primIdx) const;

      // C++ sugar.
      operator UIVertex* ();
      operator StandardVertex* ();
      operator uint32_t* ();

   private:
      static inline std::unordered_map<string, StandardMesh> MeshMap{};

      std::unique_ptr<uint8_t[]> vtx;
      std::unique_ptr<uint8_t[]> idx;
      ResHandle resVtx = NullHandle;
      ResHandle resIdx = NullHandle;
      std::vector<uint32_t> primitiveEndIdxOffsets;

      StandardMesh() = delete;

      StandardMesh(const path& relativePath, const string& id, uint32_t vtxNum, uint32_t idxNum,
         VertexType vtxType = VertexType::Standard, bool bSkeleton = false);
   };
}
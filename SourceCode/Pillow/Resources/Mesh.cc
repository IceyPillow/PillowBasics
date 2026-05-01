// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
// To avoid symbol redefinition, The implementation of cgltf is put here.
#define CGLTF_IMPLEMENTATION
#include "cgltf-1.15/cgltf.h"
#undef CGLTF_IMPLEMENTATION
#include "Mesh.h"

namespace Pillow::Graphics
{
   void StandardMesh::CreateEmptyStaticMesh(string id, uint32_t VtxNum, uint32_t IdxNum, VertexType vtxType)
   {
   }

   void StandardMesh::LoadGltf(const path gltfShortPath, bool bStrict)
   {
      if (gltfShortPath.extension() != ".gltf") throw std::runtime_error("Models must be .gltf files.");
      path relativePath = GetResourceRelativePath(gltfShortPath);
      string stemName = GetU8StringfromPath(relativePath.stem());
   }

   bool StandardMesh::FromGltf()
   {
      return RelativePath != path("");
   }

   void StandardMesh::HotReload()
   {
   }

   UIVertex* StandardMesh::GetUIVtx()
   {
      if (VtxType != VertexType::UI) throw std::runtime_error("Vertex type mismatch.");
      return reinterpret_cast<UIVertex*>(vtx.get());
   }

   StandardVertex* StandardMesh::GetStandardVtx()
   {
      if (VtxType != VertexType::Standard) throw std::runtime_error("Vertex type mismatch.");
      return reinterpret_cast<StandardVertex*>(vtx.get());
   }

   uint32_t StandardMesh::GetPrimitiveCount() const
   {
      return primitiveEndIdxOffsets.size();
   }

   uint32_t StandardMesh::GetPrimitiveIdxOffset(uint32_t primIdx) const
   {
      if (primIdx == 0)
      {
         return 0;
      }
      else if(primIdx < primitiveEndIdxOffsets.size())
      {
         return primitiveEndIdxOffsets[primIdx - 1] + 1;
      }
      throw std::runtime_error("Primitive index out of range.");
      return 0;
   }

   uint32_t StandardMesh::GetPrimitiveIdxCount(uint32_t primIdx) const
   {
      if (primIdx == 0)
      {
         return  primitiveEndIdxOffsets[primIdx] + 1;
      }
      else if (primIdx < primitiveEndIdxOffsets.size())
      {
         return primitiveEndIdxOffsets[primIdx] - primitiveEndIdxOffsets[primIdx - 1];
      }
      throw std::runtime_error("Primitive index out of range.");
      return 0;
   }

   StandardMesh::StandardMesh(path relativePath, string id,
      uint32_t vtxNum, uint32_t idxNum, VertexType vtxType, bool bSkeleton) :
      VtxNum(vtxNum),
      IdxNum(idxNum),
      VtxType(vtxType),
      SkeletonEnabled(bSkeleton),
      RelativePath(relativePath),
      UniqueName(id)
   {
      if (vtxNum < 1) throw std::runtime_error("Vertex number must be at least 1.");
      if (MeshMap.contains(id)) throw std::runtime_error("Mesh ID must be unique.");
      uint32_t vtxBufferSize = vtxNum * VertexSize[uint32_t(vtxType)];
      uint32_t idxBufferSize = idxNum * sizeof(uint32_t);
      vtx = std::make_unique<uint8_t[]>(vtxBufferSize);
      idx = std::make_unique<uint8_t[]>(idxBufferSize);
   }

   std::unique_ptr<StandardMesh> CreateCube(float xHalf, float yHalf, float zHalf)
   {
      return std::unique_ptr<StandardMesh>();
   }

   std::unique_ptr<StandardMesh> CreateSphere(float radius)
   {
      return std::unique_ptr<StandardMesh>();
   }
}

// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
// To avoid symbol redefinition, The implementation of cgltf is put here.
#define CGLTF_IMPLEMENTATION
#include "cgltf-1.15/cgltf.h"
#undef CGLTF_IMPLEMENTATION
#include "Mesh.h"

namespace
{
   // A constexpr copy of DirectX::PackedVector::XMConvertFloatToHalf
   inline constexpr HALF XMF2H(float Value) noexcept
   {
      uint32_t Result;
      uint32_t IValue = std::bit_cast<uint32_t>(Value);
      uint32_t Sign = (IValue & 0x80000000U) >> 16U;
      IValue = IValue & 0x7FFFFFFFU;      // Hack off the sign
      if (IValue >= 0x47800000 /*e+16*/)
      {
         // The number is too large to be represented as a half. Return infinity or NaN
         Result = 0x7C00U | ((IValue > 0x7F800000) ? (0x200 | ((IValue >> 13U) & 0x3FFU)) : 0U);
      }
      else if (IValue <= 0x33000000U /*e-25*/)
      {
         Result = 0;
      }
      else if (IValue < 0x38800000U /*e-14*/)
      {
         // The number is too small to be represented as a normalized half.
         // Convert it to a denormalized value.
         uint32_t Shift = 125U - (IValue >> 23U);
         IValue = 0x800000U | (IValue & 0x7FFFFFU);
         Result = IValue >> (Shift + 1);
         uint32_t s = (IValue & ((1U << Shift) - 1)) != 0;
         Result += (Result | s) & ((IValue >> Shift) & 1U);
      }
      else
      {
         // Rebias the exponent to represent the value as a normalized half.
         IValue += 0xC8000000U;
         Result = ((IValue + 0x0FFFU + ((IValue >> 13U) & 1U)) >> 13U) & 0x7FFFU;
      }
      return static_cast<HALF>(Result | Sign);
   }

   inline constexpr XMHALF4 XMVF2H(float x, float y, float z, float w)
   {
      return XMHALF4{ XMF2H(x), XMF2H(y) , XMF2H(z) , XMF2H(w) };
   }
}

#include "../Python/InlineGeometry.h"

namespace Pillow::Graphics
{
   void StandardMesh::InitializeDefaultMeshes()
   {
      // 1. Quad
      StandardMesh* mesh = CreateEmptyStaticMesh(DefaultQuadID, QuadV.size(), QuadI.size());
      StandardVertex* vtxBuffer = *mesh;
      uint32_t* idxBuffer = *mesh;
      std::copy(QuadV.data(), QuadV.data() + QuadV.size(), vtxBuffer);
      std::copy(QuadI.data(), QuadI.data() + QuadI.size(), idxBuffer);
      // 2. Cube
      mesh = CreateEmptyStaticMesh(DefaultCubeID, CubeV.size(), CubeI.size());
      vtxBuffer = *mesh;
      idxBuffer = *mesh;
      std::copy(CubeV.data(), CubeV.data() + CubeV.size(), vtxBuffer);
      std::copy(CubeI.data(), CubeI.data() + CubeI.size(), idxBuffer);
      // 3. Sphere
      mesh = CreateEmptyStaticMesh(DefaultSphereID, SphereV.size(), SphereI.size());
      vtxBuffer = *mesh;
      idxBuffer = *mesh;
      std::copy(SphereV.data(), SphereV.data() + SphereV.size(), vtxBuffer);
      std::copy(SphereI.data(), SphereI.data() + SphereI.size(), idxBuffer);
      // 4. Cylinder
      mesh = CreateEmptyStaticMesh(DefaultCylinderID, CylinderV.size(), CylinderI.size());
      vtxBuffer = *mesh;
      idxBuffer = *mesh;
      std::copy(CylinderV.data(), CylinderV.data() + CylinderV.size(), vtxBuffer);
      std::copy(CylinderI.data(), CylinderI.data() + CylinderI.size(), idxBuffer);
      // 5. Capsule
      mesh = CreateEmptyStaticMesh(DefaultCapsuleID, CapsuleV.size(), CapsuleI.size());
      vtxBuffer = *mesh;
      idxBuffer = *mesh;
      std::copy(CapsuleV.data(), CapsuleV.data() + CapsuleV.size(), vtxBuffer);
      std::copy(CapsuleI.data(), CapsuleI.data() + CapsuleI.size(), idxBuffer);
   }

   StandardMesh* StandardMesh::TryGetMesh(const string& id)
   {
      auto itr = MeshMap.find(id);
      if (itr != MeshMap.end())
      {
         return &itr->second;
      }
      return nullptr;
   }

   StandardMesh* StandardMesh::CreateEmptyStaticMesh(
      const string& id, uint32_t vtxNum, uint32_t idxNum, VertexType vtxType)
   {
      StandardMesh newMesh = StandardMesh(path(""), id, vtxNum, idxNum, vtxType);
      MeshMap.emplace(id, std::move(newMesh));
      StandardMesh* result = TryGetMesh(id);
      if (result == nullptr) throw std::runtime_error("Create mesh failed.");
      return result;
   }

   string StandardMesh::LoadGltfStaticStrict(const path shortPath_ID)
   {
      if (shortPath_ID.extension() != ".gltf") throw std::runtime_error("Models must be .gltf files.");
      path modelPath = GetResourceRelativePath(shortPath_ID);
      string modelPathU8 = GetU8StringfromPath(modelPath);
      // 1 Load the model.
      cgltf_options options = { };
      cgltf_data* data = nullptr;
      cgltf_result result = cgltf_parse_file(&options, modelPathU8.c_str(), &data);
      if (result != cgltf_result_success) throw std::runtime_error("cgltf_parse_file() failed.");
      result = cgltf_load_buffers(&options, data, modelPathU8.c_str());
      if (result != cgltf_result_success) throw std::runtime_error("cgltf_load_buffers() failed.");
      // 2 Calculate counts.
      uint32_t meshCount = data->meshes_count;
      if (meshCount != 1) throw std::runtime_error("One file must contain exactly one mesh.");
      cgltf_mesh* mesh = data->meshes;
      uint32_t primNum = mesh->primitives_count;
      uint32_t vtxNum = 0, idxNum = 0;
      for (uint32_t i = 0; i < primNum; i++)
      {
         vtxNum += mesh->primitives[i].attributes[0].data->count;
         idxNum += mesh->primitives[i].indices->count;
      }
      // 3 Deserialize data.
      StandardMesh newMesh = StandardMesh(path(""), modelPathU8, vtxNum, idxNum);
      StandardVertex* vtxBuffer = newMesh;
      uint32_t* idxBuffer = newMesh;
      for (uint32_t primIdx = 0, vtxOffset = 0, idxOffset = 0; primIdx < primNum; primIdx++)
      {
         cgltf_primitive& prim = mesh->primitives[primIdx];
         uint32_t vtxCount = prim.attributes[0].data->count;
         uint32_t idxCount = prim.indices->count;
         // 3.1 Deserialize indices.
         cgltf_accessor_unpack_indices(prim.indices, &idxBuffer[idxOffset], sizeof(uint32_t), idxCount);
         for (uint32_t idxI = 0; idxI < idxCount; idxI++)
         {
            idxBuffer[idxOffset + idxI] += vtxOffset;
         }
         // 3.2 Deserialize vertex attributes.
         uint32_t attrCount = prim.attributes_count;
         for (uint32_t vIdx = 0; vIdx < vtxCount; vIdx++)
         {
            for (uint32_t attrI = 0; attrI < attrCount; attrI++)
            {
               cgltf_attribute& attr = prim.attributes[attrI];
               if (cgltf_component_size(attr.data->component_type) != sizeof(float))
                  throw std::runtime_error("Wrong element type.");
               uint32_t elementSize = sizeof(float) * cgltf_num_components(attr.data->type);
               if (attr.type == cgltf_attribute_type_position)
               {
                  cgltf_accessor_read_float(attr.data, vIdx,
                     reinterpret_cast<float*>(&(vtxBuffer[vtxOffset + vIdx].Position)), elementSize);
               }
               else if (attr.type == cgltf_attribute_type_normal)
               {
                  XMFLOAT4A temp;
                  cgltf_accessor_read_float(attr.data, vIdx,
                     reinterpret_cast<float*>(&temp), elementSize);
                  XMVECTOR _temp = XMLoadFloat4A(&temp);
                  XMStoreHalf4(&vtxBuffer[vtxOffset + vIdx].Normal, _temp);
               }
               else if (attr.type == cgltf_attribute_type_tangent)
               {
                  XMFLOAT4A temp;
                  cgltf_accessor_read_float(attr.data, vIdx,
                     reinterpret_cast<float*>(&temp), elementSize);
                  XMVECTOR _temp = XMLoadFloat4A(&temp);
                  XMStoreHalf4(&vtxBuffer[vtxOffset + vIdx].Tangent, _temp);
               }
               else if (attr.type == cgltf_attribute_type_texcoord)
               {
                  //...
               }
            }
         }
         vtxOffset += vtxCount;
         idxOffset += idxCount;
      }
      cgltf_free(data);
      // 4 Store the mesh.
      MeshMap.emplace(modelPathU8, std::move(newMesh));
      return modelPathU8;
   }

   bool StandardMesh::FromGltf()
   {
      return RelativePath != path("");
   }

   void StandardMesh::HotReload()
   {
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
      else if (primIdx < primitiveEndIdxOffsets.size())
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

   StandardMesh::operator StandardVertex* ()
   {
      if (VtxType != VertexType::Standard) throw std::runtime_error("Vertex type mismatch.");
      return reinterpret_cast<StandardVertex*>(vtx.get());
   }

   StandardMesh::operator UIVertex* ()
   {
      if (VtxType != VertexType::UI) throw std::runtime_error("Vertex type mismatch.");
      return reinterpret_cast<UIVertex*>(vtx.get());
   }

   StandardMesh::operator uint32_t* ()
   {
      return reinterpret_cast<uint32_t*>(idx.get());
   }

   StandardMesh::StandardMesh( const path& relativePath, const string& id,
      uint32_t vtxNum, uint32_t idxNum, VertexType vtxType, bool bSkeleton) :
      f_VtxNum(vtxNum),
      f_IdxNum(idxNum),
      VtxType(vtxType),
      SkeletonEnabled(bSkeleton),
      RelativePath(relativePath),
      UniqueName(id)
   {
      if (vtxNum < 1) throw std::runtime_error("Vertex number must be at least 1.");
      if (MeshMap.contains(id)) throw std::runtime_error("Mesh ID must be unique.");
      uint32_t vtxBufferSize = vtxNum * VertexSize[uint32_t(vtxType)];
      uint32_t idxBufferSize = idxNum * sizeof(uint32_t);
      // Create CPU resources.
      vtx = std::make_unique<uint8_t[]>(vtxBufferSize);
      idx = std::make_unique<uint8_t[]>(idxBufferSize);
      // Create GPU resources.
      GraphicsResourceDesc resInfo;
      resInfo.Type = GraphicsResourceType::VertexBuffer;
      resInfo.VtxIdxBuffer.VtxType = vtxType;
      resInfo.VtxIdxBuffer.Count = vtxNum;
      //IRenderer::GetInstance()->ResourceCreate()
   }
}

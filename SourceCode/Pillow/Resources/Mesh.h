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
      Static,
      Skeletal
   };

   struct BasicVertex
   {
      XMFLOAT3 Position;
      XMUINT4 TexIdx_Vertex;
      XMFLOAT4 UV01;
   };

   struct StaticVertex : BasicVertex
   {
      XMFLOAT4 Normal;
      XMFLOAT4 Tangent;
   };

   struct SkeletalVertex : StaticVertex
   {
      XMUINT4 BoneIdx;
      XMFLOAT4 BoneWeight;
   };

   constexpr int32_t VertexSize[3]
   {
      sizeof(BasicVertex),
      sizeof(StaticVertex),
      sizeof(SkeletalVertex)
   };

   constexpr int32_t VertexElementNum[3]{ 3, 5, 7 };

   struct BoneData
   {
      XMVECTOR quaternion;
      XMVECTOR translation;
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

   class GenericMeshInfo
   {

   };

   std::unique_ptr<StaticMesh> CreateCube(float xHalf = 0.5f, float yHalf = 0.5f, float zHalf = 0.5f);
   std::unique_ptr<StaticMesh> CreateSphere(float radius = 0.5f);
}
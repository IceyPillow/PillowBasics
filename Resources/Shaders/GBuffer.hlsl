// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#include "Pillow.hlsl"

#ifndef SKELETAL_VERTEX
Standard_Vertex2Pixel VertexShader(StaticVertex vertex)
#else
Standard_Vertex2Pixel VertexShader(SkeletalVertex vertex)
#endif
{
   Standard_Vertex2Pixel output;
  // Transform to homogeneous clip space.
   output.PositionW = mul(ObjectCB.MatrixModel, float4(vertex.Position, 1.0f));
   output.PositionH = mul(PassCB.MatrixViewProjection, output.PositionW);
   output.TexIdxA = vertex.TexIdxA;
   output.TexIdxB = vertex.TexIdxB;
   output.UV01 = vertex.UV01;
   output.NormalW = mul((float3x3)ObjectCB.MatrixModelInvTrans, vertex.Normal);
   
   output.TangentW.xyz = mul((float3x3)ObjectCB.MatrixModel, vertex.Tangent.xyz);
   output.TangentW.w = vertex.Tangent.w;
   output.ID_Instance = vertex.ID_Instance;
   output.ID_Vertex = vertex.ID_Vertex;
   return output;
}

[earlydepthstencil]
 CompactGBufferPixelOutput PixelShader(Standard_Vertex2Pixel pixelMetadata)
{
   CompactGBufferPixelOutput output;

   // gBuffer0 = rgb(albedo) + a(metallic)
   // gBuffer1 = rgb(normal) + a(smoothness)
   float3 texCoordinates = float3(pixelMetadata.UV01.xy, pixelMetadata.TexIdxA[1]);
   float texArrayArrayIdx = NonUniformResourceIndex(pixelMetadata.TexIdxA[0]);
   output.gBuffer0.rgb = TexArrays[texArrayArrayIdx].Sample(AniWrap, texCoordinates).rgb;

   texCoordinates.b = pixelMetadata.TexIdxA[3];
   texArrayArrayIdx = NonUniformResourceIndex(pixelMetadata.TexIdxA[2]);
   float2 metallic_smoothness = TexArrays[texArrayArrayIdx].Sample(AniWrap, texCoordinates).rg;
   output.gBuffer0.a = metallic_smoothness.r * ObjectCB.Metallic;
   output.gBuffer1.a = metallic_smoothness.g * ObjectCB.Smoothness;

   texCoordinates.b = pixelMetadata.TexIdxB[1];
   texArrayArrayIdx = NonUniformResourceIndex(pixelMetadata.TexIdxB[0]);
   float3 normalTBN = TexArrays[texArrayArrayIdx].Sample(AniWrap, texCoordinates);
   normalTBN = normalTBN * 2.0f - 1.0f;
   normalTBN.z = sqrt(saturate(1.0f - dot(normalTBN.xy, normalTBN.xy)));
   
   float3 normal = normalize(pixelMetadata.NormalW);
   float3 tangent = normalize(pixelMetadata.TangentW.xyz);
   tangent = normalize(tangent - dot(tangent, normal) * normal);
   float3 bitangent = cross(normal, tangent) * pixelMetadata.TangentW.w;
   float3x3 MatrixTBN = float3x3(tangent, bitangent, normal);
   output.gBuffer1.xyz = normalize(mul(normalTBN, MatrixTBN));

   return output;
}
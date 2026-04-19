// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#include "Pillow.hlsl"

//#define SKELETON

Standard_Vertex2Pixel VertexShader(StandardVertex vertex)

{
   Standard_Vertex2Pixel output;
   // Transform coordinates.
#ifdef SKELETON
   // Double-stage Quadratic Bezier Skinning (DQBS), an optimized version of Linear Blend Skinning (LBS).
   float w = vertex.BoneWeight_Bone3.x;
   float3 posM_1 = mul(MatrixBones[vertex.Bone1].MatrixPalette, float4(vertex.Position, 1.f));
   float3 posM_2 = mul(MatrixBones[vertex.Bone2].MatrixPalette, float4(vertex.Position, 1.f));
   float3 posM_3 = mul(MatrixBones[vertex.BoneWeight_Bone3.w].MatrixPalette, float4(vertex.Position, 1.f));
   float3 posM = w * w * posM_1 + 2 * w * (1 - w) * posM_2 + (1 - w) * (1 - w) * posM_3;
   output.PositionW = mul(ObjectCB.MatrixModel, float4(posM, 1));
   float3 normalL_1 = mul((float3x3) MatrixBones[vertex.Bone1].MatrixPalette, (float3) vertex.Normal);
   float3 normalL_2 = mul((float3x3) MatrixBones[vertex.Bone2].MatrixPalette, (float3) vertex.Normal);
   float3 normalL = Slerp(normalL_2, normalL_1, w);
   output.NormalW = mul((float3x3) ObjectCB.MatrixModelInvTrans, normalL);
   float3 tangentL_1 = mul((float3x3) MatrixBones[vertex.Bone1].MatrixPalette, (float3) vertex.Tangent);
   float3 tangentL_2 = mul((float3x3) MatrixBones[vertex.Bone2].MatrixPalette, (float3) vertex.Tangent);
   float3 tangentL = Slerp(tangentL_2, tangentL_1, w);
   output.TangentW = mul((float3x3) ObjectCB.MatrixModel, tangentL);
#else
   output.PositionW = mul(ObjectCB.MatrixModel, float4(vertex.Position, 1));
   output.NormalW = mul((float3x3) ObjectCB.MatrixModelInvTrans, (float3)vertex.Normal);
   output.TangentW = mul((float3x3) ObjectCB.MatrixModel, (float3)vertex.Tangent);
#endif
   output.PositionH = mul(PassCB.MatrixViewProjection, output.PositionW);
   output.TangentW.w = vertex.Tangent.w;
   // Transfer other data.
   output.TexIdxA = vertex.TexIdxA;
   output.TexIdxB = vertex.TexIdxB;
   output.UV01 = vertex.UV01;
   output.ID_Instance = vertex.ID_Instance;
   output.ID_Vertex = vertex.ID_Vertex;
   return output;
}

[earlydepthstencil]
 CompactGBufferPixelOutput PixelShader(Standard_Vertex2Pixel pixelMetadata)
{
   CompactGBufferPixelOutput output;
   // gBuffer0 = rgb(albedo) + a(metallic)
   // gBuffer1 = rg(normal) + b(smoothness)
   // Albedo
   float3 texCoords = float3(pixelMetadata.UV01.xy, pixelMetadata.TexIdxA.y);
   uint texArrayArrayIdx = NonUniformResourceIndex(pixelMetadata.TexIdxA.x);
   output.gBuffer0.rgb = Gallery[texArrayArrayIdx].Sample(AniWrap, texCoords).rgb;
   // Metallic + smoothness
   texCoords.z = pixelMetadata.TexIdxA.w;
   texArrayArrayIdx = NonUniformResourceIndex(pixelMetadata.TexIdxA.z);
   float2 metallic_smoothness = Gallery[texArrayArrayIdx].Sample(AniWrap, texCoords).rg;
   output.gBuffer0.a = metallic_smoothness.x * ObjectCB.Metallic;
   output.gBuffer1.b = metallic_smoothness.y * ObjectCB.Smoothness;
   // World Normal
   texCoords.z = pixelMetadata.TexIdxB.y;
   texArrayArrayIdx = NonUniformResourceIndex(pixelMetadata.TexIdxB.x);
   float3 normalTBN = UnpackNormal((float2)Gallery[texArrayArrayIdx].Sample(AniWrap, texCoords));
   float3 normal = normalize(pixelMetadata.NormalW.xyz);
   float3 tangent = normalize(pixelMetadata.TangentW.xyz);
   tangent = normalize(tangent - dot(tangent, normal) * normal);
   float3 bitangent = cross(normal, tangent) * pixelMetadata.TangentW.w;
   float3x3 MatrixTBN = float3x3(tangent, bitangent, normal);
   float3 normalW = normalize(mul(MatrixTBN, normalTBN));
   output.gBuffer1.rg = EncodeOctahedron(normalW);
   return output;
}
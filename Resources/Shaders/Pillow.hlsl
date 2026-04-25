// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.

// Common HLSL header, all shaders should include it.
// To avoid importing unwanted shader entries, this header will not include any shader entries.
// If you plan to use standard shader entries, please include other independent headers.
//
// Annotations:
// #include search paths: The current folder of a file, the root shader folder.
// Shader Model = 6.4
// mtx = matrix

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////128SLASHES
// Static constants

static const float PI = 3.141592654f;
static const float PI2 = 6.283185307f;
static const float PI_1DIVPI = 0.318309886f;
static const float PI_1DIV2PI = 0.159154943f;
static const float PI_DIV2 = 1.570796327f;
static const float PI_DIV4 = 0.785398163f;

static const float TimeLapseMax = 3600 * PI2; // Seconds
static const uint FrameIdxMax = 1000000;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////128SLASHES
// Bound resources

struct ObjectStruct
{
   float3x4 MatrixModel;
   float3x3 MatrixModelInvTrans;
   float4 ColorObject;
   uint4 TexIdx;
   // 16B
   uint SamplerIdx;
   uint InstanceNum;
   float Metallic;
   float Smoothness;
};

struct LightStruct
{
   // 16B
   float3 PositionWorld;
   uint TypeLight;
   // 16B
   float3 DirectionWorld;
   float Intensity;
   // 16B
   float3 ColorLight;
   float RangeMax;
   // 16B
   float4 ShapeLight;
};

struct CameraStruct
{
   float3x4 MatrixView;
   float4x4 MatrixProjection;
   float4x4 MatrixViewProjection;
   float4x4 MatrixViewProjectionInv;
   float4 ViewportSizeAndRecip;
   // 16B
   float3 CameraPositionWorld;
   uint FrameArrayIdx;
   // 16B
   float DistanceNear;
   float DistanceFar;
   float TimeDelta;
   float TimeLapse;
};

struct BoneData
{
   float3x4 MatrixPalette;
};

// Static samplers.
// Use the uniform branches to select them in shaders (see the func SampleTexSmart).
SamplerState PointClamp : register(s0);
SamplerState PointWrap : register(s1);
SamplerState TriClamp : register(s2);
SamplerState TriWrap : register(s3);
SamplerState AniClamp : register(s4);
SamplerState AniWrap : register(s5);
SamplerComparisonState Cmp : register(s6);

// Bound resources.
ConstantBuffer<ObjectStruct> ObjectCB : register(b0, space0);
ConstantBuffer<LightStruct> LightCB : register(b1, space0);
ConstantBuffer<CameraStruct> CameraCB : register(b2, space0);
StructuredBuffer<BoneData> MatrixBones : register(t0, space0);
TextureCubeArray CubeGallery : register(t1, space0);
Texture2DArray Gallery[] : register(t0, space1);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////128SLASHES
// Unified root signature

#define CSU_DESC_HEAP_SIZE 65535
#define MIPS_MAX 16.f

// Value to string literal
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define TEX_CLAMP(mipMax) \
   "addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, " \
   "mipLODBias = 0, maxAnisotropy = 4, borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK, " \
   "minLOD = 0, maxLOD = " STR(mipMax) ", space = 0, visibility = SHADER_VISIBILITY_ALL"

#define TEX_WRAP(mipMax) \
   "addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, " \
   "mipLODBias = 0, maxAnisotropy = 4, borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK, " \
   "minLOD = 0, maxLOD = " STR(mipMax) ", space = 0, visibility = SHADER_VISIBILITY_ALL"

#define CMP_NV "comparisonFunc = COMPARISON_NEVER"

#define CMP_GE "comparisonFunc = COMPARISON_GREATER_EQUAL"

#define ROOT_SIGN \
   "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
   "CBV(b0, space = 0), " \
   "CBV(b1, space = 0), " \
   "CBV(b2, space = 0), " \
   "SRV(t0, space = 0), " \
   "DescriptorTable(SRV(t1, space = 0, numDescriptors = 1)), " \
   "DescriptorTable(SRV(t0, space = 1, numDescriptors = " STR(CSU_DESC_HEAP_SIZE) ")), " \
   "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT, " TEX_CLAMP(0) ", " CMP_NV "), " \
   "StaticSampler(s1, filter = FILTER_MIN_MAG_MIP_POINT, " TEX_WRAP(0) ", " CMP_NV "), " \
   "StaticSampler(s2, filter = FILTER_MIN_MAG_MIP_LINEAR, " TEX_CLAMP(MIPS_MAX) ", " CMP_NV "), " \
   "StaticSampler(s3, filter = FILTER_MIN_MAG_MIP_LINEAR, " TEX_WRAP(MIPS_MAX) ", " CMP_NV "), " \
   "StaticSampler(s4, filter = FILTER_ANISOTROPIC, " TEX_CLAMP(MIPS_MAX) ", " CMP_NV "), " \
   "StaticSampler(s5, filter = FILTER_ANISOTROPIC, " TEX_WRAP(MIPS_MAX) ", " CMP_NV "), " \
   "StaticSampler(s6, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, " TEX_WRAP(0) ", " CMP_GE ")"

//cmd line: dxc -T rootsig_1_0 -E ROOT_SIGN_DUMMY -Fo PillowRS.cso -Fe errors.log ./Pillow.hlsl
#define ROOT_SIGN_DUMMY "RootFlags(0), " \
   "CBV(b0), " \
   "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT, " TEX_WRAP(0) ", " CMP_NV ")"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////120-Slashes
// Input layouts

struct UIVertex
{
   float3 Position : POSITION;
   uint Unknown : SCALAR0;
   uint4 TexIdx : VECTOR0;
   float4 UV01 : VECTOR1;
   // System values
   uint ID_Instance : SV_InstanceID;
   uint ID_Vertex : SV_VertexID;
};

struct StandardVertex
{
   float3 Position : POSITION;
   uint Bone1 : SCALAR0;
   uint Bone2 : SCALAR1;
   uint4 TexIdxA : VECTOR0;
   uint4 TexIdxB : VECTOR1;
   float4 UV01 : VECTOR2;
   uint4 BoneWeight_Bone3 : VECTOR3;
   float4 Normal : VECTOR4;
   float4 Tangent : VECTOR5;
   // System values
   uint ID_Instance : SV_InstanceID;
   uint ID_Vertex : SV_VertexID;
};

struct Basic_Vertex2Pixel
{
   float4 PositionH : SV_POSITION;
   float4 PositionW : POSITION;
   nointerpolation uint4 TexIdxA : VECTOR0;
   nointerpolation uint4 TexIdxB : VECTOR1;
   float4 UV01 : VECTOR2;
   // Pass system values to the pixel shader
   nointerpolation uint ID_Instance : SCALAR0;
   nointerpolation uint ID_Vertex : SCALAR1;
};

struct Standard_Vertex2Pixel
{
   float4 PositionH : SV_POSITION;
   float4 PositionW : POSITION;
   nointerpolation uint4 TexIdxA : VECTOR0;
   nointerpolation uint4 TexIdxB : VECTOR1;
   float4 UV01 : VECTOR2;
   float4 NormalW : VECTOR3;
   float4 TangentW : VECTOR4;
   // Pass system values to the pixel shader
   nointerpolation uint ID_Instance : SCALAR0;
   nointerpolation uint ID_Vertex : SCALAR1;
};

struct SinglePixelOutput
{
   float4 output : SV_Target;
};

struct CompactGBufferPixelOutput
{
   float4 gBuffer0 : SV_Target1; // rgb(albedo) + a(metallic)
   float4 gBuffer1 : SV_Target2; // rg(normal) + b(smoothness)
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////120-Slashes
// Functions

float SampleDepth(float2 uv, SamplerState _sampler)
{
   const uint TexIdx_Depth = 1;
   return Gallery[TexIdx_Depth].Sample(_sampler, float3(uv, 0)).r;
}

float2 SampleMotionVector(float2 uv, SamplerState _sampler)
{
   const uint TexIdx_MotionVector = 2;
   return Gallery[TexIdx_MotionVector].Sample(_sampler, float3(uv, 0)).rg;
}

// General buffer
float4 SampleGBuffer(uint bufferIdx, float2 uv, SamplerState _sampler)
{
   const uint TexIdx_GeneralBuffer = 3;
   return Gallery[TexIdx_GeneralBuffer].Sample(_sampler, float3(uv, bufferIdx), 0);
}
// Half-size general buffer
float4 SampleHalfGBuffer(uint bufferIdx, float2 uv, SamplerState _sampler)
{
   const uint TexIdx_GeneralBuffer = 3;
   return Gallery[TexIdx_GeneralBuffer].Sample(_sampler, float3(uv, bufferIdx), 1);
}

float4 SampleBackBuffer(uint frameArrayIdx, float2 uv, SamplerState _sampler)
{
   static const uint TexIdx_BackBufferOffset = 4;
   return Gallery[TexIdx_BackBufferOffset + frameArrayIdx].Sample(_sampler, float3(uv, 0));
}

// samplerIdx can be set dynamically.
float4 SampleTexSmart(Texture2DArray tex, float3 uvi, uint samplerIdx)
{
   [branch] // Uniform branches, the instruction flow is consistent in a warp (32Threads).
   if (samplerIdx == 0)
   {
      return tex.Sample(PointClamp, uvi);
   }
   else if (samplerIdx == 1)
   {
      return tex.Sample(PointWrap, uvi);
   }
   else if (samplerIdx == 2)
   {
      return tex.Sample(TriClamp, uvi);
   }
   else if (samplerIdx == 3)
   {
      return tex.Sample(TriWrap, uvi);
   }
   else if (samplerIdx == 4)
   {
      return tex.Sample(AniClamp, uvi);
   }
   else if (samplerIdx >= 5)
   {
      return tex.Sample(AniWrap, uvi);
   }
   return (float4)0;
}

float4 EncodeSRGB(float4 color)
{
   float4 result;
   result.rgb = 1.055 * pow(color.rgb, 1.0 / 2.4) - 0.055;
   result.a = color.a;
   return result;
}

float3 GetWorldNormal(float3 value, float3 nW, float3 tW)
{
   nW = normalize(nW);
   tW = normalize(tW - nW * dot(nW, tW));
   float3x3 T2W = float3x3(tW, nW, cross(tW, nW));
   value = 2 * value - 1;
   value = normalize(mul(value, T2W));
   return value;
}

// A safe version of spherical linear interpolation (slerp).
float3 Slerp(float3 xNormalized, float3 yNormalized, float t)
{
   // lerp
   float3 lerp = lerp(xNormalized, yNormalized, t);
   // slerp
   float dot = dot(xNormalized, yNormalized);
   float theta = acos(clamp(dot, -1.f, 1.f));
   float t_theta = t * theta;
   float3 sphereLerp = sin(theta - t_theta) * xNormalized;
   sphereLerp += sin(t_theta) * yNormalized;
   sphereLerp /= sin(theta);
   // select one safely.
   float3 result = (abs(dot) < 0.99f) ? sphereLerp : lerp;
   result = normalize(result);
   return result;
}

float3 UnpackNormal(float2 rg)
{
   float3 normal;
   normal.xy = rg * 2.0f - 1.0f;
   normal.z = sqrt(saturate(1.0f - dot(normal.xy, normal.xy)));
   return normal;
}


float2 EncodeOctahedron(float3 nNormalized)
{
   float3 n = nNormalized /= abs(nNormalized.x) + abs(nNormalized.y) + abs(nNormalized.z);
   // Symmetry axis: y = 1-x.
   float2 folded = (1.0f - abs(n.yx)) * (n.xy >= 0.0f ? 1.0f : -1.0f);
   float2 uv = (n.z >= 0.0f) ? n.xy : folded;
   uv = uv * 0.5f + 0.5f;
   return uv;
}

float3 DecodeOctahedron(float2 uv)
{
   uv = uv * 2.0f - 1.0f;
   float3 n = float3(uv.xy, 1.0f - abs(uv.x) - abs(uv.y));
   float t = saturate(-n.z);
   n.xy += (n.xy >= 0.0f ? -t : t);
   float3 nNormalized = normalize(n);
   return nNormalized;
}
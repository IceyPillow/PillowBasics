// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include "Common.h"
#include "DirectXMath-jun2026/DirectXMath.h"

using namespace Pillow::Common;

namespace Pillow::Graphics
{
   using namespace DirectX;

   // Texture (pixel) format.
   enum class TextureFormat : uint8_t
   {
      // 1.Supports .hdr files.
      // 2.R8G8B8 isn't supported in DXGI_FORMAT, use R8G8B8A8 to store it.
      UnsignedNormalized_R8,
      UnsignedNormalized_R8G8,
      UnsignedNormalized_R8G8B8,
      UnsignedNormalized_R8G8B8A8,
      Float_R16,
      Float_R16G16,
      Float_R16G16B16A16,
      Count
   };

   //                     Subresource Indexing                       //
   //                                         ______________________ //
   // subres(0) subres(3) -> Row: Mip Slice 0 |subres(6) subres(9) | //
   // subres(1) subres(4)                     |subres(7) subres(10)| //
   // subres(2) subres(5)                     |subres(8) subres(11)| //
   //     V                                   ---------------------- //
   // Col: Array Slice 0                      ^^^ Plane Slice 1 ^^^  //
   //                                                                //
   // Pseudo Code: Subres Res[PlaneSlice][ArraySlice][MipSlice];     //

   // Generic texture info, to a raw (unaligned) texture in the system memory.
   class TextureInfo
   {
   public:
      // Texture usage.
      // It's also a hint to the GPU resident management.
      enum class Tag : uint8_t
      {
         GameUI,
         Character,
         Prop,
         Artifact,
         Nature,
         Development // Screen captures, debug UI, profile UI, etc.
      };

      // Resource type.
      enum class Type : uint8_t
      {
         Tex,
         TexArray, // Include cube maps.
         BakedTex,
         BakedTexArray,
      };

      // Texture compression type.
      enum class ZipType : uint8_t
      {
         None,
         Hardware,
         HardwareWithDithering
      };

   public:
      static const int32_t MaxWidth = 1 << 13;
      static const int32_t MaxArraySize = UINT8_MAX;

      static TextureInfo DefineTexture(Tag tag, TextureFormat format, ZipType zip, int32_t width, int32_t height, bool useMip);
      static TextureInfo DefineTextureArray(Tag tag, TextureFormat format, ZipType zip, int32_t width, int32_t height, int32_t count, bool useMip);
      static TextureInfo DefineTextureArray(const TextureInfo& info, int32_t count);
      // Baked textures cannot have mipmaps. They are designed for screen captures, not for pragmatic textures.
      // Pillow Basics doesn't support GPU mipmap generation currently.
      static TextureInfo DefineBakedTexture(TextureFormat format, int32_t width, int32_t height);
      // Baked textures cannot have mipmaps. They are designed for screen captures, not for pragmatic textures.
      // Pillow Basics doesn't support GPU mipmap generation currently.
      static TextureInfo DefineBakedTextureArray(TextureFormat format, int32_t width, int32_t height, int32_t count);

   public:
      // Format
      const Tag TexTag;
      const Type TexType;
      const TextureFormat TexFormat;
      const ZipType CompressionType;
      const uint16_t Width;
      const uint16_t Height;
      const uint8_t MipCount;
      const uint8_t ArrayCount;
      // Size
      const uint32_t ArraySliceSize;
      const uint32_t TotalSize;
      // Resource Tracking
      std::vector<uint32_t> ArrayTracking;

   public:
      TextureInfo() = delete;
      // TextureInfo::Tag (texture usage) is not considered.
      bool operator==(const TextureInfo& right) const;
      // The highest mipmap level is 0.
      int32_t GetMipmapSize(uint32_t mipLevel) const;

   private:
      TextureInfo(Tag tag, Type type, TextureFormat format, ZipType zip, int32_t w, int32_t h, int32_t mips, int32_t arrayNum);
   };

   // Enquire the original pixel size.
   int32_t GetPixelSize(TextureFormat format); 
   // Enquire the original pixel size.
   int32_t GetPixelSize(const TextureInfo& info);
   void LoadTexture(const string& relativePath);

   ForceInline void ColorFloat2Byte(uint8_t& destination, float color)
   {
      destination = uint8_t(color * float(UINT8_MAX) + 0.5f);
   }

   ForceInline void XM_CALLCONV ColorFloat2Byte(uint8_t* destination, FXMVECTOR color)
   {
      XMVECTOR result = XMVectorMultiplyAdd(color, XMVectorReplicate(UINT8_MAX), XMVectorReplicate(0.5f));
      XMFLOAT4A _result;
      XMStoreFloat4A(&_result, result);
      destination[0] = uint8_t(_result.x);
      destination[1] = uint8_t(_result.y);
      destination[2] = uint8_t(_result.z);
      destination[3] = uint8_t(_result.w);
   }

   ForceInline void ColorByte2Float(float& destination, uint8_t color)
   {
      constexpr float factor = 1 / float(UINT8_MAX);
      destination = float(color) * factor;
   }

   ForceInline void XM_CALLCONV ColorByte2Float(XMVECTOR& destination, const uint8_t* color)
   {
      XMVECTOR v1 = XMVectorSet(color[0], color[1], color[2], color[3]);
      const XMVECTOR v2 = XMVectorReplicate(1 / float(UINT8_MAX));
      destination = XMVectorMultiply(v1, v2);
   }

   ForceInline XMVECTOR XM_CALLCONV DecodeRGB565(const uint16_t color)
   {
      XMVECTOR result = XMVectorSet((color >> 11) & 31, (color >> 5) & 63, (color >> 0) & 31, 0);
      const XMVECTOR factor = XMVectorSet(1 / 31.f, 1 / 63.f, 1 / 31.f, 0);
      return XMVectorMultiply(result, factor);
   }

   ForceInline uint16_t XM_CALLCONV EncodeRGB565(FXMVECTOR color)
   {
      XMVECTOR temp = XMVectorClamp(color, XMVectorZero(), XMVectorReplicate(1));
      temp = XMVectorMultiplyAdd(temp, XMVectorSet(31.f, 63.f, 31.f, 0), XMVectorReplicate(0.5f));
      XMFLOAT4A _temp;
      XMStoreFloat4A(&_temp, temp);
      return uint16_t(int32_t(_temp.x) << 11 | int32_t(_temp.y) << 5 | int32_t(_temp.z));
   }
}
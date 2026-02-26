// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 2-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include "Core/Auxiliaries.h"
#include "DirectXMath-apr2025/DirectXMath.h"

namespace Pillow::Graphics
{
   using namespace DirectX;

   //                     Subresource Indexing                       //
   //                                         ______________________ //
   // subres(0) subres(3) -> Row: Mip Slice 0 |subres(6) subres(9) | //
   // subres(1) subres(4)                     |subres(7) subres(10)| //
   // subres(2) subres(5)                     |subres(8) subres(11)| //
   //     V                                   ---------------------- //
   // Col: Array Slice 0                      ^^^ Plane Slice 1 ^^^  //
   //                                                                //
   // Pseudo Code: Subres Res[PlaneSlice][ArraySlice][MipSlice];     //

   // Generic texture info to the raw (unaligned) texture the in system memory.
   class TextureInfo
   {
   public:
      // Resource type.
      enum class Type : uint8_t
      {
         Texture,
         TextureAray,
         Cubemap,
         RenderTexture,
      };

      // Texture compression type.
      enum class ZipType : uint8_t
      {
         None,
         Hardware,
         HardwareWithDithering
      };

      // Texture (pixel) format.
      enum class Format : uint8_t
      {
         // 1.Supports .hdr files.
         // 2.R8G8B8 isn't supported in DXGI_FORMAT, use R8G8B8A8 to store it.
         UnsignedNormalized_R8,
         UnsignedNormalized_R8G8,
         UnsignedNormalized_R8G8B8,
         UnsignedNormalized_R8G8B8A8,
         Count
      };

      const uint8_t PixelBytes[int32_t(Format::Count)]
      {
         1, // UnsignedNormalized_R8
         2, // UnsignedNormalized_R8G8
         3, // UnsignedNormalized_R8G8B8
         4, // UnsignedNormalized_R8G8B8A8
      };

   public:
      // Format
      Format TexFormat;
      ZipType CompressionType;
      uint16_t Width;
      uint16_t Height;
      uint8_t ArraySize;
      uint8_t MipCount;
      bool IsCubemap;
      bool IsRenderTexture;
      // Size
      uint16_t ArraySliceSize;
      uint16_t TotalSize;

      static const int32_t MaxArraySize = UINT8_MAX;
      static const int32_t MaxMipCount = UINT8_MAX;

      GenericTextureInfo() = default;
      GenericTextureInfo(const GenericTextureInfo&) = default;
      GenericTextureInfo(GenericTexFmt format, int32_t width,  bool bMips = true, CompressionMode compMode = CompressionMode::HardwareWithDithering, bool bCube = false, int32_t arraySize = 1);
   };
      static TextureInfo CreateTexture();

      static TextureInfo CreateTextureArray();

      static TextureInfo CreateCubemap();

      static TextureInfo CreateRenderTexture();

   };

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
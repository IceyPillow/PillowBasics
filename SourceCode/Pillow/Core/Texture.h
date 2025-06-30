#pragma once
#include <cstdint>
#include <cmath>
#include "Auxiliaries.h"

namespace Pillow::Graphics
{
   enum class CompressionMode : uint8_t
   {
      None,
      Hardware,
      HardwareWithDithering
   };

   enum class GenericTexFmt : uint8_t
   {
      // 1.Supports .hdr files.
      // 2.R8G8B8 isn't supported in DXGI_FORMAT, use R8G8B8A8 to store it.
      UnsignedNormalized_R8G8B8A8,
      UnsignedNormalized_R8G8,
      UnsignedNormalized_R8,
      Count
   };

   const int32_t PixelSize[int32_t(GenericTexFmt::Count)]
   {
      4, // UnsignedNormalized_R8G8B8A8
      2, // UnsignedNormalized_R8G8
      1, // UnsignedNormalized_R8
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
   class GenericTextureInfo
   {
      // Format
      ReadonlyProperty(GenericTexFmt, Format)
         ReadonlyProperty(uint8_t, PixelSize)
         ReadonlyProperty(uint16_t, Width)
         ReadonlyProperty(uint8_t, MipCount)
         ReadonlyProperty(uint8_t, ArrayCount)
         ReadonlyProperty(bool, IsCubemap)
         ReadonlyProperty(CompressionMode, CompressionMode)
         // Size
         ReadonlyProperty(uint16_t, MipZeroSize)
         ReadonlyProperty(uint16_t, ArraySliceSize)
         ReadonlyProperty(uint16_t, TotalSize)

   public:
      static const int32_t MaxArraySize = UINT8_MAX;
      static const int32_t MaxMipCount = UINT8_MAX;

      GenericTextureInfo() = default;
      GenericTextureInfo(const GenericTextureInfo&) = default;
      GenericTextureInfo(GenericTexFmt format, int32_t width,  bool bMips = true, CompressionMode compMode = CompressionMode::HardwareWithDithering, bool bCube = false, int32_t arraySize = 1);
   };

   class GenericTexture
   {
   public:
      const GenericTextureInfo Info;
   };

   void LoadTexture(const std::wstring& relativePath);

   ForceInline uint8_t ColorFloat2UInt8(float c)
   {
      return uint8_t(c * float(UINT8_MAX) + 0.5f);
   }

   ForceInline float ColorUInt82Float(uint8_t c)
   {
      return float(c) / float(UINT8_MAX);
   }
}
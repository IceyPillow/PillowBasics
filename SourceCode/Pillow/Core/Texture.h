#pragma once
#include <cstdint>
#include <cmath>
#include "Auxiliaries.h"

namespace Pillow::Graphics
{
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
   // Array Slice 0                           ^^^ Plane Slice 1 ^^^  //
   //                                                                //
   // Pseudo Code: Subres Res[PlaneSlice][ArraySlice][MipSlice];     //
   class GenericTexInfo
   {
      // Format
      ReadonlyProperty(GenericTexFmt, Format)
         ReadonlyProperty(uint8_t, PixelSize)
         ReadonlyProperty(uint16_t, Width)
         ReadonlyProperty(uint8_t, MipCount)
         ReadonlyProperty(uint8_t, ArrayCount)
         ReadonlyProperty(bool, IsCubemap)
         ReadonlyProperty(bool, UseCompression)
         // Size
         ReadonlyProperty(uint16_t, MipZeroSize)
         ReadonlyProperty(uint16_t, ArraySliceSize)
         ReadonlyProperty(uint16_t, TotalSize)

   public:
      GenericTexInfo() = default;
      GenericTexInfo(const GenericTexInfo&) = default;
      GenericTexInfo(GenericTexFmt format, int32_t width,  bool bMips = true, bool bCompression = true, bool bCube = false, int32_t arraySize = 1);
   };

   class GenericTexture
   {
   public:
      const GenericTexInfo Info;
   };

   void LoadTexture(const std::wstring& relativePath);
}
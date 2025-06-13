#pragma once
#include <cstdint>
#include <cmath>
#include "Auxiliaries.h"

namespace Pillow::Graphics
{
   enum class GenericTextureFormat : int8_t
   {
      // 1.Supports .hdr files.
      // 2.R8G8B8 isn't supported in DXGI_FORMAT, use R8G8B8A8 to store it.
      UnsignedNormalized_R8G8B8A8,
      UnsignedNormalized_R8G8,
      UnsignedNormalized_R8,
      Count
   };

   const int32_t PixelSize[(int32_t)GenericTextureFormat::Count]
   {
      4, // UnsignedNormalized_R8G8B8A8
      2, // UnsignedNormalized_R8G8
      1, // UnsignedNormalized_R8
   };

   class GenericTextureInfo
   {
      ReadonlyProperty(int32_t, Width)
         ReadonlyProperty(int32_t, PixelSize)
         ReadonlyProperty(int32_t, MipSliceCount)
         ReadonlyProperty(int32_t, ArraySliceCount)
         ReadonlyProperty(int32_t, Mip0Size)
         ReadonlyProperty(int32_t, MipSliceSize)
         ReadonlyProperty(int32_t, TotalSize)
         ReadonlyProperty(GenericTextureFormat, Format)
         ReadonlyProperty(bool, IsCubemap)
         ReadonlyProperty(bool, useCompression)

   public:
      GenericTextureInfo() = default;
      GenericTextureInfo(const GenericTextureInfo&) = default;
      GenericTextureInfo(int32_t width, GenericTextureFormat format, bool hasMips = true, bool isCube = false, bool useCompression = true, int32_t arraySize = 1);
   };

   class GenericTexture
   {
   public:
      const GenericTextureInfo Info;
   };

   void LoadTexture(const std::wstring& relativePath);
}
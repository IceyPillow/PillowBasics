#pragma once
#include <cstdint>
#include <cmath>
#include "Auxiliaries.h"

namespace Pillow::Graphics
{
   enum class GenericTextureFormat : int8_t
   {
      UNORM_R8G8B8A8,
      UNORM_R8G8B8,
      UNORM_R8G8,
      UNORM_R8,
      Count
   };

   const int32_t PixelSize[(int32_t)GenericTextureFormat::Count]
   {
      4, // UNORM_R8G8B8A8
      3, // UNORM_R8G8B8
      2, // UNORM_R8G8
      1, // UNORM_R8
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
      GenericTextureInfo(int width, GenericTextureFormat format, bool hasMips = true, bool isCube = false, int arraySize = 1);
   };

   class GenericTexture
   {
   public:
      const GenericTextureInfo Info;
   };

   void LoadTexture(const std::wstring& relativePath);
}
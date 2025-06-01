#pragma once
#include <cstdint>
#include <cmath>
#include "Auxiliaries.h"

namespace Pillow::Graphics
{
   enum class GenericTextureFormat : int8_t
   {
      RGB10A2_UNORM,
      RGBA8_UNORM,
      R8_UNORM,
      R32_FLOAT,
      Count
   };

   const int32_t PixelSize[(int32_t)GenericTextureFormat::Count]
   {
      4, // RGB10A2_UNORM
      4, // RGBA8_UNORM
      1, // R8_UNORM
      4, // R32_FLOAT
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
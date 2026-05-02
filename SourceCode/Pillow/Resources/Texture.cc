// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#include "Texture.h"
#include <bit>
#include "fstream"
#include "filesystem"
#include "lodepng-apr2025/lodepng.h"

using namespace Pillow;
using namespace Pillow::Common;
using namespace Pillow::Graphics;
using namespace DirectX;

namespace
{
   const uint8_t PixelBytes[int32_t(TextureFormat::Count)]
   {
      1, // UnsignedNormalized_R8
      2, // UnsignedNormalized_R8G8
      3, // UnsignedNormalized_R8G8B8
      4, // UnsignedNormalized_R8G8B8A8
   };

   void CheckTextureSize(uint32_t w, uint32_t h, uint32_t count, bool bPowerOf2)
   {
      if (w > TextureInfo::MaxWidth || h > TextureInfo::MaxWidth)
         throw std::runtime_error(std::format("Texture exceeds the maxmium edge size: {}", TextureInfo::MaxWidth));
      if (count > TextureInfo::MaxArraySize)
         throw std::runtime_error(std::format("Texture array exceeds the maxmium size: {}", TextureInfo::MaxArraySize));
      if (bPowerOf2 && (std::bit_floor(w) != w || std::bit_floor(h) != h))
         throw std::runtime_error("Texture's edge size is not the power of 2.");
   }

   // W and h should be the powers of 2.
   uint8_t CalculateMipCount(uint16_t w, uint16_t h)
   {
      uint16_t max = std::max(w, h);
      uint16_t exponent = std::bit_width(max);
      return uint8_t(exponent + 1);
   }

   uint32_t CalculateArraySliceSize(TextureFormat format, uint32_t w, uint32_t h)
   {
      uint32_t min = std::min(w, h);
      uint32_t max = std::max(w, h);
      uint32_t minMips = CalculateMipCount(min, min);
      uint32_t an = max >> (minMips - 1);
      // Geometric sequence, q=1/4, (a1-an*q)/(1-q)=(4a1-an)/3
      uint32_t size = (4 * w * h - an) / 3;
      // Geometric sequence, q=1/2, (a1'-an'*q)/(1-q)=(an/2-1*q)/(1-q)=an-1
      size += an > 1 ? an - 1 : 0;
      size *= GetPixelSize(format);
      return size;
   }

   void BicubicDownsampling(const uint8_t* input, uint8_t* output, int32_t inputWidth, bool is4Channels = true)
   {
      //auto ToFloat_DecodeSRGB = [](uint8_t x) -> float
      //   {
      //      float output = (float)x / UINT8_MAX;
      //      output = MathF.Pow(((output + 0.055f) / 1.055f), 2.4f);
      //      output = MathF.Round(output * byte.MaxValue);
      //      value = (byte)output;
      //   };
      //auto ToUNORM = [](float x) -> uint8_t
      //   {

      //   };
      // Catmull-Rom spline kernel. Renowned for high sharpness.
      auto constexpr Weight = [](float x) -> float
         {
            x = x < 0 ? -x : x;
            if (x <= 1)
            {
               return 1.0 - 2.0 * x * x + x * x * x;
            }
            else if (x < 2)
            {
               return 4.0 - 8.0 * x + 5.0 * x * x - x * x * x;
            }
            return 0;
         };
      // Const & constexpr params.
      const int32_t scale = 2;
      const int32_t channels = is4Channels ? 4 : 1;
      const int32_t outputWidth = inputWidth / scale;
      const int32_t count = outputWidth * outputWidth;
      constexpr float w[2]{ Weight(0.5f), Weight(1.5f) };
      constexpr XMFLOAT4 _k0{ w[1] * w[0], w[0] * w[0], w[0] * w[0], w[1] * w[0] };
      constexpr XMFLOAT4 _k1{ w[1] * w[1], w[0] * w[1], w[0] * w[1], w[1] * w[1] };
      const XMVECTOR k0 = XMLoadFloat4(&_k0);
      const XMVECTOR k1 = XMLoadFloat4(&_k1);     
      // Downsampling.
      for (int32_t i = 0; i < count; i++)
      {
         // 1 Define the kernel and the coordinates of samples.
         // DirectX texture coordinate definition:
         // |---> (u)
         // |
         // v (v)
         int32_t u = (i % outputWidth) * scale;
         int32_t v = (i / outputWidth) * scale;
         XMVECTOR min = XMVectorZero();
         XMVECTOR max = XMVectorReplicateInt(inputWidth - 1);
         XMVECTOR sample_u = XMVectorSetInt(u - 1, u, u + 1, u);
         XMVECTOR sample_v = XMVectorSetInt(v - 1, v, v + 1, v);
         sample_u = XMVectorClamp(sample_u, min, max);
         sample_v = XMVectorClamp(sample_v, min, max);
         XMVECTOR result = XMVectorZero();
         // 2 Sampling and convolution.
         for (int row = 0; row < 4; row++)
         {
            const XMVECTOR& kernel = (row == 0 || row == 3) ? k1 : k0;
            XMVECTOR rowOffset = XMVectorReplicateInt(((int32_t*)&sample_v)[row] * inputWidth * channels);
            XMVECTOR offset = XMVectorAdd(rowOffset, XMVectorMultiply(sample_u, XMVectorReplicateInt(channels)));
            XMINT4 _offset;
            XMStoreSInt4(&_offset, offset);
            XMVECTOR sample = XMVectorZero();
            if (is4Channels)
            {
               XMVECTOR channelIndex = XMVectorSet(0, 1, 2, 3);
               for (int chl = 0; chl < 4; chl++)
               {
                  XMVECTOR singleChannel = XMVectorSet(input[_offset.x + chl], input[_offset.y + chl], input[_offset.z + chl], input[_offset.w + chl]);
                  XMVECTOR channelMask = XMVectorEqual(XMVectorReplicate(chl), channelIndex);
                  sample = XMVectorSelect(sample, XMVector4Dot(singleChannel, kernel), channelMask);
               }
            }
            else
            {
               sample = XMVectorSet(input[_offset.x], input[_offset.y], input[_offset.z], input[_offset.w]);
               sample = XMVector4Dot(sample, kernel);
            }
            result = XMVectorAdd(result, sample);
         }
         // 3 Store the result.
         if (is4Channels)
         {
            XMStoreFloat4((XMFLOAT4*)(output + i * channels), result);
         }
         else
         {
            *(output + i) = XMVectorGetX(result);
         }
      }
   }
}

TextureInfo TextureInfo::DefineTexture(Tag tag, TextureFormat format, ZipType zip, int32_t width, int32_t height, bool useMip)
{
   CheckTextureSize(width, height, 1, true);
   uint8_t mips = useMip ? CalculateMipCount(width, height) : 1;
   return TextureInfo(tag, Type::Tex, format, zip, width, height, mips, 1);
}

TextureInfo TextureInfo::DefineTextureArray(Tag tag, TextureFormat format, ZipType zip, int32_t width, int32_t height, int32_t count, bool useMip)
{
   if (count < 2) throw std::runtime_error("A texture array should have at least 2 textures.");
   CheckTextureSize(width, height, count, true);
   uint8_t mips = useMip ? CalculateMipCount(width, height) : 1;
   return TextureInfo(tag, Type::TexArray, format, zip, width, height, mips, count);
}

TextureInfo Pillow::Graphics::TextureInfo::DefineTextureArray(const TextureInfo& info, int32_t count)
{
   if (count < 2) throw std::runtime_error("A texture array should have at least 2 textures.");
   return TextureInfo(info.TexTag, info.TexType, info.TexFormat, info.CompressionType, info.Width, info.Height, info.MipCount, count);
}


TextureInfo TextureInfo::DefineBakedTexture(TextureFormat format, int32_t width, int32_t height)
{
   CheckTextureSize(width, height, 1, false);
   return TextureInfo(Tag::Development, Type::BakedTex, format, ZipType::None, width, height, 1, 1);
}

TextureInfo TextureInfo::DefineBakedTextureArray(TextureFormat format, int32_t width, int32_t height, int32_t count)
{
   if (count < 2) throw std::runtime_error("A baked-texture array should have at least 2 textures.");
   CheckTextureSize(width, height, count, false);
   return TextureInfo(Tag::Development, Type::BakedTex, format, ZipType::None, width, height, 1, count);
}

bool TextureInfo::operator==(const TextureInfo& right) const
{
   bool result = this->TexType == right.TexType;
   result &= this->TexFormat == right.TexFormat;
   result &= this->CompressionType == right.CompressionType;
   result &= this->Width == right.Width;
   result &= this->Height == right.Height;
   result &= this->MipCount == right.MipCount;
   result &= this->ArrayCount == right.ArrayCount;
   return result;
}

int32_t TextureInfo::GetMipmapSize(uint32_t mipLevel) const
{
   int32_t w = std::max(Width >> mipLevel, 1);
   int32_t h = std::max(Width >> mipLevel, 1);
   return w * h * GetPixelSize(TexFormat);
}

TextureInfo::TextureInfo(Tag tag, Type type, TextureFormat format, ZipType zip, int32_t w, int32_t h, int32_t mips, int32_t arrayNum) :
   TexTag(tag),
   TexType(type),
   TexFormat(format),
   CompressionType(zip),
   Width(uint16_t(w)),
   Height(uint16_t(h)),
   MipCount(uint8_t(mips)),
   ArrayCount(arrayNum),
   ArraySliceSize(CalculateArraySliceSize(TexFormat, Width, Height)),
   TotalSize(ArraySliceSize * ArrayCount),
   ArrayTracking(arrayNum, 0)
{
   // No need to write the body.
}

int32_t Pillow::Graphics::GetPixelSize(TextureFormat format)
{
   return PixelBytes[(int32_t)format];
}

int32_t Pillow::Graphics::GetPixelSize(const TextureInfo& info)
{
   return GetPixelSize(info.TexFormat);
}

void Pillow::Graphics::LoadTexture(const string& relativePath)
{
   // Read the binary file.
   path filePath = GetResourceRelativePath(path(relativePath));
   std::ifstream file(filePath, std::ios::binary | std::ios::ate);
   if (!file.is_open()) throw std::runtime_error("Unable to open file");
   std::streamsize size = file.tellg();
   file.seekg(0, std::ios::beg);
   std::vector<unsigned char> fileData(size);
   if (size > 0 && !file.read((char*)fileData.data(), size)) throw std::runtime_error("Error reading file");
   file.close();
   // Decode it.
   std::vector<unsigned char> imageData;
   uint32_t w, h;
   lodepng::State state;
   //state.decoder.ignore_crc = 1;
   //state.decoder.zlibsettings.ignore_adler32 = 1;
   lodepng::decode(imageData, w, h, state, fileData);
   if (state.info_raw.bitdepth != 8) throw std::exception("Bitdepth should be 8.");
   if (w!=h) throw std::exception("The image should be square.");
   //TextureInfo texInfo;
   //if (state.info_raw.colortype == LCT_GREY)
   //{
   //   texInfo = TextureInfo(GenericTexFmt::UnsignedNormalized_R8, w);
   //}
   //else if (state.info_raw.colortype == LCT_RGB)
   //{
   //   texInfo = TextureInfo(GenericTexFmt::UnsignedNormalized_R8G8B8A8, w);
   //}
   //else if (state.info_raw.colortype == LCT_RGBA)
   //{
   //   texInfo = TextureInfo(GenericTexFmt::UnsignedNormalized_R8G8B8A8, w);
   //}
}

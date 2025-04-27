#include "Texture.h"

using namespace Crucible;

GenericTextureInfo::GenericTextureInfo(int width, bool hasMips, bool isCube, GenericTextureFormat format, int arraySize) :
   _Width(width),
   _PixelSize(PixelSize[(int32_t)format]),
   _ArraySliceCount(arraySize* (isCube ? 6 : 1)),
   _Mip0Size(width * width * PixelSize[(int32_t)format]),
   _Format(format),
   _IsCubemap(isCube)
{
   if (width & (width - 1)) throw std::exception("Texture width must be 2^n");
   int power = std::log2f(width);
   _MipSliceCount = hasMips ? power + 1 : 1;
   _MipSliceSize = hasMips ? (((1 << 2 * _MipSliceCount) - 1) / 3) * _PixelSize : _Mip0Size;
   _TotalSize = _MipSliceSize * _ArraySliceCount;
}
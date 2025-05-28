#include <d3dcommon.h>

namespace Pillow::AppConsts
{
   // Best anisotropy level value considering both performance and quality.
   const int32_t AnisotropyLevel = 4;

   const int32_t SwapChainSize = 3;

   // 11_0 feature level in DX12 can support GPU down to GeForce 400 series!
   const D3D_FEATURE_LEVEL DX12FeatureLevel = D3D_FEATURE_LEVEL_11_0;

   const float CleanColor[4] = {0.2f, 0.21f, 0.2f, 0.0f};

   // 1 Unit = 1 km

   const int32_t MaxStaticRenderItems = 1 << 10;

   const int32_t MaxUIRenderItems = 1 << 8;
}
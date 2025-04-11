#include <d3dcommon.h>

namespace Crucible::AppConsts
{
   /// <summary>Best anisotropy level value considering both performance and quality.</summary>
   const int AnisotropyLevel = 4;

   const int SwapChainSize = 3;

   /// <summary>11_0 feature level in DX12 can support GPU down to GeForce 400 series!</summary>
   const D3D_FEATURE_LEVEL DX12FeatureLevel = D3D_FEATURE_LEVEL_11_0;

   const float CleanColor[4] = {0.2f, 0.21f, 0.2f, 0.0f};

   // 1 Unit = 1 km

   const int MaxStaticRenderItems = 1 << 10;

   const int MaxUIRenderItems = 1 << 8;
}
#pragma once
#include "Auxiliaries.h"
#include "DirectXMath-apr2025/DirectXMath.h"
#if defined(_WIN64)
#include <d3dcommon.h>
#elif defined(__ANDROID__)
#endif
namespace Pillow::Constants
{
   // Best anisotropy level value considering both performance and quality.
   const int32_t AnisotropyLevel = 4;

   const int32_t SwapChainSize = 3;

#if defined(_WIN64)
   // 11_0 feature level in DX12 can support GPU down to GeForce 400 series!
   const D3D_FEATURE_LEVEL DX12FeatureLevel = D3D_FEATURE_LEVEL_11_0;
#endif
   const DirectX::XMFLOAT4 CleanColor{0.2f, 0.21f, 0.2f, 0.0f};

   // 1 Unit = 1 km

   const int32_t MaxStaticRenderItems = 1 << 10;

   const int32_t MaxUIRenderItems = 1 << 8;

   extern int32_t ThreadNumRenderer, ThreadNumPhysics, ThreadNumTick;

   void SetThreadNumbers();
}
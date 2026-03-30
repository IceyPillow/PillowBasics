// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 2-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include <bit>
#include <limits>
#include "DirectXMath-apr2025/DirectXMath.h"

namespace Pillow::Constants
{
   // Best anisotropy level value considering both performance and quality.
   const int32_t AnisotropyLevel = 4;

   const int32_t SwapChainSize = 3;

   const DirectX::XMFLOAT4 CleanColor{0.2f, 0.21f, 0.2f, 0.0f};

   // 1 Unit = 1 km

   const int32_t MaxStaticRenderItems = 1 << 10;

   const int32_t MaxUIRenderItems = 1 << 8;

   const int32_t MaxThreadNumRenderer = 4, MaxThreadNumOther = 8;

   extern int32_t ThreadNumRenderer, ThreadNumPhysics, ThreadNumTick;

   constexpr float FloatInfinity = std::bit_cast<float>(std::uint32_t(0X7F800000U));
   
   constexpr float FloatNegativeInfinity = -FloatInfinity;

   static_assert(sizeof(float) == sizeof(std::uint32_t));
   static_assert(std::numeric_limits<float>::is_iec559); // IEEE-754

   constexpr float FrameTime5FPS = 0.2f;

   constexpr float FrameTime10FPS = 0.1f;
   
   constexpr float FrameTime15FPS = 1.f / 15.f;

   constexpr float FrameTime30FPS = 1.f / 30.f;

   constexpr float FrameTime60FPS = 1.f / 60.f;
   
   constexpr float FrameTime120FPS = 1.f / 120.f;

   void SetThreadNumbers();
}
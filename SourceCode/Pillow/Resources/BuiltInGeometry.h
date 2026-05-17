// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.

namespace
{
   constexpr float x = 1.0f;

   constexpr float hx = 0.5f;

   constexpr float r = 0.5f;

   inline constexpr XMHALF4 XMVF2H(float x, float y, float z, float w)
   {
      return XMHALF4{ XMF2H(x), XMF2H(y) , XMF2H(z) , XMF2H(w) };
   }

   constexpr StandardVertex QuadV[4] =
   {
      { XMFLOAT4A{-hx,0,-hx,0}, {}, {}, XMVF2H(0.f,0.f,0.f,0.f), {}, XMVF2H(0.f,1.f,0.f,0.f), XMVF2H(1.f,0.f,0.f,0.f) },
      { XMFLOAT4A{-hx,0, hx,0}, {}, {}, XMVF2H(0.f,1.f,0.f,0.f), {}, XMVF2H(0.f,1.f,0.f,0.f), XMVF2H(1.f,0.f,0.f,0.f) },
      { XMFLOAT4A{ hx,0, hx,0}, {}, {}, XMVF2H(1.f,1.f,0.f,0.f), {}, XMVF2H(0.f,1.f,0.f,0.f), XMVF2H(1.f,0.f,0.f,0.f) },
      { XMFLOAT4A{ hx,0,-hx,0}, {}, {}, XMVF2H(1.f,0.f,0.f,0.f), {}, XMVF2H(0.f,1.f,0.f,0.f), XMVF2H(1.f,0.f,0.f,0.f) }
   };

   constexpr uint32_t QuadI[6] = { 0, 1, 2, 0, 2, 3 };

   constexpr StandardVertex CubeV[24] =
   {
      // Front
      { XMFLOAT4A{-hx,-hx,-hx,0}, {}, {}, XMVF2H(0.f,0.f,0.f,0.f), {}, XMVF2H(0.f,0.f,-1.f,0.f), XMVF2H(1.f,0.f,0.f,0.f) },
      { XMFLOAT4A{-hx, hx,-hx,0}, {}, {}, XMVF2H(0.f,1.f,0.f,0.f), {}, XMVF2H(0.f,0.f,-1.f,0.f), XMVF2H(1.f,0.f,0.f,0.f) },
      { XMFLOAT4A{ hx, hx,-hx,0}, {}, {}, XMVF2H(1.f,1.f,0.f,0.f), {}, XMVF2H(0.f,0.f,-1.f,0.f), XMVF2H(1.f,0.f,0.f,0.f) },
      { XMFLOAT4A{ hx,-hx,-hx,0}, {}, {}, XMVF2H(1.f,0.f,0.f,0.f), {}, XMVF2H(0.f,0.f,-1.f,0.f), XMVF2H(1.f,0.f,0.f,0.f) },
      // Back
      { XMFLOAT4A{ hx,-hx, hx,0}, {}, {}, XMVF2H(0.f,0.f,0.f,0.f), {}, XMVF2H(0.f,0.f,1.f,0.f), XMVF2H(-1.f,0.f,0.f,0.f) },
      { XMFLOAT4A{ hx, hx, hx,0}, {}, {}, XMVF2H(0.f,1.f,0.f,0.f), {}, XMVF2H(0.f,0.f,1.f,0.f), XMVF2H(-1.f,0.f,0.f,0.f) },
      { XMFLOAT4A{-hx, hx, hx,0}, {}, {}, XMVF2H(1.f,1.f,0.f,0.f), {}, XMVF2H(0.f,0.f,1.f,0.f), XMVF2H(-1.f,0.f,0.f,0.f) },
      { XMFLOAT4A{-hx,-hx, hx,0}, {}, {}, XMVF2H(1.f,0.f,0.f,0.f), {}, XMVF2H(0.f,0.f,1.f,0.f), XMVF2H(-1.f,0.f,0.f,0.f) },
      // Left
      { XMFLOAT4A{-hx,-hx, hx,0}, {}, {}, XMVF2H(0.f,0.f,0.f,0.f), {}, XMVF2H(-1.f,0.f,0.f,0.f), XMVF2H(0.f,0.f,-1.f,0.f) },
      { XMFLOAT4A{-hx, hx, hx,0}, {}, {}, XMVF2H(0.f,1.f,0.f,0.f), {}, XMVF2H(-1.f,0.f,0.f,0.f), XMVF2H(0.f,0.f,-1.f,0.f) },
      { XMFLOAT4A{-hx, hx,-hx,0}, {}, {}, XMVF2H(1.f,1.f,0.f,0.f), {}, XMVF2H(-1.f,0.f,0.f,0.f), XMVF2H(0.f,0.f,-1.f,0.f) },
      { XMFLOAT4A{-hx,-hx,-hx,0}, {}, {}, XMVF2H(1.f,0.f,0.f,0.f), {}, XMVF2H(-1.f,0.f,0.f,0.f), XMVF2H(0.f,0.f,-1.f,0.f) },
      // Right
      { XMFLOAT4A{ hx,-hx,-hx,0}, {}, {}, XMVF2H(0.f,0.f,0.f,0.f), {}, XMVF2H(1.f,0.f,0.f,0.f), XMVF2H(0.f,0.f,1.f,0.f) },
      { XMFLOAT4A{ hx, hx,-hx,0}, {}, {}, XMVF2H(0.f,1.f,0.f,0.f), {}, XMVF2H(1.f,0.f,0.f,0.f), XMVF2H(0.f,0.f,1.f,0.f) },
      { XMFLOAT4A{ hx, hx, hx,0}, {}, {}, XMVF2H(1.f,1.f,0.f,0.f), {}, XMVF2H(1.f,0.f,0.f,0.f), XMVF2H(0.f,0.f,1.f,0.f) },
      { XMFLOAT4A{ hx,-hx, hx,0}, {}, {}, XMVF2H(1.f,0.f,0.f,0.f), {}, XMVF2H(1.f,0.f,0.f,0.f), XMVF2H(0.f,0.f,1.f,0.f) },
      // Top
      { XMFLOAT4A{-hx, hx,-hx,0}, {}, {}, XMVF2H(0.f,0.f,0.f,0.f), {}, XMVF2H(0.f,1.f,0.f,0.f), XMVF2H(1.f,0.f,0.f,0.f) },
      { XMFLOAT4A{-hx, hx, hx,0}, {}, {}, XMVF2H(0.f,1.f,0.f,0.f), {}, XMVF2H(0.f,1.f,0.f,0.f), XMVF2H(1.f,0.f,0.f,0.f) },
      { XMFLOAT4A{ hx, hx, hx,0}, {}, {}, XMVF2H(1.f,1.f,0.f,0.f), {}, XMVF2H(0.f,1.f,0.f,0.f), XMVF2H(1.f,0.f,0.f,0.f) },
      { XMFLOAT4A{ hx, hx,-hx,0}, {}, {}, XMVF2H(1.f,0.f,0.f,0.f), {}, XMVF2H(0.f,1.f,0.f,0.f), XMVF2H(1.f,0.f,0.f,0.f) },
      // Bottom
      { XMFLOAT4A{-hx,-hx, hx,0}, {}, {}, XMVF2H(0.f,0.f,0.f,0.f), {}, XMVF2H(0.f,-1.f,0.f,0.f), XMVF2H(1.f,0.f,0.f,0.f) },
      { XMFLOAT4A{-hx,-hx,-hx,0}, {}, {}, XMVF2H(0.f,1.f,0.f,0.f), {}, XMVF2H(0.f,-1.f,0.f,0.f), XMVF2H(1.f,0.f,0.f,0.f) },
      { XMFLOAT4A{ hx,-hx,-hx,0}, {}, {}, XMVF2H(1.f,1.f,0.f,0.f), {}, XMVF2H(0.f,-1.f,0.f,0.f), XMVF2H(1.f,0.f,0.f,0.f) },
      { XMFLOAT4A{ hx,-hx, hx,0}, {}, {}, XMVF2H(1.f,0.f,0.f,0.f), {}, XMVF2H(0.f,-1.f,0.f,0.f), XMVF2H(1.f,0.f,0.f,0.f) }
   };

   constexpr uint32_t CubeI[36] =
   {
        0, 1, 2, 0, 2, 3,
        4, 5, 7, 5, 6, 7,
        8, 9,10, 8,10,11,
       12,13,15,13,14,15,
       16,17,18,16,18,19,
       20,21,23,21,22,23,
   };
}
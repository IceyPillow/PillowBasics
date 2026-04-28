// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
// To avoid symbol redefinition, The implementation of cgltf is put here.
#define CGLTF_IMPLEMENTATION
#include "cgltf-1.15/cgltf.h"
#undef CGLTF_IMPLEMENTATION
#include "Mesh.h"

std::unique_ptr<StandardMesh> Pillow::Graphics::CreateCube(float xHalf, float yHalf, float zHalf)
{
   return std::unique_ptr<StandardMesh>();
}

std::unique_ptr<StandardMesh> Pillow::Graphics::CreateSphere(float radius)
{
   return std::unique_ptr<StandardMesh>();
}

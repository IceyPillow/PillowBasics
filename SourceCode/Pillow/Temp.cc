// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#include "Resources/Mesh.h"
#include "DirectXMath-apr2025/DirectXMath.h"
#include "Auxiliaries.h"
#include "Resources/Texture.h"

#include "PhysX-4.1/PxPhysicsAPI.h"
#include <iostream>
using namespace physx;

// 简单的错误回调类
class SimpleErrorCallback : public PxErrorCallback {
public:
   void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) override {
      std::cerr << "PhysX Error: " << message << " in " << file << " at line " << line << std::endl;
   }
};

void TempCode()
{
   //SetWindowMode(true);
   //D3D12Renderer renderer(windowHandle, 2);
   //
   //
   //using namespace DirectX;
   //XMVECTOR v = XMVectorSet(1, 1, 1, 1);
   //XMVECTOR v2 = XMVector4Dot(v, v);
   //float result;
   //XMStoreFloat(&result, v2);
   //bool SSE4Check = SSE4::XMVerifySSE4Support();
   //LoadTexture(L"Textures\\SRGBInterpolationExample.png");

   //static PxDefaultAllocator allocator;
   //static SimpleErrorCallback errorCallback;
   //PxFoundation* foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
   //if (!foundation) {
   //   std::cerr << "Failed to create PhysX Foundation!" << std::endl;
   //   //return 1;
   //}

   //// Initialize PhysX SDK
   //PxPhysics* physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale());
   //if (!physics) {
   //   std::cerr << "Failed to create PhysX SDK!" << std::endl;
   //   foundation->release();
   //   //return 1;
   //}

   //// Create a simple scene
   //PxSceneDesc sceneDesc(physics->getTolerancesScale());
   //sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
   //PxDefaultCpuDispatcher* dispatcher = PxDefaultCpuDispatcherCreate(1);
   //sceneDesc.cpuDispatcher = dispatcher;
   //sceneDesc.filterShader = PxDefaultSimulationFilterShader;

   //PxScene* scene = physics->createScene(sceneDesc);
   //if (!scene) {
   //   std::cerr << "Failed to create PhysX Scene!" << std::endl;
   //   dispatcher->release();
   //   physics->release();
   //   foundation->release();
   //   //return 1;
   //}

   //// Output
   //std::cout << "PhysX initialized successfully! Scene created." << std::endl;

   //// Release
   //scene->release();
   //dispatcher->release();
   //physics->release();
   //foundation->release();
}
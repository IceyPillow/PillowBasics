// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#include "World.h"

namespace Pillow::World
{
   void Camera::Write(Graphics::PassConstantBuffer& passCB)
   {
      // Matrix View
      XMVECTOR pos = XMLoadFloat4A(&Position);
      XMVECTOR dir = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), XMLoadFloat4A(&Quaternion));
      XMVECTOR up = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), XMLoadFloat4A(&Quaternion));
      XMMATRIX matrixView = XMMatrixLookToLH(pos, dir, up);
      XMStoreFloat3x4A(&passCB.MatrixView, matrixView);
      // Matrix Projection
      XMMATRIX matrixProj = XMMatrixPerspectiveFovLH(Config.VerticalFOV, Config.AspectRatio, Config.NearZ, Config.FarZ);
      XMStoreFloat4x4A(&passCB.MatrixProj, matrixProj);
      // Matrix View-Projection
      XMMATRIX matrixViewProj = XMMatrixMultiply(matrixProj, matrixView);
      XMStoreFloat4x4A(&passCB.MatrixViewProjection, matrixViewProj);
      // Matrix Inverse View-Projection
      XMMATRIX matrixViewProjInv = XMMatrixInverse(nullptr, matrixViewProj);
      // Other information
      passCB.CameraPositionWorld = {Position.x, Position.y, Position.z};
      passCB.DistanceNear = Config.NearZ;
      passCB.DistanceFar = Config.FarZ;
   }
}
// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#include "World.h"

namespace Pillow::World
{
   void Camera::UpdateConstBuffer(uint32_t frameIdx, float deltaTime, float timeLapse, XMINT2 viewportSize)
   {
      // Matrix View
      XMVECTOR pos = XMLoadFloat4A(&Position);
      XMVECTOR dir = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), XMLoadFloat4A(&Quaternion));
      XMVECTOR up = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), XMLoadFloat4A(&Quaternion));
      XMMATRIX matrixView = XMMatrixLookToLH(pos, dir, up);
      XMStoreFloat3x4A(&ConstBuffer.MatrixView, matrixView);
      // Matrix Projection
      XMMATRIX matrixProj;
      if(Config.VerticalFOV > 0)
      {
         matrixProj = XMMatrixPerspectiveFovLH(Config.VerticalFOV, Config.AspectRatio, Config.NearZ, Config.FarZ);
      }
      else
      {
         matrixProj = XMMatrixOrthographicLH(Config.Width, Config.Height, Config.NearZ, Config.FarZ);
      }
      XMStoreFloat4x4A(&ConstBuffer.MatrixProjection, matrixProj);
      // Matrix View-Projection
      XMMATRIX matrixViewProj = XMMatrixMultiply(matrixProj, matrixView);
      XMStoreFloat4x4A(&ConstBuffer.MatrixViewProjection, matrixViewProj);
      // Matrix Inverse View-Projection
      XMMATRIX matrixViewProjInv = XMMatrixInverse(nullptr, matrixViewProj);
      // Other information
      ConstBuffer.CameraPositionWorld = { Position.x, Position.y, Position.z };
      ConstBuffer.DistanceNear = Config.NearZ;
      ConstBuffer.DistanceFar = Config.FarZ;
      ConstBuffer.ViewportSizeAndRecip[0] = (float)viewportSize.x;
      ConstBuffer.ViewportSizeAndRecip[1] = (float)viewportSize.y;
      ConstBuffer.ViewportSizeAndRecip[2] = 1.0f / (float)viewportSize.x;
      ConstBuffer.ViewportSizeAndRecip[3] = 1.0f / (float)viewportSize.y;
      ConstBuffer.TimeDelta = deltaTime;
      ConstBuffer.TimeLapse = timeLapse;
      ConstBuffer.FrameIdx = frameIdx;
   }
}
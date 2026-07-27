// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#include "World.h"

namespace Pillow::World
{
   void Camera::Tick(double deltaTime, double timeLapse)
   {
      XMINT2 screenSize = IRenderer::GetInstance()->GetBackBufferSize();
      float aspectRatio = (float)screenSize.x / (float)screenSize.y;
      // Matrix View
      XMVECTOR pos = XMLoadFloat4A(&Trans.Position);
      XMVECTOR dir = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), XMLoadFloat4A(&Trans.Quaternion));
      XMVECTOR up = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), XMLoadFloat4A(&Trans.Quaternion));
      XMMATRIX matrixView = XMMatrixLookToLH(pos, dir, up);
      XMStoreFloat3x4A(&ConstBuffer.MatrixView, matrixView);
      // Matrix Projection
      XMMATRIX matrixProj;
      if(Config.VerticalFOV > 0)
      {
         matrixProj = XMMatrixPerspectiveFovLH(Config.VerticalFOV, aspectRatio, Config.NearZ, Config.FarZ);
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
      ConstBuffer.CameraPositionWorld = { Trans.Position.x, Trans.Position.y, Trans.Position.z };
      ConstBuffer.DistanceNear = Config.NearZ;
      ConstBuffer.DistanceFar = Config.FarZ;
      ConstBuffer.ViewportSizeAndRecip[0] = (float)screenSize.x;
      ConstBuffer.ViewportSizeAndRecip[1] = (float)screenSize.y;
      ConstBuffer.ViewportSizeAndRecip[2] = 1.0f / (float)screenSize.x;
      ConstBuffer.ViewportSizeAndRecip[3] = 1.0f / (float)screenSize.y;
      ConstBuffer.TimeDelta = deltaTime;
      ConstBuffer.TimeLapse = (float)std::fmod(timeLapse, Constants::TimeLapseMax);
      ConstBuffer.FrameArrayIdx = IRenderer::GetInstance()->GetFrameArrayIdx();
   }
}
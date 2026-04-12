// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#include "UI.h"

namespace Pillow::TempUISubSystem_April2025
{
   void UITick()
   {
      bool bFPSMode = Input::GetCursorMode();
      float deltaTime = Pillow::GetFrameDeltaTime();
      if (bFPSMode)
      {
         for (IControl* control : IControl::controls)
         {
            if (control->IsVisible == false) continue;
            if (control->MixedEvent) control->MixedEvent(deltaTime, false);
         }
      }
      else
      {
         XMFLOAT2A f_cursor = Input::GetMicePos();
         XMVECTOR cursor = XMLoadFloat2A(&f_cursor);
         IControl* focuedControl = nullptr;
         // Find the focus.
         float minZ = Pillow::Constants::FloatInfinity;
         for (IControl* control : IControl::controls)
         {
            if (control->IsVisible == false) continue;
            XMVECTOR min = XMVectorSet(control->Position.x, control->Position.y, 0, 0);
            XMVECTOR max = XMVectorSet(control->Position.x + control->Size.x, control->Position.y + control->Size.y, 0, 0);
            XMVECTOR v_result = XMVectorAndInt(XMVectorGreaterOrEqual(cursor, min), XMVectorLessOrEqual(cursor, max));
            XMUINT2 result;
            XMStoreUInt2(&result, v_result);
            if ((result.x && result.y) == false) continue;
            if (control->Position.z < minZ)
            {
               minZ = control->Position.z;
               focuedControl = control;
            }
         }
         // Invoke events.
         for (IControl* control : IControl::controls)
         {
            if (control->IsVisible == false) continue;
            if (control->MixedEvent) control->MixedEvent(deltaTime, control == focuedControl);
         }
      }
   }
}
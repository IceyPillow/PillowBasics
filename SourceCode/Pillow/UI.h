// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include <vector>
#include <string>
#include <functional>
#include "Resources/Mesh.h"
#include "Input.h"

namespace Pillow::TempUISubSystem_April2025
{
   using namespace Pillow::Graphics;
   using UIEvent = void(*)(float deltaTime, bool isFocused);

   // The base class for all controls.
   class IControl
   {
      DeleteDefautedMethods(IControl)

   public:
      const std::string NameID;
      XMFLOAT3 Position{ 0,0,0 };
      XMFLOAT2 Size{ 0, 0 };
      bool IsVisible = true;
      UIEvent MixedEvent = nullptr;

      friend void UITick();

      virtual ~IControl() = 0;

   protected:
      IControl(std::string ID, XMFLOAT3 pos, XMFLOAT2 size): NameID(ID), Position(pos), Size(size)
      {

      }

      std::vector<UIVertex> vertices;

   protected:
      static inline std::vector<IControl*> controls{};
   };

   // Generic container, can act as a window, an image, a chart, or a canvas.
   class Panel final : public IControl
   {
   public:

   private:

   };

   // Text container.
   class TextArea final : public IControl
   {
   public:

   private:

   };

   // Generic Button.
   class Button final : public IControl
   {
   public:

   private:

   };

   // Generic list, can be used as a dropdown menu or a selection list.
   class ItemList final : public IControl
   {
   public:

   private:

   };
}
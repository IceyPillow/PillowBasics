// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.

namespace Pillow::UI
{
   // The base class for all controls.
   class IControl
   {
   public:
      virtual ~IControl() = 0;

   protected:
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
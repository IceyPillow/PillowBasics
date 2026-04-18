// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 3-Clause License. Do not remove, obscure, or alter this notice.
#pragma once
#include <vector>
namespace Pillow::World
{
   using ComponentEvent = void(*)(float deltaTime);

   class GameObject
   {
      std::vector<ComponentEvent> ticks{};
   };

   class GenericCamera : public GameObject
   {

   };
}
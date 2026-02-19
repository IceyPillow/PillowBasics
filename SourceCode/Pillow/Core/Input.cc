// PillowBasics Copyright (c) 2025, Icey Pillow. BSD 2-Clause License. Do not remove, obscure, or alter this notice.
#include "Input.h"

using namespace Pillow;

namespace
{
   enum class State : char
   {
      Released,
      Pressed,
      Down,
      Up
   };
}


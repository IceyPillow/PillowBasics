#include"Input.h"

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


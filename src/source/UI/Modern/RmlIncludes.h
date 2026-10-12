#pragma once

// Include RmlUi only through this header. Core/Math/ZzzMathLib.h defines function-like
// macros (Vector4, DotProduct) that collide with RmlUi member names, so they are hidden
// while the RmlUi headers are parsed and restored afterwards.

#pragma push_macro("Vector4")
#pragma push_macro("DotProduct")
#undef Vector4
#undef DotProduct

#include <RmlUi/Core.h>

#pragma pop_macro("DotProduct")
#pragma pop_macro("Vector4")

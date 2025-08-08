#ifndef SYNERGY_CORE_INCLUDED
#define SYNERGY_CORE_INCLUDED

#include "SynergyCoreMath.h"

// Common defines

// TRANSLATION UNIT & SOURCE INC FILE SYSTEM

#ifndef TRANSLATION_UNIT
#define ASSERT_RESULT 0
#else
#define ASSERT_RESULT 1
#endif

// Files meant to be compiled as a translation unit should define a value for TRANSLATION_UNIT.

// Used at the top of INC files, to streamline checking for a defined Translation Unit the INC file is part of.
#define SOURCE_INC_FILE() static_assert(ASSERT_RESULT, "INC File " __FILE__ " must be included within a translation unit and NOT compiled on its own !");


#endif // SYNERGY_CORE_INCLUDED
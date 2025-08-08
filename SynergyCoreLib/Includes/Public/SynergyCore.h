#ifndef SYNERGY_CORE_INCLUDED
#define SYNERGY_CORE_INCLUDED

#include "SynergyCoreMemory.h"
#include "SynergyCoreMath.h"

// Common defines

// TRANSLATION UNIT & SOURCE INC FILE SYSTEM

#ifndef TRANSLATION_UNIT
#define INSIDE_TRANSLATION_UNIT 0
#else
#define INSIDE_TRANSLATION_UNIT 1
#endif

#define PRE_STR(str) #str
#define STR(str) PRE_STR(str)

// Files meant to be compiled as a translation unit should define a value for TRANSLATION_UNIT.

#if INSIDE_TRANSLATION_UNIT
// Used at the top of INC files, to streamline checking for a defined Translation Unit the INC file is part of.
#define SOURCE_INC_FILE() static_assert(1, "This file is part of " STR(TRANSLATION_UNIT));
#else
// Used at the top of INC files, to streamline checking for a defined Translation Unit the INC file is part of.
#define SOURCE_INC_FILE() static_assert(0, "INC File " __FILE__ " must be included within a translation unit and NOT compiled on its own !");
#endif

#endif // SYNERGY_CORE_INCLUDED
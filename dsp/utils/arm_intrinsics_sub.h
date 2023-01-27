/*
 * This file is included in ARCH_ARM64 to provide the various SIMDE intrinsics
 */

/*
 * Approach 1: Recommended by andrew. Just include rack.hpp. 
 */
#include "rack.hpp"


/*
 * Approach 2: If you want we can also include simde directly.That would involve a statement like

#define SIMDE_ENABLE_NATIVE_ALIASES
#include "simde/x86/sse2.h"

 * which would allow this code to ahve just simde as a dependency. But since it seems we are rack bound
 * all the way into this dsp code, the rack.hpp is the most stable approach
 */

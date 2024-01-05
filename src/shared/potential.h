/* potential.h: routines dealing with view potential */

#ifndef _POTENTIAL_H_
#define _POTENTIAL_H_

#include "skin/Patch.h"

/* Determines the actual directly received potential of the patches in
 * the scene. */
extern void UpdateDirectPotential();

/* Updates view visibility status of all patches. */
extern void UpdateDirectVisibility();

#endif

/* patchlist_geom.h: makes PATCHLISTs look like GEOMs. Required for shaft culling. */

#ifndef _PATCHLIST_GEOM_H_
#define _PATCHLIST_GEOM_H_

#include "skin/geom.h"

extern GEOM_METHODS GLOBAL_skin_patchListGeometryMethods;

/* "returns" the GEOM methods for a patchlist */
#define PatchListMethods() (&GLOBAL_skin_patchListGeometryMethods)

#endif

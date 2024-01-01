/* patchlist_geom.h: makes PATCHLISTs look like GEOMs. Required for shaft culling. */

#ifndef _PATCHLIST_GEOM_H_
#define _PATCHLIST_GEOM_H_

#include "skin/Geometry.h"

extern GEOM_METHODS GLOBAL_skin_patchListGeometryMethods;

/* "returns" the Geometry methods for a getPatchList */
#define PatchListMethods() (&GLOBAL_skin_patchListGeometryMethods)

#endif

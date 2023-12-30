/* patchlist_geom.h: makes PATCHLISTs look like GEOMs. Required for shaft culling. */

#ifndef _PATCHLIST_GEOM_H_
#define _PATCHLIST_GEOM_H_

#include "skin/geom.h"

extern GEOM_METHODS patchlistMethods;

/* "returns" the GEOM methods for a patchlist */
#define PatchListMethods() (&patchlistMethods)

#endif

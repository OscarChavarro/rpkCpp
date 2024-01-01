#ifndef __COMPOUND__
#define __COMPOUND__

#include "skin/geomlist.h"

class GEOM_METHODS;

#define COMPOUND GEOMLIST

extern GEOM_METHODS GLOBAL_skin_compoundGeometryMethods;

extern COMPOUND *compoundCreate(GEOMLIST *geomlist);

#define CompoundMethods() (&GLOBAL_skin_compoundGeometryMethods)

#endif

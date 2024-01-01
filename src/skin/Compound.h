#ifndef __COMPOUND__
#define __COMPOUND__

#include "skin/geomlist.h"

class GEOM_METHODS;

#define COMPOUND GeometryListNode

extern GEOM_METHODS GLOBAL_skin_compoundGeometryMethods;

extern COMPOUND *compoundCreate(GeometryListNode *geomlist);

#define CompoundMethods() (&GLOBAL_skin_compoundGeometryMethods)

#endif

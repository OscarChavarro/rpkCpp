#ifndef __COMPOUND__
#define __COMPOUND__

#include "java/util/ArrayList.h"
#include "skin/geomlist.h"

class GEOM_METHODS;

class Compound /*: public Geometry */ {
public:
    GeometryListNode children;
};

extern GEOM_METHODS GLOBAL_skin_compoundGeometryMethods;

extern Compound *compoundCreate(GeometryListNode *geometryList);

#define CompoundMethods() (&GLOBAL_skin_compoundGeometryMethods)

#endif

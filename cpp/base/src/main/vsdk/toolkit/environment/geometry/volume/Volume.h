#ifndef __VOLUME__
#define __VOLUME__

#include "vsdk/toolkit/environment/geometry/Geometry.h"
class PolyhedralBoundedSolid;

class Volume : public Geometry {
public:
    virtual ~Volume() {}
    virtual PolyhedralBoundedSolid* exportToPolyhedralBoundedSolid();
};

#endif

#ifndef __VOLUME__
#define __VOLUME__

#include "vsdk/toolkit/environment/geometry/Geometry.h"
class PolyhedralBoundedSolid;

class Volume : public Geometry {
public:
    static const Class CLASS;
    virtual const Class *getClass() const;

    virtual ~Volume() {}
    virtual PolyhedralBoundedSolid* exportToPolyhedralBoundedSolid();
};

#endif

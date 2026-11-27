#ifndef __SURFACE__
#define __SURFACE__

#include "vsdk/toolkit/environment/geometry/Geometry.h"
class Surface : public Geometry {
public:
    static const Class CLASS;
    virtual const Class *getClass() const;

    virtual ~Surface() {}
};

#endif

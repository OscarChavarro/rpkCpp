#ifndef __SOLID__
#define __SOLID__

#include "vsdk/toolkit/environment/geometry/volume/Volume.h"
class Solid : public Volume {
public:
    static const Class CLASS;
    virtual const Class *getClass() const;

    virtual ~Solid() {}
    virtual Vector3Dd doCenterOfMass();
};

#endif

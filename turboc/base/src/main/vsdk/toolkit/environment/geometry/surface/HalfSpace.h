#ifndef __HALF_SPACE__
#define __HALF_SPACE__

#include "vsdk/toolkit/environment/geometry/surface/Surface.h"
class HalfSpace : public Surface {
public:
    static const Class CLASS;
    virtual const Class *getClass() const;

    virtual ~HalfSpace() {}
};

#endif

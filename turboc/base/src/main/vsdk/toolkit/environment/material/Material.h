#ifndef __MATERIAL__
#define __MATERIAL__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Class.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class Material {
  public:
    virtual ~Material() {}

    virtual Material *copy() const = 0;
    virtual Material *translate(Vector3Dd *vector) = 0;
    virtual Material *rotate(Vector3Dd *vector) = 0;
    virtual Material *scale(Vector3Dd *vector) = 0;

    /**
    @return the descriptor of the class of this material (Java
    `getClass()`, implemented by the material classes)
    */
    virtual const Class *getClass() const = 0;

    /**
    @return true if this material is an instance of the class or of one of
    its subclasses (Java `instanceof`)
    */
    BOOLEAN instanceOf(const Class &type) const
    {
        return type.isAssignableFrom(getClass());
    }
};

#endif

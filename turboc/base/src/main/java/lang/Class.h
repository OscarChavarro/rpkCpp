#ifndef __CLASS__
#define __CLASS__

#include <stddef.h>
#include "vsdk/toolkit/common/VSDK.h"

/**
Emulation of `java.lang.Class`, the run time description of a class of the
port: its simple name and its superclass.

Each class of a described hierarchy declares
<pre>
    static const Class CLASS;
    virtual const Class *getClass() const;
</pre>
and defines them in its implementation file, i.e.
<pre>
    const Class Sphere::CLASS = { "Sphere", &Solid::CLASS };

    const Class *
    Sphere::getClass() const
    {
        return &CLASS;
    }
</pre>
so `instanceof` tests are written as `geometry->instanceOf(Sphere::CLASS)`,
followed by a conventional cast `(Sphere *)geometry`.

C++ port note: the members are public so the descriptors are aggregates,
initialized at compile time (independent of the order of initialization of
the static objects of each module). Only the first superclass is
described: interfaces (as `Material`) are not part of the chain.
*/
class Class {
  public:
    const char *simpleName;
    const Class *superclass;

    /**
    @return the name of the class, without namespaces
    */
    const char *getSimpleName() const
    {
        return simpleName;
    }

    /**
    @return the superclass, or NULL for the root of a hierarchy
    */
    const Class *getSuperclass() const
    {
        return superclass;
    }

    /**
    @return true if `other` is this class or one of its subclasses
    */
    BOOLEAN isAssignableFrom(const Class *other) const
    {
        for ( const Class *c = other; c != NULL; c = c->superclass ) {
            if ( c == this ) {
                return TRUE;
            }
        }
        return FALSE;
    }
};

#endif

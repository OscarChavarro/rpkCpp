#ifndef __POLYHEDRAL_BOUNDED_SOLID_VERTEX__
#define __POLYHEDRAL_BOUNDED_SOLID_VERTEX__

#include "java/lang/String.h"

#include "vsdk/toolkit/common/dataStructures/ArrayLinkable.h"

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class _PolyhedralBoundedSolidHalfEdge;

class _PolyhedralBoundedSolidVertex : public ArrayLinkable {
private:
    /// Place of this element in the list that holds it, kept up to date by
    /// the list (see `ArrayLinkable`).
    const void* listOwner;
    int listIndex;

public:
    virtual const void* getListOwner() const override { return listOwner; }
    virtual int getListIndex() const override { return listIndex; }
    virtual void setListSlot(const void* owner, int index) override
    {
        listOwner = owner;
        listIndex = index;
    }

public:
    int id;
    Vector3Dd position;
    _PolyhedralBoundedSolidHalfEdge* emanatingHalfEdge;
    ColorRgb debugColor;

    _PolyhedralBoundedSolidVertex(const Vector3Dd& position, int id)
        : listOwner(nullptr), listIndex(-1), id(id), position(position), emanatingHalfEdge(0), debugColor(1, 0, 0) {}

    /**
    @return a description of the node, as Java `toString`
    */
    java::String toString() const;
};

#endif

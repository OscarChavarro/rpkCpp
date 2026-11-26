#ifndef __POLYHEDRAL_BOUNDED_SOLID_EDGE__
#define __POLYHEDRAL_BOUNDED_SOLID_EDGE__

#include "java/lang/String.h"

#include "vsdk/toolkit/common/dataStructures/ArrayLinkable.h"

#include "vsdk/toolkit/common/color/ColorRgb.h"
class _PolyhedralBoundedSolidHalfEdge;

class _PolyhedralBoundedSolidEdge : public ArrayLinkable {
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
    _PolyhedralBoundedSolidHalfEdge* rightHalf;
    _PolyhedralBoundedSolidHalfEdge* leftHalf;
    int id;
    ColorRgb debugColor;

    _PolyhedralBoundedSolidEdge();
    int getEndingVertexId() const;
    int getStartingVertexId() const;

    /**
    @return a description of the node, as Java `toString`
    */
    java::String toString() const;
};

#endif

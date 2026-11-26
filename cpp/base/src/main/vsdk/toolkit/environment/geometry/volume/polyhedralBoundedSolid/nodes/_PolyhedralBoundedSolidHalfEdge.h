#ifndef __POLYHEDRAL_BOUNDED_SOLID_HALF_EDGE__
#define __POLYHEDRAL_BOUNDED_SOLID_HALF_EDGE__

#include "java/lang/String.h"

#include "vsdk/toolkit/common/dataStructures/ArrayLinkable.h"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class _PolyhedralBoundedSolidLoop;
class _PolyhedralBoundedSolidEdge;
class _PolyhedralBoundedSolidVertex;

class _PolyhedralBoundedSolidHalfEdge : public ArrayLinkable {
private:
    /// Place of this half-edge in `parentLoop->halfEdgesList`. It plays the
    /// role of the intrusive `nxt`/`prv` links of [MANT1988] Program 10.1:
    /// `next()` and `previous()` are O(1) through it. The list keeps it up
    /// to date (see `ArrayLinkable`).
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
    static const int LEFT_SIDE = 1;
    static const int RIGHT_SIDE = 2;
    static const int NO_SIDE = 3;

    _PolyhedralBoundedSolidLoop* parentLoop;
    _PolyhedralBoundedSolidEdge* parentEdge;
    _PolyhedralBoundedSolidVertex* startingVertex;
    int id;

    _PolyhedralBoundedSolidHalfEdge(_PolyhedralBoundedSolidVertex* v, _PolyhedralBoundedSolidLoop* parentLoop);

    _PolyhedralBoundedSolidHalfEdge* previous();
    _PolyhedralBoundedSolidHalfEdge* next();
    _PolyhedralBoundedSolidHalfEdge* mirrorHalfEdge();
    bool vertexPositionMatch(_PolyhedralBoundedSolidHalfEdge* other, double tolerance);

    /**
    @return a description of the node, as Java `toString`
    */
    java::String toString();
};

#endif

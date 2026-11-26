#ifndef __POLYHEDRAL_BOUNDED_SOLID_LOOP__
#define __POLYHEDRAL_BOUNDED_SOLID_LOOP__

#include "java/lang/String.h"

#include "vsdk/toolkit/common/dataStructures/ArrayLinkable.h"
#include "vsdk/toolkit/common/dataStructures/CircularArrayList.h"

class _PolyhedralBoundedSolidFace;
class _PolyhedralBoundedSolidHalfEdge;

class _PolyhedralBoundedSolidLoop : public ArrayLinkable {
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
    _PolyhedralBoundedSolidFace* parentFace;
    _PolyhedralBoundedSolidHalfEdge* boundaryStartHalfEdge;
    CircularArrayList<_PolyhedralBoundedSolidHalfEdge*> halfEdgesList;

    explicit _PolyhedralBoundedSolidLoop(_PolyhedralBoundedSolidFace* parent);
    ~_PolyhedralBoundedSolidLoop();

    void unlistHalfEdge(_PolyhedralBoundedSolidHalfEdge* he);
    _PolyhedralBoundedSolidHalfEdge* halfEdgeVertices(int a, int b);
    _PolyhedralBoundedSolidHalfEdge* firstHalfEdgeAtVertex(int a);
    void delhe(_PolyhedralBoundedSolidHalfEdge* he);
    void revert();

    /**
    @return a description of the node, as Java `toString`
    */
    java::String toString() const;
};

#endif

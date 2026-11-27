#ifndef __PLYHDRL_BNDD_SLD_LP__
#define __PLYHDRL_BNDD_SLD_LP__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/dataStructures/ArrayLinkable.h"
#include "vsdk/toolkit/common/dataStructures/CircularArrayList.h"
// The half-edge list needs the half-edge class complete (see CircularArrayList)
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
class _PolyhedralBoundedSolidFace;
class _PolyhedralBoundedSolidHalfEdge;

class _PolyhedralBoundedSolidLoop : public ArrayLinkable {
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
    String toString() const;
};

#endif

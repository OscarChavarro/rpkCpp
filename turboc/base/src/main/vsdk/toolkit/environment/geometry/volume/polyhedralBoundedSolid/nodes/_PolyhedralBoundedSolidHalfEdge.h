#ifndef __PLYHDRL_BNDD_SLD_HLF_EDG__
#define __PLYHDRL_BNDD_SLD_HLF_EDG__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/common/dataStructures/ArrayLinkable.h"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class _PolyhedralBoundedSolidLoop;
class _PolyhedralBoundedSolidEdge;
class _PolyhedralBoundedSolidVertex;

class _PolyhedralBoundedSolidHalfEdge : public ArrayLinkable {
public:
    enum {
        LEFT_SIDE = 1,
        RIGHT_SIDE = 2,
        NO_SIDE = 3
    };

    _PolyhedralBoundedSolidLoop* parentLoop;
    _PolyhedralBoundedSolidEdge* parentEdge;
    _PolyhedralBoundedSolidVertex* startingVertex;
    int id;

    _PolyhedralBoundedSolidHalfEdge(_PolyhedralBoundedSolidVertex* v, _PolyhedralBoundedSolidLoop* parentLoop);

    _PolyhedralBoundedSolidHalfEdge* previous();
    _PolyhedralBoundedSolidHalfEdge* next();
    _PolyhedralBoundedSolidHalfEdge* mirrorHalfEdge();
    BOOLEAN vertexPositionMatch(_PolyhedralBoundedSolidHalfEdge* other, double tolerance);

    /**
    @return a description of the node, as Java `toString`
    */
    String toString();
};

#endif

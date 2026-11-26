#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
static int pbs_half_edge_current_id = 1;

_PolyhedralBoundedSolidHalfEdge::_PolyhedralBoundedSolidHalfEdge(_PolyhedralBoundedSolidVertex* v, _PolyhedralBoundedSolidLoop* inParentLoop)
    : listOwner(nullptr), listIndex(-1), parentLoop(inParentLoop), parentEdge(0), startingVertex(v), id(pbs_half_edge_current_id++)
{
}

_PolyhedralBoundedSolidHalfEdge* _PolyhedralBoundedSolidHalfEdge::previous()
{
    return (parentLoop != 0) ? parentLoop->halfEdgesList.previousOf(this) : 0;
}

_PolyhedralBoundedSolidHalfEdge* _PolyhedralBoundedSolidHalfEdge::next()
{
    return (parentLoop != 0) ? parentLoop->halfEdgesList.nextOf(this) : 0;
}

_PolyhedralBoundedSolidHalfEdge* _PolyhedralBoundedSolidHalfEdge::mirrorHalfEdge()
{
    if ( parentEdge == 0 ) return 0;
    if ( this == parentEdge->rightHalf ) return parentEdge->leftHalf;
    return parentEdge->rightHalf;
}

bool _PolyhedralBoundedSolidHalfEdge::vertexPositionMatch(_PolyhedralBoundedSolidHalfEdge* other, double tolerance)
{
    if ( other == 0 || startingVertex == 0 || other->startingVertex == 0 ) return false;
    return startingVertex->position.subtract(other->startingVertex->position).length() <= tolerance;
}

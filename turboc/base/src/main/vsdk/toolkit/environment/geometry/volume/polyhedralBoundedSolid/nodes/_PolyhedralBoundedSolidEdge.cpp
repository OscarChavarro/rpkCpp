#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
static int pbs_edge_current_id = 1;

_PolyhedralBoundedSolidEdge::_PolyhedralBoundedSolidEdge()
    : rightHalf(0), leftHalf(0), id(pbs_edge_current_id++), debugColor(1,1,1)
{
}

int _PolyhedralBoundedSolidEdge::getEndingVertexId() const
{
    if ( leftHalf == 0 || leftHalf->startingVertex == 0 ) return -1;
    return leftHalf->startingVertex->id;
}

int _PolyhedralBoundedSolidEdge::getStartingVertexId() const
{
    if ( rightHalf == 0 || rightHalf->startingVertex == 0 ) return -1;
    return rightHalf->startingVertex->id;
}

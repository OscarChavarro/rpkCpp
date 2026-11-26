//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidRepair.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

PolyhedralBoundedSolidRepair::Report::Report(int relinkedVertices,
    int removedVertices)
    : relinkedVertices(relinkedVertices), removedVertices(removedVertices)
{
}

int PolyhedralBoundedSolidRepair::Report::getRelinkedVertices() const
{
    return relinkedVertices;
}

int PolyhedralBoundedSolidRepair::Report::getRemovedVertices() const
{
    return removedVertices;
}

bool PolyhedralBoundedSolidRepair::Report::isClean() const
{
    return relinkedVertices == 0 && removedVertices == 0;
}

java::String PolyhedralBoundedSolidRepair::Report::toString() const
{
    return java::String("Report{relinkedVertices=") +
        java::String::valueOf(relinkedVertices) +
        ", removedVertices=" + java::String::valueOf(removedVertices) + "}";
}

PolyhedralBoundedSolidRepair::Report
PolyhedralBoundedSolidRepair::remakeEmanatingHalfEdgeReferences(
    PolyhedralBoundedSolid* solid)
{
    int relinked = 0;
    int removed = 0;
    int i;
    int j;

    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        _PolyhedralBoundedSolidVertex* v = solid->getVerticesList().get(i);
        _PolyhedralBoundedSolidHalfEdge* he = v->emanatingHalfEdge;
        if ( he == 0 || he->startingVertex != v ||
             he->parentLoop == 0 ||
             !he->parentLoop->halfEdgesList.contains(he) ) {
            relinked++;
        }
        v->emanatingHalfEdge = 0;
    }

    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        for ( j = 0; j < face->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop* loop;
            _PolyhedralBoundedSolidHalfEdge* he;
            _PolyhedralBoundedSolidHalfEdge* heStart;

            loop = face->boundariesList.get(j);
            he = loop->boundaryStartHalfEdge;
            if ( he == 0 ) {
                continue;
            }
            heStart = he;
            do {
                he->startingVertex->emanatingHalfEdge = he;
                he = he->next();
                if ( he == 0 ) {
                    break;
                }
            } while( he != heStart );
        }
    }

    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        if ( solid->getVerticesList().get(i)->emanatingHalfEdge == 0 ) {
            solid->getVerticesList().remove(i);
            relinked--;
            removed++;
            i--;
        }
    }
    return Report(relinked, removed);
}

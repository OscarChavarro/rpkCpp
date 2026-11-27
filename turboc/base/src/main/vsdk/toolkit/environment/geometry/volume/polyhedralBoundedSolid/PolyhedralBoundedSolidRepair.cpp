//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/Cpp98Support.h"
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

BOOLEAN PolyhedralBoundedSolidRepair::Report::isClean() const
{
    return relinkedVertices == 0 && removedVertices == 0;
}

String PolyhedralBoundedSolidRepair::Report::toString() const
{
    String out("Report{relinkedVertices=");
    out += Cpp98Support::toString(relinkedVertices).c_str();
    out += ", removedVertices=";
    out += Cpp98Support::toString(removedVertices).c_str();
    out += "}";
    return out;
}

/**
Rebuilds the vertex-to-emanating-half-edge links required by the vertex
node of [MANT1988].10.2.1 and used throughout the Euler operators of
chapter [MANT1988].11: every vertex is linked to the last half-edge
that starts at it, in face, loop and half-edge order, and vertices that
no half-edge starts at are removed from the solid.
C++ port note: as in Java, removed vertices are only unlisted from the
solid; they are not deleted here.
@return what was wrong before the rebuild
*/
PolyhedralBoundedSolidRepair::Report
PolyhedralBoundedSolidRepair::remakeEmanatingHalfEdgeReferences(
    PolyhedralBoundedSolid* solid)
{
    int relinked = 0;
    int removed = 0;
    long int i;
    long int j;

    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        _PolyhedralBoundedSolidVertex* v = solid->getVerticesList().get(i);
        _PolyhedralBoundedSolidHalfEdge* he = v->emanatingHalfEdge;
        if ( he == NULL || he->startingVertex != v ||
             he->parentLoop == NULL ||
             !he->parentLoop->halfEdgesList.contains(he) ) {
            relinked++;
        }
        v->emanatingHalfEdge = NULL;
    }

    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        for ( j = 0; j < face->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop* loop;
            _PolyhedralBoundedSolidHalfEdge* he;
            _PolyhedralBoundedSolidHalfEdge* heStart;

            loop = face->boundariesList.get(j);
            he = loop->boundaryStartHalfEdge;
            if ( he == NULL ) {
                continue;
            }
            heStart = he;
            do {
                he->startingVertex->emanatingHalfEdge = he;
                he = he->next();
                if ( he == NULL ) {
                    break;
                }
            } while ( he != heStart );
        }
    }

    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        if ( solid->getVerticesList().get(i)->emanatingHalfEdge == NULL ) {
            solid->getVerticesList().remove(i);
            relinked--;
            removed++;
            i--;
        }
    }
    return Report(relinked, removed);
}

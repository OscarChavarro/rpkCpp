
#include "java/util/Collections.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
_PolyhedralBoundedSolidLoop::_PolyhedralBoundedSolidLoop(_PolyhedralBoundedSolidFace* parent)
    : listOwner(nullptr), listIndex(-1), parentFace(parent), boundaryStartHalfEdge(0)
{
    if ( parentFace != 0 ) {
        parentFace->boundariesList.add(this);
    }
}

_PolyhedralBoundedSolidLoop::~_PolyhedralBoundedSolidLoop()
{
    for (long int i = 0; i < halfEdgesList.size(); ++i) {
        if (halfEdgesList.get(i) != 0) {
            delete halfEdgesList.get(i);
        }
    }
}

void _PolyhedralBoundedSolidLoop::unlistHalfEdge(_PolyhedralBoundedSolidHalfEdge* he)
{
    if ( halfEdgesList.locateWindowAtElem(he) ) {
        halfEdgesList.removeElemAtWindow();
    }
    if ( halfEdgesList.size() > 0  ) {
        boundaryStartHalfEdge = halfEdgesList.get(0);
    }
    else {
        boundaryStartHalfEdge = 0;
    }
}

/**
Locates a half edge that goes from vertex with id `a` to vertex with id `b`,
traversing the loop from its boundary start half edge.
@param a starting vertex id
@param b ending vertex id
@return requested halfedge, or null if no such half edge exists in this loop
*/
_PolyhedralBoundedSolidHalfEdge* _PolyhedralBoundedSolidLoop::halfEdgeVertices(int a, int b)
{
    _PolyhedralBoundedSolidHalfEdge* he;
    _PolyhedralBoundedSolidHalfEdge* oldhe;

    he = boundaryStartHalfEdge;
    if ( he == 0 ) {
        return 0;
    }
    do {
        oldhe = he;
        he = he->next();
        if ( he == 0 ) {
            // Loop is not closed!
            break;
        }

        if ( oldhe->startingVertex->id == a && he->startingVertex->id == b ) {
            return oldhe;
        }

    } while ( he != boundaryStartHalfEdge );
    return 0;
}

/**
Locates the first half edge starting at vertex with id `a`, traversing the
loop from its boundary start half edge.
@param a starting vertex id
@return requested halfedge, or null if no such half edge exists in this loop
*/
_PolyhedralBoundedSolidHalfEdge* _PolyhedralBoundedSolidLoop::firstHalfEdgeAtVertex(int a)
{
    _PolyhedralBoundedSolidHalfEdge* he;
    _PolyhedralBoundedSolidHalfEdge* oldhe;

    he = boundaryStartHalfEdge;
    if ( he == 0 ) {
        return 0;
    }
    do {
        oldhe = he;
        he = he->next();
        if ( he == 0 ) {
            // Loop is not closed!
            break;
        }

        if ( oldhe->startingVertex->id == a ) {
            return oldhe;
        }

    } while ( he != boundaryStartHalfEdge );
    return 0;
}

/**
Vitral SDK's current implementation of original `delhe` utility function
presented at program [MANT1988].11.4. and section [MANT1988].11.2.2.
Note that current implementation is quite diferent from the original from
[MANT1988]. When the loop becomes empty, the boundary start is kept.
@param he half edge to remove from this loop
*/
void _PolyhedralBoundedSolidLoop::delhe(_PolyhedralBoundedSolidHalfEdge* he)
{
    halfEdgesList.locateWindowAtElem(he);
    halfEdgesList.removeElemAtWindow();

    if ( halfEdgesList.size() > 0 ) {
        boundaryStartHalfEdge = halfEdgesList.get(0);
    }
}

/**
Reverses the loop orientation, moving each parent edge reference (and its
side) to the following half edge, so the edges keep their half edges.
*/
void _PolyhedralBoundedSolidLoop::revert()
{
    long int n = halfEdgesList.size();

    if ( n <= 0 ) {
        return;
    }

    //-----------------------------------------------------------------
    _PolyhedralBoundedSolidHalfEdge* he;
    java::ArrayList<_PolyhedralBoundedSolidEdge*> edges = java::Collections::nCopies<_PolyhedralBoundedSolidEdge*>((size_t)n, nullptr);
    java::ArrayList<bool> sides = java::Collections::nCopies<bool>((size_t)n, false);
    long int i;

    for ( i = 0; i < n; i++ ) {
        he = halfEdgesList.get(i);
        // Degenerate loops can be left behind transiently after lkemr
        // during boolean finishing. They are not traversable via an edge.
        if ( he->parentEdge == 0 ) {
            return;
        }
        edges[i] = he->parentEdge;
        sides[i] = false;
        if ( he->parentEdge->rightHalf == he ) {
            sides[i] = true;
        }
    }

    for ( i = 1; i < n; i++ ) {
        halfEdgesList.get(i)->parentEdge = edges[i-1];
        if ( sides[i-1] ) {
            edges[i-1]->rightHalf = halfEdgesList.get(i);
        }
        else {
            edges[i-1]->leftHalf = halfEdgesList.get(i);
        }
    }
    halfEdgesList.get(0)->parentEdge = edges[i-1];
    if ( sides[i-1] ) {
        edges[i-1]->rightHalf = halfEdgesList.get(0);
    }
    else {
        edges[i-1]->leftHalf = halfEdgesList.get(0);
    }

    //-----------------------------------------------------------------
    halfEdgesList.reverse();
}

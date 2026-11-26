#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidBooleanTopologyPredicates.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

bool _PolyhedralBoundedSolidBooleanTopologyPredicates::planesCoincidentIgnoringOrientation(
    const InfinitePlane* a,
    const InfinitePlane* b,
    double tolerance)
{
    double a1;
    double b1;
    double c1;
    double d1;
    double a2;
    double b2;
    double c2;
    double d2;
    double l1;
    double l2;

    if ( a == 0 || b == 0 ) {
        return false;
    }

    a1 = a->getA();
    b1 = a->getB();
    c1 = a->getC();
    d1 = a->getD();
    a2 = b->getA();
    b2 = b->getB();
    c2 = b->getC();
    d2 = b->getD();

    l1 = std::sqrt(a1*a1 + b1*b1 + c1*c1);
    l2 = std::sqrt(a2*a2 + b2*b2 + c2*c2);
    if ( l1 <= tolerance || l2 <= tolerance ) {
        return false;
    }

    a1 /= l1;
    b1 /= l1;
    c1 /= l1;
    d1 /= l1;
    a2 /= l2;
    b2 /= l2;
    c2 /= l2;
    d2 /= l2;

    bool sameOrientation =
        std::fabs(a2 - a1) <= tolerance &&
        std::fabs(b2 - b1) <= tolerance &&
        std::fabs(c2 - c1) <= tolerance &&
        std::fabs(d2 - d1) <= tolerance;

    bool oppositeOrientation =
        std::fabs(a2 + a1) <= tolerance &&
        std::fabs(b2 + b1) <= tolerance &&
        std::fabs(c2 + c1) <= tolerance &&
        std::fabs(d2 + d1) <= tolerance;

    return sameOrientation || oppositeOrientation;
}

bool _PolyhedralBoundedSolidBooleanTopologyPredicates::loopsCoincidentFrom(
    _PolyhedralBoundedSolidHalfEdge* startA,
    _PolyhedralBoundedSolidHalfEdge* startB,
    bool reverse,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext)
{
    _PolyhedralBoundedSolidHalfEdge* heA;
    _PolyhedralBoundedSolidHalfEdge* heB;

    heA = startA;
    heB = startB;
    do {
        if ( !PolyhedralBoundedSolidNumericPolicy::pointsCoincident(
            heA->startingVertex->position, heB->startingVertex->position,
            numericContext) ) {
            return false;
        }
        heA = heA->next();
        heB = reverse ? heB->previous() : heB->next();
    } while ( heA != startA );

    return true;
}

bool _PolyhedralBoundedSolidBooleanTopologyPredicates::loopsCoincident(
    _PolyhedralBoundedSolidLoop* a,
    _PolyhedralBoundedSolidLoop* b,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext)
{
    long int i;
    _PolyhedralBoundedSolidHalfEdge* startA;
    _PolyhedralBoundedSolidHalfEdge* scanB;

    if ( a == 0 || b == 0 ||
         a->boundaryStartHalfEdge == 0 || b->boundaryStartHalfEdge == 0 ) {
        return false;
    }
    if ( a->halfEdgesList.size() != b->halfEdgesList.size() ) {
        return false;
    }

    startA = a->boundaryStartHalfEdge;
    scanB = b->boundaryStartHalfEdge;
    for ( i = 0; i < b->halfEdgesList.size(); i++ ) {
        if ( PolyhedralBoundedSolidNumericPolicy::pointsCoincident(
            startA->startingVertex->position, scanB->startingVertex->position,
            numericContext) ) {
            if ( loopsCoincidentFrom(startA, scanB, false, numericContext) ||
                 loopsCoincidentFrom(startA, scanB, true, numericContext) ) {
                return true;
            }
        }
        scanB = scanB->next();
    }

    return false;
}

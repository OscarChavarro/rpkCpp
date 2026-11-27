#include <math.h>
#include <stdio.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidGeometricValidator.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidFaceGeometry.h"
#include "java/util/ArrayDeque.h"
#include "java/util/TreeMap.txx"
#include "java/lang/Double.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
#include "vsdk/toolkit/common/Cpp98Support.h"
#include "java/lang/Math.h"


typedef PolyhedralBoundedSolidNumericPolicy::ToleranceContext ToleranceContext;

static const double INVERTED_FACE_THRESHOLD = 0.5;

static void append(String* msg, const String& text)
{
    if ( msg != 0 ) {
        *msg += text.c_str();
    }
}

static String str(int value)
{
    return Cpp98Support::toString(value);
}

static int dominantCoordinateForPlane(const InfinitePlane* plane)
{
    Vector3Dd n = plane->getNormal();
    if ( fabs(n.x()) >= fabs(n.y()) && fabs(n.x()) >= fabs(n.z()) ) {
        return 1;
    }
    if ( fabs(n.y()) >= fabs(n.x()) && fabs(n.y()) >= fabs(n.z()) ) {
        return 2;
    }
    return 3;
}

static int dominantCoordinateForFace(_PolyhedralBoundedSolidFace* face)
{
    InfinitePlane* plane = face->getContainingPlane();
    int dominantCoordinate = dominantCoordinateForPlane(plane);
    delete plane;
    return dominantCoordinate;
}

static Vector2Dd projectPointTo2D(const Vector3Dd& in, int dominantCoordinate)
{
    if ( dominantCoordinate == 1 ) {
        return Vector2Dd(in.y(), in.z());
    }
    if ( dominantCoordinate == 2 ) {
        return Vector2Dd(in.x(), in.z());
    }
    return Vector2Dd(in.x(), in.y());
}

static double orientation2D(const Vector2Dd& a, const Vector2Dd& b, const Vector2Dd& c)
{
    return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
}

static BOOLEAN pointOnSegment2D(const Vector2Dd& p, const Vector2Dd& a, const Vector2Dd& b,
                      double orientationTolerance,
                      double linearTolerance)
{
    if ( fabs(orientation2D(a, b, p)) > orientationTolerance ) {
        return FALSE;
    }
    double minX = Math::min(a.x, b.x) - linearTolerance;
    double maxX = Math::max(a.x, b.x) + linearTolerance;
    double minY = Math::min(a.y, b.y) - linearTolerance;
    double maxY = Math::max(a.y, b.y) + linearTolerance;
    return p.x >= minX && p.x <= maxX && p.y >= minY && p.y <= maxY;
}

static BOOLEAN segmentsIntersect2D(const Vector2Dd& a1, const Vector2Dd& a2,
                         const Vector2Dd& b1, const Vector2Dd& b2,
                         const ToleranceContext& numericContext)
{
    double o1 = orientation2D(a1, a2, b1);
    double o2 = orientation2D(a1, a2, b2);
    double o3 = orientation2D(b1, b2, a1);
    double o4 = orientation2D(b1, b2, a2);

    double orientationTolerance = PolyhedralBoundedSolidNumericPolicy
        ::orientationTolerance2D(a1, a2, b1, numericContext);
    orientationTolerance = Math::max(orientationTolerance,
        PolyhedralBoundedSolidNumericPolicy
            ::orientationTolerance2D(a1, a2, b2, numericContext));
    orientationTolerance = Math::max(orientationTolerance,
        PolyhedralBoundedSolidNumericPolicy
            ::orientationTolerance2D(b1, b2, a1, numericContext));
    orientationTolerance = Math::max(orientationTolerance,
        PolyhedralBoundedSolidNumericPolicy
            ::orientationTolerance2D(b1, b2, a2, numericContext));
    double linearTolerance =
        PolyhedralBoundedSolidNumericPolicy::linearTolerance2D(numericContext);

    BOOLEAN proper = ((o1 > orientationTolerance && o2 < -orientationTolerance) ||
                   (o1 < -orientationTolerance && o2 > orientationTolerance)) &&
                  ((o3 > orientationTolerance && o4 < -orientationTolerance) ||
                   (o3 < -orientationTolerance && o4 > orientationTolerance));
    if ( proper ) {
        return TRUE;
    }

    return pointOnSegment2D(b1, a1, a2, orientationTolerance, linearTolerance) ||
           pointOnSegment2D(b2, a1, a2, orientationTolerance, linearTolerance) ||
           pointOnSegment2D(a1, b1, b2, orientationTolerance, linearTolerance) ||
           pointOnSegment2D(a2, b1, b2, orientationTolerance, linearTolerance);
}

static BOOLEAN loopHasSelfIntersection(_PolyhedralBoundedSolidFace* face,
                             _PolyhedralBoundedSolidLoop* loop,
                             const ToleranceContext& numericContext,
                             String* msg)
{
    long int n = loop->halfEdgesList.size();
    if ( n < 3 ) {
        append(msg, "  - Face [" + str(face->id) +
            "] has a loop with fewer than 3 edges.\n");
        return TRUE;
    }

    int dominantCoordinate = dominantCoordinateForFace(face);
    long int i;
    long int j;
    for ( i = 0; i < n; i++ ) {
        _PolyhedralBoundedSolidHalfEdge* heA = loop->halfEdgesList.get(i);
        _PolyhedralBoundedSolidHalfEdge* heANext = heA->next();
        if ( heANext == 0 ) {
            append(msg, "  - Face [" + str(face->id) +
                "] has a non-closed loop during strict validation.\n");
            return TRUE;
        }
        Vector2Dd a1 = projectPointTo2D(heA->startingVertex->position, dominantCoordinate);
        Vector2Dd a2 = projectPointTo2D(heANext->startingVertex->position, dominantCoordinate);

        for ( j = i+1; j < n; j++ ) {
            if ( j == (i+1)%n || i == (j+1)%n ) {
                continue;
            }
            _PolyhedralBoundedSolidHalfEdge* heB = loop->halfEdgesList.get(j);
            _PolyhedralBoundedSolidHalfEdge* heBNext = heB->next();
            if ( heBNext == 0 ) {
                append(msg, "  - Face [" + str(face->id) +
                    "] has a non-closed loop during strict validation.\n");
                return TRUE;
            }
            Vector2Dd b1 = projectPointTo2D(heB->startingVertex->position, dominantCoordinate);
            Vector2Dd b2 = projectPointTo2D(heBNext->startingVertex->position, dominantCoordinate);

            if ( segmentsIntersect2D(a1, a2, b1, b2, numericContext) ) {
                append(msg, "  - Face [" + str(face->id) +
                    "] has a self-intersecting loop.\n");
                return TRUE;
            }
        }
    }
    return FALSE;
}

static BOOLEAN loopsIntersect(_PolyhedralBoundedSolidFace* face,
                    _PolyhedralBoundedSolidLoop* loopA,
                    _PolyhedralBoundedSolidLoop* loopB,
                    const ToleranceContext& numericContext,
                    String* msg)
{
    int dominantCoordinate = dominantCoordinateForFace(face);
    long int i;
    long int j;
    for ( i = 0; i < loopA->halfEdgesList.size(); i++ ) {
        _PolyhedralBoundedSolidHalfEdge* heA = loopA->halfEdgesList.get(i);
        _PolyhedralBoundedSolidHalfEdge* heANext = heA->next();
        if ( heANext == 0 ) {
            append(msg, "  - Face [" + str(face->id) +
                "] has a non-closed loop during strict validation.\n");
            return TRUE;
        }
        Vector2Dd a1 = projectPointTo2D(heA->startingVertex->position, dominantCoordinate);
        Vector2Dd a2 = projectPointTo2D(heANext->startingVertex->position, dominantCoordinate);

        for ( j = 0; j < loopB->halfEdgesList.size(); j++ ) {
            _PolyhedralBoundedSolidHalfEdge* heB = loopB->halfEdgesList.get(j);
            _PolyhedralBoundedSolidHalfEdge* heBNext = heB->next();
            if ( heBNext == 0 ) {
                append(msg, "  - Face [" + str(face->id) +
                    "] has a non-closed loop during strict validation.\n");
                return TRUE;
            }
            Vector2Dd b1 = projectPointTo2D(heB->startingVertex->position, dominantCoordinate);
            Vector2Dd b2 = projectPointTo2D(heBNext->startingVertex->position, dominantCoordinate);
            if ( segmentsIntersect2D(a1, a2, b1, b2, numericContext) ) {
                append(msg, "  - Face [" + str(face->id) +
                    "] has intersecting loops.\n");
                return TRUE;
            }
        }
    }
    return FALSE;
}

static BOOLEAN faceAgreesWithNeighbours(_PolyhedralBoundedSolidFace* face, String* msg)
{
    InfinitePlane* planeF;
    Vector3Dd nF;
    long int i;
    long int j;
    int neighbourCount;
    int anomalousCount;
    double worstDot;

    if ( face == 0 || face->boundariesList.size() == 0 ) {
        return TRUE;
    }
    planeF = face->getContainingPlane();
    if ( planeF == 0 ) {
        return TRUE;
    }
    nF = planeF->getNormal();
    delete planeF;
    neighbourCount = 0;
    anomalousCount = 0;
    worstDot = 1.0;
    for ( i = 0; i < face->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(i);
        if ( loop == 0 ) {
            continue;
        }
        for ( j = 0; j < loop->halfEdgesList.size(); j++ ) {
            _PolyhedralBoundedSolidHalfEdge* he = loop->halfEdgesList.get(j);
            if ( he == 0 || he->mirrorHalfEdge() == 0 ||
                 he->mirrorHalfEdge()->parentLoop == 0 ) {
                continue;
            }
            _PolyhedralBoundedSolidFace* neighbour =
                he->mirrorHalfEdge()->parentLoop->parentFace;
            if ( neighbour == 0 || neighbour == face ) {
                continue;
            }
            InfinitePlane* planeN = neighbour->getContainingPlane();
            if ( planeN == 0 ) {
                continue;
            }
            Vector3Dd nN = planeN->getNormal();
            delete planeN;
            neighbourCount++;
            double dot = nF.dotProduct(nN);
            if ( dot < worstDot ) {
                worstDot = dot;
            }
            if ( dot < -INVERTED_FACE_THRESHOLD ) {
                anomalousCount++;
            }
        }
    }
    // Flag only when EVERY neighbour disagrees strongly (consistent
    // inversion sign).  A single sharp dihedral is not enough.
    if ( neighbourCount >= 2 && anomalousCount == neighbourCount ) {
        char worst[64];
        snprintf(worst, sizeof(worst), "%.3f", worstDot);
        append(msg, "  - Face [" + str(face->id) + "] is opposed to all " +
            str(neighbourCount) + " neighbours (worst cos=" + worst + ")\n");
        return FALSE;
    }
    return TRUE;
}

static BOOLEAN facesAreCoplanar(_PolyhedralBoundedSolidFace* faceA,
                      _PolyhedralBoundedSolidFace* faceB,
                      InfinitePlane* planeA, InfinitePlane* planeB,
                      const ToleranceContext& numericContext)
{
    (void)faceB;
    if ( planeA == 0 || planeB == 0 ) {
        return FALSE;
    }

    Vector3Dd nA = planeA->getNormal().multiply(1.0);
    Vector3Dd nB = planeB->getNormal().multiply(1.0);
    nA = nA.normalized();
    nB = nB.normalized();
    if ( fabs(fabs(nA.dotProduct(nB)) - 1.0) >
         numericContext.coplanarDotTolerance() ) {
        return FALSE;
    }

    for ( long int i = 0; i < faceA->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = faceA->boundariesList.get(i);
        if ( loop->halfEdgesList.size() > 0 ) {
            Vector3Dd p = loop->halfEdgesList.get(0)->startingVertex->position;
            return fabs(planeB->pointDistance(p)) <=
                numericContext.bigEpsilon();
        }
    }
    return FALSE;
}

static BOOLEAN segmentSharesEndpoint(_PolyhedralBoundedSolidHalfEdge* a,
                           _PolyhedralBoundedSolidHalfEdge* b)
{
    _PolyhedralBoundedSolidHalfEdge* an = a->next();
    _PolyhedralBoundedSolidHalfEdge* bn = b->next();
    if ( an == 0 || bn == 0 ) {
        return FALSE;
    }
    _PolyhedralBoundedSolidVertex* a0 = a->startingVertex;
    _PolyhedralBoundedSolidVertex* a1 = an->startingVertex;
    _PolyhedralBoundedSolidVertex* b0 = b->startingVertex;
    _PolyhedralBoundedSolidVertex* b1 = bn->startingVertex;
    return a0 == b0 || a0 == b1 || a1 == b0 || a1 == b1;
}

static BOOLEAN vertexStrictlyInsideFace(_PolyhedralBoundedSolidVertex* v,
                              _PolyhedralBoundedSolidFace* face,
                              const PolyhedralBoundedSolidFaceGeometry* faceGeometry,
                              const ToleranceContext& numericContext)
{
    InfinitePlane* plane = faceGeometry->getPlane();
    if ( plane->doContainmentTest(v->position,
        numericContext.bigEpsilon()) != Geometry::LIMIT ) {
        return FALSE;
    }
    if ( faceGeometry->projectionExcludes(v->position,
             numericContext.bigEpsilon()) ) {
        return FALSE;
    }
    return face->testPointInside(v->position, numericContext.bigEpsilon(),
        plane) == Geometry::INSIDE;
}

static BOOLEAN edgePiercesFaceInterior(_PolyhedralBoundedSolidHalfEdge* he,
                             _PolyhedralBoundedSolidFace* face,
                             const PolyhedralBoundedSolidFaceGeometry* faceGeometry,
                             const ToleranceContext& numericContext)
{
    _PolyhedralBoundedSolidHalfEdge* next = he->next();
    InfinitePlane* plane = faceGeometry->getPlane();
    if ( next == 0 || plane == 0 ) {
        return FALSE;
    }

    Vector3Dd p0 = he->startingVertex->position;
    Vector3Dd p1 = next->startingVertex->position;
    double d0 = plane->pointDistance(p0);
    double d1 = plane->pointDistance(p1);

    if ( fabs(d0) <= numericContext.bigEpsilon() &&
         fabs(d1) <= numericContext.bigEpsilon() ) {
        return FALSE;
    }
    if ( d0*d1 > 0 ) {
        return FALSE;
    }

    double denom = d0 - d1;
    if ( PolyhedralBoundedSolidNumericPolicy::isZero(denom, numericContext) ) {
        return FALSE;
    }
    double t = d0 / denom;
    if ( !PolyhedralBoundedSolidNumericPolicy
        ::unitIntervalContainsStrictly(t, numericContext) ) {
        return FALSE;
    }

    Vector3Dd p = p0.add(p1.subtract(p0).multiply(t));
    if ( faceGeometry->projectionExcludes(p, numericContext.bigEpsilon()) ) {
        return FALSE;
    }
    return face->testPointInside(p, numericContext.bigEpsilon(), plane) ==
        Geometry::INSIDE;
}

static void appendImproperIntersection(String* msg,
    _PolyhedralBoundedSolidFace* faceA, _PolyhedralBoundedSolidFace* faceB)
{
    append(msg, "  - Faces [" + str(faceA->id) + "] and [" + str(faceB->id) +
        "] intersect improperly.\n");
}

static BOOLEAN facesHaveImproperIntersection(_PolyhedralBoundedSolidFace* faceA,
                                   _PolyhedralBoundedSolidFace* faceB,
                                   const PolyhedralBoundedSolidFaceGeometry* geometryA,
                                   const PolyhedralBoundedSolidFaceGeometry* geometryB,
                                   const ToleranceContext& numericContext,
                                   String* msg)
{
    long int i;
    long int j;
    long int k;
    _PolyhedralBoundedSolidHalfEdge* he;

    for ( i = 0; i < faceA->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = faceA->boundariesList.get(i);
        for ( j = 0; j < loop->halfEdgesList.size(); j++ ) {
            he = loop->halfEdgesList.get(j);
            if ( geometryB->getPlane() != 0 &&
                 vertexStrictlyInsideFace(he->startingVertex, faceB,
                                          geometryB, numericContext) ) {
                appendImproperIntersection(msg, faceA, faceB);
                return TRUE;
            }
            if ( edgePiercesFaceInterior(he, faceB, geometryB,
                                         numericContext) ) {
                appendImproperIntersection(msg, faceA, faceB);
                return TRUE;
            }
        }
    }

    for ( i = 0; i < faceB->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = faceB->boundariesList.get(i);
        for ( j = 0; j < loop->halfEdgesList.size(); j++ ) {
            he = loop->halfEdgesList.get(j);
            if ( geometryA->getPlane() != 0 &&
                 vertexStrictlyInsideFace(he->startingVertex, faceA,
                                          geometryA, numericContext) ) {
                appendImproperIntersection(msg, faceA, faceB);
                return TRUE;
            }
            if ( edgePiercesFaceInterior(he, faceA, geometryA,
                                         numericContext) ) {
                appendImproperIntersection(msg, faceA, faceB);
                return TRUE;
            }
        }
    }

    if ( facesAreCoplanar(faceA, faceB, geometryA->getPlane(),
             geometryB->getPlane(), numericContext) ) {
        int dominantCoordinate = dominantCoordinateForFace(faceA);
        for ( i = 0; i < faceA->boundariesList.size(); i++ ) {
            _PolyhedralBoundedSolidLoop* loopA = faceA->boundariesList.get(i);
            for ( j = 0; j < loopA->halfEdgesList.size(); j++ ) {
                _PolyhedralBoundedSolidHalfEdge* heA = loopA->halfEdgesList.get(j);
                _PolyhedralBoundedSolidHalfEdge* heANext = heA->next();
                if ( heANext == 0 ) {
                    continue;
                }
                Vector2Dd a1 = projectPointTo2D(heA->startingVertex->position, dominantCoordinate);
                Vector2Dd a2 = projectPointTo2D(heANext->startingVertex->position, dominantCoordinate);

                for ( k = 0; k < faceB->boundariesList.size(); k++ ) {
                    _PolyhedralBoundedSolidLoop* loopB = faceB->boundariesList.get(k);
                    for ( long int m = 0; m < loopB->halfEdgesList.size(); m++ ) {
                        _PolyhedralBoundedSolidHalfEdge* heB = loopB->halfEdgesList.get(m);
                        _PolyhedralBoundedSolidHalfEdge* heBNext = heB->next();
                        if ( heBNext == 0 ) {
                            continue;
                        }
                        if ( heA->parentEdge == heB->parentEdge ) {
                            continue;
                        }
                        Vector2Dd b1 = projectPointTo2D(heB->startingVertex->position, dominantCoordinate);
                        Vector2Dd b2 = projectPointTo2D(heBNext->startingVertex->position, dominantCoordinate);
                        if ( segmentsIntersect2D(a1, a2, b1, b2, numericContext) &&
                             !segmentSharesEndpoint(heA, heB) ) {
                            appendImproperIntersection(msg, faceA, faceB);
                            return TRUE;
                        }
                    }
                }
            }
        }
    }

    return FALSE;
}


BOOLEAN PolyhedralBoundedSolidGeometricValidator::validateFacePointsAreCoplanar(
    ArrayList<Vector3Dd>& points)
{
    return validateFacePointsAreCoplanar(points,
        PolyhedralBoundedSolidNumericPolicy::forPoints(points));
}

/**
Implements the coplanarity precondition needed before applying the face
equation ideas of [MANT1988].13.1 to a polyhedral face from
[MANT1988].10.2.1. As in Java, the reference plane is the one defined by the
last valid point triple found.
*/
BOOLEAN PolyhedralBoundedSolidGeometricValidator::validateFacePointsAreCoplanar(
    ArrayList<Vector3Dd>& points,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext)
{
    if ( points.size() < 3 ) {
        return FALSE;
    }

    Vector3Dd p0;
    Vector3Dd p1;
    Vector3Dd p2;
    p0 = points.get(0);
    BOOLEAN foundSeparatedPair = FALSE;

    long int i;
    for ( i = 1; i < points.size(); i++ ) {
        p1 = points.get(i);
        if ( PolyhedralBoundedSolidNumericPolicy::pointsSeparated(
                 p0, p1, numericContext) ) {
            foundSeparatedPair = TRUE;
            break;
        }
    }
    if ( !foundSeparatedPair ) {
        return FALSE;
    }

    Vector3Dd a;
    Vector3Dd b;
    Vector3Dd n;
    double aDotB;
    InfinitePlane* facePlane = 0;
    long int j;
    long int k;

    // The plane is the one of the last pair (i, j), in ascending
    // order, whose first admissible third point k is not collinear
    // with it. The pairs are visited from the last one backwards and
    // the search stops at the first that gives a plane: the same
    // plane as visiting every pair and keeping the last one, which
    // took a time cubic in the number of points (performance.md).
    for ( i = points.size() - 1; i >= 0 && facePlane == 0; i-- ) {
        for ( j = points.size() - 1; j >= 0 && facePlane == 0; j-- ) {
            for ( k = 0; k < points.size(); k++ ) {
                if ( i == j || i == k || j == k ) {
                    continue;
                }
                p0 = points.get(i);
                p1 = points.get(j);
                p2 = points.get(k);
                if ( PolyhedralBoundedSolidNumericPolicy::pointsSeparated(
                         p0, p2, numericContext) &&
                     PolyhedralBoundedSolidNumericPolicy::pointsSeparated(
                         p1, p2, numericContext) ) {
                    a = p2.subtract(p0);
                    b = p1.subtract(p0);
                    a = a.normalized();
                    b = b.normalized();
                    aDotB = fabs(a.dotProduct(b));
                    if ( aDotB < 1.0 - numericContext.unitVectorTolerance() ) {
                        n = a.crossProduct(b);
                        n = n.normalized();
                        delete facePlane;
                        facePlane = new InfinitePlane(n, p0);
                    }
                    break;
                }
            }
        }
    }

    if ( facePlane == 0 ) {
        return FALSE;
    }

    BOOLEAN coplanar = TRUE;
    for ( i = 1; i < points.size(); i++ ) {
        p0 = points.get(i);
        if ( facePlane->doContainmentTest(p0, numericContext.epsilon()) !=
             Geometry::LIMIT ) {
            coplanar = FALSE;
            break;
        }
    }

    delete facePlane;
    return coplanar;
}

BOOLEAN PolyhedralBoundedSolidGeometricValidator::extractPointsFromFace(
    _PolyhedralBoundedSolidFace* face,
    ArrayList<Vector3Dd>& outPoints)
{
    long int j;

    for ( j = 0; j < face->boundariesList.size(); j++ ) {
        _PolyhedralBoundedSolidLoop* loop;
        _PolyhedralBoundedSolidHalfEdge* he;
        _PolyhedralBoundedSolidHalfEdge* heStart;

        loop = face->boundariesList.get(j);
        he = loop->boundaryStartHalfEdge;
        if ( he == 0 ) {
            return FALSE;
        }
        heStart = he;
        do {
            he = he->next();
            if ( he == 0 ) {
                return FALSE;
            }
            outPoints.add(he->startingVertex->position);
        } while ( he != heStart );
    }
    return TRUE;
}

BOOLEAN PolyhedralBoundedSolidGeometricValidator::validateFaceIsPlanar(
    _PolyhedralBoundedSolidFace* face)
{
    return validateFaceIsPlanar(face,
        PolyhedralBoundedSolidNumericPolicy::forFace(face));
}

BOOLEAN PolyhedralBoundedSolidGeometricValidator::validateFaceIsPlanar(
    _PolyhedralBoundedSolidFace* face,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext)
{
    ArrayList<Vector3Dd> points;
    return extractPointsFromFace(face, points) &&
        validateFacePointsAreCoplanar(points, numericContext);
}

BOOLEAN PolyhedralBoundedSolidGeometricValidator::validateAllFacesPlanarityAndPlanes(
    PolyhedralBoundedSolid* solid, String* msg)
{
    return validateAllFacesPlanarityAndPlanes(solid,
        PolyhedralBoundedSolidNumericPolicy::forSolid(solid), msg);
}

/**
Applies the face-planarity and face-equation checks corresponding to
[MANT1988].10.2.1 and [MANT1988].13.1 to every face of the solid.
*/
BOOLEAN PolyhedralBoundedSolidGeometricValidator::validateAllFacesPlanarityAndPlanes(
    PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
    String* msg)
{
    long int i;
    BOOLEAN test = TRUE;
    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        if ( validateFaceIsPlanar(face, numericContext) ) {
            InfinitePlane* plane = face->getContainingPlane();
            if ( plane == 0 ) {
                append(msg, "  - Face [" + str(face->id) +
                    "] was not able to compute containing plane\n");
                test = FALSE;
            }
            delete plane;
        }
        else {
            append(msg, "  - Face [" + str(face->id) + "] is not coplanar\n");
            test = FALSE;
        }
    }
    return test;
}

BOOLEAN PolyhedralBoundedSolidGeometricValidator::validateConsistentFaceOrientations(
    PolyhedralBoundedSolid* solid, String* msg)
{
    return validateConsistentFaceOrientations(solid,
        PolyhedralBoundedSolidNumericPolicy::forSolid(solid), msg);
}

/**
Heuristic check for the consistent face-orientation invariant required by
the 2-manifold boundary model of [MANT1988].10.2.1: each face plane normal is
compared against the normals of its neighbours through shared edges, and a
face is flagged only when it is strongly opposed to all of them.
*/
BOOLEAN PolyhedralBoundedSolidGeometricValidator::validateConsistentFaceOrientations(
    PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
    String* msg)
{
    long int i;
    BOOLEAN test;

    (void)numericContext;
    if ( solid == 0 || solid->getPolygonsList().size() == 0 ) {
        return TRUE;
    }
    test = TRUE;
    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        if ( !faceAgreesWithNeighbours(face, msg) ) {
            test = FALSE;
        }
    }
    return test;
}

BOOLEAN PolyhedralBoundedSolidGeometricValidator::validateLoopsStrict(
    PolyhedralBoundedSolid* solid, String* msg)
{
    return validateLoopsStrict(solid,
        PolyhedralBoundedSolidNumericPolicy::forSolid(solid), msg);
}

/**
Enforces strict geometric loop consistency for the planar polygons assumed
by [MANT1988].10.2.1 and manipulated by the geometric tools of chapter
[MANT1988].13.
*/
BOOLEAN PolyhedralBoundedSolidGeometricValidator::validateLoopsStrict(
    PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
    String* msg)
{
    long int i;
    long int j;
    long int k;
    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        InfinitePlane* plane = face->getContainingPlane();
        if ( plane == 0 ) {
            append(msg, "  - Face [" + str(face->id) +
                "] has no containing plane for strict checks.\n");
            return FALSE;
        }
        delete plane;

        for ( j = 0; j < face->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(j);
            if ( loopHasSelfIntersection(face, loop, numericContext, msg) ) {
                return FALSE;
            }
        }
        for ( j = 0; j < face->boundariesList.size(); j++ ) {
            for ( k = j+1; k < face->boundariesList.size(); k++ ) {
                if ( loopsIntersect(face, face->boundariesList.get(j),
                                    face->boundariesList.get(k),
                                    numericContext, msg) ) {
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}

BOOLEAN PolyhedralBoundedSolidGeometricValidator::validateFaceIntersectionsStrict(
    PolyhedralBoundedSolid* solid, String* msg)
{
    return validateFaceIntersectionsStrict(solid,
        PolyhedralBoundedSolidNumericPolicy::forSolid(solid), msg);
}

/**
Applies strict face/face intersection checks to preserve the validity
condition from [MANT1988].15.2, criterion 3, using predicates from chapter
[MANT1988].13.
*/
BOOLEAN PolyhedralBoundedSolidGeometricValidator::validateFaceIntersectionsStrict(
    PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
    String* msg)
{
    long int i;
    long int j;
    long int n = solid->getPolygonsList().size();
    ArrayList<_PolyhedralBoundedSolidFace*> faces;
    ArrayList<PolyhedralBoundedSolidFaceGeometry*> geometry;
    ArrayList<double> margin;
    double coplanarSlack;
    BOOLEAN ok = TRUE;

    // Stage G (Fable5.2Analysis.md): planes are computed once per pass
    // instead of once per face pair and per point, and face pairs whose
    // grown boxes are apart are skipped. Every improper intersection has
    // a witness point that lies in both grown boxes: a vertex of one face
    // inside the other, the point where an edge pierces a face, or a
    // crossing of two coplanar edges. For the last one the faces may be
    // tilted by up to acos(1 - coplanarDotTolerance), which coplanarSlack
    // bounds over the whole solid.
    PolyhedralBoundedSolidFaceGeometry::of(solid, geometry);
    coplanarSlack = 2.0 * sqrt(2.0 *
        numericContext.coplanarDotTolerance()) * solidDiagonal(solid);
    for ( i = 0; i < n; i++ ) {
        faces.add(solid->getPolygonsList().get(i));
        margin.add(geometry[i]->containmentMargin(
            numericContext.bigEpsilon()));
    }
    for ( i = 0; i < n && ok; i++ ) {
        for ( j = i+1; j < n; j++ ) {
            if ( geometry[i]->boxSeparatedFrom(geometry[j],
                     margin[i] + margin[j] + coplanarSlack) ) {
                continue;
            }
            if ( facesHaveImproperIntersection(faces[i], faces[j],
                     geometry[i], geometry[j], numericContext, msg) ) {
                ok = FALSE;
                break;
            }
        }
    }
    PolyhedralBoundedSolidFaceGeometry::dispose(geometry);
    return ok;
}

double PolyhedralBoundedSolidGeometricValidator::solidDiagonal(
    PolyhedralBoundedSolid* solid)
{
    double* box = solid->getMinMax();
    double diagonal = sqrt((box[3] - box[0]) * (box[3] - box[0]) +
        (box[4] - box[1]) * (box[4] - box[1]) +
        (box[5] - box[2]) * (box[5] - box[2]));
    delete[] box;
    return diagonal;
}

/**
Area below which a face is collapsed: VSDK.EPSILON scaled by the square
of the model scale, so that it equals EPSILON for models of unit size and
is invariant under a uniform change of units.
*/
double PolyhedralBoundedSolidGeometricValidator::collapsedFaceAreaThreshold(
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext)
{
    return VSDK::EPSILON * numericContext.modelScale() *
        numericContext.modelScale();
}

/**
Area of a face: half the length of the Newell vector summed over all its
loops, so inner loops (holes, oriented the other way) are subtracted.
*/
double PolyhedralBoundedSolidGeometricValidator::faceArea(
    _PolyhedralBoundedSolidFace* face)
{
    double nx = 0.0;
    double ny = 0.0;
    double nz = 0.0;
    long int i;

    for ( i = 0; i < face->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(i);
        _PolyhedralBoundedSolidHalfEdge* start;
        _PolyhedralBoundedSolidHalfEdge* he;
        if ( loop == 0 || loop->boundaryStartHalfEdge == 0 ) {
            continue;
        }
        start = loop->boundaryStartHalfEdge;
        he = start;
        do {
            _PolyhedralBoundedSolidHalfEdge* next = he->next();
            if ( next == 0 ) {
                break;
            }
            Vector3Dd p = he->startingVertex->position;
            Vector3Dd q = next->startingVertex->position;
            nx += (p.y() - q.y()) * (p.z() + q.z());
            ny += (p.z() - q.z()) * (p.x() + q.x());
            nz += (p.x() - q.x()) * (p.y() + q.y());
            he = next;
        } while ( he != start );
    }
    return 0.5 * sqrt(nx * nx + ny * ny + nz * nz);
}

/**
Rejects collapsed laminae. A lamina is a shell made of exactly two faces
(which then share all their edges, back to back); it has zero volume and
is a valid result. It is collapsed when its area is below
collapsedFaceAreaThreshold. Small faces of shells that enclose volume
(slivers of a tessellation) are not judged by this rule.
*/
BOOLEAN PolyhedralBoundedSolidGeometricValidator::validateNoCollapsedLaminae(
    PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
    String* msg)
{
    double threshold = collapsedFaceAreaThreshold(numericContext);
    // C++ port note: an identity TreeMap stands for Java's IdentityHashMap
    TreeMap<_PolyhedralBoundedSolidFace*, int> shellOf;
    ArrayList<ArrayList<_PolyhedralBoundedSolidFace*>*> shells;
    BOOLEAN ok = TRUE;
    long int i;
    long int j;
    long int k;

    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* seed = solid->getPolygonsList().get(i);
        if ( shellOf.containsKey(seed) ) {
            continue;
        }
        ArrayList<_PolyhedralBoundedSolidFace*>* shell =
            new ArrayList<_PolyhedralBoundedSolidFace*>();
        ArrayDeque<_PolyhedralBoundedSolidFace*> pending;
        shellOf.put(seed, (int)shells.size());
        pending.addLast(seed);
        while ( !pending.isEmpty() ) {
            _PolyhedralBoundedSolidFace* face = pending.pollFirst();
            shell->add(face);
            for ( j = 0; j < face->boundariesList.size(); j++ ) {
                _PolyhedralBoundedSolidLoop* loop =
                    face->boundariesList.get(j);
                for ( k = 0; k < loop->halfEdgesList.size(); k++ ) {
                    _PolyhedralBoundedSolidHalfEdge* he =
                        loop->halfEdgesList.get(k);
                    _PolyhedralBoundedSolidHalfEdge* mate = 0;
                    if ( he->parentEdge != 0 ) {
                        mate = he->mirrorHalfEdge();
                    }
                    if ( mate == 0 || mate->parentLoop == 0 ) {
                        continue;
                    }
                    _PolyhedralBoundedSolidFace* other =
                        mate->parentLoop->parentFace;
                    if ( other != 0 && !shellOf.containsKey(other) ) {
                        shellOf.put(other, (int)shells.size());
                        pending.addLast(other);
                    }
                }
            }
        }
        shells.add(shell);
    }

    for ( i = 0; i < shells.size(); i++ ) {
        ArrayList<_PolyhedralBoundedSolidFace*>* shell = shells.get(i);
        if ( shell->size() != 2 ) {
            continue;
        }
        double area = faceArea(shell->get(0));
        if ( area < threshold ) {
            append(msg, "  - Lamina of faces [" + str(shell->get(0)->id) +
                "] and [" + str(shell->get(1)->id) + "] is collapsed: area " +
                Double::toString(area) + " < " + Double::toString(threshold) +
                ".\n");
            ok = FALSE;
        }
    }
    for ( i = 0; i < shells.size(); i++ ) {
        delete shells.get(i);
    }
    return ok;
}

BOOLEAN PolyhedralBoundedSolidGeometricValidator::validateNoCoincidentVertices(
    PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& context,
    String* msg)
{
    long int i;
    long int j;
    BOOLEAN ok;
    _PolyhedralBoundedSolidVertex* vi;
    _PolyhedralBoundedSolidVertex* vj;

    ok = TRUE;
    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        vi = solid->getVerticesList().get(i);
        if ( vi == 0 ) {
            continue;
        }
        for ( j = i + 1; j < solid->getVerticesList().size(); j++ ) {
            vj = solid->getVerticesList().get(j);
            if ( vj == 0 ) {
                continue;
            }
            if ( PolyhedralBoundedSolidNumericPolicy::pointsCoincident(
                     vi->position, vj->position, context) ) {
                String* position = vi->position.toString();
                append(msg, "  coincident vertices: v" + str(vi->id) +
                    " and v" + str(vj->id) + " at " + position->c_str() + "\n");
                delete position;
                ok = FALSE;
            }
        }
    }
    return ok;
}

BOOLEAN PolyhedralBoundedSolidGeometricValidator::validateUniqueFaceAndVertexIds(
    PolyhedralBoundedSolid* solid, String* msg)
{
    BOOLEAN ok;
    long int i;
    long int j;
    _PolyhedralBoundedSolidFace* fi;
    _PolyhedralBoundedSolidFace* fj;
    _PolyhedralBoundedSolidVertex* vi;
    _PolyhedralBoundedSolidVertex* vj;

    ok = TRUE;

    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        fi = solid->getPolygonsList().get(i);
        if ( fi == 0 ) {
            continue;
        }
        if ( fi->id > solid->getMaxFaceId() ) {
            append(msg, "  face id " + str(fi->id) + " exceeds maxFaceId=" +
                str(solid->getMaxFaceId()) + "\n");
            ok = FALSE;
        }
        for ( j = i + 1; j < solid->getPolygonsList().size(); j++ ) {
            fj = solid->getPolygonsList().get(j);
            if ( fj != 0 && fi->id == fj->id ) {
                append(msg, "  duplicate face id=" + str(fi->id) + "\n");
                ok = FALSE;
            }
        }
    }

    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        vi = solid->getVerticesList().get(i);
        if ( vi == 0 ) {
            continue;
        }
        if ( vi->id > solid->getMaxVertexId() ) {
            append(msg, "  vertex id " + str(vi->id) + " exceeds maxVertexId=" +
                str(solid->getMaxVertexId()) + "\n");
            ok = FALSE;
        }
        for ( j = i + 1; j < solid->getVerticesList().size(); j++ ) {
            vj = solid->getVerticesList().get(j);
            if ( vj != 0 && vi->id == vj->id ) {
                append(msg, "  duplicate vertex id=" + str(vi->id) + "\n");
                ok = FALSE;
            }
        }
    }

    return ok;
}

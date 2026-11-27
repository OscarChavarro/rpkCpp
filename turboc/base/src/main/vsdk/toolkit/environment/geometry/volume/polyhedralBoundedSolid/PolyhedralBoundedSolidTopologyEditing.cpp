//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =
//= [.wMANT2008] Mantyla Martti. "Personal Home Page", <<shar> > archive     =
//=     containing the C programs from [MANT1988]. Available at             =
//=     http://www.cs.hut.fi/~mam . Last visited April 12 / 2008.           =


#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidRepair.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidGeometricValidator.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidTopologyEditing.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidBooleanTopologyPredicates.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidTopologicalValidator.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
#include "vsdk/toolkit/common/Cpp98Support.h"


typedef PolyhedralBoundedSolidNumericPolicy::ToleranceContext ToleranceContext;

static const char* const CLASS_NAME = "PolyhedralBoundedSolidTopologyEditing";

static String str(int value)
{
    return String(Cpp98Support::toString(value).c_str());
}

static void remakeLoopBoundaryStartHalfEdgesReferences(PolyhedralBoundedSolid* solid)
{
    long int i;
    long int j;

    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        for ( j = 0; j < face->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(j);
            loop->parentFace = face;
            if ( loop->halfEdgesList.size() > 0 ) {
                loop->boundaryStartHalfEdge = loop->halfEdgesList.get(0);
            }
            else {
                loop->boundaryStartHalfEdge = 0;
            }
        }
    }
}

/**
Finds a pair of coincident vertices between two loops so `loopGlue` can
start from a geometrically meaningful bridge instead of assuming each loop
starts at the matching vertex.

This helper is not part of the original [MANT1988] text; it was added to
make the implementation more robust when intermediate Boolean topology
leaves valid loops with arbitrary boundary start half-edges.
@param solid target solid instance
@param first start half-edge of the first loop
@param second start half-edge of the second loop
@param outH1 receives the matching half-edge of the first loop
@param outH2 receives the matching half-edge of the second loop
@return true when a match exists (Java returns a non null pair)
*/
static BOOLEAN findMatchingLoopVertices(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* first,
    _PolyhedralBoundedSolidHalfEdge* second,
    _PolyhedralBoundedSolidHalfEdge** outH1,
    _PolyhedralBoundedSolidHalfEdge** outH2)
{
    ToleranceContext numericContext =
        PolyhedralBoundedSolidNumericPolicy::forSolid(solid);
    if ( first == 0 || second == 0 ) {
        return FALSE;
    }

    _PolyhedralBoundedSolidHalfEdge* h1 = first;
    do {
        _PolyhedralBoundedSolidHalfEdge* h2 = second;
        do {
            if ( h1->vertexPositionMatch(h2, numericContext.bigEpsilon()) ) {
                *outH1 = h1;
                *outH2 = h2;
                return TRUE;
            }
            h2 = h2->next();
        } while ( h2 != second );
        h1 = h1->next();
    } while ( h1 != first );

    return FALSE;
}

/**
Detects loops that do not contain enough distinct geometric vertices to
describe an area.

This helper is not part of the original [MANT1988] text; it was added to
make the implementation more robust when cleanup stages produce collapsed
loops that should be removed instead of glued as regular rings.
@param solid target solid instance
@param loop loop to evaluate
@return true when the loop is degenerate, false otherwise
*/
static BOOLEAN isDegenerateLoop(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidLoop* loop)
{
    if ( loop == 0 || loop->halfEdgesList.size() < 3 ) {
        return TRUE;
    }

    ToleranceContext numericContext =
        PolyhedralBoundedSolidNumericPolicy::forSolid(solid);
    int distinctCount = 0;
    long int i;
    for ( i = 0; i < loop->halfEdgesList.size(); i++ ) {
        long int j;
        for ( j = 0; j < i; j++ ) {
            if ( PolyhedralBoundedSolidNumericPolicy::pointsCoincident(
                    loop->halfEdgesList.get(i)->startingVertex->position,
                    loop->halfEdgesList.get(j)->startingVertex->position,
                    numericContext) ) {
                // Self-touching loop: vertex i shares position with vertex j.
                // This creates a figure-8 boundary that cannot bound a valid face.
                Logger::reportMessage(CLASS_NAME, Logger::WARNING,
                    "isDegenerateLoop",
                    String("finish: skipped degenerate loop with self-touching boundary ") +
                    "(vertex " + str(loop->halfEdgesList.get(i)->startingVertex->id) +
                    " coincides with vertex " +
                    str(loop->halfEdgesList.get(j)->startingVertex->id) + ")");
                return TRUE;
            }
        }
        distinctCount++;
    }
    // A valid face boundary needs at least 3 geometrically distinct vertices.
    return distinctCount < 3;
}

/**
Removes a loop reference from a face without applying a full Euler
operator, for cases where the loop has already collapsed geometrically.

This helper is not part of the original [MANT1988] text; it was added to
make the implementation more robust around degenerate intermediate loops
created by Boolean cleanup.
@param face owner face
@param loop loop to remove
*/
static void removeLoop(_PolyhedralBoundedSolidFace* face,
    _PolyhedralBoundedSolidLoop* loop)
{
    long int i;
    for ( i = 0; i < face->boundariesList.size(); i++ ) {
        if ( face->boundariesList.get(i) == loop ) {
            face->boundariesList.remove(i);
            return;
        }
    }
}

static BOOLEAN planesCoincidentIgnoringOrientation(
    const InfinitePlane* a,
    const InfinitePlane* b,
    double tolerance)
{
    return _PolyhedralBoundedSolidBooleanTopologyPredicates
        ::planesCoincidentIgnoringOrientation(a, b, tolerance);
}

static BOOLEAN loopsCoincident(
    _PolyhedralBoundedSolidLoop* a,
    _PolyhedralBoundedSolidLoop* b,
    const ToleranceContext& numericContext)
{
    return _PolyhedralBoundedSolidBooleanTopologyPredicates
        ::loopsCoincident(a, b, numericContext);
}

/**
Detects the duplicated coplanar-ring configuration that can appear after
boolean result integration. The reorganization is meant to expose the
pair of coincident loops that `loopglue` consumes in section
[MANT1988].12.4.2, as required by the maximal-face cleanup from section
[MANT1988].15.5 and the finishing stage of program [MANT1988].15.15.
@param solid target solid instance
@param multiLoopFace multi-loop candidate face
@param simpleFace single-loop candidate face
@param numericContext numeric tolerance context
@return true when a reduction was applied, false otherwise
*/
static BOOLEAN reduceCoincidentSimpleFaceOnMultiLoopFace(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidFace* multiLoopFace,
    _PolyhedralBoundedSolidFace* simpleFace,
    const ToleranceContext& numericContext)
{
    ArrayList<_PolyhedralBoundedSolidLoop*> coincidentLoops;
    ArrayList<_PolyhedralBoundedSolidLoop*> loopsToMove;
    _PolyhedralBoundedSolidLoop* simpleLoop;
    _PolyhedralBoundedSolidLoop* outerGlueLoop;
    _PolyhedralBoundedSolidLoop* duplicateGlueLoop;
    long int i;

    if ( multiLoopFace == 0 || simpleFace == 0 ||
         multiLoopFace == simpleFace ) {
        return FALSE;
    }
    if ( multiLoopFace->boundariesList.size() < 2 ||
         simpleFace->boundariesList.size() != 1 ) {
        return FALSE;
    }
    InfinitePlane* multiLoopPlane = multiLoopFace->getContainingPlane();
    InfinitePlane* simplePlane = simpleFace->getContainingPlane();
    BOOLEAN coincidentPlanes = multiLoopPlane != 0 && simplePlane != 0 &&
        planesCoincidentIgnoringOrientation(multiLoopPlane, simplePlane,
            numericContext.epsilon());
    delete multiLoopPlane;
    delete simplePlane;
    if ( !coincidentPlanes ) {
        return FALSE;
    }

    simpleLoop = simpleFace->boundariesList.get(0);
    for ( i = 0; i < multiLoopFace->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* candidate;
        candidate = multiLoopFace->boundariesList.get(i);
        if ( loopsCoincident(candidate, simpleLoop, numericContext) ) {
            coincidentLoops.add(candidate);
        }
    }

    if ( coincidentLoops.size() < 2 ) {
        return FALSE;
    }

    outerGlueLoop = coincidentLoops.get(0);
    duplicateGlueLoop = coincidentLoops.get(1);
    for ( i = 0; i < multiLoopFace->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* candidate;
        candidate = multiLoopFace->boundariesList.get(i);
        if ( candidate != outerGlueLoop && candidate != duplicateGlueLoop ) {
            loopsToMove.add(candidate);
        }
    }

    if ( loopsToMove.size() == 0 ) {
        return FALSE;
    }

    for ( i = 0; i < loopsToMove.size(); i++ ) {
        if ( !PolyhedralBoundedSolidEulerOperators::lringmv(
                solid, loopsToMove.get(i), simpleFace, FALSE) ) {
            return FALSE;
        }
    }

    if ( multiLoopFace->boundariesList.size() != 2 ) {
        return FALSE;
    }

    PolyhedralBoundedSolidEulerOperators::lringmv(
        solid, outerGlueLoop, multiLoopFace, TRUE);
    PolyhedralBoundedSolidTopologyEditing::loopGlue(solid, multiLoopFace->id);
    return TRUE;
}

static int countCoincidentLoops(
    _PolyhedralBoundedSolidFace* multiLoopFace,
    _PolyhedralBoundedSolidFace* simpleFace,
    const ToleranceContext& numericContext)
{
    long int i;
    int coincidentCount;
    _PolyhedralBoundedSolidLoop* simpleLoop;

    if ( multiLoopFace == 0 || simpleFace == 0 ||
         multiLoopFace->boundariesList.size() < 2 ||
         simpleFace->boundariesList.size() != 1 ) {
        return 0;
    }

    simpleLoop = simpleFace->boundariesList.get(0);
    coincidentCount = 0;
    for ( i = 0; i < multiLoopFace->boundariesList.size(); i++ ) {
        if ( loopsCoincident(multiLoopFace->boundariesList.get(i), simpleLoop,
                numericContext) ) {
            coincidentCount++;
        }
    }
    return coincidentCount;
}

static BOOLEAN hasCoincidentMultiLoopReductionCandidate(
    _PolyhedralBoundedSolidFace* faceA,
    _PolyhedralBoundedSolidFace* faceB,
    const ToleranceContext& numericContext)
{
    return countCoincidentLoops(faceA, faceB, numericContext) >= 2 ||
           countCoincidentLoops(faceB, faceA, numericContext) >= 2;
}

static void removeEdgeRecord(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidEdge* edge)
{
    long int i;

    if ( edge == 0 ) {
        return;
    }

    for ( i = 0; i < solid->getEdgesList().size(); i++ ) {
        if ( solid->getEdgesList().get(i) == edge ) {
            solid->getEdgesList().remove(i);
            return;
        }
    }
}

static void removeEmptyLoopAndFaceIfNeeded(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidFace* face,
    _PolyhedralBoundedSolidLoop* loop)
{
    if ( face == 0 || loop == 0 ) {
        return;
    }

    if ( loop->halfEdgesList.size() == 0 ) {
        face->boundariesList.locateWindowAtElem(loop);
        face->boundariesList.removeElemAtWindow();
    }

    if ( face->boundariesList.size() == 0 ) {
        solid->getPolygonsList().locateWindowAtElem(face);
        solid->getPolygonsList().removeElemAtWindow();
    }
}

/**
Repairs a duplicated geometric strut represented as two consecutive
topological edges over the same geometric segment, each mirrored on
different faces.

This helper is not part of the original [MANT1988] text; it was added to
make the implementation more robust when `maximizeFaces` encounters the
kind of duplicated geometric strut that can survive Boolean connect/finish.
@param solid target solid instance
@param first first consecutive half-edge
@param second second consecutive half-edge
@param iteration current cleanup iteration
@return true when a repair was applied, false otherwise
*/
static BOOLEAN zipConsecutiveGeometricDanglingEdgePair(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* first,
    _PolyhedralBoundedSolidHalfEdge* second,
    int iteration)
{
    _PolyhedralBoundedSolidHalfEdge* firstMirror;
    _PolyhedralBoundedSolidHalfEdge* secondMirror;
    _PolyhedralBoundedSolidEdge* keptEdge;
    _PolyhedralBoundedSolidEdge* droppedEdge;
    _PolyhedralBoundedSolidLoop* degenerateLoop;
    _PolyhedralBoundedSolidFace* degenerateFace;

    (void)iteration;
    firstMirror = first->mirrorHalfEdge();
    secondMirror = second->mirrorHalfEdge();
    if ( firstMirror == 0 || secondMirror == 0 ||
         firstMirror->parentLoop == 0 || secondMirror->parentLoop == 0 ||
         firstMirror->parentLoop->parentFace == 0 ||
         secondMirror->parentLoop->parentFace == 0 ||
         firstMirror->parentLoop->parentFace ==
         secondMirror->parentLoop->parentFace ) {
        return FALSE;
    }

    keptEdge = first->parentEdge;
    droppedEdge = second->parentEdge;
    degenerateLoop = first->parentLoop;
    degenerateFace = degenerateLoop->parentFace;

    degenerateLoop->unlistHalfEdge(first);
    degenerateLoop->unlistHalfEdge(second);

    removeEdgeRecord(solid, droppedEdge);
    keptEdge->rightHalf = firstMirror;
    keptEdge->leftHalf = secondMirror;
    firstMirror->parentEdge = keptEdge;
    secondMirror->parentEdge = keptEdge;

    PolyhedralBoundedSolidEulerOperators::lkef(solid, firstMirror, secondMirror);
    removeEmptyLoopAndFaceIfNeeded(solid, degenerateFace, degenerateLoop);
    return TRUE;
}

/**
Searches the solid for a duplicated geometric strut that can be repaired
by `zipConsecutiveGeometricDanglingEdgePair`.

This helper is not part of the original [MANT1988] text; it was added to
make the implementation more robust by broadening maximal-face cleanup
beyond the exact topological cases described in the book.
@param solid target solid instance
@param numericContext numeric tolerance context
@param iteration current cleanup iteration
@return true when a repair was applied, false otherwise
*/
static BOOLEAN zipConsecutiveGeometricDanglingEdgePairs(
    PolyhedralBoundedSolid* solid,
    const ToleranceContext& numericContext,
    int iteration)
{
    long int i;
    long int j;
    _PolyhedralBoundedSolidHalfEdge* first;
    _PolyhedralBoundedSolidHalfEdge* second;
    _PolyhedralBoundedSolidHalfEdge* firstMirror;
    _PolyhedralBoundedSolidHalfEdge* secondMirror;

    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        for ( j = 0; j < face->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(j);
            long int k;

            if ( loop->halfEdgesList.size() < 2 ) {
                continue;
            }

            for ( k = 0; k < loop->halfEdgesList.size(); k++ ) {
                first = loop->halfEdgesList.get(k);
                second = first->next();
                if ( second == 0 || first == second ||
                     first->parentEdge == 0 ||
                     second->parentEdge == 0 ||
                     first->parentEdge == second->parentEdge ||
                     first->startingVertex == 0 ||
                     second->startingVertex == 0 ) {
                    continue;
                }

                if ( !PolyhedralBoundedSolidNumericPolicy::pointsCoincident(
                        first->startingVertex->position,
                        second->startingVertex->position,
                        numericContext) ) {
                    continue;
                }

                firstMirror = first->mirrorHalfEdge();
                secondMirror = second->mirrorHalfEdge();
                if ( firstMirror == 0 || secondMirror == 0 ||
                     firstMirror->parentLoop == 0 ||
                     secondMirror->parentLoop == 0 ||
                     firstMirror->parentLoop->parentFace == 0 ||
                     secondMirror->parentLoop->parentFace == 0 ||
                     firstMirror->parentLoop->parentFace ==
                     secondMirror->parentLoop->parentFace ) {
                    continue;
                }

                InfinitePlane* firstMirrorPlane =
                    firstMirror->parentLoop->parentFace->getContainingPlane();
                InfinitePlane* secondMirrorPlane =
                    secondMirror->parentLoop->parentFace->getContainingPlane();
                BOOLEAN coincident = firstMirrorPlane != 0 &&
                    secondMirrorPlane != 0 &&
                    planesCoincidentIgnoringOrientation(
                        firstMirrorPlane,
                        secondMirrorPlane,
                        numericContext.epsilon());
                delete firstMirrorPlane;
                delete secondMirrorPlane;
                if ( !coincident ) {
                    continue;
                }

                return zipConsecutiveGeometricDanglingEdgePair(
                    solid, first, second, iteration);
            }
        }
    }
    return FALSE;
}

static void remakeEmanatingHalfedgesReferences(PolyhedralBoundedSolid* solid)
{
    PolyhedralBoundedSolidRepair::remakeEmanatingHalfEdgeReferences(solid);
}

static void appendLoopPoints(_PolyhedralBoundedSolidHalfEdge* start,
    ArrayList<Vector3Dd>& points)
{
    _PolyhedralBoundedSolidHalfEdge* cur = start;
    if ( cur != 0 ) {
        do {
            if ( cur->startingVertex != 0 ) {
                points.add(cur->startingVertex->position);
            }
            cur = cur->next();
        } while ( cur != 0 && cur != start );
    }
}

/**
Section 9.3 planarity guard: collects vertices from both loops of an edge's
two adjacent faces and checks that all combined points are coplanar.
Prevents `maximizeFaces` from merging two faces whose union would be
non-planar even though their individual planes nominally overlap.
*/
static BOOLEAN wouldMergedFaceBeCoplanar(
    _PolyhedralBoundedSolidHalfEdge* rightHalf,
    _PolyhedralBoundedSolidHalfEdge* leftHalf,
    const ToleranceContext& numericContext)
{
    ArrayList<Vector3Dd> points;

    appendLoopPoints(rightHalf->parentLoop->boundaryStartHalfEdge, points);
    appendLoopPoints(leftHalf->parentLoop->boundaryStartHalfEdge, points);
    return PolyhedralBoundedSolidGeometricValidator
        ::validateFacePointsAreCoplanar(points, numericContext);
}

/**
Bounding box diagonal of the semiloop that starts after `heStart` and
stops at `heStop`, with the box seeded inverted from the solid min-max
values (as in the Java version).
*/
static double semiloopExtent(
    const double* minmax,
    _PolyhedralBoundedSolidHalfEdge* heStart,
    _PolyhedralBoundedSolidHalfEdge* heStop)
{
    Vector3Dd min(minmax[3], minmax[4], minmax[5]);
    Vector3Dd max(minmax[0], minmax[1], minmax[2]);
    Vector3Dd p;
    _PolyhedralBoundedSolidHalfEdge* he = heStart;

    do {
        he = he->next();
        if ( he == 0 ) {
            // Loop is not closed!
            break;
        }
        p = he->startingVertex->position;
        if ( p.x() > max.x() ) max = max.withX(p.x());
        if ( p.y() > max.y() ) max = max.withY(p.y());
        if ( p.z() > max.z() ) max = max.withZ(p.z());
        if ( p.x() < min.x() ) min = min.withX(p.x());
        if ( p.y() < min.y() ) min = min.withY(p.y());
        if ( p.z() < min.z() ) min = min.withZ(p.z());
    } while ( he != heStart && he != heStop );
    return max.subtract(min).length();
}


void PolyhedralBoundedSolidTopologyEditing::loopGlue(PolyhedralBoundedSolid* solid, int faceId)
{
    _PolyhedralBoundedSolidFace* face;

    face = solid->findFace(faceId);
    if ( face == 0 ) {
        Logger::reportMessage(CLASS_NAME, Logger::WARNING, "loopGlue",
            String("Face ") + str(faceId) + " not found.");
        return;
    }
    loopGlue(solid, face);
}

void PolyhedralBoundedSolidTopologyEditing::loopGlue(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidFace* face)
{
    if ( face == 0 ) {
        Logger::reportMessage(CLASS_NAME, Logger::WARNING, "loopGlue",
            "Null face received.");
        return;
    }
    if ( face->boundariesList.size() < 2 ) {
        Logger::reportMessage(CLASS_NAME, Logger::WARNING, "loopGlue",
            String("Face ") + str(face->id) +
            " does not contain at least two loops.");
        return;
    }

    //-----------------------------------------------------------------
    _PolyhedralBoundedSolidHalfEdge* h1 = 0;
    _PolyhedralBoundedSolidHalfEdge* h2 = 0;
    _PolyhedralBoundedSolidHalfEdge* h1next;

    BOOLEAN gluePairFound = FALSE;
    long int i;
    long int j;
    for ( i = 0; i < face->boundariesList.size() && !gluePairFound; i++ ) {
        for ( j = i + 1; j < face->boundariesList.size(); j++ ) {
            gluePairFound = findMatchingLoopVertices(
                solid,
                face->boundariesList.get(i)->boundaryStartHalfEdge,
                face->boundariesList.get(j)->boundaryStartHalfEdge,
                &h1, &h2);
            if ( gluePairFound ) {
                break;
            }
        }
    }

    if ( !gluePairFound ) {
        Logger::reportMessage(CLASS_NAME, Logger::WARNING, "loopGlue",
            "No matching starting vertex found between candidate loops.");
        return;
    }

    if ( isDegenerateLoop(solid, h1->parentLoop) &&
         isDegenerateLoop(solid, h2->parentLoop) ) {
        removeLoop(face, h1->parentLoop);
        removeLoop(face, h2->parentLoop);
        remakeLoopBoundaryStartHalfEdgesReferences(solid);
        return;
    }
    // Section 9.5: A degenerate ring (size < 3) cannot be bridged by lmekr
    // without creating a self-loop edge. The gluePair search may return
    // either loop as h1 or h2 - check both to cover the swapped case.
    if ( isDegenerateLoop(solid, h2->parentLoop) ) {
        removeLoop(face, h2->parentLoop);
        remakeLoopBoundaryStartHalfEdgesReferences(solid);
        return;
    }
    if ( isDegenerateLoop(solid, h1->parentLoop) ) {
        removeLoop(face, h1->parentLoop);
        remakeLoopBoundaryStartHalfEdgesReferences(solid);
        return;
    }

    PolyhedralBoundedSolidEulerOperators::lmekr(solid, h1, h2);
    PolyhedralBoundedSolidEulerOperators::lkev(
        solid, h1->previous(), h2->previous());

    while ( h1->next() != h2 ) {
        h1next = h1->next();
        PolyhedralBoundedSolidEulerOperators::lmef(
            solid,
            h1->next(),
            h1->previous(),
            solid->getMaxFaceId() + 1);
        PolyhedralBoundedSolidEulerOperators::lkev(
            solid,
            h1->next(),
            (h1->next())->mirrorHalfEdge());
        PolyhedralBoundedSolidEulerOperators::lkef(
            solid,
            h1->mirrorHalfEdge(),
            h1);
        h1 = h1next;
    }
    PolyhedralBoundedSolidEulerOperators::lkef(
        solid,
        h1->mirrorHalfEdge(),
        h1);
    remakeLoopBoundaryStartHalfEdgesReferences(solid);
}

void PolyhedralBoundedSolidTopologyEditing::compactIds(PolyhedralBoundedSolid* solid)
{
    long int i;
    long int j;

    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        solid->getVerticesList().get(i)->id = (int)(i + 1);
    }
    solid->setMaxVertexId((int)i);
    for ( i = 0; i < solid->getEdgesList().size(); i++ ) {
        solid->getEdgesList().get(i)->id = (int)(i + 1);
    }

    int k = 1;
    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        face->id = (int)(i + 1);
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
                he->id = k;
                k++;
                he = he->next();
                if ( he == 0 ) {
                    break;
                }
            } while ( he != heStart );
        }
    }
    solid->setMaxFaceId((int)i);
}

void PolyhedralBoundedSolidTopologyEditing::maximizeFaces(PolyhedralBoundedSolid* solid)
{
    long int i;
    long int j;
    _PolyhedralBoundedSolidEdge* e;
    _PolyhedralBoundedSolidHalfEdge* he;
    Vector3Dd p0;
    Vector3Dd p1;
    Vector3Dd p2;
    _PolyhedralBoundedSolidHalfEdge* heStart;
    BOOLEAN restart;
    int iteration;

    restart = TRUE;
    iteration = 0;
    while ( restart ) {
        restart = FALSE;
        iteration++;
        ToleranceContext numericContext =
            PolyhedralBoundedSolidNumericPolicy::forSolid(solid);
        remakeEmanatingHalfedgesReferences(solid);
        //- Collapse residual line-faces --------------------------------
        for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
            _PolyhedralBoundedSolidFace* face;
            _PolyhedralBoundedSolidLoop* loop;
            _PolyhedralBoundedSolidHalfEdge* collapseHe;
            _PolyhedralBoundedSolidHalfEdge* neighborHe;

            face = solid->getPolygonsList().get(i);
            if ( face->boundariesList.size() != 1 ) {
                continue;
            }
            loop = face->boundariesList.get(0);
            if ( loop->halfEdgesList.size() != 2 ) {
                continue;
            }
            collapseHe = loop->boundaryStartHalfEdge;
            if ( collapseHe == 0 ) {
                continue;
            }
            neighborHe = collapseHe->mirrorHalfEdge();
            if ( collapseHe->parentEdge == 0 || neighborHe == 0 ||
                 neighborHe->parentLoop == 0 ||
                 neighborHe->parentLoop->parentFace == 0 ||
                 neighborHe->parentLoop->parentFace == face ) {
                continue;
            }
            PolyhedralBoundedSolidEulerOperators::lkef(
                solid, collapseHe, neighborHe);
            restart = TRUE;
            break;
        }
        if ( restart ) {
            continue;
        }

        //- Eliminate null edges --------------------------------------
        for ( i = 0; i < solid->getEdgesList().size(); i++ ) {
            e = solid->getEdgesList().get(i);
            p1 = e->rightHalf->startingVertex->position;
            p2 = e->leftHalf->startingVertex->position;
            if ( PolyhedralBoundedSolidNumericPolicy
                ::pointsCoincident(p1, p2, numericContext) ) {
                PolyhedralBoundedSolidEulerOperators::lkev(
                    solid, e->rightHalf, e->leftHalf);
                restart = TRUE;
                break;
            }
        }
        if ( restart ) {
            continue;
        }

        //- Zip duplicated geometric struts ---------------------------
        if ( zipConsecutiveGeometricDanglingEdgePairs(
                solid, numericContext, iteration) ) {
            restart = TRUE;
            continue;
        }

        //- Join coplanar faces ---------------------------------------
        for ( i = 0; i < solid->getEdgesList().size(); i++ ) {
            e = solid->getEdgesList().get(i);
            InfinitePlane* a =
                e->rightHalf->parentLoop->parentFace->getContainingPlane();
            InfinitePlane* b =
                e->leftHalf->parentLoop->parentFace->getContainingPlane();
            BOOLEAN planesOverlap = a != 0 && b != 0 &&
                a->overlapsWith(*b, numericContext.epsilon());
            delete a;
            delete b;
            if ( e->rightHalf->parentLoop->parentFace ==
                 e->leftHalf->parentLoop->parentFace &&
                 e->rightHalf->parentLoop != e->leftHalf->parentLoop ) {
                // Case 1: need to remove an edge separating to
                // different coplanar faces (join faces). Order doesn't
                // matter.
                PolyhedralBoundedSolidEulerOperators::lkemr(
                    solid, e->rightHalf, e->leftHalf);
                restart = TRUE;
                break;
            }
            else if ( planesOverlap &&
                      e->rightHalf->parentLoop != e->leftHalf->parentLoop ) {
                if ( hasCoincidentMultiLoopReductionCandidate(
                         e->rightHalf->parentLoop->parentFace,
                         e->leftHalf->parentLoop->parentFace,
                         numericContext) ) {
                    continue;
                }
                // Section 9.3 guard: even though both face planes overlap,
                // the merged polygon may be non-planar due to floating-point
                // drift (e.g. triangulated curved surfaces). Skip if the
                // combined vertex set is not coplanar.
                if ( !wouldMergedFaceBeCoplanar(
                         e->rightHalf, e->leftHalf, numericContext) ) {
                    continue;
                }
                PolyhedralBoundedSolidEulerOperators::lkef(
                    solid, e->rightHalf, e->leftHalf);
                restart = TRUE;
                break;
            }
            else if ( e->rightHalf->parentLoop->parentFace ==
                      e->leftHalf->parentLoop->parentFace &&
                      e->rightHalf->parentLoop == e->leftHalf->parentLoop &&
                      (e->leftHalf == e->rightHalf->next() ||
                       e->rightHalf == e->leftHalf->next()) ) {
                // Case 3:. Need to remove a dangling edge, with two
                // half-edges lying over the same face. Do not remove any
                // face, rather, remove the dangling edge and its dangling
                // vertex. To test, use object from figure [MANT1988].15.1.
                // or code from SimpleTestGeometryLibrary method
                // createTestObjectPairMANT1988_15_1
                if ( e->leftHalf == e->rightHalf->next() ) {
                    heStart = e->leftHalf;
                }
                else {
                    heStart = e->rightHalf;
                }
                PolyhedralBoundedSolidEulerOperators::lkev(
                    solid, heStart, heStart->mirrorHalfEdge());
                restart = TRUE;
                break;
            }
            else if ( e->rightHalf->parentLoop->parentFace ==
                      e->leftHalf->parentLoop->parentFace &&
                      e->rightHalf->parentLoop == e->leftHalf->parentLoop &&
                      (e->leftHalf != e->rightHalf->next() &&
                       e->rightHalf != e->leftHalf->next()) ) {
                // Case 4. Need to remove an edge on a self-intersecting
                // loop, causing that loop to break on two rings. To test
                // use "buildCsgTest4" pair on
                // PolyhedralBoundedSolidModelingTools testsuite program
                // (union of two L-shaped boxes to form a hollowed brick).
                // It is important to break the loops in such a way that
                // bigger loop be the first loop, and smaller loop is the
                // inner ring.
                double* minmax = solid->getMinMax();

                // Estimate the size of semiloop starting at e.leftHalf
                double leftDistance =
                    semiloopExtent(minmax, e->leftHalf, e->rightHalf);

                // Estimate the size of semiloop starting at e.rightHalf
                double rightDistance =
                    semiloopExtent(minmax, e->rightHalf, e->leftHalf);
                delete[] minmax;

                // Determine outer loop acording to major extent
                _PolyhedralBoundedSolidHalfEdge* heOuter;
                _PolyhedralBoundedSolidHalfEdge* heInner;

                if ( leftDistance > rightDistance ) {
                    heOuter = e->leftHalf;
                    heInner = e->rightHalf;
                }
                else {
                    heOuter = e->rightHalf;
                    heInner = e->leftHalf;
                }
                PolyhedralBoundedSolidEulerOperators::lkemr(
                    solid, heInner, heOuter);
                restart = TRUE;
                break;
            }
        }
        if ( restart ) {
            continue;
        }

        //- Merge coplanar overlapping faces when one lies entirely over
        //- another that already carries rings. This completes the
        //- "maximal face" reduction expected by [MANT1988].15.5.
        // Which faces have a plane is computed once per sweep (Stage G):
        // the sweep restarts as soon as a reduction changes the solid.
        ArrayList<BOOLEAN> hasPlane;
        for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
            InfinitePlane* plane =
                solid->getPolygonsList().get(i)->getContainingPlane();
            hasPlane.add(plane != 0);
            delete plane;
        }
        for ( i = 0; i < solid->getPolygonsList().size() && !restart; i++ ) {
            _PolyhedralBoundedSolidFace* faceA = solid->getPolygonsList().get(i);
            if ( !hasPlane[i] ) {
                continue;
            }
            for ( j = i + 1; j < solid->getPolygonsList().size(); j++ ) {
                _PolyhedralBoundedSolidFace* faceB = solid->getPolygonsList().get(j);
                if ( !hasPlane[j] ) {
                    continue;
                }

                if ( reduceCoincidentSimpleFaceOnMultiLoopFace(
                         solid, faceA, faceB, numericContext) ||
                     reduceCoincidentSimpleFaceOnMultiLoopFace(
                         solid, faceB, faceA, numericContext) ) {
                    restart = TRUE;
                    break;
                }
            }
        }
        if ( restart ) {
            continue;
        }

        //- Eliminate vertices between colinear edges -----------------
        _PolyhedralBoundedSolidHalfEdge* heMirror;
        _PolyhedralBoundedSolidVertex* v;
        int nedges;

        for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
            v = solid->getVerticesList().get(i);
            heStart = v->emanatingHalfEdge;
            if ( heStart == 0 ) {
                continue;
            }
            he = heStart;
            nedges = 0;
            j = 0;
            do {
                nedges++;
                if ( nedges > 2 ) break;

                if ( he == 0 ) {
                    throw VSDKFatalException(
                        "maximizeFaces: Inconsistent model! Null HalfEdge. Check.");
                }

                heMirror = he->mirrorHalfEdge();
                if ( heMirror == 0 ) {
                    nedges = 0;
                    continue;
                }

                he = heMirror->next();
                if ( he == 0 ) {
                    throw VSDKFatalException(
                        "maximizeFaces: Inconsistent model! HalfEdge without next. Check.");
                }
                j++;
            } while ( he != heStart );

            if ( nedges == 2 ) {
                p0 = heStart->startingVertex->position;
                p1 = heStart->next()->startingVertex->position.subtract(p0);
                p2 = heStart->previous()->startingVertex->position.subtract(p0);
                if ( PolyhedralBoundedSolidNumericPolicy
                    ::vectorsColinear(p1, p2, numericContext) ) {
                    if ( p1.dotProduct(p2) < 0 ) {
                        PolyhedralBoundedSolidEulerOperators::lkev(
                            solid, heStart, heStart->mirrorHalfEdge());
                        restart = TRUE;
                        break;
                    }
                }
            }
        }
    }

    //- Eliminate rings with a single vertex --------------------------
    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        _PolyhedralBoundedSolidHalfEdge* outerloophe;

        outerloophe = face->boundariesList.get(0)->boundaryStartHalfEdge;

        for ( j = 1; j < face->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop* loop;

            loop = face->boundariesList.get(j);
            he = loop->boundaryStartHalfEdge;
            if ( he->parentEdge == 0 ||
                 loop->halfEdgesList.size() == 1 ) {
                // Kill ring
                _PolyhedralBoundedSolidVertex* vtodelete;
                vtodelete = he->startingVertex;
                PolyhedralBoundedSolidEulerOperators::lmekr(solid, outerloophe, he);

                // Kill edge and vertex
                _PolyhedralBoundedSolidHalfEdge* hej;
                hej = outerloophe;
                do {
                    hej = hej->next();
                    if ( hej == 0 ) {
                        // Loop is not closed!
                        break;
                    }
                    if ( hej->startingVertex == vtodelete ) {
                        PolyhedralBoundedSolidEulerOperators::lkev(
                            solid, hej, hej->mirrorHalfEdge());
                        break;
                    }
                } while ( hej != outerloophe );
            }
        }
    }
    // Here should be a code searching for faces inside faces ...
    remakeEmanatingHalfedgesReferences(solid);
}

int PolyhedralBoundedSolidTopologyEditing::weldCoincidentVertices(
    PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& context)
{
    int weldCount;
    BOOLEAN found;
    long int i;
    _PolyhedralBoundedSolidEdge* edge;
    _PolyhedralBoundedSolidVertex* v1;
    _PolyhedralBoundedSolidVertex* v2;

    weldCount = 0;
    do {
        found = FALSE;
        for ( i = 0; i < solid->getEdgesList().size(); i++ ) {
            edge = solid->getEdgesList().get(i);
            if ( edge == 0 ||
                 edge->rightHalf == 0 ||
                 edge->leftHalf == 0 ||
                 edge->rightHalf->startingVertex == 0 ||
                 edge->leftHalf->startingVertex == 0 ) {
                continue;
            }
            v1 = edge->rightHalf->startingVertex;
            v2 = edge->leftHalf->startingVertex;
            if ( PolyhedralBoundedSolidNumericPolicy::pointsCoincident(
                     v1->position, v2->position, context) ) {
                PolyhedralBoundedSolidEulerOperators::lkev(
                    solid, edge->rightHalf, edge->leftHalf);
                weldCount++;
                found = TRUE;
                break;
            }
        }
    } while ( found );
    return weldCount;
}

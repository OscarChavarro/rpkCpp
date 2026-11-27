//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =
//= [.wMANT2008] Mantyla Martti. "Personal Home Page", <<shar> > archive     =
//=     containing the C programs from [MANT1988]. Available at             =
//=     http://www.cs.hut.fi/~mam . Last visited April 12 / 2008.           =


#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/statistics/PolyhedralBoundedSolidStatistics.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
#include "vsdk/toolkit/common/Cpp98Support.h"


static const char* const CLASS_NAME = "PolyhedralBoundedSolidEulerOperators";
static const char* const FACE_MESSAGE = "Face ";
static const char* const EDGE_MESSAGE = "Edge ";
static const char* const NOT_FOUND_MESSAGE = " not found.";
static const char* const EDGE_NOT_FOUND_IN_FACE_WILDCARD_MESSAGE =
    " - * not found in face ";
static const char* const EDGE_NOT_FOUND_IN_FACE_MESSAGE = " not found in face ";
static const char* const LRINGMV_MESSAGE = "lringmv";
static const char* const ADDHE_MESSAGE = "addhe";
static const char* const DOT = ".";

static void report(int level, const char* method, const String& message)
{
    Logger::reportMessage(CLASS_NAME, level, method, message);
}

static String str(int value)
{
    return String(Cpp98Support::toString(value).c_str());
}

static _PolyhedralBoundedSolidHalfEdge* addhe(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidEdge* e,
    _PolyhedralBoundedSolidVertex* v,
    _PolyhedralBoundedSolidHalfEdge* where,
    int sign)
{
    _PolyhedralBoundedSolidHalfEdge* he;

    (void)solid;
    if ( where == 0 ) {
        report(Logger::WARNING, ADDHE_MESSAGE,
            "Cannot create a half-edge because the reference half-edge is null.");
        return 0;
    }
    if ( where->parentLoop == 0 ) {
        report(Logger::WARNING, ADDHE_MESSAGE,
            "Cannot create a half-edge because the reference half-edge has no parent loop.");
        return 0;
    }
    if ( e == 0 ) {
        report(Logger::WARNING, ADDHE_MESSAGE,
            "Cannot create a half-edge because the target edge is null.");
        return 0;
    }

    if ( where->parentEdge == 0 ) {
        he = where;
    }
    else {
        he = new _PolyhedralBoundedSolidHalfEdge(v, where->parentLoop);
        where->parentLoop->halfEdgesList.insertBefore(he, where);
        he->startingVertex = v;
    }
    he->parentEdge = e;
    he->parentLoop = where->parentLoop;

    if ( sign == PolyhedralBoundedSolid::PLUS ) {
        e->leftHalf = he;
    }
    else {
        e->rightHalf = he;
    }

    return he;
}

static void splitVertexNeighborhood(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* he1,
    _PolyhedralBoundedSolidHalfEdge* he2,
    int vertexId,
    const Vector3Dd& p)
{
    _PolyhedralBoundedSolidHalfEdge* he;
    _PolyhedralBoundedSolidVertex* newVertex;
    _PolyhedralBoundedSolidEdge* newEdge;

    if ( vertexId > solid->getMaxVertexId() ) solid->setMaxVertexId(vertexId);

    newEdge = new _PolyhedralBoundedSolidEdge();
    solid->getEdgesList().add(newEdge);
    newVertex = new _PolyhedralBoundedSolidVertex(p, vertexId);
    solid->getVerticesList().add(newVertex);

    //-----------------------------------------------------------------
    he = he1;
    while ( he != he2 ) {
        he->startingVertex = newVertex;
        he = (he->mirrorHalfEdge())->next();
    }

    //-----------------------------------------------------------------
    addhe(solid, newEdge, newVertex, he2, PolyhedralBoundedSolid::PLUS);
    addhe(solid, newEdge, he2->startingVertex, he1, PolyhedralBoundedSolid::MINUS);

    //-----------------------------------------------------------------
    newVertex->emanatingHalfEdge = he2->previous();
    he2->startingVertex->emanatingHalfEdge = he2;
}

static void insertLineDrawingEdge(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* he,
    int vertexId,
    const Vector3Dd& p)
{
    _PolyhedralBoundedSolidVertex* oldVertex;
    _PolyhedralBoundedSolidVertex* newVertex;
    _PolyhedralBoundedSolidEdge* newEdge;

    if ( vertexId > solid->getMaxVertexId() ) solid->setMaxVertexId(vertexId);

    newEdge = new _PolyhedralBoundedSolidEdge();
    solid->getEdgesList().add(newEdge);
    newVertex = new _PolyhedralBoundedSolidVertex(p, vertexId);
    solid->getVerticesList().add(newVertex);
    oldVertex = he->startingVertex;

    addhe(solid, newEdge, oldVertex, he, PolyhedralBoundedSolid::PLUS);
    addhe(solid, newEdge, newVertex, he, PolyhedralBoundedSolid::MINUS);

    newVertex->emanatingHalfEdge = he->previous();
    oldVertex->emanatingHalfEdge = he;
}

static BOOLEAN failLringmv(const String& message)
{
    PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
    report(Logger::WARNING, LRINGMV_MESSAGE, message);
    return FALSE;
}

static _PolyhedralBoundedSolidFace* validateLringmvInput(
    _PolyhedralBoundedSolidLoop* loop,
    _PolyhedralBoundedSolidFace* destinationFace)
{
    if ( loop == 0 || destinationFace == 0 ) {
        failLringmv("Null input loop or destination face.");
        return 0;
    }

    _PolyhedralBoundedSolidFace* sourceFace = loop->parentFace;
    if ( sourceFace == 0 ) {
        failLringmv("Given loop does not have a parent face.");
        return 0;
    }
    if ( sourceFace->parentSolid != destinationFace->parentSolid ) {
        failLringmv("Loop move across different solids is not supported.");
        return 0;
    }
    return sourceFace;
}

static void promoteLoopAsOuter(
    _PolyhedralBoundedSolidFace* face,
    _PolyhedralBoundedSolidLoop* loop)
{
    long numberOfLoops = face->boundariesList.size();
    if ( numberOfLoops <= 0 || face->boundariesList.get(0) == loop ) {
        return;
    }
    face->boundariesList.swapElements(loop, face->boundariesList.get(0));
}

static BOOLEAN demoteLoopAsInner(
    _PolyhedralBoundedSolidFace* face,
    _PolyhedralBoundedSolidLoop* loop)
{
    long numberOfLoops = face->boundariesList.size();
    if ( numberOfLoops <= 1 ) {
        return failLringmv("Cannot mark the only boundary loop as inner.");
    }
    if ( face->boundariesList.get(0) != loop ) {
        return TRUE;
    }
    face->boundariesList.swapElements(loop, face->boundariesList.get(1));
    return TRUE;
}

static BOOLEAN reorderLoopInSameFace(
    _PolyhedralBoundedSolidLoop* loop,
    _PolyhedralBoundedSolidFace* face,
    BOOLEAN setAsOuterLoop)
{
    if ( !face->boundariesList.locateWindowAtElem(loop) ) {
        return failLringmv(
            "Given loop was not found in its parent face boundaries.");
    }
    if ( setAsOuterLoop ) {
        promoteLoopAsOuter(face, loop);
        return TRUE;
    }
    return demoteLoopAsInner(face, loop);
}

static BOOLEAN moveLoopAcrossFaces(
    _PolyhedralBoundedSolidLoop* loop,
    _PolyhedralBoundedSolidFace* sourceFace,
    _PolyhedralBoundedSolidFace* destinationFace,
    BOOLEAN setAsOuterLoop)
{
    if ( sourceFace->boundariesList.size() <= 1 ) {
        return failLringmv("Cannot move the only boundary loop of a face.");
    }
    if ( !setAsOuterLoop && destinationFace->boundariesList.size() <= 0 ) {
        return failLringmv(
            "Cannot insert an inner loop into a face without outer loop.");
    }
    if ( !sourceFace->boundariesList.locateWindowAtElem(loop) ) {
        return failLringmv(
            "Given loop was not found in source face boundaries.");
    }

    sourceFace->boundariesList.removeElemAtWindow();
    loop->parentFace = destinationFace;

    if ( setAsOuterLoop ) {
        destinationFace->boundariesList.push(loop);
    }
    else {
        destinationFace->boundariesList.add(loop);
    }
    return TRUE;
}


//= LOW LEVEL EULER OPERATIONS ====================================

/**
mvfs: MakeVertexFaceSolid, as described in sections [MANT1988].9.2.2,
[MANT1988].11.3.1 and [MANT1988].11.5.1, following the structure of sample
program [MANT1988].11.5.
*/
void PolyhedralBoundedSolidEulerOperators::mvfs(PolyhedralBoundedSolid* solid, const Vector3Dd& p, int vertexId, int faceId)
{
    _PolyhedralBoundedSolidFace* newFace;
    _PolyhedralBoundedSolidLoop* newLoop;
    _PolyhedralBoundedSolidHalfEdge* newHalfEdge;
    _PolyhedralBoundedSolidVertex* newVertex;

    if ( vertexId > solid->getMaxVertexId() ) solid->setMaxVertexId(vertexId);
    if ( faceId > solid->getMaxFaceId() ) solid->setMaxFaceId(faceId);

    newFace = new _PolyhedralBoundedSolidFace(solid, faceId);
    solid->getPolygonsList().add(newFace);
    newLoop = new _PolyhedralBoundedSolidLoop(newFace);
    newVertex = new _PolyhedralBoundedSolidVertex(p, vertexId);
    solid->getVerticesList().add(newVertex);
    newHalfEdge = new _PolyhedralBoundedSolidHalfEdge(newVertex, newLoop);
    newLoop->halfEdgesList.add(newHalfEdge);
    newLoop->boundaryStartHalfEdge = newHalfEdge;
    // The lone vertex of an empty loop points at the loop's edge-less
    // half-edge, so every vertex has a live emanating half-edge.
    newVertex->emanatingHalfEdge = newHalfEdge;
}

/**
kvfs: KillVertexFaceSolid, inverse of mvfs, as described in sections
[MANT1988].9.2.2 and [MANT1988].11.5.1. The solid must be the skeletal one.
*/
void PolyhedralBoundedSolidEulerOperators::kvfs(PolyhedralBoundedSolid* solid)
{
    if ( solid->getPolygonsList().size() != 1 ) {
        throw VSDKFatalException(
            "kvfs: Not skeletal solid, not having exactly one loop!");
    }
    if ( solid->getEdgesList().size() != 0 ) {
        throw VSDKFatalException(
            "kvfs: Not skeletal solid, having some edges!");
    }
    if ( solid->getVerticesList().size() != 1 ) {
        throw VSDKFatalException(
            "kvfs: Not skeletal solid, not having exactly one vertex !");
    }
    _PolyhedralBoundedSolidFace* killedFace = solid->getPolygonsList().get(0);
    _PolyhedralBoundedSolidVertex* killedVertex = solid->getVerticesList().get(0);
    solid->getPolygonsList().remove(0L);
    solid->getVerticesList().remove(0L);
    // C++ port: the solid keeps the unlinked nodes (Java leaves them to
    // its garbage collector)
    long i;
    for ( i = 0; i < killedFace->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = killedFace->boundariesList.get(i);
        long j;
        for ( j = 0; j < loop->halfEdgesList.size(); j++ ) {
            solid->retireHalfEdge(loop->halfEdgesList.get(j));
        }
        solid->retireLoop(loop);
    }
    solid->retireFace(killedFace);
    solid->retireVertex(killedVertex);
}

/**
lmev: low level make edge vertex (vertex splitting operation), as described
in sections [MANT1988].9.2.3, [MANT1988].11.3.2 and [MANT1988].11.5.1 and
following sample program [MANT1988].11.6. When `he1` and `he2` are the same
half-edge, the line-drawing strut is created.
*/
void PolyhedralBoundedSolidEulerOperators::lmev(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* he1,
    _PolyhedralBoundedSolidHalfEdge* he2,
    int vertexId,
    const Vector3Dd& p)
{
    PolyhedralBoundedSolidStatistics::recordLmevCall();
    if ( he1 == 0 || he2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordInvalidHalfEdgeInputCase();
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lmev", "Calling with empty half-edge!");
        return;
    }
    if ( he1->startingVertex != he2->startingVertex ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        throw VSDKFatalException(
            "lmev: Half-edges not starting at the same vertex. Not supported case!");
    }
    if ( he1 == he2 ) {
        PolyhedralBoundedSolidStatistics::recordHe1EqualsHe2Case();
        insertLineDrawingEdge(solid, he1, vertexId, p);
        return;
    }
    splitVertexNeighborhood(solid, he1, he2, vertexId, p);
}

/**
lkev: low level kill edge vertex (vertex joining operation), inverse of
lmev, as described in sections [MANT1988].9.2.3, [MANT1988].11.3.4 and
[MANT1988].11.5.1. Answer to problem [MANT1988].11.3, borrowed from
[.wMANT2008].
*/
void PolyhedralBoundedSolidEulerOperators::lkev(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* he1,
    _PolyhedralBoundedSolidHalfEdge* he2)
{
    PolyhedralBoundedSolidStatistics::recordLkevCall();
    //-----------------------------------------------------------------
    if ( he1 == 0 || he2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordInvalidHalfEdgeInputCase();
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lkev",
            "Two half-edges are needed for this Euler operator two work!");
        return;
    }
    if ( he1->parentEdge == 0 || he1->parentEdge != he2->parentEdge ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lkev",
            "Given half-edges must be the two halves of one edge!");
        return;
    }
    if ( he1 == he2 ) {
        PolyhedralBoundedSolidStatistics::recordHe1EqualsHe2Case();
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lkev",
            "Given half-edges must be different!");
        return;
    }

    //-----------------------------------------------------------------
    // Answer borrowed from [.wMANT2008]
    _PolyhedralBoundedSolidHalfEdge* he;
    _PolyhedralBoundedSolidHalfEdge* he2next;

    he = he2->next();
    while ( he != he1 ) {
        he->startingVertex = he2->startingVertex;
        he = he->mirrorHalfEdge()->next();
    }

    // The surviving vertex must keep a live emanating half-edge
    // (defect E-05): he1.next() starts at he2's vertex; when that is he2
    // itself (strut seen from the other side) use he2.next() instead.
    _PolyhedralBoundedSolidHalfEdge* he1next;
    he1next = he1->next();
    he2next = he2->next();
    he1->parentLoop->unlistHalfEdge(he1);
    he2->parentLoop->unlistHalfEdge(he2);
    if ( he1next != he1 && he1next != he2 ) {
        he2->startingVertex->emanatingHalfEdge = he1next;
    }
    else if ( he2next != he1 && he2next != he2 ) {
        he2->startingVertex->emanatingHalfEdge = he2next;
    }
    else {
        // Loop becomes empty; he2 survives below as its edge-less
        // half-edge.
        he2->startingVertex->emanatingHalfEdge = he2;
    }

    _PolyhedralBoundedSolidEdge* killedEdge = he1->parentEdge;
    _PolyhedralBoundedSolidVertex* killedVertex = he1->startingVertex;
    solid->getEdgesList().locateWindowAtElem(he1->parentEdge);
    solid->getEdgesList().removeElemAtWindow();
    solid->getVerticesList().locateWindowAtElem(he1->startingVertex);
    solid->getVerticesList().removeElemAtWindow();

    BOOLEAN he2Survives = FALSE;
    if ( he2->parentLoop->halfEdgesList.size() <= 0 ) {
        he2->parentEdge = 0;
        he2->parentLoop->halfEdgesList.add(he2);
        he2->parentLoop->boundaryStartHalfEdge = he2;
        he2Survives = TRUE;
    }
    // C++ port: the solid keeps the unlinked nodes (Java leaves them to
    // its garbage collector)
    solid->retireHalfEdge(he1);
    if ( !he2Survives ) {
        solid->retireHalfEdge(he2);
    }
    solid->retireEdge(killedEdge);
    solid->retireVertex(killedVertex);
}

/**
lkef: Low Level Kill Edge Face, inverse of lmef. Removes the edge of `he1`
and `he2` and joins their two faces; `he2.parentLoop.parentFace` is removed.
*/
void PolyhedralBoundedSolidEulerOperators::lkef(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* he1,
    _PolyhedralBoundedSolidHalfEdge* he2)
{
    PolyhedralBoundedSolidStatistics::recordLkefCall();
    if ( he1->parentEdge != he2->parentEdge ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        throw VSDKFatalException(
            "lkef: Given half-edges must lie over the same edge. Operation aborted.");
    }
    if ( he1->parentLoop->parentFace == he2->parentLoop->parentFace ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        throw VSDKFatalException(
            "lkef: Given half-edges must belong to different faces. Operation aborted.");
    }

    _PolyhedralBoundedSolidEdge* edgeToBeKilled;
    _PolyhedralBoundedSolidLoop* loopToBeKilled;
    _PolyhedralBoundedSolidFace* faceToBeKilled;
    _PolyhedralBoundedSolidHalfEdge* halfEdgePivot;

    halfEdgePivot = he1->next();
    _PolyhedralBoundedSolidHalfEdge* he1nextBeforeKill = halfEdgePivot;
    _PolyhedralBoundedSolidHalfEdge* he2nextBeforeKill = he2->next();
    edgeToBeKilled = he1->parentEdge;
    loopToBeKilled = he2->parentLoop;
    faceToBeKilled = loopToBeKilled->parentFace;

    ArrayList<_PolyhedralBoundedSolidHalfEdge*> migratedHalfEdges;

    _PolyhedralBoundedSolidHalfEdge* he;
    he = he2->next();
    long maxTraversal = he2->parentLoop->halfEdgesList.size() + 1;
    long traversed = 0;
    while ( he != he2 ) {
        if ( he == 0 || traversed > maxTraversal ) {
            PolyhedralBoundedSolidStatistics::recordConsistencyWarningCase();
            report(Logger::WARNING, "lkef",
                "Detected inconsistent next-chain; falling back to halfEdgesList order.");
            migratedHalfEdges.clear();
            long idx;
            for ( idx = 0; idx < he2->parentLoop->halfEdgesList.size(); idx++ ) {
                _PolyhedralBoundedSolidHalfEdge* candidate;
                candidate = he2->parentLoop->halfEdgesList.get(idx);
                if ( candidate != he2 ) {
                    migratedHalfEdges.add(candidate);
                }
            }
            break;
        }
        migratedHalfEdges.add(he);
        he = he->next();
        traversed++;
    }

    he1->parentLoop->unlistHalfEdge(he1);
    he2->parentLoop->unlistHalfEdge(he2);
    solid->getEdgesList().locateWindowAtElem(edgeToBeKilled);
    solid->getEdgesList().removeElemAtWindow();

    faceToBeKilled->boundariesList.locateWindowAtElem(loopToBeKilled);
    faceToBeKilled->boundariesList.removeElemAtWindow();
    solid->getPolygonsList().locateWindowAtElem(faceToBeKilled);
    solid->getPolygonsList().removeElemAtWindow();

    long i;
    for ( i = migratedHalfEdges.size() - 1; i >= 0; i-- ) {
        he = migratedHalfEdges.get(i);
        he->parentLoop = he1->parentLoop;
        he1->parentLoop->halfEdgesList.insertBefore(he, halfEdgePivot);
        halfEdgePivot = he;
    }

    // If the killed face had additional loops (inner rings), transfer them
    // to the surviving face so their half-edges remain reachable.
    while ( faceToBeKilled->boundariesList.size() > 0 ) {
        _PolyhedralBoundedSolidLoop* orphanedLoop;
        orphanedLoop = faceToBeKilled->boundariesList.get(0);
        orphanedLoop->parentFace = he1->parentLoop->parentFace;
        faceToBeKilled->boundariesList.remove(0L);
        he1->parentLoop->parentFace->boundariesList.add(orphanedLoop);
    }

    // Redirect vertex back-pointers away from the killed half-edges:
    // he2.next() starts at he1's vertex and he1.next() at he2's.
    if ( he1->startingVertex->emanatingHalfEdge == he1 ||
         he1->startingVertex->emanatingHalfEdge == he2 ) {
        he1->startingVertex->emanatingHalfEdge = he2nextBeforeKill;
    }
    if ( he2->startingVertex->emanatingHalfEdge == he1 ||
         he2->startingVertex->emanatingHalfEdge == he2 ) {
        he2->startingVertex->emanatingHalfEdge = he1nextBeforeKill;
    }
    // C++ port: the solid keeps the unlinked nodes (Java leaves them to
    // its garbage collector)
    solid->retireHalfEdge(he1);
    solid->retireHalfEdge(he2);
    solid->retireEdge(edgeToBeKilled);
    solid->retireLoop(loopToBeKilled);
    solid->retireFace(faceToBeKilled);
}

/**
lmef: low level make edge face (face splitting operator), as described in
sections [MANT1988].9.2.3, [MANT1988].11.3.3 and [MANT1988].11.5.1 and
following sample program [MANT1988].11.7. `he1` ends in the new face.
*/
_PolyhedralBoundedSolidFace* PolyhedralBoundedSolidEulerOperators::lmef(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* he1,
    _PolyhedralBoundedSolidHalfEdge* he2,
    int newFaceId)
{
    PolyhedralBoundedSolidStatistics::recordLmefCall();
    _PolyhedralBoundedSolidFace* newFace;
    _PolyhedralBoundedSolidLoop* newLoop;
    _PolyhedralBoundedSolidLoop* oldLoop;
    _PolyhedralBoundedSolidEdge* newEdge;
    _PolyhedralBoundedSolidHalfEdge* he;
    _PolyhedralBoundedSolidHalfEdge* nhe1;
    _PolyhedralBoundedSolidHalfEdge* nhe2;

    if ( he1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordInvalidHalfEdgeInputCase();
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lmef",
            "Cannot split face because first half-edge is null.");
        return 0;
    }
    if ( he2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordInvalidHalfEdgeInputCase();
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lmef",
            "Cannot split face because second half-edge is null.");
        return 0;
    }
    if ( he1->parentLoop == 0 || he2->parentLoop == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lmef",
            "Cannot split face because one input half-edge has no parent loop.");
        return 0;
    }
    if ( he1->startingVertex == 0 || he2->startingVertex == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lmef",
            "Cannot split face because one input half-edge has null starting vertex.");
        return 0;
    }
    if ( he1->parentLoop != he2->parentLoop ) {
        // [MANT1988] Sec. 11.3.3 precondition (defect E-06): rejected
        // before any change to the solid.
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lmef",
            "Cannot split face because the half-edges belong to different loops.");
        return 0;
    }

    if ( newFaceId > solid->getMaxFaceId() ) solid->setMaxFaceId(newFaceId);

    if ( he1 == he2 ) {
        return lmefCircularFace(solid, he1, newFaceId);
    }

    newFace = new _PolyhedralBoundedSolidFace(solid, newFaceId);
    solid->getPolygonsList().add(newFace);
    oldLoop = he1->parentLoop;
    newLoop = new _PolyhedralBoundedSolidLoop(newFace);
    newEdge = new _PolyhedralBoundedSolidEdge();
    solid->getEdgesList().add(newEdge);

    ArrayList<_PolyhedralBoundedSolidHalfEdge*> migratedHalfEdges;

    he = he1;
    while ( he != he2 ) {
        migratedHalfEdges.add(he);
        he = he->next();
        if ( he == he1 ) break;
    }

    long i;
    for ( i = 0; i < migratedHalfEdges.size(); i++ ) {
        he = migratedHalfEdges.get(i);
        he->parentLoop = newLoop;
        oldLoop->unlistHalfEdge(he);
        newLoop->halfEdgesList.add(he);
    }

    nhe1 = addhe(solid, newEdge, he2->startingVertex, he1, PolyhedralBoundedSolid::MINUS);
    nhe2 = addhe(solid, newEdge, he1->startingVertex, he2, PolyhedralBoundedSolid::PLUS);
    if ( nhe1 == 0 || nhe2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lmef",
            "Cannot split face because one generated half-edge is null.");
        return 0;
    }

    newLoop->boundaryStartHalfEdge = nhe1;
    he2->parentLoop->boundaryStartHalfEdge = nhe2;

    return newFace;
}

/**
`lmef(he, he)`: the "circular" face of Program 11.7 (defect E-04). The new
edge starts and ends at `he.startingVertex`; one half lies alone in the
new face's loop, the other is inserted before `he` in the old loop. When
`he` is the edge-less half-edge of an empty loop it becomes the new
face's half-edge, as `addhe` reuses it in the book.
*/
_PolyhedralBoundedSolidFace* PolyhedralBoundedSolidEulerOperators::lmefCircularFace(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* he,
    int newFaceId)
{
    _PolyhedralBoundedSolidFace* newFace;
    _PolyhedralBoundedSolidLoop* newLoop;
    _PolyhedralBoundedSolidLoop* oldLoop;
    _PolyhedralBoundedSolidEdge* newEdge;
    _PolyhedralBoundedSolidHalfEdge* nhe1;
    _PolyhedralBoundedSolidHalfEdge* nhe2;
    _PolyhedralBoundedSolidVertex* v;

    oldLoop = he->parentLoop;
    v = he->startingVertex;
    newFace = new _PolyhedralBoundedSolidFace(solid, newFaceId);
    solid->getPolygonsList().add(newFace);
    newLoop = new _PolyhedralBoundedSolidLoop(newFace);
    newEdge = new _PolyhedralBoundedSolidEdge();
    solid->getEdgesList().add(newEdge);

    if ( he->parentEdge == 0 ) {
        oldLoop->unlistHalfEdge(he);
        nhe1 = he;
        nhe1->parentLoop = newLoop;
        newLoop->halfEdgesList.add(nhe1);
        nhe2 = new _PolyhedralBoundedSolidHalfEdge(v, oldLoop);
        oldLoop->halfEdgesList.add(nhe2);
    }
    else {
        nhe1 = new _PolyhedralBoundedSolidHalfEdge(v, newLoop);
        newLoop->halfEdgesList.add(nhe1);
        nhe2 = new _PolyhedralBoundedSolidHalfEdge(v, oldLoop);
        oldLoop->halfEdgesList.insertBefore(nhe2, he);
    }
    nhe1->parentEdge = newEdge;
    nhe2->parentEdge = newEdge;
    newEdge->rightHalf = nhe1;
    newEdge->leftHalf = nhe2;
    newLoop->boundaryStartHalfEdge = nhe1;
    oldLoop->boundaryStartHalfEdge = nhe2;
    v->emanatingHalfEdge = nhe2;
    return newFace;
}

/**
lkemr: low level kill edge make ring (loop splitting operator), as
described in sections [MANT1988].9.2.3 and [MANT1988].11.3.4, following
sample program [MANT1988].11.8.
*/
void PolyhedralBoundedSolidEulerOperators::lkemr(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* he1,
    _PolyhedralBoundedSolidHalfEdge* he2)
{
    PolyhedralBoundedSolidStatistics::recordLkemrCall();
    //-----------------------------------------------------------------
    if ( he1 == 0 || he2 == 0 || he1 == he2 ||
         he1->parentEdge == 0 || he1->parentEdge != he2->parentEdge ||
         he1->parentLoop != he2->parentLoop ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lkemr",
            String("Given half-edges must be the two halves of one edge in one loop. ")
            + "Operation aborted.");
        return;
    }

    //-----------------------------------------------------------------
    // Following Program 11.8: the loop he1, R, he2, O (R and O being the
    // half-edges strictly between he1 and he2, and between he2 and he1)
    // becomes the old loop {he1, O} and the new ring {he2, R}; then the
    // two halves of the bridge are deleted. A component that ends up
    // without half-edges keeps its bridge half as the edge-less
    // half-edge of an empty loop (the book's delhe), so both "one
    // resulting loop empty" cases of Sec. 11.3.4 are handled (defect
    // E-02). Every vertex keeps a live emanating half-edge (E-03).
    _PolyhedralBoundedSolidHalfEdge* he;
    _PolyhedralBoundedSolidLoop* newLoop;
    _PolyhedralBoundedSolidLoop* oldLoop;
    _PolyhedralBoundedSolidEdge* killedEdge;
    ArrayList<_PolyhedralBoundedSolidHalfEdge*> ring;
    long i;

    oldLoop = he1->parentLoop;
    killedEdge = he1->parentEdge;
    for ( he = he1->next(); he != he2; he = he->next() ) {
        ring.add(he);
    }
    newLoop = new _PolyhedralBoundedSolidLoop(oldLoop->parentFace);

    for ( i = 0; i < ring.size(); i++ ) {
        he = ring.get(i);
        oldLoop->halfEdgesList.locateWindowAtElem(he);
        oldLoop->halfEdgesList.removeElemAtWindow();
        newLoop->halfEdgesList.add(he);
        he->parentLoop = newLoop;
    }

    // he2 always leaves the old loop.
    oldLoop->halfEdgesList.locateWindowAtElem(he2);
    oldLoop->halfEdgesList.removeElemAtWindow();
    if ( ring.isEmpty() ) {
        he2->parentEdge = 0;
        he2->parentLoop = newLoop;
        newLoop->halfEdgesList.add(he2);
    }
    newLoop->boundaryStartHalfEdge = newLoop->halfEdgesList.get(0);
    he2->startingVertex->emanatingHalfEdge = newLoop->boundaryStartHalfEdge;

    BOOLEAN he1Survives = TRUE;
    if ( oldLoop->halfEdgesList.size() <= 1 ) {
        // Only he1 is left: it becomes the edge-less half-edge.
        he1->parentEdge = 0;
        oldLoop->boundaryStartHalfEdge = he1;
        he1->startingVertex->emanatingHalfEdge = he1;
    }
    else {
        _PolyhedralBoundedSolidHalfEdge* he1next = he1->next();
        oldLoop->halfEdgesList.locateWindowAtElem(he1);
        oldLoop->halfEdgesList.removeElemAtWindow();
        oldLoop->boundaryStartHalfEdge = he1next;
        he1->startingVertex->emanatingHalfEdge = he1next;
        he1Survives = FALSE;
    }

    solid->getEdgesList().locateWindowAtElem(killedEdge);
    solid->getEdgesList().removeElemAtWindow();
    // C++ port: the solid keeps the unlinked nodes (Java leaves them to
    // its garbage collector)
    if ( !he1Survives ) {
        solid->retireHalfEdge(he1);
    }
    if ( !ring.isEmpty() ) {
        solid->retireHalfEdge(he2);
    }
    solid->retireEdge(killedEdge);
}

/**
lkfmrh: low level kill face make ring hole in same shell, from exercise
[MANT1988].11.5 and section [MANT1988].11.5.1. `face2` must be simple.
*/
void PolyhedralBoundedSolidEulerOperators::lkfmrh(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidFace* face1,
    _PolyhedralBoundedSolidFace* face2)
{
    if ( face1 == 0 || face2 == 0 || face1 == face2 ||
         face1->parentSolid != solid || face2->parentSolid != solid ) {
        report(Logger::WARNING, "lkfmrh",
            "Faces must be two distinct faces of the given solid!");
        return;
    }
    if ( face2->boundariesList.size() > 1 ) {
        report(Logger::WARNING, "lkfmrh",
            "Internal face to form new loop must have just one boundary!");
        return;
    }

    _PolyhedralBoundedSolidLoop* newLoop;
    _PolyhedralBoundedSolidLoop* oldLoop;
    _PolyhedralBoundedSolidHalfEdge* he;
    long i;

    oldLoop = face2->boundariesList.get(0);
    newLoop = new _PolyhedralBoundedSolidLoop(face1);

    for ( i = 0; i < oldLoop->halfEdgesList.size(); i++ ) {
        he = oldLoop->halfEdgesList.get(i);
        he->parentLoop = newLoop;
        newLoop->halfEdgesList.add(he);
    }
    newLoop->boundaryStartHalfEdge = newLoop->halfEdgesList.get(0);

    solid->getPolygonsList().locateWindowAtElem(face2);
    solid->getPolygonsList().removeElemAtWindow();
    // C++ port: the solid keeps the unlinked nodes (Java leaves them to
    // its garbage collector)
    solid->retireLoop(oldLoop);
    solid->retireFace(face2);
}

/**
lmfkrh: low level make face kill ring hole, inverse of lkfmrh, from
exercise [MANT1988].11.5 and section [MANT1988].11.5.1.
*/
_PolyhedralBoundedSolidFace* PolyhedralBoundedSolidEulerOperators::lmfkrh(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidLoop* l,
    int newFaceId)
{
    _PolyhedralBoundedSolidFace* newFace;
    newFace = new _PolyhedralBoundedSolidFace(solid, newFaceId);
    solid->getPolygonsList().add(newFace);

    if ( newFaceId > solid->getMaxFaceId() ) solid->setMaxFaceId(newFaceId);

    l->parentFace->boundariesList.locateWindowAtElem(l);
    l->parentFace->boundariesList.removeElemAtWindow();
    l->parentFace = newFace;
    newFace->boundariesList.add(l);

    return newFace;
}

/**
lkimrh: naming alias of lkfmrh for the [MANT1988].11.5 operator family.
*/
void PolyhedralBoundedSolidEulerOperators::lkimrh(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidFace* face1,
    _PolyhedralBoundedSolidFace* face2)
{
    lkfmrh(solid, face1, face2);
}

/**
lmikrh: naming alias of lmfkrh for the [MANT1988].11.5 operator family.
*/
_PolyhedralBoundedSolidFace* PolyhedralBoundedSolidEulerOperators::lmikrh(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidLoop* l,
    int newFaceId)
{
    return lmfkrh(solid, l, newFaceId);
}

/**
lringmv moves the loop `l` from its parent face to `toFace`, as inner loop
or as outer loop following `setAsOuterLoop`. It is an addendum to lmef, not
an Euler operator, following section [MANT1988].11.5.1.
*/
BOOLEAN PolyhedralBoundedSolidEulerOperators::lringmv(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidLoop* l,
    _PolyhedralBoundedSolidFace* toFace,
    BOOLEAN setAsOuterLoop)
{
    PolyhedralBoundedSolidStatistics::recordLringmvCall();
    (void)solid;
    _PolyhedralBoundedSolidFace* fromFace = validateLringmvInput(l, toFace);
    if ( fromFace == 0 ) {
        return FALSE;
    }
    if ( fromFace == toFace ) {
        return reorderLoopInSameFace(l, toFace, setAsOuterLoop);
    }
    return moveLoopAcrossFaces(l, fromFace, toFace, setAsOuterLoop);
}

/**
lmekr: low level make edge kill ring, inverse of lkemr. Answer to exercise
[MANT1988].11.4, with the signature from section [MANT1988].11.5.1.
*/
void PolyhedralBoundedSolidEulerOperators::lmekr(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* he1,
    _PolyhedralBoundedSolidHalfEdge* he2)
{
    PolyhedralBoundedSolidStatistics::recordLmekrCall();
    //-----------------------------------------------------------------
    if ( he1->parentLoop == he2->parentLoop ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lmekr",
            "Given half-edges are on the same loop. Operation aborted.");
        return;
    }
    if ( he1->parentLoop->parentFace != he2->parentLoop->parentFace ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lmekr",
            "Given half-edges are not on the same face. Operation aborted.");
        return;
    }

    //-----------------------------------------------------------------
    ArrayList<_PolyhedralBoundedSolidHalfEdge*> migratedHalfEdges;
    _PolyhedralBoundedSolidHalfEdge* he;
    _PolyhedralBoundedSolidLoop* ringToKill;

    ringToKill = he2->parentLoop;

    he = he2;
    do {
        migratedHalfEdges.add(he);
        he = he->next();
    } while ( he != he2 );

    //-----------------------------------------------------------------
    long i;

    // The loop of `he2` is the one removed. When it is the outer loop of
    // the face (`he1` is on a ring and `he2` on the outer boundary), the
    // merged loop is the outer boundary from now on and must take its
    // place. It used to stay where the ring of `he1` was, and whatever
    // other ring came first in the list was taken for the outer loop
    // (plan-stage11 Phase F).
    BOOLEAN killsOuterLoop =
        ringToKill->parentFace->boundariesList.get(0) == ringToKill;

    ringToKill->halfEdgesList.clear();
    ringToKill->parentFace->boundariesList.locateWindowAtElem(ringToKill);
    ringToKill->parentFace->boundariesList.removeElemAtWindow();
    // C++ port: the solid keeps the unlinked nodes (Java leaves them to
    // its garbage collector)
    solid->retireLoop(ringToKill);
    if ( killsOuterLoop ) {
        promoteLoopAsOuter(he1->parentLoop->parentFace, he1->parentLoop);
    }

    //-----------------------------------------------------------------
    _PolyhedralBoundedSolidEdge* newEdge;
    _PolyhedralBoundedSolidVertex* v1;
    _PolyhedralBoundedSolidVertex* v2;

    v1 = he1->startingVertex;
    v2 = he2->startingVertex;

    newEdge = new _PolyhedralBoundedSolidEdge();
    solid->getEdgesList().add(newEdge);

    _PolyhedralBoundedSolidHalfEdge* heLast;

    heLast = addhe(solid, newEdge, v2, he1, PolyhedralBoundedSolid::MINUS);
    newEdge->rightHalf = addhe(solid, newEdge, v1, heLast, PolyhedralBoundedSolid::MINUS);
    newEdge->leftHalf = heLast;

    // Alas! This rare condition of not adding a migrated half edges
    // list of size 1 is to avoid adding an 0-length half-edge with no
    // parent edge and no mirror edge when loop is of size 1 vertex.
    for ( i = 0;
          i < migratedHalfEdges.size() && migratedHalfEdges.size() > 1;
          i++ ) {
        he = migratedHalfEdges.get(i);
        he->parentLoop = he1->parentLoop;
        he1->parentLoop->halfEdgesList.insertBefore(he, heLast);
    }
    // heLast starts at v2 and is always in the merged loop; a lone ring
    // vertex had its edge-less half-edge dropped above (defect E-03).
    v2->emanatingHalfEdge = heLast;
}

//= HIGH LEVEL EULER OPERATIONS ===================================

/**
smev: "Strut" or line-drawing "Simplified" version of mev operator.
*/
BOOLEAN PolyhedralBoundedSolidEulerOperators::smev(
    PolyhedralBoundedSolid* solid, int f1, int v1, int v4, const Vector3Dd& p)
{
    _PolyhedralBoundedSolidFace* oldFace1;
    _PolyhedralBoundedSolidHalfEdge* he1;

    oldFace1 = solid->findFace(f1);
    if ( oldFace1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mev", FACE_MESSAGE + str(f1) + NOT_FOUND_MESSAGE);
        return FALSE;
    }
    he1 = oldFace1->findHalfEdge(v1);
    if ( he1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mev",
            EDGE_MESSAGE + str(v1) + EDGE_NOT_FOUND_IN_FACE_WILDCARD_MESSAGE +
            str(f1) + DOT);
        return FALSE;
    }
    lmev(solid, he1, he1, v4, p);
    return TRUE;
}

/**
mev: (high level version) make edge vertex (vertex splitting operation).
Edges from `v1` -> `v2` (inclusive) to `v1` -> `v3` (exclusive) become
adjacent to the new vertex `v4`.
*/
BOOLEAN PolyhedralBoundedSolidEulerOperators::mev(
    PolyhedralBoundedSolid* solid, int f1, int f2,
    int v1, int v2, int v3, int v4, const Vector3Dd& p)
{
    _PolyhedralBoundedSolidFace* oldFace1;
    _PolyhedralBoundedSolidFace* oldFace2;
    _PolyhedralBoundedSolidHalfEdge* he1;
    _PolyhedralBoundedSolidHalfEdge* he2;

    oldFace1 = solid->findFace(f1);
    if ( oldFace1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mev", FACE_MESSAGE + str(f1) + NOT_FOUND_MESSAGE);
        return FALSE;
    }
    oldFace2 = solid->findFace(f2);
    if ( oldFace2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mev", FACE_MESSAGE + str(f2) + NOT_FOUND_MESSAGE);
        return FALSE;
    }
    he1 = oldFace1->findHalfEdge(v1, v2);
    if ( he1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mev",
            EDGE_MESSAGE + str(v1) + " - " + str(v2) +
            EDGE_NOT_FOUND_IN_FACE_MESSAGE + str(f1) + DOT);
        return FALSE;
    }
    he2 = oldFace2->findHalfEdge(v1, v3);
    if ( he2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mev",
            EDGE_MESSAGE + str(v1) + " - " + str(v3) +
            EDGE_NOT_FOUND_IN_FACE_MESSAGE + str(f2) + DOT);
        return FALSE;
    }
    lmev(solid, he1, he2, v4, p);
    return TRUE;
}

/**
smef: simplified version of mef operator.
*/
BOOLEAN PolyhedralBoundedSolidEulerOperators::smef(
    PolyhedralBoundedSolid* solid, int f1, int v1, int v3, int newFaceId)
{
    _PolyhedralBoundedSolidFace* oldFace1;
    _PolyhedralBoundedSolidHalfEdge* he1;
    _PolyhedralBoundedSolidHalfEdge* he2;

    oldFace1 = solid->findFace(f1);
    if ( oldFace1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "smef", FACE_MESSAGE + str(f1) + NOT_FOUND_MESSAGE);
        return FALSE;
    }
    he1 = oldFace1->findHalfEdge(v1);
    if ( he1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "smef",
            EDGE_MESSAGE + str(v1) + EDGE_NOT_FOUND_IN_FACE_WILDCARD_MESSAGE +
            str(f1) + DOT);
        return FALSE;
    }
    he2 = oldFace1->findHalfEdge(v3);
    if ( he2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "smef",
            EDGE_MESSAGE + str(v3) + EDGE_NOT_FOUND_IN_FACE_WILDCARD_MESSAGE +
            str(f1) + DOT);
        return FALSE;
    }
    lmef(solid, he1, he2, newFaceId);
    return TRUE;
}

/**
mef: (high level version) make edge face (face splitting operation).
Executes a lmef in half-edges `v1`-`v2`, `v3`-`v4` of faces `f1` and `f2`.
*/
BOOLEAN PolyhedralBoundedSolidEulerOperators::mef(
    PolyhedralBoundedSolid* solid, int f1, int f2,
    int v1, int v2, int v3, int v4, int newFaceId)
{
    _PolyhedralBoundedSolidFace* oldFace1;
    _PolyhedralBoundedSolidFace* oldFace2;
    _PolyhedralBoundedSolidHalfEdge* he1;
    _PolyhedralBoundedSolidHalfEdge* he2;

    oldFace1 = solid->findFace(f1);
    if ( oldFace1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mef", FACE_MESSAGE + str(f1) + NOT_FOUND_MESSAGE);
        return FALSE;
    }
    oldFace2 = solid->findFace(f2);
    if ( oldFace2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mef", FACE_MESSAGE + str(f2) + NOT_FOUND_MESSAGE);
        return FALSE;
    }
    he1 = oldFace1->findHalfEdge(v1, v2);
    if ( he1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mef",
            EDGE_MESSAGE + str(v1) + " - " + str(v2) +
            EDGE_NOT_FOUND_IN_FACE_MESSAGE + str(f1) + DOT);
        return FALSE;
    }
    he2 = oldFace2->findHalfEdge(v3, v4);
    if ( he2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mef",
            EDGE_MESSAGE + str(v3) + " - " + str(v4) +
            EDGE_NOT_FOUND_IN_FACE_MESSAGE + str(f2) + DOT);
        return FALSE;
    }
    lmef(solid, he1, he2, newFaceId);
    return TRUE;
}

/**
kemr: (high level version) kill edge make ring (loop splitting operation).
*/
BOOLEAN PolyhedralBoundedSolidEulerOperators::kemr(
    PolyhedralBoundedSolid* solid, int f1, int f2,
    int v1, int v2, int v3, int v4)
{
    _PolyhedralBoundedSolidFace* oldFace1;
    _PolyhedralBoundedSolidFace* oldFace2;
    _PolyhedralBoundedSolidHalfEdge* he1;
    _PolyhedralBoundedSolidHalfEdge* he2;

    oldFace1 = solid->findFace(f1);
    if ( oldFace1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "kemr", FACE_MESSAGE + str(f1) + NOT_FOUND_MESSAGE);
        return FALSE;
    }
    oldFace2 = solid->findFace(f2);
    if ( oldFace2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "kemr", FACE_MESSAGE + str(f2) + NOT_FOUND_MESSAGE);
        return FALSE;
    }
    he1 = oldFace1->findHalfEdge(v1, v2);
    if ( he1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "kemr",
            EDGE_MESSAGE + str(v1) + " - " + str(v2) +
            EDGE_NOT_FOUND_IN_FACE_MESSAGE + str(f1) + DOT);
        return FALSE;
    }
    he2 = oldFace2->findHalfEdge(v3, v4);
    if ( he2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "kemr",
            EDGE_MESSAGE + str(v3) + " - " + str(v4) +
            EDGE_NOT_FOUND_IN_FACE_MESSAGE + str(f2) + DOT);
        return FALSE;
    }
    lkemr(solid, he1, he2);
    return TRUE;
}

/**
kfmrh: (high level version) KillFaceMakeRingHole, for the same-shell case
of section [MANT1988].9.2.4.
*/
BOOLEAN PolyhedralBoundedSolidEulerOperators::kfmrh(PolyhedralBoundedSolid* solid, int f1, int f2)
{
    _PolyhedralBoundedSolidFace* oldFace1;
    _PolyhedralBoundedSolidFace* oldFace2;

    oldFace1 = solid->findFace(f1);
    if ( oldFace1 == 0 ) {
        report(Logger::WARNING, "kfmrh", FACE_MESSAGE + str(f1) + NOT_FOUND_MESSAGE);
        return FALSE;
    }
    oldFace2 = solid->findFace(f2);
    if ( oldFace2 == 0 ) {
        report(Logger::WARNING, "kfmrh", FACE_MESSAGE + str(f2) + NOT_FOUND_MESSAGE);
        return FALSE;
    }
    lkfmrh(solid, oldFace1, oldFace2);
    return TRUE;
}

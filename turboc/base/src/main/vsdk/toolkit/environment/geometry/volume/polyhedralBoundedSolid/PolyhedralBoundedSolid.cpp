#include <float.h>
#include <math.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/StringBuilder.h"

#include "java/util/ArrayList.txx"
#include "java/util/TreeSet.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/element/Vertex2D.h"
#include "vsdk/toolkit/environment/geometry/surface/polygon/Polygon2D.h"
#include "vsdk/toolkit/processing/ComputationalGeometry.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidPredicates.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
#include "vsdk/toolkit/common/Cpp98Support.h"
#include "java/lang/Math.h"

static const char* const SOLID_WITH_MESSAGE = "Solid with ";

static Vector3Dd dropCoordinate(const Vector3Dd& in, int coordinate)
{
    switch ( coordinate ) {
      case 1:
        return Vector3Dd(in.y(), in.z(), 0);
      case 2:
        return Vector3Dd(in.x(), in.z(), 0);
      default:
        return Vector3Dd(in.x(), in.y(), 0);
    }
}

static int dominantCoordinateForPlane(const InfinitePlane* plane)
{
    Vector3Dd n = plane->getNormal();

    if ( fabs(n.x()) >= fabs(n.y()) &&
         fabs(n.x()) >= fabs(n.z()) ) {
        return 1;
    }
    if ( fabs(n.y()) >= fabs(n.x()) &&
         fabs(n.y()) >= fabs(n.z()) ) {
        return 2;
    }
    return 3;
}

/**
Point in face test used by ray intersection: each loop is classified with
`Polygon2D::isPointInsideContour` in the dominant projection plane,
and the point is inside when it is inside an odd number of loops.
@param face face to test
@param plane containing plane of `face`
@param point point over the containing plane
@param tolerance distance under which the point is at the face border
@return Geometry::INSIDE, Geometry::LIMIT or Geometry::OUTSIDE
*/
static int testPointInsideForRayIntersection(
    _PolyhedralBoundedSolidFace* face,
    const InfinitePlane* plane,
    const Vector3Dd& point,
    double tolerance)
{
    int dominantCoordinate;
    int insideLoopCount;
    long int i;
    Vector3Dd projectedPoint;

    if ( face == 0 || plane == 0 ) {
        return Geometry::OUTSIDE;
    }

    dominantCoordinate = dominantCoordinateForPlane(plane);
    projectedPoint = dropCoordinate(point, dominantCoordinate);
    Vertex2D projectedPoint2D(projectedPoint.x(), projectedPoint.y());
    insideLoopCount = 0;

    for ( i = 0; i < face->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(i);
        _PolyhedralBoundedSolidHalfEdge* he = loop->boundaryStartHalfEdge;
        _PolyhedralBoundedSolidHalfEdge* start;
        ArrayList<Vertex2D> projectedLoopVertices;
        signed char loopStatus;

        if ( he == 0 ) {
            return Geometry::OUTSIDE;
        }
        start = he;

        do {
            if ( point.subtract(he->startingVertex->position).length()
                 < 2 * tolerance ) {
                return Geometry::LIMIT;
            }
            if ( ComputationalGeometry::lineSegmentContainmentTest(
                     he->startingVertex->position,
                     he->next()->startingVertex->position,
                     point, tolerance) == Geometry::LIMIT ) {
                return Geometry::LIMIT;
            }

            projectedPoint = dropCoordinate(he->startingVertex->position,
                dominantCoordinate);
            projectedLoopVertices.add(
                Vertex2D(projectedPoint.x(), projectedPoint.y()));
            he = he->next();
        } while ( he != start );

        loopStatus = Polygon2D::isPointInsideContour(
            projectedPoint2D, projectedLoopVertices);
        if ( loopStatus == 0 ) {
            return Geometry::LIMIT;
        }
        if ( loopStatus > 0 ) {
            insideLoopCount++;
        }
    }

    return ((insideLoopCount % 2) == 1) ?
        Geometry::INSIDE : Geometry::OUTSIDE;
}

static String intPreSpaces(int val, int fieldSize)
{
    String cad = Cpp98Support::toString(val);
    String sb;
    int remain = fieldSize - (int)cad.length();

    for ( ; remain > 0; remain-- ) {
        sb += " ";
    }
    sb += cad;
    return String(sb.c_str());
}


PolyhedralBoundedSolid::PolyhedralBoundedSolid()
    : maxVertexId(-1),
      maxFaceId(-1),
      modelIsValid(FALSE),
      queryPlaneCache(0),
      queryFaceAabb(0),
      queryNumericContext(0),
      queryFaceCount(0)
{
}

PolyhedralBoundedSolid::~PolyhedralBoundedSolid()
{
    clearVisibilityQueryCache();
    long int i;
    for (i = 0; i < polygonsList.size(); ++i) delete polygonsList.get(i);
    for (i = 0; i < edgesList.size(); ++i) delete edgesList.get(i);
    for (i = 0; i < verticesList.size(); ++i) delete verticesList.get(i);
    for (i = 0; i < retiredFaces.size(); ++i) delete retiredFaces.get(i);
    for (i = 0; i < retiredLoops.size(); ++i) delete retiredLoops.get(i);
    for (i = 0; i < retiredHalfEdges.size(); ++i) delete retiredHalfEdges.get(i);
    for (i = 0; i < retiredEdges.size(); ++i) delete retiredEdges.get(i);
    for (i = 0; i < retiredVertices.size(); ++i) delete retiredVertices.get(i);
}

void PolyhedralBoundedSolid::retireFace(_PolyhedralBoundedSolidFace* face)
{
    if ( face == NULL ) return;
    face->boundariesList.clear();
    retiredFaces.add(face);
}

void PolyhedralBoundedSolid::retireLoop(_PolyhedralBoundedSolidLoop* loop)
{
    if ( loop == NULL ) return;
    loop->halfEdgesList.clear();
    retiredLoops.add(loop);
}

void PolyhedralBoundedSolid::retireHalfEdge(_PolyhedralBoundedSolidHalfEdge* he)
{
    if ( he == NULL ) return;
    retiredHalfEdges.add(he);
}

void PolyhedralBoundedSolid::retireEdge(_PolyhedralBoundedSolidEdge* edge)
{
    if ( edge == NULL ) return;
    retiredEdges.add(edge);
}

void PolyhedralBoundedSolid::retireVertex(_PolyhedralBoundedSolidVertex* vertex)
{
    if ( vertex == NULL ) return;
    retiredVertices.add(vertex);
}

/*
C++ port note: overloads per node type instead of function templates. Turbo
C++ 3.0 rejects `static` function templates and cannot deduce `T` from an
argument such as `CircularArrayList<T*>&`.
*/
#define VSDK_RETIRED_NODE_HELPERS(T) \
static void removeOwnedFromRetired(ArrayList<T*>& list, TreeSet<T*>& set) \
{ \
    long int i = 0; \
    while ( i < list.size() ) { \
        if ( set.contains(list.get(i)) ) list.remove(i); \
        else i++; \
    } \
}

#define VSDK_LISTED_NODE_HELPERS(T) \
VSDK_RETIRED_NODE_HELPERS(T) \
static void collectOwnedNodes(TreeSet<T*>& set, CircularArrayList<T*>& live, \
    ArrayList<T*>& retired) \
{ \
    long int i; \
    for ( i = 0; i < live.size(); i++ ) set.add(live.get(i)); \
    for ( i = 0; i < retired.size(); i++ ) set.add(retired.get(i)); \
} \
static void removeOwnedFromLive(CircularArrayList<T*>& list, TreeSet<T*>& set) \
{ \
    long int i = 0; \
    while ( i < list.size() ) { \
        if ( set.contains(list.get(i)) ) list.remove(i); \
        else i++; \
    } \
}

VSDK_LISTED_NODE_HELPERS(_PolyhedralBoundedSolidFace)
VSDK_LISTED_NODE_HELPERS(_PolyhedralBoundedSolidEdge)
VSDK_LISTED_NODE_HELPERS(_PolyhedralBoundedSolidVertex)
VSDK_RETIRED_NODE_HELPERS(_PolyhedralBoundedSolidLoop)
VSDK_RETIRED_NODE_HELPERS(_PolyhedralBoundedSolidHalfEdge)

void PolyhedralBoundedSolid::unlistNodesOwnedBy(PolyhedralBoundedSolid* reference)
{
    if ( reference == NULL || reference == this ) return;
    TreeSet<_PolyhedralBoundedSolidFace*> faces;
    TreeSet<_PolyhedralBoundedSolidEdge*> edges;
    TreeSet<_PolyhedralBoundedSolidVertex*> vertices;
    TreeSet<_PolyhedralBoundedSolidLoop*> loops;
    TreeSet<_PolyhedralBoundedSolidHalfEdge*> halfEdges;
    long int i;

    collectOwnedNodes(faces, reference->polygonsList, reference->retiredFaces);
    collectOwnedNodes(edges, reference->edgesList, reference->retiredEdges);
    collectOwnedNodes(vertices, reference->verticesList, reference->retiredVertices);
    for ( i = 0; i < reference->retiredLoops.size(); i++ ) {
        loops.add(reference->retiredLoops.get(i));
    }
    for ( i = 0; i < reference->retiredHalfEdges.size(); i++ ) {
        halfEdges.add(reference->retiredHalfEdges.get(i));
    }
    removeOwnedFromLive(polygonsList, faces);
    removeOwnedFromLive(edgesList, edges);
    removeOwnedFromLive(verticesList, vertices);
    removeOwnedFromRetired(retiredFaces, faces);
    removeOwnedFromRetired(retiredEdges, edges);
    removeOwnedFromRetired(retiredVertices, vertices);
    removeOwnedFromRetired(retiredLoops, loops);
    removeOwnedFromRetired(retiredHalfEdges, halfEdges);
}

_PolyhedralBoundedSolidFace* PolyhedralBoundedSolid::findFace(int id)
{
    for (long int i = 0; i < polygonsList.size(); ++i) {
        if (polygonsList.get(i) != 0 && polygonsList.get(i)->id == id) {
            return polygonsList.get(i);
        }
    }
    return 0;
}

_PolyhedralBoundedSolidVertex* PolyhedralBoundedSolid::findVertex(int id)
{
    for (long int i = 0; i < verticesList.size(); ++i) {
        if (verticesList.get(i) != 0 && verticesList.get(i)->id == id) {
            return verticesList.get(i);
        }
    }
    return 0;
}

CircularArrayList<_PolyhedralBoundedSolidFace*>&
PolyhedralBoundedSolid::getPolygonsList()
{
    return polygonsList;
}

void PolyhedralBoundedSolid::setPolygonsList(
    CircularArrayList<_PolyhedralBoundedSolidFace*>& v)
{
    polygonsList.clear();
    for (long int i = 0; i < v.size(); ++i) {
        polygonsList.add(v.get(i));
    }
}

CircularArrayList<_PolyhedralBoundedSolidEdge*>&
PolyhedralBoundedSolid::getEdgesList()
{
    return edgesList;
}

void PolyhedralBoundedSolid::setEdgesList(
    CircularArrayList<_PolyhedralBoundedSolidEdge*>& v)
{
    edgesList.clear();
    for (long int i = 0; i < v.size(); ++i) {
        edgesList.add(v.get(i));
    }
}

CircularArrayList<_PolyhedralBoundedSolidVertex*>&
PolyhedralBoundedSolid::getVerticesList()
{
    return verticesList;
}

void PolyhedralBoundedSolid::setVerticesList(
    CircularArrayList<_PolyhedralBoundedSolidVertex*>& v)
{
    verticesList.clear();
    for (long int i = 0; i < v.size(); ++i) {
        verticesList.add(v.get(i));
    }
}

int PolyhedralBoundedSolid::getMaxVertexId() const { return maxVertexId; }
void PolyhedralBoundedSolid::setMaxVertexId(int v) { maxVertexId = v; }
int PolyhedralBoundedSolid::getMaxFaceId() const { return maxFaceId; }
void PolyhedralBoundedSolid::setMaxFaceId(int v) { maxFaceId = v; }

Ray* PolyhedralBoundedSolid::doIntersectionFirstHit(const Ray& inOutRay)
{
    RayHit hit(RayHit::DETAIL_NONE, TRUE);
    if (doIntersectionFirstHit(inOutRay, &hit) && hit.getRay() != 0) {
        return new Ray(*hit.getRay());
    }
    return 0;
}

BOOLEAN PolyhedralBoundedSolid::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit)
{
    double minT = DBL_MAX;
    RayHit bestInfo;
    BOOLEAN found = FALSE;
    PolyhedralBoundedSolidNumericPolicy::ToleranceContext numericContext =
        PolyhedralBoundedSolidNumericPolicy::forSolid(this);

    for (long int i = 0; i < polygonsList.size(); ++i) {
        _PolyhedralBoundedSolidFace* face = polygonsList.get(i);
        if (face == 0) {
            continue;
        }

        InfinitePlane* containingPlane = face->getContainingPlane();
        if (containingPlane == 0) {
            continue;
        }

        RayHit planeHit;
        BOOLEAN hitsPlane = containingPlane->doIntersectionFirstHit(inRay, &planeHit);
        if (hitsPlane && planeHit.getRay() != 0 && planeHit.getRay()->getT() < minT) {
            Ray hit = *(planeHit.getRay());
            hit = hit.withDirection(hit.getDirection().normalized());
            Vector3Dd p = hit.getOrigin().add(hit.getDirection().multiply(hit.getT()));
            int pos = testPointInsideForRayIntersection(
                face, containingPlane, p, numericContext.bigEpsilon());
            if (pos == Geometry::INSIDE || pos == Geometry::LIMIT) {
                minT = hit.getT();
                bestInfo = planeHit;
                found = TRUE;
            }
        }

        delete containingPlane;
    }

    if (!found) {
        return FALSE;
    }

    if (outHit != 0) {
        outHit->clone(bestInfo);
        outHit->setRay(inRay.withT(minT));
    }
    return TRUE;
}

void PolyhedralBoundedSolid::doExtraInformation(
    const Ray& inRay,
    double inT,
    RayHit* outData)
{
    if (outData == 0) {
        return;
    }
    doIntersectionFirstHit(inRay.withT(inT), outData);
}

double* PolyhedralBoundedSolid::getMinMax()
{
    double* minMax = new double[6];
    double minX = DBL_MAX;
    double minY = DBL_MAX;
    double minZ = DBL_MAX;
    double maxX = -DBL_MAX;
    double maxY = -DBL_MAX;
    double maxZ = -DBL_MAX;

    for (long int i = 0; i < verticesList.size(); ++i) {
        _PolyhedralBoundedSolidVertex* v = verticesList.get(i);
        if (v == 0) {
            continue;
        }
        double x = v->position.x();
        double y = v->position.y();
        double z = v->position.z();

        if ( x < minX ) minX = x;
        if ( y < minY ) minY = y;
        if ( z < minZ ) minZ = z;
        if ( x > maxX ) maxX = x;
        if ( y > maxY ) maxY = y;
        if ( z > maxZ ) maxZ = z;
    }

    minMax[0] = minX;
    minMax[1] = minY;
    minMax[2] = minZ;
    minMax[3] = maxX;
    minMax[4] = maxY;
    minMax[5] = maxZ;
    return minMax;
}

BOOLEAN PolyhedralBoundedSolid::isValid() const { return modelIsValid; }
void PolyhedralBoundedSolid::setValidationState(BOOLEAN flag) { modelIsValid = flag; }

void PolyhedralBoundedSolid::merge(PolyhedralBoundedSolid* other)
{
    if (other == 0) return;
    int offsetFacesId = getMaxFaceId();
    int offsetVertexId = getMaxVertexId();

    while (other->getPolygonsList().size() != 0) {
        _PolyhedralBoundedSolidFace* f = other->getPolygonsList().get(0);
        f->id += offsetFacesId;
        // The merged face now belongs to this solid: Euler operators
        // (i.e. `lkfmrh` when gluing) check `parentSolid`
        f->parentSolid = this;
        if (f->id > maxFaceId) maxFaceId = f->id;
        polygonsList.add(f);
        other->getPolygonsList().remove(0L);
    }
    while (other->getEdgesList().size() != 0) {
        edgesList.add(other->getEdgesList().get(0));
        other->getEdgesList().remove(0L);
    }
    while (other->getVerticesList().size() != 0) {
        _PolyhedralBoundedSolidVertex* v = other->getVerticesList().get(0);
        v->id += offsetVertexId;
        if (v->id > maxVertexId) maxVertexId = v->id;
        verticesList.add(v);
        other->getVerticesList().remove(0L);
    }
}

void PolyhedralBoundedSolid::computeFaceAabb(
    _PolyhedralBoundedSolidFace* face,
    int index)
{
    double minX = DBL_MAX;
    double minY = DBL_MAX;
    double minZ = DBL_MAX;
    double maxX = -DBL_MAX;
    double maxY = -DBL_MAX;
    double maxZ = -DBL_MAX;

    if (face != 0) {
        for (long int b = 0; b < face->boundariesList.size(); ++b) {
            _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(b);
            if (loop == 0 || loop->boundaryStartHalfEdge == 0) {
                continue;
            }

            _PolyhedralBoundedSolidHalfEdge* he = loop->boundaryStartHalfEdge;
            _PolyhedralBoundedSolidHalfEdge* start = he;
            do {
                if (he->startingVertex != 0) {
                    Vector3Dd q = he->startingVertex->position;
                    minX = Math::min(minX, q.x());
                    minY = Math::min(minY, q.y());
                    minZ = Math::min(minZ, q.z());
                    maxX = Math::max(maxX, q.x());
                    maxY = Math::max(maxY, q.y());
                    maxZ = Math::max(maxZ, q.z());
                }
                he = he->next();
            } while (he != 0 && he != start);
        }
    }

    if (minX == DBL_MAX) {
        minX = minY = minZ = maxX = maxY = maxZ = 0.0;
    }

    int o = index * 6;
    queryFaceAabb[o] = minX;
    queryFaceAabb[o + 1] = minY;
    queryFaceAabb[o + 2] = minZ;
    queryFaceAabb[o + 3] = maxX;
    queryFaceAabb[o + 4] = maxY;
    queryFaceAabb[o + 5] = maxZ;
}

BOOLEAN PolyhedralBoundedSolid::rayReachesFaceAabb(
    const Vector3Dd& origin,
    double dirX,
    double dirY,
    double dirZ,
    double maxT,
    int faceIndex,
    double pad)
{
    int o = faceIndex * 6;
    double tMin = 0.0;
    double tMax = maxT;

    if (Cpp98Support::abs(dirX) < 1.0e-12) {
        if (origin.x() < queryFaceAabb[o] - pad ||
            origin.x() > queryFaceAabb[o + 3] + pad) {
            return FALSE;
        }
    }
    else {
        double t1 = (queryFaceAabb[o] - pad - origin.x()) / dirX;
        double t2 = (queryFaceAabb[o + 3] + pad - origin.x()) / dirX;
        if (t1 > t2) { double t = t1; t1 = t2; t2 = t; }
        if (t1 > tMin) tMin = t1;
        if (t2 < tMax) tMax = t2;
        if (tMin > tMax) return FALSE;
    }

    if (Cpp98Support::abs(dirY) < 1.0e-12) {
        if (origin.y() < queryFaceAabb[o + 1] - pad ||
            origin.y() > queryFaceAabb[o + 4] + pad) {
            return FALSE;
        }
    }
    else {
        double t1 = (queryFaceAabb[o + 1] - pad - origin.y()) / dirY;
        double t2 = (queryFaceAabb[o + 4] + pad - origin.y()) / dirY;
        if (t1 > t2) { double t = t1; t1 = t2; t2 = t; }
        if (t1 > tMin) tMin = t1;
        if (t2 < tMax) tMax = t2;
        if (tMin > tMax) return FALSE;
    }

    if (Cpp98Support::abs(dirZ) < 1.0e-12) {
        if (origin.z() < queryFaceAabb[o + 2] - pad ||
            origin.z() > queryFaceAabb[o + 5] + pad) {
            return FALSE;
        }
    }
    else {
        double t1 = (queryFaceAabb[o + 2] - pad - origin.z()) / dirZ;
        double t2 = (queryFaceAabb[o + 5] + pad - origin.z()) / dirZ;
        if (t1 > t2) { double t = t1; t1 = t2; t2 = t; }
        if (t1 > tMin) tMin = t1;
        if (t2 < tMax) tMax = t2;
        if (tMin > tMax) return FALSE;
    }

    return TRUE;
}

void PolyhedralBoundedSolid::clearVisibilityQueryCache()
{
    if (queryPlaneCache != 0) {
        for (int i = 0; i < queryFaceCount; ++i) {
            delete queryPlaneCache[i];
        }
        delete[] queryPlaneCache;
        queryPlaneCache = 0;
    }
    delete[] queryFaceAabb;
    queryFaceAabb = 0;
    delete queryNumericContext;
    queryNumericContext = 0;
    queryFaceCount = 0;
}

void PolyhedralBoundedSolid::beginVisibilityQueries()
{
    clearVisibilityQueryCache();

    queryNumericContext = new PolyhedralBoundedSolidNumericPolicy::ToleranceContext(
        PolyhedralBoundedSolidNumericPolicy::forSolid(this));
    queryFaceCount = ((int)(polygonsList.size()));
    queryPlaneCache = new InfinitePlane*[queryFaceCount];
    queryFaceAabb = new double[queryFaceCount * 6];

    for (int i = 0; i < queryFaceCount; ++i) {
        queryPlaneCache[i] = 0;
        _PolyhedralBoundedSolidFace* face = polygonsList.get(i);
        if (face != 0) {
            queryPlaneCache[i] = face->getContainingPlane();
        }
        computeFaceAabb(face, i);
    }
}

void PolyhedralBoundedSolid::endVisibilityQueries()
{
    clearVisibilityQueryCache();
}

BOOLEAN PolyhedralBoundedSolid::visibilityQueriesActive() const
{
    return queryPlaneCache != 0;
}

InfinitePlane* PolyhedralBoundedSolid::cachedFacePlane(int faceIndex)
{
    if (queryPlaneCache != 0 &&
        faceIndex >= 0 &&
        faceIndex < queryFaceCount) {
        return queryPlaneCache[faceIndex];
    }
    if (faceIndex < 0 || faceIndex >= polygonsList.size()) {
        return 0;
    }
    return polygonsList.get(faceIndex)->getContainingPlane();
}

PolyhedralBoundedSolidNumericPolicy::ToleranceContext
PolyhedralBoundedSolid::queryToleranceContext()
{
    if (queryNumericContext != 0) {
        return *queryNumericContext;
    }
    return PolyhedralBoundedSolidNumericPolicy::forSolid(this);
}

BOOLEAN PolyhedralBoundedSolid::queryRayReachesFace(
    const Vector3Dd& origin,
    double dirX,
    double dirY,
    double dirZ,
    double maxT,
    int faceIndex,
    double pad)
{
    if (queryFaceAabb == 0) {
        return TRUE;
    }
    return rayReachesFaceAabb(
        origin, dirX, dirY, dirZ, maxT, faceIndex, pad);
}

BOOLEAN PolyhedralBoundedSolid::queryPointNearFace(
    const Vector3Dd& point,
    int faceIndex,
    double pad)
{
    if (queryFaceAabb == 0) {
        return TRUE;
    }
    int o = faceIndex * 6;
    return point.x() >= queryFaceAabb[o] - pad &&
        point.x() <= queryFaceAabb[o + 3] + pad &&
        point.y() >= queryFaceAabb[o + 1] - pad &&
        point.y() <= queryFaceAabb[o + 4] + pad &&
        point.z() >= queryFaceAabb[o + 2] - pad &&
        point.z() <= queryFaceAabb[o + 5] + pad;
}

/**
Check the general interface contract in superclass method
Geometry.doContainmentTest. Delegated to the robust predicate layer
(`PolyhedralBoundedSolidPredicates::classifyPoint`): a point on a face is
`LIMIT`, any other is classified by a ray-cast parity test.

C++ port note: the point is a reference, so the Java check for a null
point is not needed.
@param p point to classify, in the space of the solid
@param distanceTolerance distance to the faces under which the point is
classified as `LIMIT`
@return INSIDE, OUTSIDE or LIMIT constant value
*/
int PolyhedralBoundedSolid::doContainmentTest(const Vector3Dd& p,
                                              double distanceTolerance)
{
    return PolyhedralBoundedSolidPredicates::classifyPoint(this, p,
        distanceTolerance);
}

int PolyhedralBoundedSolid::computeQuantitativeInvisibility(
    const Vector3Dd& origin,
    const Vector3Dd& p)
{
    return PolyhedralBoundedSolidPredicates::quantitativeInvisibility(
        this, origin, p);
}

int PolyhedralBoundedSolid::compareValue(double a, double b, double tolerance)
{
    double delta = Cpp98Support::abs(a - b);
    if (delta < tolerance) return 0;
    return (a > b) ? 1 : -1;
}

void PolyhedralBoundedSolid::revert()
{
    for (long int i = 0; i < polygonsList.size(); ++i) {
        if (polygonsList.get(i) != 0) {
            polygonsList.get(i)->revert();
        }
    }
}

String PolyhedralBoundedSolid::toString()
{
    StringBuilder msg;
    long int i;
    long int j;

    msg.append("= POLYHEDRAL BOUNDED SOLID STRUCTURE ==========================================\n");
    msg.append(SOLID_WITH_MESSAGE).append(verticesList.size()).append(" vertices:\n");
    for ( i = 0; i < verticesList.size(); i++ ) {
        msg.append("  - ").append(verticesList.get(i)->toString().c_str()).append("\n");
    }

    msg.append(SOLID_WITH_MESSAGE).append(edgesList.size()).append(" edges:\n");
    for ( i = 0; i < edgesList.size(); i++ ) {
        msg.append("  - ").append(edgesList.get(i)->toString().c_str()).append("\n");
    }
    msg.append(SOLID_WITH_MESSAGE).append(polygonsList.size()).append(" faces:\n");

    for ( i = 0; i < polygonsList.size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = polygonsList.get(i);
        msg.append("  - ").append(face->toString().c_str()).append("\n");
        for ( j = 0; j < face->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop* loop;
            _PolyhedralBoundedSolidHalfEdge* he;
            _PolyhedralBoundedSolidHalfEdge* heStart;

            msg.append("    . Loop ").append(j).append(", with half-edges: \n");
            loop = face->boundariesList.get(j);

            msg.append("      | HeID  | StartVertex | End Vertex | nccw He | pccw He | parentEdge | mirror He | neighbor face\n");
            msg.append("      +-------+-------------+------------+---------+---------+------------+-----------+-------------+\n");

            he = loop->boundaryStartHalfEdge;
            if ( he == 0 ) {
                msg.append("<Loop without starting half-edge!>\n");
                continue;
            }
            heStart = he;
            do {
                he = he->next();
                if ( he == 0 ) {
                    // Loop is not closed!
                    msg.append("      |  - (not closed loop)\n");
                    break;
                }

                msg.append("      | ")
                    .append(intPreSpaces(he->id, 4).c_str())
                    .append(((he == loop->boundaryStartHalfEdge) ? "*" : " "))
                    .append(" | ")
                    .append(intPreSpaces(he->startingVertex->id, 11).c_str())
                    .append(" | ")
                    .append(intPreSpaces(he->next()->startingVertex->id, 10).c_str())
                    .append(" | ")
                    .append(intPreSpaces(he->next()->id, 7).c_str())
                    .append(" | ")
                    .append(intPreSpaces(he->previous()->id, 7).c_str())
                    .append(" | ");
                if ( he->parentEdge != 0 ) {
                    msg.append(intPreSpaces(he->parentEdge->id, 10).c_str());
                }
                else {
                    msg.append("    <null>");
                }
                msg.append(" | ");
                if ( he->mirrorHalfEdge() != 0 ) {
                    msg.append(intPreSpaces(he->mirrorHalfEdge()->id, 9).c_str())
                        .append(" | ")
                        .append(intPreSpaces(he->mirrorHalfEdge()->parentLoop->parentFace->id, 11).c_str())
                        .append(" | ");
                }
                else {
                    msg.append(" No Mirror Half Edge!   | ");
                }

                msg.append("\n");

            } while ( he != heStart );
        }
    }
    msg.append("= END OF POLYHEDRAL BOUNDED SOLID STRUCTURE ===================================\n");
    return msg.toString();
}

PolyhedralBoundedSolid* PolyhedralBoundedSolid::exportToPolyhedralBoundedSolid()
{
    return this;
}

const Class PolyhedralBoundedSolid::CLASS = { "PolyhedralBoundedSolid", &Solid::CLASS };

const Class *
PolyhedralBoundedSolid::getClass() const
{
    return &CLASS;
}

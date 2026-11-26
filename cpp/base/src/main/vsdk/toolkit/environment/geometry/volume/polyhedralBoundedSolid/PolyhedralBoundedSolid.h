#ifndef __POLYHEDRAL_BOUNDED_SOLID__
#define __POLYHEDRAL_BOUNDED_SOLID__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/dataStructures/CircularArrayList.h"
#include "vsdk/toolkit/environment/geometry/volume/Solid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"
class Ray;
class RayHit;
class Vector3Dd;
class InfinitePlane;
class _PolyhedralBoundedSolidFace;
class _PolyhedralBoundedSolidEdge;
class _PolyhedralBoundedSolidVertex;
class _PolyhedralBoundedSolidLoop;
class _PolyhedralBoundedSolidHalfEdge;

class PolyhedralBoundedSolid : public Solid {
public:
    static const int PLUS = 1;
    static const int MINUS = 0;

private:
    CircularArrayList<_PolyhedralBoundedSolidFace*> polygonsList;
    CircularArrayList<_PolyhedralBoundedSolidEdge*> edgesList;
    CircularArrayList<_PolyhedralBoundedSolidVertex*> verticesList;
    int maxVertexId;
    int maxFaceId;
    bool modelIsValid;

    // C++ port: nodes unlinked by the Euler operators (see retire*). Java
    // leaves them to its garbage collector.
    java::ArrayList<_PolyhedralBoundedSolidFace*> retiredFaces;
    java::ArrayList<_PolyhedralBoundedSolidLoop*> retiredLoops;
    java::ArrayList<_PolyhedralBoundedSolidHalfEdge*> retiredHalfEdges;
    java::ArrayList<_PolyhedralBoundedSolidEdge*> retiredEdges;
    java::ArrayList<_PolyhedralBoundedSolidVertex*> retiredVertices;

    InfinitePlane** queryPlaneCache;
    double* queryFaceAabb;
    PolyhedralBoundedSolidNumericPolicy::ToleranceContext* queryNumericContext;
    int queryFaceCount;

    void computeFaceAabb(_PolyhedralBoundedSolidFace* face, int index);
    bool rayReachesFaceAabb(
        const Vector3Dd& origin,
        double dirX,
        double dirY,
        double dirZ,
        double maxT,
        int faceIndex,
        double pad);
    void clearVisibilityQueryCache();

public:
    PolyhedralBoundedSolid();
    virtual ~PolyhedralBoundedSolid() override;

    // C++ port: the Euler operators hand the nodes they unlink to these
    // methods instead of deleting them, since the callers may still read
    // them after the operator returns (Java leaves them to its garbage
    // collector). The solid deletes them when it is deleted. A retired face
    // or loop must not hold nodes anymore: its list is emptied here, without
    // deleting the elements.
    void retireFace(_PolyhedralBoundedSolidFace* face);
    void retireLoop(_PolyhedralBoundedSolidLoop* loop);
    void retireHalfEdge(_PolyhedralBoundedSolidHalfEdge* he);
    void retireEdge(_PolyhedralBoundedSolidEdge* edge);
    void retireVertex(_PolyhedralBoundedSolidVertex* vertex);
    /**
    C++ port: a boolean operation moves nodes between the copies of its
    operands and its result, and a node can stay listed in more than one of
    them for a while. This removes, from the lists of this solid (live and
    retired), every node that `reference` lists (live or retired), so that
    each node has one owner.
    */
    void unlistNodesOwnedBy(PolyhedralBoundedSolid* reference);

    _PolyhedralBoundedSolidFace* findFace(int id);
    _PolyhedralBoundedSolidVertex* findVertex(int id);

    CircularArrayList<_PolyhedralBoundedSolidFace*>& getPolygonsList();
    void setPolygonsList(CircularArrayList<_PolyhedralBoundedSolidFace*>& polygonsList);
    CircularArrayList<_PolyhedralBoundedSolidEdge*>& getEdgesList();
    void setEdgesList(CircularArrayList<_PolyhedralBoundedSolidEdge*>& edgesList);
    CircularArrayList<_PolyhedralBoundedSolidVertex*>& getVerticesList();
    void setVerticesList(CircularArrayList<_PolyhedralBoundedSolidVertex*>& verticesList);

    int getMaxVertexId() const;
    void setMaxVertexId(int maxVertexId);
    int getMaxFaceId() const;
    void setMaxFaceId(int maxFaceId);

    Ray* doIntersectionFirstHit(const Ray& inOutRay);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit) override;
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData) override;
    virtual double* getMinMax() override;

    bool isValid() const;
    void setValidationState(bool flag);

    void merge(PolyhedralBoundedSolid* other);

    void beginVisibilityQueries();
    void endVisibilityQueries();
    bool visibilityQueriesActive() const;
    InfinitePlane* cachedFacePlane(int faceIndex);
    PolyhedralBoundedSolidNumericPolicy::ToleranceContext queryToleranceContext();
    bool queryRayReachesFace(
        const Vector3Dd& origin,
        double dirX,
        double dirY,
        double dirZ,
        double maxT,
        int faceIndex,
        double pad);
    bool queryPointNearFace(
        const Vector3Dd& point,
        int faceIndex,
        double pad);

    virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance) override;
    virtual int computeQuantitativeInvisibility(const Vector3Dd& origin, const Vector3Dd& p) override;
    static int compareValue(double a, double b, double tolerance);
    void revert();

    /**
    @return a textual dump of the vertices, edges, faces and loops of the
    solid, as Java `toString`
    */
    java::String toString();

    virtual PolyhedralBoundedSolid* exportToPolyhedralBoundedSolid() override;
};

#endif

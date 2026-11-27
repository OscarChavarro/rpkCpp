#ifndef __TRNGL_ACCLRTN_STRCTR__
#define __TRNGL_ACCLRTN_STRCTR__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/TriangleMeshAccelerationMethod.h"

/**
Ray acceleration structure over the triangles of an indexed triangle mesh
(`TriangleMesh` arrays: three coordinates per vertex, three vertex indexes
per triangle). Every implementation answers exactly what the brute force
loop answers (`firstHitBruteForce`): the closest hit of
`Triangle::intersectRay`, ties resolved by the lowest triangle index, with
the same `t`, `u`, `v` values.

Built once, single threaded, before tracing; queries are read only and safe
from several threads. The structure is a snapshot of the mesh: it must be
rebuilt when the vertices or triangles change (`TriangleMesh` releases it
from its own mutators and checks the counts before using it).
*/
class TriangleAccelerationStructure {
  protected:
    int builtVertexCount;
    int builtTriangleCount;
    double vertexBounds[6];

    /** Stores the sizes and the vertex bounds; every `build` calls it */
    void recordBuiltMesh(const ArrayList<double> &positions,
        const ArrayList<int> &indexes);

  public:
    TriangleAccelerationStructure();
    virtual ~TriangleAccelerationStructure();

    virtual TriangleMeshAccelerationMethod::Value getMethod() const = 0;

    virtual void build(const ArrayList<double> &positions,
        const ArrayList<int> &indexes) = 0;

    /**
    Closest hit of the ray against the triangles given to `build` (the same
    arrays must be passed). Returns false on a miss.
    */
    virtual BOOLEAN firstHit(const Ray &ray,
        const ArrayList<double> &positions,
        const ArrayList<int> &indexes,
        double &outT, double &outU, double &outV, int &outTriangle) const = 0;

    /** Caller owns the returned copy */
    virtual TriangleAccelerationStructure *copy() const = 0;

    /** Number of nodes, for reports */
    virtual long int getNodeCount() const = 0;

    /** Depth of the hierarchy (a single leaf has depth 1), for reports */
    virtual int getDepth() const = 0;

    /** True when built for meshes of these sizes */
    BOOLEAN matches(int vertexCount, int triangleCount) const;

    /**
    Minimum and maximum vertex coordinates when built (x, y, z, X, Y, Z),
    computed exactly as `TriangleMesh::getMinMax`, which walks every vertex
    on each call: callers testing a ray against the mesh bounds use this
    copy instead.
    */
    void getVertexBounds(double *outMinMax) const;

    /** Enlarges `[lo, hi]` a little, so that rounding never hides a hit of
    `Triangle::intersectRay` inside it */
    static void padBox(double *lo, double *hi);

    /** Box of one triangle, padded with `padBox` */
    static void triangleBox(const ArrayList<double> &positions,
        const ArrayList<int> &indexes, int triangle,
        double *lo, double *hi);

    /** `Triangle::intersectRay` over one triangle of the arrays */
    static BOOLEAN testTriangle(const Ray &ray,
        const ArrayList<double> &positions,
        const ArrayList<int> &indexes, int triangle,
        double &t, double &u, double &v);

    /** The reference result: every triangle is tested */
    static BOOLEAN firstHitBruteForce(const Ray &ray,
        const ArrayList<double> &positions,
        const ArrayList<int> &indexes,
        double &outT, double &outU, double &outV, int &outTriangle);

    /** New, unbuilt structure for the method; null for `BRUTE_FORCE` */
    static TriangleAccelerationStructure *create(TriangleMeshAccelerationMethod::Value method);
};

/**
Owning slot for an optional `TriangleAccelerationStructure`, copied deeply,
so a class holding one keeps its implicit copy and assignment.
*/
class TriangleAccelerationStructureSlot {
  private:
    TriangleAccelerationStructure *structure;

  public:
    TriangleAccelerationStructureSlot();
    TriangleAccelerationStructureSlot(const TriangleAccelerationStructureSlot &other);
    TriangleAccelerationStructureSlot &operator=(const TriangleAccelerationStructureSlot &other);
    ~TriangleAccelerationStructureSlot();

    const TriangleAccelerationStructure *get() const;
    /** Takes ownership of `newStructure` (may be null) */
    void reset(TriangleAccelerationStructure *newStructure);
};

inline const TriangleAccelerationStructure *
TriangleAccelerationStructureSlot::get() const
{
    return structure;
}

#endif

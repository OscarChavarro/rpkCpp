#ifndef __TRIANGLE_MESH_GROUP__
#define __TRIANGLE_MESH_GROUP__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/surface/Surface.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/BinnedSahHierarchy.h"
class Ray;
class RayHit;

class TriangleMeshGroup : public Surface {
private:
    java::ArrayList<TriangleMesh> meshes;
    int intersectionMeshIndex;
    int intersectionTriangleIndex;

    // Top level ray acceleration over the meshes (the TLAS role of the
    // Vulkan raytracer), see prepareAccelerationStructures. Valid while
    // preparedMeshCount == meshes.size(); -1 when there is none.
    BinnedSahHierarchy meshHierarchy;
    long int preparedMeshCount;
    double preparedBounds[6];

    double* computeMinMaxPositions();
    void buildMeshHierarchy();
    void setIntersectionIndexes(int mesh, int triangle);

public:
    TriangleMeshGroup();
    TriangleMeshGroup(const java::ArrayList<TriangleMesh>& meshes);
    TriangleMeshGroup(const TriangleMeshGroup& group);

    java::ArrayList<TriangleMesh>& getMeshes();
    void setMeshes(const java::ArrayList<TriangleMesh>& meshes);
    void addMesh(const TriangleMesh& mesh);
    TriangleMesh& getMeshAt(int index);

    virtual double* getMinMax();
    void calculateMinMaxPositions();

    Ray* doIntersectionFirstHit(const Ray& inOut_Ray);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData);

    int* doIntersectionInformation();

    /**
    `TriangleMesh::prepareAccelerationStructure` on every mesh and, unless
    the method is `BRUTE_FORCE`, a `BinnedSahHierarchy` ([WALD2007]) over the
    mesh boxes, so a ray visits the meshes nearest first instead of all of
    them. Single threaded and idempotent: a second call with the same method
    and unchanged meshes only reads. The hit does not depend on the method.
    `setMeshes` / `addMesh` drop the top level; changes made to the meshes
    through `getMeshes` / `getMeshAt` require calling this method again.
    */
    void prepareAccelerationStructures(TriangleMeshAccelerationMethod method);
    /** `getMinMax` from the prepared structures, when every mesh has one */
    bool getPreparedMinMax(double* outMinMax) const;
    /** Top level hierarchy over the meshes, or null when not prepared */
    const BinnedSahHierarchy* getMeshHierarchy() const;

    virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance);

    java::String toString() const;

    TriangleMeshGroup* exportToTriangleMeshGroup();
};

#endif

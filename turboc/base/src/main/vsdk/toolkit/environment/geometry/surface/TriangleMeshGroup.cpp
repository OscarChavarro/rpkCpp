#include <stdio.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMeshGroup.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/TriangleAccelerationStructure.h"
TriangleMeshGroup::TriangleMeshGroup()
    : intersectionMeshIndex(-1), intersectionTriangleIndex(-1), preparedMeshCount(-1)
{
}

TriangleMeshGroup::TriangleMeshGroup(const ArrayList<TriangleMesh>& meshes)
    : meshes(meshes), intersectionMeshIndex(-1), intersectionTriangleIndex(-1),
      preparedMeshCount(-1)
{
}

TriangleMeshGroup::TriangleMeshGroup(const TriangleMeshGroup& group)
    : meshes(group.meshes),
      intersectionMeshIndex(group.intersectionMeshIndex),
      intersectionTriangleIndex(group.intersectionTriangleIndex),
      meshHierarchy(group.meshHierarchy),
      preparedMeshCount(group.preparedMeshCount)
{
    for (int i = 0; i < 6; i++) preparedBounds[i] = group.preparedBounds[i];
}

ArrayList<TriangleMesh>& TriangleMeshGroup::getMeshes()
{
    return meshes;
}

void TriangleMeshGroup::prepareAccelerationStructures(TriangleMeshAccelerationMethod::Value method)
{
    BOOLEAN changed = FALSE;
    for (long int i = 0; i < meshes.size(); i++) {
        const TriangleAccelerationStructure* before = meshes[i].getAccelerationStructure();
        meshes[i].prepareAccelerationStructure(method);
        if (meshes[i].getAccelerationStructure() != before) changed = TRUE;
    }
    if (method == TriangleMeshAccelerationMethod::BRUTE_FORCE) {
        if (preparedMeshCount != -1) preparedMeshCount = -1;
        return;
    }
    // Nothing written when already prepared: safe from tracing threads.
    if (!changed && preparedMeshCount == meshes.size()) return;
    buildMeshHierarchy();
}

void TriangleMeshGroup::buildMeshHierarchy()
{
    preparedMeshCount = -1;
    double bounds[6];
    if (!getPreparedMinMax(bounds)) return;  // some mesh has no structure
    // [WALD2007] top level: one padded box per mesh, from its cached bounds.
    ArrayList<double> boxes(6 * (meshes.size() > 0 ? meshes.size() : 1));
    for (long int i = 0; i < meshes.size(); i++) {
        double mm[6];
        meshes[i].getPreparedMinMax(mm);
        TriangleAccelerationStructure::padBox(mm, mm + 3);
        for (int k = 0; k < 6; k++) boxes.add(mm[k]);
    }
    meshHierarchy.build(boxes);
    for (int k = 0; k < 6; k++) preparedBounds[k] = bounds[k];
    preparedMeshCount = meshes.size();
}

const BinnedSahHierarchy* TriangleMeshGroup::getMeshHierarchy() const
{
    return preparedMeshCount == meshes.size() ? &meshHierarchy : (const BinnedSahHierarchy*)NULL;
}

BOOLEAN TriangleMeshGroup::getPreparedMinMax(double* outMinMax) const
{
    if (preparedMeshCount >= 0 && preparedMeshCount == meshes.size()) {
        for (int k = 0; k < 6; k++) outMinMax[k] = preparedBounds[k];
        return TRUE;
    }
    // Same combination as computeMinMaxPositions, from cached mesh bounds.
    if (meshes.size() == 0) return FALSE;
    double minX = 0, minY = 0, minZ = 0, maxX = 0, maxY = 0, maxZ = 0;
    for (long int i = 0; i < meshes.size(); i++) {
        double mm[6];
        if (!meshes[i].getPreparedMinMax(mm)) return FALSE;
        if (i == 0) {
            minX = mm[0]; minY = mm[1]; minZ = mm[2];
            maxX = mm[3]; maxY = mm[4]; maxZ = mm[5];
        }
        if (mm[0] < minX) minX = mm[0];
        if (mm[1] < minY) minY = mm[1];
        if (mm[2] < minZ) minZ = mm[2];
        if (mm[3] > maxX) maxX = mm[3];
        if (mm[4] > maxY) maxY = mm[4];
        if (mm[5] > maxZ) maxZ = mm[5];
    }
    outMinMax[0] = minX; outMinMax[1] = minY; outMinMax[2] = minZ;
    outMinMax[3] = maxX; outMinMax[4] = maxY; outMinMax[5] = maxZ;
    return TRUE;
}

void TriangleMeshGroup::setMeshes(const ArrayList<TriangleMesh>& inMeshes)
{
    meshes = inMeshes;
    preparedMeshCount = -1;
}

void TriangleMeshGroup::addMesh(const TriangleMesh& mesh)
{
    meshes.add(mesh);
    preparedMeshCount = -1;
}

TriangleMesh& TriangleMeshGroup::getMeshAt(int index)
{
    return meshes[index];
}

/*
@return a new 6 valued double array containing the coordinates of a min-max
bounding box for current geometry.
*/
double* TriangleMeshGroup::computeMinMaxPositions()
{
    double* minMax = new double[6];

    BOOLEAN first = TRUE;
    double minX = 1e308, minY = 1e308, minZ = 1e308;
    double maxX = -1e308, maxY = -1e308, maxZ = -1e308;

    for (long int i = 0; i < meshes.size(); i++) {
        double* mm = meshes[i].getMinMax();
        double x = mm[0], y = mm[1], z = mm[2];
        double X = mm[3], Y = mm[4], Z = mm[5];

        if (first) {
            minX = x; maxX = X;
            minY = y; maxY = Y;
            minZ = z; maxZ = Z;
            first = FALSE;
        }

        if (x < minX) minX = x;
        if (y < minY) minY = y;
        if (z < minZ) minZ = z;
        if (X > maxX) maxX = X;
        if (Y > maxY) maxY = Y;
        if (Z > maxZ) maxZ = Z;

        delete [] mm;
    }

    if (first) {
        minX = minY = minZ = 0.0;
        maxX = maxY = maxZ = 0.0;
    }

    minMax[0] = minX;
    minMax[1] = minY;
    minMax[2] = minZ;
    minMax[3] = maxX;
    minMax[4] = maxY;
    minMax[5] = maxZ;
    return minMax;
}

double* TriangleMeshGroup::getMinMax()
{
    return computeMinMaxPositions();
}

/*
Kept for source and binary compatibility. MinMax values are calculated
without storing state by getMinMax().
Note that for each position in this array:
    0 - min (x)
    1 - min (y)
    2 - min (z)
    3 - max (x)
    4 - max (y)
    5 - max (z)
*/
void TriangleMeshGroup::calculateMinMaxPositions()
{
    double* mm = computeMinMaxPositions();
    delete [] mm;
}

/*
Check the general interface contract in superclass method
Geometry.doIntersectionFirstHit.

@param inOut_Ray
@return true if given ray intersects current TriangleMeshGroup
*/
Ray* TriangleMeshGroup::doIntersectionFirstHit(const Ray& inOut_Ray)
{
    RayHit hit(RayHit::DETAIL_NONE, TRUE);
    if (doIntersectionFirstHit(inOut_Ray, &hit) && hit.getRay() != 0) {
        return new Ray(*hit.getRay());
    }
    return 0;
}

/**
Closest hit over the meshes of a group: the leaf test of the top level
hierarchy and, called for every mesh in order, the linear loop. Keeps the
first (lowest index) mesh among equal distances, as the linear loop does.
*/
class _TriangleMeshGroupClosestHit : public BinnedSahLeafVisitor {
  public:
    ArrayList<TriangleMesh>& meshes;
    const Ray& ray;
    RayHit bestHit;
    BOOLEAN hasBestHit;
    int bestMesh;
    int bestTriangle;

    _TriangleMeshGroupClosestHit(ArrayList<TriangleMesh>& inMeshes, const Ray& inRay)
        : meshes(inMeshes), ray(inRay), hasBestHit(FALSE), bestMesh(-1), bestTriangle(-1) {}

    void reset()
    {
        hasBestHit = FALSE;
        bestMesh = -1;
        bestTriangle = -1;
    }

    void visit(int mesh, double& inOutBestT)
    {
        RayHit meshHit;
        int triangle = -1;
        if (!meshes[mesh].doIntersectionFirstHit(ray, &meshHit, &triangle) || meshHit.getRay() == 0)
            return;
        const double t = meshHit.getRay()->getT();
        if (t < inOutBestT || (t == inOutBestT && hasBestHit && mesh < bestMesh)) {
            inOutBestT = t;
            bestHit.clone(meshHit);
            bestMesh = mesh;
            bestTriangle = triangle;
            hasBestHit = TRUE;
        }
    }
};

void TriangleMeshGroup::setIntersectionIndexes(int mesh, int triangle)
{
    // Legacy last-hit state written by every ray: skip writes that change
    // nothing, so tracing threads do not keep invalidating its cache line.
    if (intersectionMeshIndex != mesh) intersectionMeshIndex = mesh;
    if (intersectionTriangleIndex != triangle) intersectionTriangleIndex = triangle;
}

BOOLEAN TriangleMeshGroup::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit)
{
    _TriangleMeshGroupClosestHit closest(meshes, inRay);
    double minT = 1e308;
    BOOLEAN traversed = FALSE;
    if (preparedMeshCount >= 0 && preparedMeshCount == meshes.size()) {
        traversed = meshHierarchy.traverseClosest(inRay, closest, minT);
        if (!traversed) closest.reset();  // stack overflow: linear loop below
    }
    if (!traversed) {
        minT = 1e308;
        for (long int i = 0; i < meshes.size(); i++) closest.visit((int)i, minT);
    }

    if (!closest.hasBestHit) {
        setIntersectionIndexes(-1, -1);
        return FALSE;
    }

    setIntersectionIndexes(closest.bestMesh, closest.bestTriangle);
    RayHit& bestHit = closest.bestHit;

    if (outHit != 0) {
        outHit->clone(bestHit);
    }
    return TRUE;
}

/*
Check the general interface contract in superclass method
Geometry.doExtraInformation.
@param inRay
@param inT
@param outData
*/
void TriangleMeshGroup::doExtraInformation(const Ray& inRay, double inT, RayHit* outData)
{
    if (outData == 0) {
        return;
    }
    doIntersectionFirstHit(inRay.withT(inT), outData);
}

int* TriangleMeshGroup::doIntersectionInformation()
{
    int* info = new int[2];
    info[0] = intersectionMeshIndex;
    info[1] = intersectionTriangleIndex;
    return info;
}

/*
Check the general interface contract in superclass method
Geometry.doContainmentTest.
\todo  Check efficiency for this implementation. Note that for the
special application of volume rendering generation, it is better
to provide another method, to add voxels after a path following
over the line.
@return INSIDE, OUTSIDE or LIMIT constant value
*/
int TriangleMeshGroup::doContainmentTest(const Vector3Dd& p, double distanceTolerance)
{
    for (long int i = 0; i < meshes.size(); i++) {
        int status = meshes[i].doContainmentTest(p, distanceTolerance);
        if (status != OUTSIDE) {
            return status;
        }
    }
    return OUTSIDE;
}

/*
Provides an object to text report convertion, optimized for human
readability and debugging. Do not use for serialization or persistence
purposes.
@return human readable report from current mesh group
*/
static String intToStr(int val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", val);
    return String(buf);
}

String TriangleMeshGroup::toString() const
{
    return String("TriangleMeshGroup < #Mesh: ") + intToStr((int)meshes.size()) + " >";
}

TriangleMeshGroup* TriangleMeshGroup::exportToTriangleMeshGroup()
{
    return this;
}

const Class TriangleMeshGroup::CLASS = { "TriangleMeshGroup", &Surface::CLASS };

const Class *
TriangleMeshGroup::getClass() const
{
    return &CLASS;
}

#include <string.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/statistics/RaytraceStatistics.h"
#include "vsdk/toolkit/environment/geometry/element/Triangle.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/BinnedSahTriangleHierarchy.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/GridTriangleHierarchy.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/TriangleAccelerationStructure.h"

// Relative and absolute padding of the triangle boxes: Moller-Trumbore can
// accept points a few ulps outside the exact triangle, the slab test must
// not reject them.
static const double RELATIVE_BOX_PADDING = 1e-9;
static const double ABSOLUTE_BOX_PADDING = 1e-12;

TriangleMeshAccelerationMethod::Value
TriangleMeshAccelerationMethodUtil::fromCode(int code)
{
    if (code == 1) {
        return TriangleMeshAccelerationMethod::WALD2007_BINNED_SAH;
    }
    if (code == 2) {
        return TriangleMeshAccelerationMethod::VOXEL_GRID;
    }
    return TriangleMeshAccelerationMethod::BRUTE_FORCE;
}

const char *
TriangleMeshAccelerationMethodUtil::name(TriangleMeshAccelerationMethod::Value value)
{
    switch (value) {
        case TriangleMeshAccelerationMethod::BRUTE_FORCE: return "bruteforce";
        case TriangleMeshAccelerationMethod::WALD2007_BINNED_SAH: return "wald2007";
        case TriangleMeshAccelerationMethod::VOXEL_GRID: return "voxel";
    }
    return "unknown";
}

BOOLEAN
TriangleMeshAccelerationMethodUtil::fromName(const char *text,
    TriangleMeshAccelerationMethod::Value &outValue)
{
    if (text == 0) {
        return FALSE;
    }
    for (int code = 0; code < COUNT; code++) {
        const TriangleMeshAccelerationMethod::Value candidate = fromCode(code);
        if (strcmp(text, name(candidate)) == 0) {
            outValue = candidate;
            return TRUE;
        }
    }
    return FALSE;
}

TriangleAccelerationStructure::TriangleAccelerationStructure() :
    builtVertexCount(-1), builtTriangleCount(-1)
{
    for (int a = 0; a < 3; a++) {
        vertexBounds[a] = 1e308;
        vertexBounds[a + 3] = -1e308;
    }
}

void
TriangleAccelerationStructure::recordBuiltMesh(const ArrayList<double> &positions,
    const ArrayList<int> &indexes)
{
    builtVertexCount = (int)(positions.size() / 3);
    builtTriangleCount = (int)(indexes.size() / 3);
    // Same operations and order as TriangleMesh::calculateMinMaxPositions,
    // so the cached bounds are bit identical to getMinMax().
    double minX = 1e308, minY = 1e308, minZ = 1e308;
    double maxX = -1e308, maxY = -1e308, maxZ = -1e308;
    for (int i = 0; i < builtVertexCount; i++) {
        const double x = positions[i * 3], y = positions[i * 3 + 1], z = positions[i * 3 + 2];
        minX = Math::min(minX, x); minY = Math::min(minY, y); minZ = Math::min(minZ, z);
        maxX = Math::max(maxX, x); maxY = Math::max(maxY, y); maxZ = Math::max(maxZ, z);
    }
    vertexBounds[0] = minX; vertexBounds[1] = minY; vertexBounds[2] = minZ;
    vertexBounds[3] = maxX; vertexBounds[4] = maxY; vertexBounds[5] = maxZ;
}

void
TriangleAccelerationStructure::getVertexBounds(double *outMinMax) const
{
    for (int i = 0; i < 6; i++) {
        outMinMax[i] = vertexBounds[i];
    }
}

TriangleAccelerationStructure::~TriangleAccelerationStructure()
{
}

BOOLEAN
TriangleAccelerationStructure::matches(int vertexCount, int triangleCount) const
{
    return builtVertexCount == vertexCount && builtTriangleCount == triangleCount;
}

static double
absoluteValue(double x)
{
    return x < 0 ? -x : x;
}

void
TriangleAccelerationStructure::triangleBox(const ArrayList<double> &positions,
    const ArrayList<int> &indexes, int triangle, double *lo, double *hi)
{
    int a;
    for (a = 0; a < 3; a++) {
        lo[a] = 1e30;
        hi[a] = -1e30;
    }
    for (int k = 0; k < 3; k++) {
        const int vertex = indexes[triangle * 3 + k];
        for (a = 0; a < 3; a++) {
            const double c = positions[vertex * 3 + a];
            lo[a] = c < lo[a] ? c : lo[a];
            hi[a] = c > hi[a] ? c : hi[a];
        }
    }
    padBox(lo, hi);
}

void
TriangleAccelerationStructure::padBox(double *lo, double *hi)
{
    for (int a = 0; a < 3; a++) {
        const double magnitude = absoluteValue(lo[a]) > absoluteValue(hi[a]) ?
            absoluteValue(lo[a]) : absoluteValue(hi[a]);
        const double pad = magnitude * RELATIVE_BOX_PADDING + ABSOLUTE_BOX_PADDING;
        lo[a] -= pad;
        hi[a] += pad;
    }
}

BOOLEAN
TriangleAccelerationStructure::testTriangle(const Ray &ray,
    const ArrayList<double> &positions, const ArrayList<int> &indexes,
    int triangle, double &t, double &u, double &v)
{
    const int i0 = indexes[triangle * 3];
    const int i1 = indexes[triangle * 3 + 1];
    const int i2 = indexes[triangle * 3 + 2];
    const Vector3Dd v0(positions[i0 * 3], positions[i0 * 3 + 1], positions[i0 * 3 + 2]);
    const Vector3Dd v1(positions[i1 * 3], positions[i1 * 3 + 1], positions[i1 * 3 + 2]);
    const Vector3Dd v2(positions[i2 * 3], positions[i2 * 3 + 1], positions[i2 * 3 + 2]);
    return Triangle::intersectRay(ray, v0, v1, v2, t, u, v);
}

BOOLEAN
TriangleAccelerationStructure::firstHitBruteForce(const Ray &ray,
    const ArrayList<double> &positions, const ArrayList<int> &indexes,
    double &outT, double &outU, double &outV, int &outTriangle)
{
    const int triangleCount = (int)(indexes.size() / 3);
    // No nodes: every triangle is a primitive test.
    RaytraceStatistics::recordPrimitiveLevelTraversal(0, triangleCount);
    double bestT = 1e308;
    double bestU = 0;
    double bestV = 0;
    int best = -1;
    for (int i = 0; i < triangleCount; i++) {
        double t, u, v;
        if (testTriangle(ray, positions, indexes, i, t, u, v) && t < bestT) {
            bestT = t;
            bestU = u;
            bestV = v;
            best = i;
        }
    }
    if (best < 0) {
        return FALSE;
    }
    outT = bestT;
    outU = bestU;
    outV = bestV;
    outTriangle = best;
    return TRUE;
}

TriangleAccelerationStructure *
TriangleAccelerationStructure::create(TriangleMeshAccelerationMethod::Value method)
{
    switch (method) {
        case TriangleMeshAccelerationMethod::WALD2007_BINNED_SAH:
            return new BinnedSahTriangleHierarchy();
        case TriangleMeshAccelerationMethod::VOXEL_GRID:
            return new GridTriangleHierarchy();
        case TriangleMeshAccelerationMethod::BRUTE_FORCE:
            return 0;
    }
    return 0;
}

TriangleAccelerationStructureSlot::TriangleAccelerationStructureSlot() : structure(0)
{
}

TriangleAccelerationStructureSlot::TriangleAccelerationStructureSlot(
    const TriangleAccelerationStructureSlot &other) :
    structure(other.structure != 0 ? other.structure->copy() : 0)
{
}

TriangleAccelerationStructureSlot &
TriangleAccelerationStructureSlot::operator=(const TriangleAccelerationStructureSlot &other)
{
    if (this != &other) {
        reset(other.structure != 0 ? other.structure->copy() : 0);
    }
    return *this;
}

TriangleAccelerationStructureSlot::~TriangleAccelerationStructureSlot()
{
    delete structure;
}

void
TriangleAccelerationStructureSlot::reset(TriangleAccelerationStructure *newStructure)
{
    if (newStructure == structure) {
        return;
    }
    delete structure;
    structure = newStructure;
}

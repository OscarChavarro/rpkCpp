//= References:                                                             =
//= [WALD2007] Wald, Ingo. "On fast Construction of SAH-based Bounding      =
//=     Volume Hierarchies". Proceedings of the 2007 IEEE Symposium on      =
//=     Interactive Ray Tracing, 2007, pp. 33-40.                           =

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/statistics/RaytraceStatistics.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/Triangle.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/BinnedSahTriangleHierarchy.h"

BinnedSahTriangleHierarchy::BinnedSahTriangleHierarchy()
{
}

TriangleMeshAccelerationMethod
BinnedSahTriangleHierarchy::getMethod() const
{
    return TriangleMeshAccelerationMethod::WALD2007_BINNED_SAH;
}

void
BinnedSahTriangleHierarchy::build(const java::ArrayList<double> &positions,
    const java::ArrayList<int> &indexes)
{
    const int primitiveCount = (int)(indexes.size() / 3);
    recordBuiltMesh(positions, indexes);

    // [WALD2007].3.2 Initial setup: the box of every triangle, computed once.
    java::ArrayList<double> boxes(6L * (primitiveCount > 0 ? primitiveCount : 1));
    for (int i = 0; i < primitiveCount; i++) {
        double lo[3];
        double hi[3];
        triangleBox(positions, indexes, i, lo, hi);
        for (int a = 0; a < 3; a++) {
            boxes.add(lo[a]);
        }
        for (int a = 0; a < 3; a++) {
            boxes.add(hi[a]);
        }
    }
    hierarchy.build(boxes);

    // Triangle vertices in leaf order, as the triangle buffer of the shader.
    const java::ArrayList<int> &order = hierarchy.getPrimitiveOrder();
    leafVertices.clear();
    leafVertices.reserve(9L * (primitiveCount > 0 ? primitiveCount : 1));
    for (int i = 0; i < primitiveCount; i++) {
        const int triangle = order[i];
        for (int k = 0; k < 3; k++) {
            const int vertex = indexes[triangle * 3 + k];
            leafVertices.add(positions[vertex * 3]);
            leafVertices.add(positions[vertex * 3 + 1]);
            leafVertices.add(positions[vertex * 3 + 2]);
        }
    }
}

bool
BinnedSahTriangleHierarchy::firstHit(const Ray &ray,
    const java::ArrayList<double> &positions, const java::ArrayList<int> &indexes,
    double &outT, double &outU, double &outV, int &outTriangle) const
{
    const java::ArrayList<BinnedSahHierarchyNode> &nodes = hierarchy.getNodes();
    if (builtTriangleCount <= 0 || nodes.size() == 0) {
        return false;
    }
    const BinnedSahHierarchy::RaySlab slab(ray);
    const BinnedSahHierarchyNode *nodeArray = nodes.data();
    const double *vertexArray = leafVertices.data();
    const int *orderArray = hierarchy.getPrimitiveOrder().data();

    // Same initial bound and tie rule as the brute force loop.
    double bestT = 1e308;
    double bestU = 0;
    double bestV = 0;
    int best = -1;

    const double rootDistance = slab.entry(nodeArray[0], bestT);
    if (rootDistance < 0.0) {
        RaytraceStatistics::recordPrimitiveLevelTraversal(0, 0);
        return false;
    }
    // Counted locally, reported once: no shared write per node.
    long int nodeVisits = 0;
    long int triangleTests = 0;
    // Traversal of casTraverseBlas (computeAccelerationStructure.glsl),
    // specialized for triangles: no virtual call per leaf entry.
    int nodeStack[BinnedSahHierarchy::STACK_SIZE];
    double distanceStack[BinnedSahHierarchy::STACK_SIZE];
    int sp = 0;
    nodeStack[sp] = 0;
    distanceStack[sp] = rootDistance;
    sp++;

    while (sp > 0) {
        sp--;
        // Strictly farther than the closest hit: nothing there can win, not
        // even a tie (which needs t == bestT).
        if (distanceStack[sp] > bestT) {
            continue;
        }
        const BinnedSahHierarchyNode &node = nodeArray[nodeStack[sp]];
        nodeVisits++;
        if (node.count > 0) {
            triangleTests += node.count;
            for (int i = node.leftOrFirst; i < node.leftOrFirst + node.count; i++) {
                const double *p = vertexArray + 9 * i;
                const Vector3Dd v0(p[0], p[1], p[2]);
                const Vector3Dd v1(p[3], p[4], p[5]);
                const Vector3Dd v2(p[6], p[7], p[8]);
                double t, u, v;
                if (!Triangle::intersectRay(ray, v0, v1, v2, t, u, v)) {
                    continue;
                }
                const int triangle = orderArray[i];
                if (t < bestT || (t == bestT && best >= 0 && triangle < best)) {
                    bestT = t;
                    bestU = u;
                    bestV = v;
                    best = triangle;
                }
            }
            continue;
        }
        int left = node.leftOrFirst;
        int right = left + 1;
        double dl = slab.entry(nodeArray[left], bestT);
        double dr = slab.entry(nodeArray[right], bestT);
        // Push the farther child first, so the nearer one is visited first.
        if (dl >= 0.0 && dr >= 0.0 && dl < dr) {
            const int swapNode = left;
            left = right;
            right = swapNode;
            const double swapDistance = dl;
            dl = dr;
            dr = swapDistance;
        }
        if (sp + 2 > BinnedSahHierarchy::STACK_SIZE) {
            // Unreachable with MAXIMUM_DEPTH < STACK_SIZE; never give a
            // wrong answer if it happens.
            RaytraceStatistics::recordPrimitiveLevelTraversal(nodeVisits, triangleTests);
            return firstHitBruteForce(ray, positions, indexes, outT, outU, outV, outTriangle);
        }
        if (dl >= 0.0) {
            nodeStack[sp] = left;
            distanceStack[sp] = dl;
            sp++;
        }
        if (dr >= 0.0) {
            nodeStack[sp] = right;
            distanceStack[sp] = dr;
            sp++;
        }
    }
    RaytraceStatistics::recordPrimitiveLevelTraversal(nodeVisits, triangleTests);
    if (best < 0) {
        return false;
    }
    outT = bestT;
    outU = bestU;
    outV = bestV;
    outTriangle = best;
    return true;
}

TriangleAccelerationStructure *
BinnedSahTriangleHierarchy::copy() const
{
    return new BinnedSahTriangleHierarchy(*this);
}

long int
BinnedSahTriangleHierarchy::getNodeCount() const
{
    return hierarchy.getNodeCount();
}

int
BinnedSahTriangleHierarchy::getDepth() const
{
    return hierarchy.getDepth();
}

const BinnedSahHierarchy &
BinnedSahTriangleHierarchy::getHierarchy() const
{
    return hierarchy;
}

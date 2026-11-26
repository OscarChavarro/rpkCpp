//= References:                                                             =
//= [WALD2007] Wald, Ingo. "On fast Construction of SAH-based Bounding      =
//=     Volume Hierarchies". Proceedings of the 2007 IEEE Symposium on      =
//=     Interactive Ray Tracing, 2007, pp. 33-40.                           =

#ifndef __BINNED_SAH_TRIANGLE_HIERARCHY__
#define __BINNED_SAH_TRIANGLE_HIERARCHY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/BinnedSahHierarchy.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/TriangleAccelerationStructure.h"

/**
`TriangleMeshAccelerationMethod::WALD2007_BINNED_SAH` for the triangles of
one mesh (the bottom level): a `BinnedSahHierarchy` ([WALD2007].3) over the
padded triangle boxes.

Exactness: the leaves keep a copy of their triangle vertices in leaf order
(as the triangle buffer of the shader, for memory locality), the leaf test
is `Triangle::intersectRay` over the same values, a node is skipped only
when its entry is strictly beyond the closest hit, and ties go to the
lowest triangle index: the result is the brute force one.
*/
class BinnedSahTriangleHierarchy : public TriangleAccelerationStructure {
  private:
    BinnedSahHierarchy hierarchy;
    java::ArrayList<double> leafVertices;

  public:
    BinnedSahTriangleHierarchy();

    TriangleMeshAccelerationMethod getMethod() const;
    void build(const java::ArrayList<double> &positions,
        const java::ArrayList<int> &indexes);
    bool firstHit(const Ray &ray, const java::ArrayList<double> &positions,
        const java::ArrayList<int> &indexes,
        double &outT, double &outU, double &outV, int &outTriangle) const;
    TriangleAccelerationStructure *copy() const;
    long int getNodeCount() const;
    int getDepth() const;

    const BinnedSahHierarchy &getHierarchy() const;
};

#endif

//= References:                                                             =
//= [SNYD1987] Snyder, John M. and Barr, Alan H. "Ray Tracing Complex       =
//=     Models Containing Surface Tessellations". SIGGRAPH '87, Computer    =
//=     Graphics 21(4), 1987, pp. 119-128.                                  =
//=     PDF: doc/references/[SNYD1987]_*.pdf                                =

#ifndef __GRID_TRIANGLE_HIERARCHY__
#define __GRID_TRIANGLE_HIERARCHY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/TriangleAccelerationStructure.h"

class GridSpatialIndex;

/**
`TriangleMeshAccelerationMethod::VOXEL_GRID` for the triangles of one mesh:
a `GridSpatialIndex` ([SNYD1987], RenderPark's voxel grid) over the padded
triangle boxes, all of them members of one group, so the mesh gets one
uniform grid of floor(n^(1/3) + 1) cells along each axis.

Exactness: the leaf test is `Triangle::intersectRay` over the mesh arrays,
as the brute force loop; a hit found in an earlier cell may lie beyond it
([SNYD1987] Figure 11),
but the walk goes on until it enters a cell beyond the closest hit, and the
cell holding the closest hit point lists every triangle through that point,
so the closest hit and its ties are always tested. Ties go to the lowest
triangle index: the result is the brute force one.

The query uses a workspace without mailbox, so `firstHit` allocates nothing
and is safe from several threads; a triangle listed in several cells the
ray crosses is tested again in each (the repeated test gives the same hit,
which the tie rule does not take twice).
*/
class GridTriangleHierarchy : public TriangleAccelerationStructure {
  private:
    /** Padded triangle boxes, six values per triangle, kept for `copy` */
    java::ArrayList<double> triangleBoxes;
    GridSpatialIndex *index;

    void buildIndex();

  public:
    GridTriangleHierarchy();
    GridTriangleHierarchy(const GridTriangleHierarchy &other);
    GridTriangleHierarchy &operator=(const GridTriangleHierarchy &other) = delete;
    ~GridTriangleHierarchy();

    TriangleMeshAccelerationMethod getMethod() const;
    void build(const java::ArrayList<double> &positions,
        const java::ArrayList<int> &indexes);
    bool firstHit(const Ray &ray, const java::ArrayList<double> &positions,
        const java::ArrayList<int> &indexes,
        double &outT, double &outU, double &outV, int &outTriangle) const;
    TriangleAccelerationStructure *copy() const;
    /** Number of grids (one for a mesh) */
    long int getNodeCount() const;
    int getDepth() const;

    /** Cells along each axis of the grid; 0 when empty */
    int getResolution() const;
};

#endif

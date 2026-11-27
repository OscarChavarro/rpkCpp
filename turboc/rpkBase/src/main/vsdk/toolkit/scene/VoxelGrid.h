#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __VOXEL_GRID__
#define __VOXEL_GRID__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
#include "vsdk/toolkit/skin/MinMaxBox.h"
#include "vsdk/toolkit/scene/VoxelData.h"
#include "vsdk/toolkit/scene/VoxelGridWorkspace.h"
#include "vsdk/toolkit/environment/geometry/elements/RayExclusionSet.h"

/**
Hierarchical uniform grid over the scene geometry.

Shared, read-only after construction: gridIntersect is const and writes only
to the caller's VoxelGridWorkspace and hit record. The root grid owns every
item and sub grid of the hierarchy; the item indices are dense over it.
*/
class VoxelGrid {
  private:
    #define MINIMUM_ELEMENT_COUNT_PER_CELL 10
    #define DELTA_BOUND_FACTOR ((double)1e-4)

    // Owned by the root grid only (NULL on sub grids)
    ArrayList<VoxelGrid *> *subGrids;
    ArrayList<VoxelData *> *items;
    VoxelGrid *const root;

    short xSize;
    short ySize;
    short zSize;
    Vector3Dd voxelSize;
    ArrayList<VoxelData *> **volumeListsOfItems; // 3D array of item lists
    void **gridItemPool;
    RpkAxisAlignedBoundingBox boundingBox;
    MinMaxBox *rayIntersectionBox;

    VoxelGrid(RpkGeometry *geometry, VoxelGrid *inRoot, int level);
    void build(RpkGeometry *geometry, int level);

    int addItem(VoxelData *cell);
    void addSubGrid(VoxelGrid *voxelGrid);
    int nextItemIndex() const;
    void freeVoxelGridElements();
    static short clampVoxel(short v, short max);
    static bool shouldSubdivide(const RpkGeometry *geometry);
    static int countItems(RpkGeometry *geometry);

    double voxel2x(double px) const;
    double voxel2y(double py) const;
    double voxel2z(double pz) const;
    short x2voxel(double px) const;
    short y2voxel(double py) const;
    short z2voxel(double pz) const;
    int cellIndexAddress(int a, int b, int c) const;
    void putGeometryInsideVoxelGrid(RpkGeometry *geometry, short na, short nb, short nc, int level);
    bool isSmall(const RpkAxisAlignedBoundingBox *bb) const;
    void putSubGeometryInsideVoxelGrid(RpkGeometry *geometry, int level);
    void putItemInsideVoxelGrid(VoxelData *item, const RpkAxisAlignedBoundingBox *itemBounds) const;
    void putPatchInsideVoxelGrid(Patch *patch);
    Vector3Dd toVoxelClamped(const Vector3Dd &p) const;
    void insertGeometryAsVoxelData(RpkGeometry *geometry);
    void processCompoundGeometry(RpkGeometry *geometry, int level);
    void processPatches(RpkGeometry *geometry);
    void insertSubGrid(RpkGeometry *geometry, int level);

    void
    gridTraceSetup(
        const Ray *ray,
        double t0,
        const Vector3Dd *P,
        int *g,
        Vector3Dd *tDelta,
        Vector3Dd *tNext,
        int *step,
        int *out) const;

    bool
    gridBoundsIntersect(
        const Ray *ray,
        double minimumDistance,
        double maximumDistance,
        /*OUT*/ double *t0,
        Vector3Dd *position) const;

    static RpkRayHit *
    voxelIntersect(
        const ArrayList<VoxelData *> *items,
        Ray *ray,
        unsigned int counter,
        double minimumDistance,
        double *maximumDistance,
        int hitFlags,
        RpkRayHit *hitStore,
        const RayExclusionSet &exclusions,
        VoxelGridWorkspace *workspace);

    static bool
    nextVoxel(double *t0, int *g, Vector3Dd *tNext, const Vector3Dd *tDelta, const int *step, const int *out);

  public:
    explicit VoxelGrid(RpkGeometry *geometry);
    virtual ~VoxelGrid();

    RpkRayHit *
    gridIntersect(
        Ray *ray,
        double minimumDistance,
        double *maximumDistance,
        int hitFlags,
        RpkRayHit *hitStore,
        const RayExclusionSet &exclusions,
        VoxelGridWorkspace *workspace) const;

    int getNumberOfItems() const;
    void print() const;
};

inline double
VoxelGrid::voxel2x(const double px) const {
    return px * voxelSize.x() + boundingBox.minX();
}

inline double
VoxelGrid::voxel2y(const double py) const {
    return py * voxelSize.y() + boundingBox.minY();
}

inline double
VoxelGrid::voxel2z(const double pz) const {
    return pz * voxelSize.z() + boundingBox.minZ();
}

inline short
VoxelGrid::x2voxel(const double px) const {
    return ((short)((voxelSize.x() < VSDK::EPSILON)
                                  ? 0
                                  : (px - boundingBox.minX()) / voxelSize.x()));
}

inline short
VoxelGrid::y2voxel(const double py) const {
    return ((short)((voxelSize.y() < VSDK::EPSILON)
                                  ? 0
                                  : (py - boundingBox.minY()) / voxelSize.y()));
}

inline short
VoxelGrid::z2voxel(const double pz) const {
    return ((short)((voxelSize.z() < VSDK::EPSILON)
                                  ? 0
                                  : (pz - boundingBox.minZ()) / voxelSize.z()));
}

inline int
VoxelGrid::cellIndexAddress(const int a, const int b, const int c) const {
    return (a * ySize + b) * zSize + c;
}

#endif

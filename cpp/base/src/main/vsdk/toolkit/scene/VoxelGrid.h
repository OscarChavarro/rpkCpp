#ifndef VOXEL_GRID__
#define VOXEL_GRID__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
#include "vsdk/toolkit/skin/MinMaxBox.h"
#include "vsdk/toolkit/scene/VoxelData.h"
#include "vsdk/toolkit/scene/VoxelGridWorkspace.h"
#include "vsdk/toolkit/scene/FaceBoundsTable.h"
#include "vsdk/toolkit/environment/geometry/elements/RayExclusionSet.h"

/**
Hierarchical uniform grid over the scene geometry.

Shared, read-only after construction: gridIntersect is const and writes only
to the caller's VoxelGridWorkspace and hit record. The root grid owns every
item and sub grid of the hierarchy; the item indices are dense over it.
*/
class VoxelGrid {
  private:
    static constexpr int MINIMUM_ELEMENT_COUNT_PER_CELL = 10;
    static constexpr float DELTA_BOUND_FACTOR = 1e-4F;

    // Owned by the root grid only (nullptr on sub grids)
    java::ArrayList<VoxelGrid *> *subGrids;
    java::ArrayList<VoxelData *> *items;
    VoxelGrid *const root;
    const FaceBoundsTable *const faceBounds; // Owned by the scene; bounds of the patch items

    short xSize;
    short ySize;
    short zSize;
    Vector3D voxelSize;
    java::ArrayList<VoxelData *> **volumeListsOfItems; // 3D array of item lists
    void **gridItemPool;
    RpkAxisAlignedBoundingBox boundingBox;
    MinMaxBox *rayIntersectionBox;

    VoxelGrid(RpkGeometry *geometry, VoxelGrid *inRoot, int level, const FaceBoundsTable *inFaceBounds);

    int addItem(VoxelData *cell);
    void addSubGrid(VoxelGrid *voxelGrid);
    int nextItemIndex() const;
    void freeVoxelGridElements();
    static short clampVoxel(short v, short max);
    static bool shouldSubdivide(const RpkGeometry *geometry);
    static int countItems(RpkGeometry *geometry);

    float voxel2x(float px) const;
    float voxel2y(float py) const;
    float voxel2z(float pz) const;
    short x2voxel(float px) const;
    short y2voxel(float py) const;
    short z2voxel(float pz) const;
    int cellIndexAddress(int a, int b, int c) const;
    void putGeometryInsideVoxelGrid(RpkGeometry *geometry, short na, short nb, short nc, int level);
    bool isSmall(const RpkAxisAlignedBoundingBox *bb) const;
    void putSubGeometryInsideVoxelGrid(RpkGeometry *geometry, int level);
    void putItemInsideVoxelGrid(VoxelData *item, const RpkAxisAlignedBoundingBox *itemBounds) const;
    void putPatchInsideVoxelGrid(Patch *patch);
    Vector3D toVoxelClamped(const Vector3D &p) const;
    void insertGeometryAsVoxelData(RpkGeometry *geometry);
    void processCompoundGeometry(RpkGeometry *geometry, int level);
    void processPatches(RpkGeometry *geometry);
    void insertSubGrid(RpkGeometry *geometry, int level);

    void
    gridTraceSetup(
        const RpkRay *ray,
        float t0,
        const Vector3D *P,
        int *g,
        Vector3D *tDelta,
        Vector3D *tNext,
        int *step,
        int *out) const;

    bool
    gridBoundsIntersect(
        const RpkRay *ray,
        float minimumDistance,
        float maximumDistance,
        /*OUT*/ float *t0,
        Vector3D *position) const;

    static RpkRayHit *
    voxelIntersect(
        const java::ArrayList<VoxelData *> *items,
        RpkRay *ray,
        unsigned int counter,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RpkRayHit *hitStore,
        const RayExclusionSet &exclusions,
        VoxelGridWorkspace *workspace);

    static bool
    nextVoxel(float *t0, int *g, Vector3D *tNext, const Vector3D *tDelta, const int *step, const int *out);

  public:
    VoxelGrid(RpkGeometry *geometry, const FaceBoundsTable *inFaceBounds);
    virtual ~VoxelGrid();

    RpkRayHit *
    gridIntersect(
        RpkRay *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RpkRayHit *hitStore,
        const RayExclusionSet &exclusions,
        VoxelGridWorkspace *workspace) const;

    int getNumberOfItems() const;
    void print() const;
};

inline float
VoxelGrid::voxel2x(const float px) const {
    return px * voxelSize.x + boundingBox.minX();
}

inline float
VoxelGrid::voxel2y(const float py) const {
    return py * voxelSize.y + boundingBox.minY();
}

inline float
VoxelGrid::voxel2z(const float pz) const {
    return pz * voxelSize.z + boundingBox.minZ();
}

inline short
VoxelGrid::x2voxel(const float px) const {
    return static_cast<short>((voxelSize.x < Numeric::EPSILON)
                                  ? 0
                                  : (px - boundingBox.minX()) / voxelSize.x);
}

inline short
VoxelGrid::y2voxel(const float py) const {
    return static_cast<short>((voxelSize.y < Numeric::EPSILON)
                                  ? 0
                                  : (py - boundingBox.minY()) / voxelSize.y);
}

inline short
VoxelGrid::z2voxel(const float pz) const {
    return static_cast<short>((voxelSize.z < Numeric::EPSILON)
                                  ? 0
                                  : (pz - boundingBox.minZ()) / voxelSize.z);
}

inline int
VoxelGrid::cellIndexAddress(const int a, const int b, const int c) const {
    return (a * ySize + b) * zSize + c;
}

#endif

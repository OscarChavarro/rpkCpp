#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __VOXEL_GRID__
#define __VOXEL_GRID__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/skin/Geometry.h"
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
    #define DELTA_BOUND_FACTOR ((float)1e-4)

    // Owned by the root grid only (NULL on sub grids)
    ArrayList<VoxelGrid *> *subGrids;
    ArrayList<VoxelData *> *items;
    VoxelGrid *const root;

    short xSize;
    short ySize;
    short zSize;
    Vector3Df voxelSize;
    ArrayList<VoxelData *> **volumeListsOfItems; // 3D array of item lists
    void **gridItemPool;
    BoundingBox boundingBox;
    MinMaxBox *rayIntersectionBox;

    VoxelGrid(Geometry *geometry, VoxelGrid *inRoot, int level);
    void build(Geometry *geometry, int level);

    int addItem(VoxelData *cell);
    void addSubGrid(VoxelGrid *voxelGrid);
    int nextItemIndex() const;
    void freeVoxelGridElements();
    static short clampVoxel(short v, short max);
    static bool shouldSubdivide(const Geometry *geometry);
    static int countItems(Geometry *geometry);

    float voxel2x(float px) const;
    float voxel2y(float py) const;
    float voxel2z(float pz) const;
    short x2voxel(float px) const;
    short y2voxel(float py) const;
    short z2voxel(float pz) const;
    int cellIndexAddress(int a, int b, int c) const;
    void putGeometryInsideVoxelGrid(Geometry *geometry, short na, short nb, short nc, int level);
    bool isSmall(const BoundingBox *bb) const;
    void putSubGeometryInsideVoxelGrid(Geometry *geometry, int level);
    void putItemInsideVoxelGrid(VoxelData *item, const BoundingBox *itemBounds) const;
    void putPatchInsideVoxelGrid(Patch *patch);
    Vector3Df toVoxelClamped(const Vector3Df &p) const;
    void insertGeometryAsVoxelData(Geometry *geometry);
    void processCompoundGeometry(Geometry *geometry, int level);
    void processPatches(Geometry *geometry);
    void insertSubGrid(Geometry *geometry, int level);

    void
    gridTraceSetup(
        const RpkRay *ray,
        float t0,
        const Vector3Df *P,
        int *g,
        Vector3Df *tDelta,
        Vector3Df *tNext,
        int *step,
        int *out) const;

    bool
    gridBoundsIntersect(
        const RpkRay *ray,
        float minimumDistance,
        float maximumDistance,
        /*OUT*/ float *t0,
        Vector3Df *position) const;

    static RayHit *
    voxelIntersect(
        const ArrayList<VoxelData *> *items,
        RpkRay *ray,
        unsigned int counter,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore,
        const RayExclusionSet &exclusions,
        VoxelGridWorkspace *workspace);

    static bool
    nextVoxel(float *t0, int *g, Vector3Df *tNext, const Vector3Df *tDelta, const int *step, const int *out);

  public:
    explicit VoxelGrid(Geometry *geometry);
    virtual ~VoxelGrid();

    RayHit *
    gridIntersect(
        RpkRay *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore,
        const RayExclusionSet &exclusions,
        VoxelGridWorkspace *workspace) const;

    int getNumberOfItems() const;
    void print() const;
};

inline float
VoxelGrid::voxel2x(const float px) const {
    return px * voxelSize.x() + boundingBox.minX();
}

inline float
VoxelGrid::voxel2y(const float py) const {
    return py * voxelSize.y() + boundingBox.minY();
}

inline float
VoxelGrid::voxel2z(const float pz) const {
    return pz * voxelSize.z() + boundingBox.minZ();
}

inline short
VoxelGrid::x2voxel(const float px) const {
    return ((short)((voxelSize.x() < VSDK::EPSILON)
                                  ? 0
                                  : (px - boundingBox.minX()) / voxelSize.x()));
}

inline short
VoxelGrid::y2voxel(const float py) const {
    return ((short)((voxelSize.y() < VSDK::EPSILON)
                                  ? 0
                                  : (py - boundingBox.minY()) / voxelSize.y()));
}

inline short
VoxelGrid::z2voxel(const float pz) const {
    return ((short)((voxelSize.z() < VSDK::EPSILON)
                                  ? 0
                                  : (pz - boundingBox.minZ()) / voxelSize.z()));
}

inline int
VoxelGrid::cellIndexAddress(const int a, const int b, const int c) const {
    return (a * ySize + b) * zSize + c;
}

#endif

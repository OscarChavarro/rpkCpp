#ifndef __VOXEL_GRID__
#define __VOXEL_GRID__

#include "java/util/ArrayList.h"
#include "skin/Geometry.h"
#include "scene/VoxelData.h"

class VoxelGrid {
  private:
    static java::ArrayList<VoxelGrid *> *subGridsToDelete;
    static java::ArrayList<VoxelData *> *voxelCellsToDelete;

    short xSize;
    short ySize;
    short zSize;
    Vector3D voxelSize;
    java::ArrayList<VoxelData *> **volumeListsOfItems; // 3D array of item lists
    void **gridItemPool;
    BoundingBox boundingBox;

    static void addToSubGridsDeletionCache(VoxelGrid *voxelGrid);
    static void addToCellsDeletionCache(VoxelData *cell);
    static short clampVoxel(short v, short max);
    static bool shouldSubdivide(const Geometry *geometry);

    float voxel2x(float px) const;
    float voxel2y(float py) const;
    float voxel2z(float pz) const;
    short x2voxel(float px) const;
    short y2voxel(float py) const;
    short z2voxel(float pz) const;
    int cellIndexAddress(int a, int b, int c) const;
    void putGeometryInsideVoxelGrid(Geometry *geometry, short na, short nb, short nc);
    bool isSmall(const BoundingBox *bb) const;
    void putSubGeometryInsideVoxelGrid(Geometry *geometry);
    void putItemInsideVoxelGrid(VoxelData *item, const BoundingBox *itemBounds) const;
    void putPatchInsideVoxelGrid(Patch *patch) const;
    Vector3D toVoxelClamped(const Vector3D &p) const;
    void insertGeometryAsVoxelData(Geometry *geometry) const;
    void processCompoundGeometry(Geometry *geometry);
    void processPatches(Geometry *geometry) const;
    void insertSubGrid(Geometry *geometry) const;

    void
    gridTraceSetup(
        const Ray *ray,
        float t0,
        const Vector3D *P,
        int *g,
        Vector3D *tDelta,
        Vector3D *tNext,
        int *step,
        int *out) const;

    int
    gridBoundsIntersect(
        const Ray *ray,
        float minimumDistance,
        float maximumDistance,
        /*OUT*/ float *t0,
        Vector3D *position) const;

    static RayHit *
    voxelIntersect(
        const java::ArrayList<VoxelData *> *items,
        Ray *ray,
        unsigned int counter,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore);

    static bool
    nextVoxel(float *t0, int *g, Vector3D *tNext, const Vector3D *tDelta, const int *step, const int *out);

    static int randomRayId();

public:
    explicit VoxelGrid(Geometry *geometry);
    virtual ~VoxelGrid();

    RayHit *
    gridIntersect(
        Ray *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore) const;

    void print() const;

    static void freeVoxelGridElements();
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

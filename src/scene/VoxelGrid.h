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

    inline float
    voxel2x(const float px) const {
        return px * voxelSize.x + boundingBox.coordinates[MIN_X];
    }

    inline float
    voxel2y(const float py) const {
        return py * voxelSize.y + boundingBox.coordinates[MIN_Y];
    }

    inline float
    voxel2z(const float pz) const {
        return pz * voxelSize.z + boundingBox.coordinates[MIN_Z];
    }

    inline short
    x2voxel(const float px) const {
        return (short)((voxelSize.x < Numeric::EPSILON) ? 0 : (px - boundingBox.coordinates[MIN_X]) / voxelSize.x);
    }

    inline short
    y2voxel(const float py) const {
        return (short)((voxelSize.y < Numeric::EPSILON) ? 0 : (py - boundingBox.coordinates[MIN_Y]) / voxelSize.y);
    }

    inline short
    z2voxel(const float pz) const {
        return (short)((voxelSize.z < Numeric::EPSILON) ? 0 : (pz - boundingBox.coordinates[MIN_Z]) / voxelSize.z);
    }

    inline int
    cellIndexAddress(const int a, const int b, const int c) const {
        return (a * ySize + b) * zSize + c;
    }

    void putGeometryInsideVoxelGrid(Geometry *geometry, short na, short nb, short nc);
    int isSmall(const float *boundsArr) const;
    void putSubGeometryInsideVoxelGrid(Geometry *geometry);
    void putItemInsideVoxelGrid(VoxelData *item, const BoundingBox *itemBounds);
    void putPatchInsideVoxelGrid(Patch *patch);

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

#endif

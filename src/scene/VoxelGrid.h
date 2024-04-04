#ifndef __VOXEL_GRID__
#define __VOXEL_GRID__

#include "java/util/ArrayList.h"
#include "skin/Geometry.h"
#include "scene/VoxelData.h"

class VoxelGrid {
  private:
    short xSize;
    short ySize;
    short zSize;
    Vector3D voxelSize;
    java::ArrayList<VoxelData *> **volumeListsOfItems; // 3D array of item lists
    void **gridItemPool;
    BoundingBox boundingBox;

    inline float
    voxel2x(const float px) {
        return px * voxelSize.x + boundingBox.coordinates[MIN_X];
    }

    inline float
    voxel2y(const float py) {
        return py * voxelSize.y + boundingBox.coordinates[MIN_Y];
    }

    inline float
    voxel2z(const float pz) {
        return pz * voxelSize.z + boundingBox.coordinates[MIN_Z];
    }

    inline short
    x2voxel(const float px) {
        return (short)((voxelSize.x<EPSILON) ? 0 : (px - boundingBox.coordinates[MIN_X]) / voxelSize.x);
    }

    inline short
    y2voxel(const float py) {
        return (short)((voxelSize.y < EPSILON) ? 0 : (py - boundingBox.coordinates[MIN_Y]) / voxelSize.y);
    }

    inline short
    z2voxel(const float pz) {
        return (short)((voxelSize.z < EPSILON) ? 0 : (pz - boundingBox.coordinates[MIN_Z]) / voxelSize.z);
    }

    inline int
    cellIndexAddress(const int a, const int b, const int c) const {
        return (a * ySize + b) * zSize + c;
    }

    void putGeometryInsideVoxelGrid(Geometry *geometry, short na, short nb, short nc);

    int isSmall(const float *boundsArr) const;

    void putSubGeometryInsideVoxelGrid(Geometry *geometry);

    void destroyGridRecursive() const;

    void
    gridTraceSetup(
        Ray *ray,
        float t0,
        Vector3D *P,
        /*OUT*/ int *g,
        Vector3D *tDelta,
        Vector3D *tNext,
        int *step,
        int *out);

    int
    gridBoundsIntersect(
        Ray *ray,
        float minimumDistance,
        float maximumDistance,
        /*OUT*/ float *t0,
        Vector3D *position);

    void putItemInsideVoxelGrid(VoxelData *item, const BoundingBox *itemBounds);

    void putPatchInsideVoxelGrid(Patch *patch);

    static RayHit *
    voxelIntersect(
            java::ArrayList<VoxelData *> *items,
            Ray *ray,
            unsigned int counter,
            float minimumDistance,
            float *maximumDistance,
            int hitFlags,
            RayHit *hitStore);

    static int
    nextVoxel(float *t0, int *g, Vector3D *tNext, Vector3D *tDelta, const int *step, const int *out);

    static int randomRayId();

    static void freeFirstItem(java::ArrayList<VoxelData *> *deletedItems, VoxelData *item);

public:
    explicit VoxelGrid(Geometry *geometry);
    virtual ~VoxelGrid();

    RayHit *
    gridIntersect(
            Ray *ray,
            float minimumDistance,
            float *maximumDistance,
            int hitFlags,
            RayHit *hitStore);
};

#endif

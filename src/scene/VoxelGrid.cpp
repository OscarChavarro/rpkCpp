/**
Uniform voxel grid to optimize intersection operations

Ray-grid intersection: [SNYD1987] Snyder & Barr, SIGGRAPH '87, p123, with several
optimisations/enhancements from ray shade 4.0.6 by Graig Kolb, Stanford U
*/

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/mymath.h"
#include "scene/VoxelGrid.h"

#define MINIMUM_ELEMENT_COUNT_PER_CELL 10

java::ArrayList<VoxelGrid *> * VoxelGrid::subGridsToDelete = nullptr;
java::ArrayList<VoxelData *> * VoxelGrid::voxelCellsToDelete = nullptr;

/**
Constructs a recursive grid structure containing the whole geometry
*/
VoxelGrid::VoxelGrid(Geometry *geometry):
    boundingBox()
{
    xSize = 0.0f;
    ySize = 0.0f;
    zSize = 0.0f;
    volumeListsOfItems = nullptr;
    gridItemPool = nullptr;

    static int level = 0; // TODO warning: this makes this class non re-entrant
    short gridSize;

    if ( level == 0 ) {
        geometry->geomCountItems();
    }

    double p = std::pow((double) geometry->itemCount, 0.33333) + 1;
    gridSize = (short)std::floor(p);
    fprintf(stderr, "Setting %d volumeListsOfItems in %d^3 cells level %d voxel grid ... \n", geometry->itemCount, gridSize, level);
    level++;

    putGeometryInsideVoxelGrid(geometry, gridSize, gridSize, gridSize);

    level--;
}

VoxelGrid::~VoxelGrid() {
    for ( int i = 0; i < xSize * ySize * zSize; i++ ) {
        if ( volumeListsOfItems[i] != nullptr ) {
            delete volumeListsOfItems[i];
        }
    }
    delete[] volumeListsOfItems;
    delete gridItemPool;
}

void
VoxelGrid::addToCellsDeletionCache(VoxelData *cell) {
    if ( voxelCellsToDelete == nullptr ) {
        voxelCellsToDelete = new java::ArrayList<VoxelData *>();
    }
    voxelCellsToDelete->add(cell);
}

void
VoxelGrid::addToSubGridsDeletionCache(VoxelGrid *voxelGrid) {
    if ( subGridsToDelete == nullptr ) {
        subGridsToDelete = new java::ArrayList<VoxelGrid *>();
    }
    subGridsToDelete->add(voxelGrid);
}

void
VoxelGrid::freeVoxelGridElements() {
    if ( voxelCellsToDelete != nullptr ) {
        for ( int i = 0; i < voxelCellsToDelete->size(); i++ ) {
            VoxelData *cell = voxelCellsToDelete->get(i);
            if ( cell != nullptr ) {
                delete cell;
            }
        }
    }
    delete voxelCellsToDelete;

    if ( subGridsToDelete != nullptr ) {
        for ( int i = 0; i < subGridsToDelete->size(); i++ ) {
            VoxelGrid *subGrid = subGridsToDelete->get(i);
            delete subGrid->gridItemPool;
            subGrid->gridItemPool = nullptr;
            for ( int j = 0; j < subGrid->xSize * subGrid->ySize * subGrid->zSize; j++ ) {
                delete subGrid->volumeListsOfItems[j];
            }
            delete[] subGrid->volumeListsOfItems;
            subGrid->volumeListsOfItems = nullptr;
        }
        delete subGridsToDelete;
        subGridsToDelete = nullptr;
    }
}

int
VoxelGrid::isSmall(const float *boundsArr) const {
    return (boundsArr[MAX_X] - boundsArr[MIN_X]) <= voxelSize.x &&
           (boundsArr[MAX_Y] - boundsArr[MIN_Y]) <= voxelSize.y &&
           (boundsArr[MAX_Z] - boundsArr[MIN_Z]) <= voxelSize.z;
}

void
VoxelGrid::putItemInsideVoxelGrid(VoxelData *item, const BoundingBox *itemBounds) {
    // Enlarge the boundaries by a small amount in all directions
    BoundingBox boundaries;
    float xExtent = (boundingBox.coordinates[MAX_X] - boundingBox.coordinates[MIN_X]) * 1e-4f;
    float yExtent = (boundingBox.coordinates[MAX_Y] - boundingBox.coordinates[MIN_Y]) * 1e-4f;
    float zExtent = (boundingBox.coordinates[MAX_Z] - boundingBox.coordinates[MIN_Z]) * 1e-4f;
    boundaries.copyFrom(itemBounds);
    boundaries.coordinates[MIN_X] -= xExtent;
    boundaries.coordinates[MAX_X] += xExtent;
    boundaries.coordinates[MIN_Y] -= yExtent;
    boundaries.coordinates[MAX_Y] += yExtent;
    boundaries.coordinates[MIN_Z] -= zExtent;
    boundaries.coordinates[MAX_Z] += zExtent;

    short minA = x2voxel(boundaries.coordinates[MIN_X]);
    if ( minA >= xSize ) {
        minA = (short)(xSize - 1);
    }
    if ( minA < 0 ) {
        minA = 0;
    }

    short maxA = x2voxel(boundaries.coordinates[MAX_X]);
    if ( maxA >= xSize ) {
        maxA = (short)(xSize - 1);
    }
    if ( maxA < 0 ) {
        maxA = 0;
    }

    short minB = y2voxel(boundaries.coordinates[MIN_Y]);
    if ( minB >= ySize ) {
        minB = (short)(ySize - 1);
    }
    if ( minB < 0 ) {
        minB = 0;
    }

    short maxB = y2voxel(boundaries.coordinates[MAX_Y]);
    if ( maxB >= ySize ) {
        maxB = (short)(ySize - 1);
    }
    if ( maxB < 0 ) {
        maxB = 0;
    }

    short minC = z2voxel(boundaries.coordinates[MIN_Z]);
    if ( minC >= zSize ) {
        minC = (short)(zSize - 1);
    }
    if ( minC < 0 ) {
        minC = 0;
    }

    short maxC = z2voxel(boundaries.coordinates[MAX_Z]);
    if ( maxC >= zSize ) {
        maxC = (short)(zSize - 1);
    }
    if ( maxC < 0 ) {
        maxC = 0;
    }

    // Insert the current item in to all voxels that intersects with bounding box
    for ( short a = minA; a <= maxA; a++ ) {
        for ( short b = minB; b <= maxB; b++ ) {
            for ( short c = minC; c <= maxC; c++ ) {
                java::ArrayList<VoxelData *> **voxelList = &volumeListsOfItems[cellIndexAddress(a, b, c)];
                if ( (*voxelList) == nullptr ) {
                    (*voxelList) = new java::ArrayList<VoxelData *>(1);
                }
                if ( item != nullptr ) {
                    (*voxelList)->add(item);
                }
            }
        }
    }
}

void
VoxelGrid::putPatchInsideVoxelGrid(Patch *patch) {
    BoundingBox localBounds;
    if ( patch->boundingBox != nullptr ) {
        localBounds = *patch->boundingBox;
    } else {
        patch->computeAndGetBoundingBox(&localBounds);
    }

    VoxelData *voxelData = new VoxelData(patch, VOXEL_DATA_PATCH_MASK);
    putItemInsideVoxelGrid(voxelData, &localBounds);
    addToCellsDeletionCache(voxelData);
}

void
VoxelGrid::putSubGeometryInsideVoxelGrid(Geometry *geometry) {
    if ( isSmall(geometry->boundingBox.coordinates) ) {
        if ( geometry->itemCount < MINIMUM_ELEMENT_COUNT_PER_CELL ) {
            VoxelData *voxelData = new VoxelData(geometry, VOXEL_DATA_GEOMETRY_MASK);
            putItemInsideVoxelGrid(voxelData, &geometry->boundingBox);
            addToCellsDeletionCache(voxelData);
        } else {
            VoxelGrid *subGrid = new VoxelGrid(geometry);
            VoxelData *voxelData = new VoxelData(subGrid, VOXEL_DATA_GRID_MASK);
            putItemInsideVoxelGrid(voxelData, &subGrid->boundingBox);
            addToSubGridsDeletionCache(subGrid);
            addToCellsDeletionCache(voxelData);
        }
    } else {
        if ( geometry->isCompound() ) {
            const java::ArrayList<Geometry *> *geometryList = geometry->compoundData->children;
            for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
                putSubGeometryInsideVoxelGrid(geometryList->get(i));
            }
        } else {
            const java::ArrayList<Patch *> *patches = geomPatchArrayListReference(geometry);
            for ( int i = 0; patches != nullptr && i < patches->size(); i++) {
                putPatchInsideVoxelGrid(patches->get(i));
            }
        }
    }
}

void
VoxelGrid::putGeometryInsideVoxelGrid(Geometry *geometry, const short na, const short nb, const short nc) {
    float xExtension;
    float yExtension;
    float zExtension;

    if ( na <= 0 || nb <= 0 || nc <= 0 ) {
        logError("VoxelGrid::putGeometryInsideVoxelGrid", "Invalid grid dimensions");
        exit(1);
    }

    // Enlarge the getBoundingBox by a small amount
    xExtension = (geometry->boundingBox.coordinates[MAX_X] - geometry->boundingBox.coordinates[MIN_X]) * 1e-4f;
    yExtension = (geometry->boundingBox.coordinates[MAX_Y] - geometry->boundingBox.coordinates[MIN_Y]) * 1e-4f;
    zExtension = (geometry->boundingBox.coordinates[MAX_Z] - geometry->boundingBox.coordinates[MIN_Z]) * 1e-4f;
    boundingBox.copyFrom(&geometry->boundingBox);
    boundingBox.coordinates[MIN_X] -= xExtension;
    boundingBox.coordinates[MAX_X] += xExtension;
    boundingBox.coordinates[MIN_Y] -= yExtension;
    boundingBox.coordinates[MAX_Y] += yExtension;
    boundingBox.coordinates[MIN_Z] -= zExtension;
    boundingBox.coordinates[MAX_Z] += zExtension;

    xSize = na;
    ySize = nb;
    zSize = nc;
    voxelSize.x = (boundingBox.coordinates[MAX_X] - boundingBox.coordinates[MIN_X]) / (float) na;
    voxelSize.y = (boundingBox.coordinates[MAX_Y] - boundingBox.coordinates[MIN_Y]) / (float) nb;
    voxelSize.z = (boundingBox.coordinates[MAX_Z] - boundingBox.coordinates[MIN_Z]) / (float) nc;
    volumeListsOfItems = new java::ArrayList<VoxelData *> *[na * nb * nc]();
    gridItemPool = nullptr;
    for ( int i = 0; i < na * nb * nc; i++ ) {
        volumeListsOfItems[i] = nullptr;
    }
    putSubGeometryInsideVoxelGrid(geometry);
}

int
VoxelGrid::randomRayId() {
    static int count = 0; // TODO warning: this makes this class non re-entrant
    count++;
    return (count & VOXEL_DATA_RAY_COUNT_MASK);
}

/**
Compute t0, ray's minimal intersection with the whole grid and
position P of this intersection. Returns true if the grid getBoundingBox are
intersected and false if the ray passes along the voxel grid
*/
int
VoxelGrid::gridBoundsIntersect(
    const Ray *ray,
    float minimumDistance,
    float maximumDistance,
    /*OUT*/ float *t0,
    Vector3D *position) const
{
    *t0 = minimumDistance;
    vectorSumScaled(ray->pos, *t0, ray->dir, *position);
    if ( boundingBox.outOfBounds(position) ) {
        *t0 = maximumDistance;
        if ( !boundingBox.intersect(ray, minimumDistance, t0) ) {
            return false;
        }
        vectorSumScaled(ray->pos, *t0, ray->dir, *position);
    }

    return true;
}

/**
Initializes grid tracing
*/
void
VoxelGrid::gridTraceSetup(
    const Ray *ray,
    const float t0,
    const Vector3D *P,
    int *g,
    Vector3D *tDelta,
    Vector3D *tNext,
    int *step,
    int *out) const
{
    // Compute the grid cell g where this intersection occurs
    g[0] = x2voxel(P->x);
    if ( g[0] >= xSize ) {
        g[0] = xSize - 1;
    }
    g[1] = y2voxel(P->y);
    if ( g[1] >= ySize ) {
        g[1] = ySize - 1;
    }
    g[2] = z2voxel(P->z);
    if ( g[2] >= zSize ) {
        g[2] = zSize - 1;
    }

    /*
    Setup X:
    tDelta->x is the distance increment along the ray to the adjacent
    voxel in X direction.
    tNext->x is the total distance from the ray origin to the next voxel
    in X direction.
    step[0] is either +1 or -1 according to the ray X direction.
    out[0] is -1 or xSize: the first x grid cell index outside the
    grid.
    */
    if ( std::fabs(ray->dir.x) > EPSILON ) {
        if ( ray->dir.x > 0.0 ) {
            tDelta->x = voxelSize.x / ray->dir.x;
            tNext->x = t0 + (voxel2x((float)g[0] + 1) - P->x) / ray->dir.x;
            step[0] = 1;
            out[0] = xSize;
        } else {
            tDelta->x = voxelSize.x / -ray->dir.x;
            tNext->x = t0 + (voxel2x((float)g[0]) - P->x) / ray->dir.x;
            step[0] = out[0] = -1;
        }
    } else {
        tDelta->x = 0.0;
        tNext->x = HUGE_FLOAT;
    }

    // Setup Y:
    if ( std::fabs(ray->dir.y) > EPSILON ) {
        if ( ray->dir.y > 0.0 ) {
            tDelta->y = voxelSize.y / ray->dir.y;
            tNext->y = t0 + (voxel2y((float)g[1] + 1) - P->y) / ray->dir.y;
            step[1] = 1;
            out[1] = ySize;
        } else {
            tDelta->y = voxelSize.y / -ray->dir.y;
            tNext->y = t0 + (voxel2y((float)g[1]) - P->y) / ray->dir.y;
            step[1] = out[1] = -1;
        }
    } else {
        tDelta->y = 0.0;
        tNext->y = HUGE_FLOAT;
    }

    // Setup Z:
    if ( std::fabs(ray->dir.z) > EPSILON ) {
        if ( ray->dir.z > 0.0 ) {
            tDelta->z = voxelSize.z / ray->dir.z;
            tNext->z = t0 + (voxel2z((float)g[2] + 1) - P->z) / ray->dir.z;
            step[2] = 1;
            out[2] = zSize;
        } else {
            tDelta->z = voxelSize.z / -ray->dir.z;
            tNext->z = t0 + (voxel2z((float)g[2]) - P->z) / ray->dir.z;
            step[2] = out[2] = -1;
        }
    } else {
        tDelta->z = 0.0;
        tNext->z = HUGE_FLOAT;
    }
}

/**
Advances to the next grid cell. Assumes setup with gridTraceSetup().
returns false if the current voxel was the last voxel in the grid intersected
by the ray
*/
int
VoxelGrid::nextVoxel(float *t0, int *g, Vector3D *tNext, const Vector3D *tDelta, const int *step, const int *out) {
    int inGrid;

    if ( tNext->x <= tNext->y && tNext->x <= tNext->z ) {
        // tNext->x is smallest
        g[0] += step[0];
        *t0 = tNext->x;
        tNext->x += tDelta->x;
        inGrid = g[0] - out[0]; // false if g[0]==out[0]
    } else if ( tNext->y <= tNext->z ) {
        // tNext->y is smallest
        g[1] += step[1];
        *t0 = tNext->y;
        tNext->y += tDelta->y;
        inGrid = g[1] - out[1];
    } else {
        // tNext->z is smallest
        g[2] += step[2];
        *t0 = tNext->z;
        tNext->z += tDelta->z;
        inGrid = g[2] - out[2];
    }
    return inGrid;
}

/**
Finds the nearest intersection of the ray with an item (Geometry or Patch) in
a voxel's item list. If there is an intersection, maximumDistance will contain
the distance to the intersection point measured from the ray origin
as usual. If there is no intersection, maximumDistance remains unmodified
*/
RayHit *
VoxelGrid::voxelIntersect(
    const java::ArrayList<VoxelData *> *items,
    Ray *ray,
    const unsigned int counter,
    const float minimumDistance,
    float *maximumDistance,
    const int hitFlags,
    RayHit *hitStore)
{
    RayHit *hit = nullptr;

    for ( long i = 0; items != nullptr && i < items->size(); i++ ) {
        VoxelData *item = items->get(i);
        if ( item->lastRayId() != counter ) {
            // Avoid testing objects multiple times
            RayHit *h = nullptr;
            if ( item->isPatch() ) {
                h = item->patch->intersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
            } else if ( item->isGeom() ) {
                h = item->geometry->discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
            } else if ( item->isGrid() ) {
                h = item->voxelGrid->gridIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
            }
            if ( h ) {
                hit = h;
            }

            item->updateRayId(counter);
        }
    }

    return hit;
}

/**
Traces a ray through a voxel grid. Returns nearest intersection or nullptr
*/
RayHit *
VoxelGrid::gridIntersect(
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore) const
{
    Vector3D tNext;
    Vector3D tDelta;
    Vector3D P;
    int step[3];
    int out[3];
    int g[3]{0, 0, 0};
    RayHit *hit = nullptr;
    float t0;
    int counter;

    if ( !gridBoundsIntersect(ray, minimumDistance, *maximumDistance, &t0, &P) ) {
        return nullptr;
    }

    gridTraceSetup(ray, t0, &P, g, &tDelta, &tNext, step, out);

    // Ray counter in order to avoid testing objects spanning several voxel grid cells multiple times
    counter = randomRayId();

    do {
        const java::ArrayList<VoxelData *> *list = volumeListsOfItems[cellIndexAddress(g[0], g[1], g[2])];
        if ( list != nullptr ) {
            RayHit *h = voxelIntersect(list, ray, counter, t0, maximumDistance, hitFlags, hitStore);
            if ( h != nullptr ) {
                hit = h;
            }
        }
    } while ( nextVoxel(&t0, g, &tNext, &tDelta, step, out) && t0 <= *maximumDistance );

    return hit;
}

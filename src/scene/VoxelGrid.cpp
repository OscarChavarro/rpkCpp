/**
Uniform voxel grid to optimize intersection operations

Ray-grid intersection: [SNYD1987] Snyder & Barr, SIGGRAPH '87, p123, with several
optimisations/enhancements from ray shade 4.0.6 by Graig Kolb, Stanford U
*/

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "scene/VoxelGrid.h"

/**
Constructs a recursive grid structure containing the whole geometry
*/
VoxelGrid::VoxelGrid(Geometry *geom) : boundingBox{} {
    xSize = 0.0f;
    ySize = 0.0f;
    zSize = 0.0f;
    volumeListsOfItems = nullptr;
    gridItemPool = nullptr;

    static int level = 0; // TODO warning: this makes this class non re-entrant
    short gridSize;

    if ( level == 0 ) {
        geom->geomCountItems();
    }

    double p = std::pow((double) geom->itemCount, 0.33333) + 1;
    gridSize = (short)std::floor(p);
    fprintf(stderr, "Setting %d volumeListsOfItems in %d^3 cells level %d voxel grid ... \n", geom->itemCount, gridSize, level);
    level++;

    putGeometryInsideVoxelGrid(geom, gridSize, gridSize, gridSize);

    level--;
}

int
VoxelGrid::isSmall(const float *boundsArr) const {
    return (boundsArr[MAX_X] - boundsArr[MIN_X]) <= voxelSize.x &&
           (boundsArr[MAX_Y] - boundsArr[MIN_Y]) <= voxelSize.y &&
           (boundsArr[MAX_Z] - boundsArr[MIN_Z]) <= voxelSize.z;
}

void
VoxelGrid::putItemInsideVoxelGrid(VoxelData *item, const float *itemBounds) {
    short minA;
    short minB;
    short minC;
    short maxA;
    short maxB;
    short maxC;
    short a;
    short b;
    short c;

    // Enlarge the boundaries by a small amount in all directions
    BOUNDINGBOX boundaries;
    float xExtent = (boundingBox[MAX_X] - boundingBox[MIN_X]) * 1e-4f;
    float yExtent = (boundingBox[MAX_Y] - boundingBox[MIN_Y]) * 1e-4f;
    float zExtent = (boundingBox[MAX_Z] - boundingBox[MIN_Z]) * 1e-4f;
    boundsCopy(itemBounds, boundaries);
    boundaries[MIN_X] -= xExtent;
    boundaries[MAX_X] += xExtent;
    boundaries[MIN_Y] -= yExtent;
    boundaries[MAX_Y] += yExtent;
    boundaries[MIN_Z] -= zExtent;
    boundaries[MAX_Z] += zExtent;

    minA = x2voxel(boundaries[MIN_X]);
    if ( minA >= xSize ) {
        minA = (short)(xSize - 1);
    }
    if ( minA < 0 ) {
        minA = 0;
    }
    maxA = x2voxel(boundaries[MAX_X]);
    if ( maxA >= xSize ) {
        maxA = (short)(xSize - 1);
    }
    if ( maxA < 0 ) {
        maxA = 0;
    }

    minB = y2voxel(boundaries[MIN_Y]);
    if ( minB >= ySize ) {
        minB = (short)(ySize - 1);
    }
    if ( minB < 0 ) {
        minB = 0;
    }
    maxB = y2voxel(boundaries[MAX_Y]);
    if ( maxB >= ySize ) {
        maxB = (short)(ySize - 1);
    }
    if ( maxB < 0 ) {
        maxB = 0;
    }

    minC = z2voxel(boundaries[MIN_Z]);
    if ( minC >= zSize ) {
        minC = (short)(zSize - 1);
    }
    if ( minC < 0 ) {
        minC = 0;
    }
    maxC = z2voxel(boundaries[MAX_Z]);
    if ( maxC >= zSize ) {
        maxC = (short)(zSize - 1);
    }
    if ( maxC < 0 ) {
        maxC = 0;
    }

    // Insert the current item in to all voxels that intersects with bounding box
    for ( a = minA; a <= maxA; a++ ) {
        for ( b = minB; b <= maxB; b++ ) {
            for ( c = minC; c <= maxC; c++ ) {
                java::ArrayList<VoxelData *> **voxelList = &volumeListsOfItems[cellIndexAddress(a, b, c)];
                if ( (*voxelList) == nullptr ) {
                    (*voxelList) = new java::ArrayList<VoxelData *>();
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
    BOUNDINGBOX localBounds;
    putItemInsideVoxelGrid(new VoxelData(patch, PATCH_MASK),
                           patch->boundingBox ? patch->boundingBox : patchBounds(patch, localBounds));
}

void
VoxelGrid::putSubGeometryInsideVoxelGrid(Geometry *geometry) {
    if ( isSmall(geometry->bounds) ) {
        if ( geometry->itemCount < 10 ) {
            putItemInsideVoxelGrid(new VoxelData(geometry, GEOM_MASK), geometry->bounds);
        } else {
            VoxelGrid *subgrid = new VoxelGrid(geometry);
            putItemInsideVoxelGrid(new VoxelData(subgrid, GRID_MASK), subgrid->boundingBox);
        }
    } else {
        if ( geomIsAggregate(geometry) ) {
            java::ArrayList<Geometry *> *geometryList = geomPrimList(geometry);
            for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
                putSubGeometryInsideVoxelGrid(geometryList->get(i));
            }
            delete geometryList;
        } else {
            java::ArrayList<Patch *> *patches = geomPatchArrayList(geometry);
            for ( int i = 0; patches != nullptr && i < patches->size(); i++) {
                putPatchInsideVoxelGrid(patches->get(i));
            }
            delete patches;
        }
    }
}

void
VoxelGrid::putGeometryInsideVoxelGrid(Geometry *geometry, const short na, const short nb, const short nc) {
    int i;
    float xExtension;
    float yExtension;
    float zExtension;

    if ( na <= 0 || nb <= 0 || nc <= 0 ) {
        logError("VoxelGrid::putGeometryInsideVoxelGrid", "Invalid grid dimensions");
        exit(1);
    }

    // Enlarge the getBoundingBox by a small amount
    xExtension = (geometry->bounds[MAX_X] - geometry->bounds[MIN_X]) * 1e-4f;
    yExtension = (geometry->bounds[MAX_Y] - geometry->bounds[MIN_Y]) * 1e-4f;
    zExtension = (geometry->bounds[MAX_Z] - geometry->bounds[MIN_Z]) * 1e-4f;
    boundsCopy(geometry->bounds, boundingBox);
    boundingBox[MIN_X] -= xExtension;
    boundingBox[MAX_X] += xExtension;
    boundingBox[MIN_Y] -= yExtension;
    boundingBox[MAX_Y] += yExtension;
    boundingBox[MIN_Z] -= zExtension;
    boundingBox[MAX_Z] += zExtension;

    xSize = na;
    ySize = nb;
    zSize = nc;
    voxelSize.x = (boundingBox[MAX_X] - boundingBox[MIN_X]) / (float) na;
    voxelSize.y = (boundingBox[MAX_Y] - boundingBox[MIN_Y]) / (float) nb;
    voxelSize.z = (boundingBox[MAX_Z] - boundingBox[MIN_Z]) / (float) nc;
    volumeListsOfItems = new java::ArrayList<VoxelData *> *[na * nb * nc]();
    gridItemPool = nullptr;
    for ( i = 0; i < na * nb * nc; i++ ) {
        volumeListsOfItems[i] = nullptr;
    }
    putSubGeometryInsideVoxelGrid(geometry);
}

void
VoxelGrid::destroyGridRecursive() const {
    int i;

    for ( i = 0; i < xSize * ySize * zSize; i++ ) {
        java::ArrayList<VoxelData *> *list = volumeListsOfItems[i];
        if ( list != nullptr ) {
            for ( long int j = 0; j < list->size(); j++ ) {
                VoxelData *item = list->get(j);
                if ( item->isGrid() && item->voxelGrid != nullptr ) {
                    item->voxelGrid->destroyGridRecursive();
                    item->voxelGrid = nullptr;
                }
            }
        }

        delete list;
    }
}

int
VoxelGrid::randomRayId() {
    static int count = 0; // TODO warning: this makes this class non re-entrant
    count++;
    return (count & RAY_COUNT_MASK);
}

/**
Compute t0, ray's minimal intersection with the whole grid and
position P of this intersection. Returns true if the grid getBoundingBox are
intersected and false if the ray passes along the voxel grid
*/
int
VoxelGrid::gridBoundsIntersect(
    Ray *ray,
    float minimumDistance,
    float maximumDistance,
    /*OUT*/ float *t0,
    Vector3D *P)
{
    *t0 = minimumDistance;
    VECTORSUMSCALED(ray->pos, *t0, ray->dir, *P);
    if ( outOfBounds(P, boundingBox)) {
        *t0 = maximumDistance;
        if ( !boundsIntersect(ray, boundingBox, minimumDistance, t0)) {
            return false;
        }
        VECTORSUMSCALED(ray->pos, *t0, ray->dir, *P);
    } else {
    }

    return true;
}

/**
Initializes grid tracing
*/
void
VoxelGrid::gridTraceSetup(
    Ray *ray,
    float t0,
    Vector3D *P,
    /*OUT*/ int *g,
    Vector3D *tDelta,
    Vector3D *tNext,
    int *step,
    int *out)
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
        if ( ray->dir.x > 0. ) {
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
        tDelta->x = 0.;
        tNext->x = HUGE;
    }

    // Setup Y:
    if ( std::fabs(ray->dir.y) > EPSILON ) {
        if ( ray->dir.y > 0. ) {
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
        tDelta->y = 0.;
        tNext->y = HUGE;
    }

    // Setup Z:
    if ( std::fabs(ray->dir.z) > EPSILON ) {
        if ( ray->dir.z > 0. ) {
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
        tDelta->z = 0.;
        tNext->z = HUGE;
    }
}

/**
Advances to the next grid cell. Assumes setup with gridTraceSetup().
returns false if the current voxel was the last voxel in the grid intersected
by the ray
*/
int
VoxelGrid::nextVoxel(float *t0, int *g, Vector3D *tNext, Vector3D *tDelta, const int *step, const int *out) {
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
        java::ArrayList<VoxelData *> *items,
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
                h = patchIntersect(item->patch, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
            } else if ( item->isGeom() ) {
                h = geomDiscretizationIntersect(item->geometry, ray, minimumDistance, maximumDistance, hitFlags,
                                                hitStore);
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
Prepends all hits in the voxel to the hitList. The modified hitList is returned
*/
HITLIST *
VoxelGrid::allVoxelIntersections(
    HITLIST *hitList,
    java::ArrayList<VoxelData *> *items,
    Ray *ray,
    const unsigned int counter,
    const float minimumDistance,
    const float maximumDistance,
    const int hitFlags)
{
    RayHit hitStore;
    for ( long i = 0; items != nullptr && i < items->size(); i++ ) {
        VoxelData *item = items->get(i);

        // Avoid testing objects multiple times
        if ( item->lastRayId() != counter ) {
            if ( item->isPatch()) {
                float tMax = maximumDistance;
                RayHit *h = patchIntersect(item->patch, ray, minimumDistance, &tMax, hitFlags, &hitStore);
                if ( h ) {
                    hitList = HitListAdd(hitList, duplicateHit(h));
                }
            } else if ( item->isGeom()) {
                hitList = geomAllDiscretizationIntersections(
                    hitList, item->geometry, ray, minimumDistance, maximumDistance, hitFlags);
            } else if ( item->isGrid()) {
                hitList = item->voxelGrid->allGridIntersections(
                    hitList, ray, minimumDistance, maximumDistance, hitFlags);
            }

            item->updateRayId(counter);
        }
    }

    return hitList;
}

/**
Traces a ray through a voxel grid. Returns a list of all intersections
*/
HITLIST *
VoxelGrid::allGridIntersections(
    HITLIST *hits,
    Ray *ray,
    const float minimumDistance,
    const float maximumDistance,
    const int hitFlags)
{
    Vector3D tNext;
    Vector3D tDelta;
    Vector3D P;
    int step[3];
    int out[3];
    int g[3];
    float t0;
    int counter;

    if ( !gridBoundsIntersect(ray, minimumDistance, maximumDistance, &t0, &P)) {
        return hits;
    }
    gridTraceSetup(ray, t0, &P, g, &tDelta, &tNext, step, out);

    // Ray counter in order to avoid testing objects spanning several voxel grid cells multiple times
    counter = randomRayId();

    do {
        java::ArrayList<VoxelData *> *itemList = volumeListsOfItems[cellIndexAddress(g[0], g[1], g[2])];
        if ( itemList ) {
            hits = allVoxelIntersections(hits, itemList, ray, counter, t0, maximumDistance, hitFlags);
        }
    } while ( nextVoxel(&t0, g, &tNext, &tDelta, step, out) && t0 <= maximumDistance );

    return hits;
}

void
VoxelGrid::destroyGrid() const {
    destroyGridRecursive();
    delete gridItemPool;
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
        RayHit *hitStore)
{
    Vector3D tNext;
    Vector3D tDelta;
    Vector3D P;
    int step[3];
    int out[3];
    int g[3];
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
        java::ArrayList<VoxelData *> *list = volumeListsOfItems[cellIndexAddress(g[0], g[1], g[2])];
        if ( list != nullptr ) {
            RayHit *h;
            if ((h = voxelIntersect(list, ray, counter, t0, maximumDistance, hitFlags, hitStore))) {
                hit = h;
            }
        }
    } while ( nextVoxel(&t0, g, &tNext, &tDelta, step, out) && t0 <= *maximumDistance );

    return hit;
}

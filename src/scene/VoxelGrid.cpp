/**
Uniform voxel grid to optimize intersection operations

Ray-grid intersection: [SNYD1987] Snyder & Barr, SIGGRAPH '87, p123, with several
optimisations/enhancements from ray shade 4.0.6 by Graig Kolb, Stanford U
*/

#include "java/util/ArrayList.txx"
#include "java/lang/System.h"
#include "common/error.h"
#include "scene/VoxelGrid.h"

static constexpr int MINIMUM_ELEMENT_COUNT_PER_CELL = 10;
static constexpr float DELTA_BOUND_FACTOR = 1e-4f;

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

    const double p = java::Math::pow((double) geometry->itemCount, 0.33333) + 1;
    const short gridSize = static_cast<short>(java::Math::floor(p));
    java::lang::System::err.printf("Setting %d volumeListsOfItems in %d^3 cells level %d voxel grid ... \n", geometry->itemCount, gridSize, level);
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

bool
VoxelGrid::isSmall(const BoundingBox *bb) const {
    return bb->dx() <= voxelSize.x &&
           bb->dy() <= voxelSize.y &&
           bb->dz() <= voxelSize.z;
}

void
VoxelGrid::putItemInsideVoxelGrid(VoxelData *item, const BoundingBox *itemBounds) const {

    BoundingBox boundaries;
    boundaries.copyFrom(itemBounds);
    boundaries.enlargeByFactor(DELTA_BOUND_FACTOR);

    Vector3D minVoxel = toVoxelClamped(boundaries.minPoint());
    Vector3D maxVoxel = toVoxelClamped(boundaries.maxPoint());

    short minA = static_cast<short>(minVoxel.x);
    short minB = static_cast<short>(minVoxel.y);
    short minC = static_cast<short>(minVoxel.z);

    short maxA = static_cast<short>(maxVoxel.x);
    short maxB = static_cast<short>(maxVoxel.y);
    short maxC = static_cast<short>(maxVoxel.z);

    // Insert the current item in to all voxels that intersects with bounding box
    for ( short a = minA; a <= maxA; a++ ) {
        for ( short b = minB; b <= maxB; b++ ) {
            for ( short c = minC; c <= maxC; c++ ) {

                java::ArrayList<VoxelData *> **voxelList =
                        &volumeListsOfItems[cellIndexAddress(a, b, c)];

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
VoxelGrid::putPatchInsideVoxelGrid(Patch *patch) const {
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

short
VoxelGrid::clampVoxel(short v, short max) const {
    if (v < 0) return 0;
    if (v >= max) return static_cast<short>(max - 1);
    return v;
}

Vector3D
VoxelGrid::toVoxelClamped(const Vector3D &p) const {
    return Vector3D(
            clampVoxel(x2voxel(p.x), xSize),
            clampVoxel(y2voxel(p.y), ySize),
            clampVoxel(z2voxel(p.z), zSize)
    );
}

bool
VoxelGrid::shouldSubdivide(const Geometry *geometry) const {
    return geometry->itemCount >= MINIMUM_ELEMENT_COUNT_PER_CELL;
}

void
VoxelGrid::insertGeometryAsVoxelData(Geometry *geometry) {
    VoxelData *voxelData = new VoxelData(geometry, VOXEL_DATA_GEOMETRY_MASK);
    putItemInsideVoxelGrid(voxelData, &geometry->boundingBox);
    addToCellsDeletionCache(voxelData);
}

void
VoxelGrid::insertSubGrid(Geometry *geometry) {
    VoxelGrid *subGrid = new VoxelGrid(geometry);
    VoxelData *voxelData = new VoxelData(subGrid, VOXEL_DATA_GRID_MASK);

    putItemInsideVoxelGrid(voxelData, &subGrid->boundingBox);

    addToSubGridsDeletionCache(subGrid);
    addToCellsDeletionCache(voxelData);
}

void
VoxelGrid::processCompoundGeometry(Geometry *geometry) {
    const java::ArrayList<Geometry *> *geometryList =
            ((const Compound *)geometry)->children;

    for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
        putSubGeometryInsideVoxelGrid(geometryList->get(i));
    }
}

void
VoxelGrid::processPatches(Geometry *geometry) {
    const java::ArrayList<Patch *> *patches =
            geomPatchArrayListReference(geometry);

    for ( int i = 0; patches != nullptr && i < patches->size(); i++) {
        putPatchInsideVoxelGrid(patches->get(i));
    }
}

void
VoxelGrid::putSubGeometryInsideVoxelGrid(Geometry *geometry) {
    if ( isSmall(&geometry->boundingBox) ) {
        if ( shouldSubdivide(geometry) ) {
            insertSubGrid(geometry);
        } else {
            insertGeometryAsVoxelData(geometry);
        }
        return;
    }

    if ( geometry->isCompound() ) {
        processCompoundGeometry(geometry);
        return;
    }
    processPatches(geometry);
}

void
VoxelGrid::putGeometryInsideVoxelGrid(Geometry *geometry, const short na, const short nb, const short nc) {
    if ( na <= 0 || nb <= 0 || nc <= 0 ) {
        logError("VoxelGrid::putGeometryInsideVoxelGrid", "Invalid grid dimensions");
        exit(1);
    }

    // Enlarge the getBoundingBox by a small amount
    boundingBox.copyFrom(&geometry->boundingBox);
    boundingBox.enlargeByFactor(DELTA_BOUND_FACTOR);
    xSize = na;
    ySize = nb;
    zSize = nc;

    voxelSize = boundingBox.voxelSize(na, nb, nc);

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
    position->sumScaled(ray->position, *t0, ray->direction);
    if ( boundingBox.outOfBounds(position) ) {
        *t0 = maximumDistance;
        if ( !boundingBox.intersect(ray, minimumDistance, t0) ) {
            return false;
        }
        position->sumScaled(ray->position, *t0, ray->direction);
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
    if ( java::Math::abs(ray->direction.x) > Numeric::EPSILON ) {
        if ( ray->direction.x > 0.0 ) {
            tDelta->x = voxelSize.x / ray->direction.x;
            tNext->x = t0 + (voxel2x(static_cast<float>(g[0]) + 1) - P->x) / ray->direction.x;
            step[0] = 1;
            out[0] = xSize;
        } else {
            tDelta->x = voxelSize.x / -ray->direction.x;
            tNext->x = t0 + (voxel2x(static_cast<float>(g[0])) - P->x) / ray->direction.x;
            step[0] = out[0] = -1;
        }
    } else {
        tDelta->x = 0.0;
        tNext->x = Numeric::HUGE_FLOAT_VALUE;
    }

    // Setup Y:
    if ( java::Math::abs(ray->direction.y) > Numeric::EPSILON ) {
        if ( ray->direction.y > 0.0 ) {
            tDelta->y = voxelSize.y / ray->direction.y;
            tNext->y = t0 + (voxel2y(static_cast<float>(g[1]) + 1) - P->y) / ray->direction.y;
            step[1] = 1;
            out[1] = ySize;
        } else {
            tDelta->y = voxelSize.y / -ray->direction.y;
            tNext->y = t0 + (voxel2y(static_cast<float>(g[1])) - P->y) / ray->direction.y;
            step[1] = out[1] = -1;
        }
    } else {
        tDelta->y = 0.0;
        tNext->y = Numeric::HUGE_FLOAT_VALUE;
    }

    // Setup Z:
    if ( java::Math::abs(ray->direction.z) > Numeric::EPSILON ) {
        if ( ray->direction.z > 0.0 ) {
            tDelta->z = voxelSize.z / ray->direction.z;
            tNext->z = t0 + (voxel2z(static_cast<float>(g[2]) + 1) - P->z) / ray->direction.z;
            step[2] = 1;
            out[2] = zSize;
        } else {
            tDelta->z = voxelSize.z / -ray->direction.z;
            tNext->z = t0 + (voxel2z(static_cast<float>(g[2])) - P->z) / ray->direction.z;
            step[2] = out[2] = -1;
        }
    } else {
        tDelta->z = 0.0;
        tNext->z = Numeric::HUGE_FLOAT_VALUE;
    }
}

/**
Advances to the next grid cell. Pre-condition: gridTraceSetup was called.
Returns false if the current voxel was the last voxel in the grid intersected by the ray
*/
bool
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
    int step[3]{0, 0, 0};
    int out[3]{};
    int g[3]{0, 0, 0};
    RayHit *hit = nullptr;
    float t0;

    if ( !gridBoundsIntersect(ray, minimumDistance, *maximumDistance, &t0, &P) ) {
        return nullptr;
    }

    gridTraceSetup(ray, t0, &P, g, &tDelta, &tNext, step, out);

    // Ray counter in order to avoid testing objects spanning several voxel grid cells multiple times
    const int counter = randomRayId();

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

void
VoxelGrid::print() const {
    java::lang::System::out.printf("DX: %d, DY: %d, DZ: %d\n", xSize, ySize, zSize);

    for ( short z = 0; z < zSize; z++ ) {
        java::lang::System::out.printf("Z level %d of %d\n", z + 1, zSize);

        for ( short y = 0; y < ySize; y++ ) {
            java::lang::System::out.printf("  | ");
            for ( short x = 0; x < xSize; x++ ) {
                const java::ArrayList<VoxelData *> *list = volumeListsOfItems[cellIndexAddress(z, y, x)];
                if ( list == nullptr ) {
                    java::lang::System::out.printf("[  ]");
                } else {
                    java::lang::System::out.printf("(%2ld)", list->size());
                }
                java::lang::System::out.printf(" ");
            }
            java::lang::System::out.printf(" |\n");
        }
    }
}
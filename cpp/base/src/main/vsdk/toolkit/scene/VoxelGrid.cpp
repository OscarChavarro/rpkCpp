/**
Uniform voxel grid to optimize intersection operations

Ray-grid intersection: [SNYD1987] Snyder & Barr, SIGGRAPH '87, p123, with several
optimisations/enhancements from ray shade 4.0.6 by Graig Kolb, Stanford U
*/

#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/skin/Compound.h"
#include "vsdk/toolkit/environment/geometry/elements/PatchSet.h"
#include "vsdk/toolkit/skin/MinMaxBox.h"
#include "vsdk/toolkit/scene/VoxelGrid.h"

short
VoxelGrid::clampVoxel(const short v, const short max) {
    if (v < 0) {
        return 0;
    }
    if (v >= max) {
        return static_cast<short>(max - 1);
    }
    return v;
}

bool
VoxelGrid::shouldSubdivide(const RpkGeometry *geometry) {
    return geometry->itemCount >= MINIMUM_ELEMENT_COUNT_PER_CELL;
}

/**
Constructs a recursive grid structure containing the whole geometry
*/
VoxelGrid::VoxelGrid(RpkGeometry *geometry, const FaceBoundsTable *inFaceBounds):
    VoxelGrid(geometry, nullptr, 0, inFaceBounds)
{
}

VoxelGrid::VoxelGrid(RpkGeometry *geometry, VoxelGrid *inRoot, const int level, const FaceBoundsTable *inFaceBounds):
    subGrids(nullptr),
    items(nullptr),
    root(inRoot != nullptr ? inRoot : this),
    faceBounds(inFaceBounds),
    xSize(),
    ySize(),
    zSize(),
    voxelSize(),
    volumeListsOfItems(nullptr),
    gridItemPool(nullptr),
    boundingBox(),
    rayIntersectionBox(nullptr)
{
    if ( inRoot == nullptr ) {
        subGrids = new java::ArrayList<VoxelGrid *>();
        items = new java::ArrayList<VoxelData *>();
        countItems(geometry);
    }

    const double p = java::Math::pow(geometry->itemCount, 0.33333) + 1;
    const short gridSize = static_cast<short>(java::Math::floor(p));
    java::System::err.printf("Setting %d volumeListsOfItems in %d^3 cells level %d voxel grid ... \n", geometry->itemCount, gridSize, level);

    putGeometryInsideVoxelGrid(geometry, gridSize, gridSize, gridSize, level + 1);
}

VoxelGrid::~VoxelGrid() {
    if ( root == this ) {
        freeVoxelGridElements();
    }
    if ( rayIntersectionBox != nullptr ) {
        delete rayIntersectionBox;
        rayIntersectionBox = nullptr;
    }
    for ( int i = 0; volumeListsOfItems != nullptr && i < xSize * ySize * zSize; i++ ) {
        if ( volumeListsOfItems[i] != nullptr ) {
            delete volumeListsOfItems[i];
        }
    }
    delete[] volumeListsOfItems;
    delete gridItemPool;
}

int
VoxelGrid::nextItemIndex() const {
    return root->items->size();
}

int
VoxelGrid::addItem(VoxelData *cell) {
    root->items->add(cell);
    return cell->getIndex();
}

void
VoxelGrid::addSubGrid(VoxelGrid *voxelGrid) {
    root->subGrids->add(voxelGrid);
}

int
VoxelGrid::getNumberOfItems() const {
    return root->items->size();
}

/**
Releases the items and the sub grid contents of the hierarchy. Called on the
root grid only
*/
void
VoxelGrid::freeVoxelGridElements() {
    if ( items != nullptr ) {
        for ( int i = 0; i < items->size(); i++ ) {
            VoxelData *cell = items->get(i);
            if ( cell != nullptr ) {
                delete cell;
            }
        }
        delete items;
        items = nullptr;
    }

    if ( subGrids != nullptr ) {
        for ( int i = 0; i < subGrids->size(); i++ ) {
            VoxelGrid *subGrid = subGrids->get(i);
            delete subGrid->gridItemPool;
            subGrid->gridItemPool = nullptr;
            for ( int j = 0; j < subGrid->xSize * subGrid->ySize * subGrid->zSize; j++ ) {
                delete subGrid->volumeListsOfItems[j];
            }
            delete[] subGrid->volumeListsOfItems;
            subGrid->volumeListsOfItems = nullptr;
        }
        delete subGrids;
        subGrids = nullptr;
    }
}

/**
Number of patches below a geometry, stored in the itemCount of every geometry
of the hierarchy (RenderPark's GeomCountItems, called when the top level grid
is created): it sizes each grid and decides which geometries get a sub grid
*/
int
VoxelGrid::countItems(RpkGeometry *geometry) {
    int count = 0;
    if ( geometry->isCompound() ) {
        const java::ArrayList<RpkGeometry *> *children = static_cast<const Compound *>(geometry)->children;
        for ( int i = 0; children != nullptr && i < children->size(); i++ ) {
            count += countItems(children->get(i));
        }
    } else {
        const java::ArrayList<Patch *> *patches = RpkGeometry::patchListReference(geometry);
        count = patches != nullptr ? static_cast<int>(patches->size()) : 0;
    }
    geometry->itemCount = count;
    return count;
}

bool
VoxelGrid::isSmall(const RpkAxisAlignedBoundingBox *bb) const {
    return bb->dx() <= voxelSize.x &&
           bb->dy() <= voxelSize.y &&
           bb->dz() <= voxelSize.z;
}

void
VoxelGrid::putItemInsideVoxelGrid(VoxelData *item, const RpkAxisAlignedBoundingBox *itemBounds) const {

    RpkAxisAlignedBoundingBox boundaries;
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
VoxelGrid::putPatchInsideVoxelGrid(Patch *patch) {
    VoxelData *voxelData = new VoxelData(patch, VOXEL_DATA_PATCH_MASK, nextItemIndex());
    putItemInsideVoxelGrid(voxelData, &faceBounds->get(patch->getFaceIndex()));
    addItem(voxelData);
}

Vector3D
VoxelGrid::toVoxelClamped(const Vector3D &p) const {
    return Vector3D(
            clampVoxel(x2voxel(p.x), xSize),
            clampVoxel(y2voxel(p.y), ySize),
            clampVoxel(z2voxel(p.z), zSize)
    );
}

void
VoxelGrid::insertGeometryAsVoxelData(RpkGeometry *geometry) {
    VoxelData *voxelData = new VoxelData(geometry, VOXEL_DATA_GEOMETRY_MASK, nextItemIndex());
    putItemInsideVoxelGrid(voxelData, &geometry->boundingBox);
    addItem(voxelData);
}

void
VoxelGrid::insertSubGrid(RpkGeometry *geometry, const int level) {
    VoxelGrid *subGrid = new VoxelGrid(geometry, root, level, faceBounds);
    VoxelData *voxelData = new VoxelData(subGrid, VOXEL_DATA_GRID_MASK, nextItemIndex());

    putItemInsideVoxelGrid(voxelData, &subGrid->boundingBox);

    addSubGrid(subGrid);
    addItem(voxelData);
}

void
VoxelGrid::processCompoundGeometry(RpkGeometry *geometry, const int level) {
    const java::ArrayList<RpkGeometry *> *geometryList = static_cast<const Compound *>(geometry)->children;

    for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
        putSubGeometryInsideVoxelGrid(geometryList->get(i), level);
    }
}

void
VoxelGrid::processPatches(RpkGeometry *geometry) {
    const java::ArrayList<Patch *> *patches =
            RpkGeometry::patchListReference(geometry);

    for ( int i = 0; patches != nullptr && i < patches->size(); i++) {
        putPatchInsideVoxelGrid(patches->get(i));
    }
}

void
VoxelGrid::putSubGeometryInsideVoxelGrid(RpkGeometry *geometry, const int level) {
    if ( isSmall(&geometry->boundingBox) ) {
        if ( shouldSubdivide(geometry) ) {
            insertSubGrid(geometry, level);
        } else {
            insertGeometryAsVoxelData(geometry);
        }
        return;
    }

    if ( geometry->isCompound() ) {
        processCompoundGeometry(geometry, level);
        return;
    }
    processPatches(geometry);
}

void
VoxelGrid::putGeometryInsideVoxelGrid(RpkGeometry *geometry, const short na, const short nb, const short nc, const int level) {
    if ( na <= 0 || nb <= 0 || nc <= 0 ) {
        Logger::reportMessage("VoxelGrid", Logger::ERROR, "VoxelGrid::putGeometryInsideVoxelGrid", "Invalid grid dimensions");
        java::System::exit(1);
    }

    // Enlarge the getBoundingBox by a small amount
    boundingBox.copyFrom(&geometry->boundingBox);
    boundingBox.enlargeByFactor(DELTA_BOUND_FACTOR);
    if ( rayIntersectionBox == nullptr ) {
        rayIntersectionBox = new MinMaxBox(&boundingBox);
    } else {
        rayIntersectionBox->updateFromBoundingBox(&boundingBox);
    }
    xSize = na;
    ySize = nb;
    zSize = nc;

    voxelSize = boundingBox.voxelSize(na, nb, nc);

    volumeListsOfItems = new java::ArrayList<VoxelData *> *[na * nb * nc]();
    gridItemPool = nullptr;
    for ( int i = 0; i < na * nb * nc; i++ ) {
        volumeListsOfItems[i] = nullptr;
    }
    putSubGeometryInsideVoxelGrid(geometry, level);
}

/**
Compute t0, ray's minimal intersection with the whole grid and
position P of this intersection. Returns true if the grid getBoundingBox are
intersected and false if the ray passes along the voxel grid
*/
bool
VoxelGrid::gridBoundsIntersect(
    const RpkRay *ray,
    float minimumDistance,
    float maximumDistance,
    /*OUT*/ float *t0,
    Vector3D *position) const
{
    *t0 = minimumDistance;
    position->sumScaled(ray->position, *t0, ray->direction);
    if ( boundingBox.outOfBounds(position) ) {
        *t0 = maximumDistance;
        if ( !rayIntersectionBox->intersect(ray, minimumDistance, t0) ) {
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
    const RpkRay *ray,
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
RpkRayHit *
VoxelGrid::voxelIntersect(
    const java::ArrayList<VoxelData *> *items,
    RpkRay *ray,
    const unsigned int counter,
    const float minimumDistance,
    float *maximumDistance,
    const int hitFlags,
    RpkRayHit *hitStore,
    const RayExclusionSet &exclusions,
    VoxelGridWorkspace *workspace)
{
    RpkRayHit *hit = nullptr;

    for ( long i = 0; items != nullptr && i < items->size(); i++ ) {
        const VoxelData *item = items->get(i);
        const int itemIndex = item->getIndex();
        if ( workspace->lastRayId(itemIndex) != counter ) {
            // Avoid testing objects multiple times
            RpkRayHit *h = nullptr;
            if ( item->isPatch() ) {
                h = item->getPatch()->intersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
            } else if ( item->isGeom() ) {
                h = item->getGeometry()->discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
            } else if ( item->isGrid() ) {
                h = item->getVoxelGrid()->gridIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions, workspace);
            }
            if ( h ) {
                hit = h;
            }

            workspace->updateRayId(itemIndex, counter);
        }
    }

    return hit;
}

/**
Traces a ray through a voxel grid. Returns nearest intersection or nullptr
*/
RpkRayHit *
VoxelGrid::gridIntersect(
    RpkRay *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RpkRayHit *hitStore,
    const RayExclusionSet &exclusions,
    VoxelGridWorkspace *workspace) const
{
    Vector3D tNext;
    Vector3D tDelta;
    Vector3D P;
    int step[3]{0, 0, 0};
    int out[3]{};
    int g[3]{0, 0, 0};
    RpkRayHit *hit = nullptr;
    float t0;

    if ( !gridBoundsIntersect(ray, minimumDistance, *maximumDistance, &t0, &P) ) {
        return nullptr;
    }

    gridTraceSetup(ray, t0, &P, g, &tDelta, &tNext, step, out);

    // Ray counter in order to avoid testing objects spanning several voxel grid cells multiple times
    const unsigned int counter = workspace->nextRayId();

    do {
        const java::ArrayList<VoxelData *> *list = volumeListsOfItems[cellIndexAddress(g[0], g[1], g[2])];
        if ( list != nullptr ) {
            RpkRayHit *h = voxelIntersect(list, ray, counter, t0, maximumDistance, hitFlags, hitStore, exclusions, workspace);
            if ( h != nullptr ) {
                hit = h;
            }
        }
    } while ( nextVoxel(&t0, g, &tNext, &tDelta, step, out) && t0 <= *maximumDistance );

    return hit;
}

void
VoxelGrid::print() const {
    java::System::out.printf("DX: %d, DY: %d, DZ: %d\n", xSize, ySize, zSize);

    for ( short z = 0; z < zSize; z++ ) {
        java::System::out.printf("Z level %d of %d\n", z + 1, zSize);

        for ( short y = 0; y < ySize; y++ ) {
            java::System::out.printf("  | ");
            for ( short x = 0; x < xSize; x++ ) {
                const java::ArrayList<VoxelData *> *list = volumeListsOfItems[cellIndexAddress(z, y, x)];
                if ( list == nullptr ) {
                    java::System::out.printf("[  ]");
                } else {
                    java::System::out.printf("(%2ld)", list->size());
                }
                java::System::out.printf(" ");
            }
            java::System::out.printf(" |\n");
        }
    }
}

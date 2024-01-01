/* grid.c: Global_Raytracer_activeRaytracer acceleration using a uniform grid */

#include <cstdlib>

#include "common/error.h"
#include "scene/grid.h"
#include "scene/gridP.h"
#include "skin/hitlist.h"

#define NEWGRIDITEM(grid) (GRIDITEM *)malloc(sizeof(GRIDITEM))

/* *********************************************************************** */
/* voxel grid construction */

static int IsSmall(float *bounds, GRID *grid) {
    return ((bounds[MAX_X] - bounds[MIN_X]) <= grid->voxsize.x &&
            (bounds[MAX_Y] - bounds[MIN_Y]) <= grid->voxsize.y &&
            (bounds[MAX_Z] - bounds[MIN_Z]) <= grid->voxsize.z);
}

static GRIDITEM *NewGridItem(void *ptr, unsigned flags, GRID *grid) {
    GRIDITEM *item = NEWGRIDITEM(grid);
    item->ptr = ptr;
    item->flags = flags;
    return item;
}

static void EngridItem(GRIDITEM *item, float *itembounds, GRID *grid) {
    short mina, minb, minc, maxa, maxb, maxc, a, b, c;

    /* enlarge the getBoundingBox by a small amount in all directions */
    BOUNDINGBOX bounds;
    float xext = (grid->bounds[MAX_X] - grid->bounds[MIN_X]) * 1e-4,
            yext = (grid->bounds[MAX_Y] - grid->bounds[MIN_Y]) * 1e-4,
            zext = (grid->bounds[MAX_Z] - grid->bounds[MIN_Z]) * 1e-4;
    BoundsCopy(itembounds, bounds);
    bounds[MIN_X] -= xext;
    bounds[MAX_X] += xext;
    bounds[MIN_Y] -= yext;
    bounds[MAX_Y] += yext;
    bounds[MIN_Z] -= zext;
    bounds[MAX_Z] += zext;

    mina = x2voxel(bounds[MIN_X], grid);
    if ( mina >= grid->xsize ) {
        mina = grid->xsize - 1;
    }
    if ( mina < 0 ) {
        mina = 0;
    }
    maxa = x2voxel(bounds[MAX_X], grid);
    if ( maxa >= grid->xsize ) {
        maxa = grid->xsize - 1;
    }
    if ( maxa < 0 ) {
        maxa = 0;
    }

    minb = y2voxel(bounds[MIN_Y], grid);
    if ( minb >= grid->ysize ) {
        minb = grid->ysize - 1;
    }
    if ( minb < 0 ) {
        minb = 0;
    }
    maxb = y2voxel(bounds[MAX_Y], grid);
    if ( maxb >= grid->ysize ) {
        maxb = grid->ysize - 1;
    }
    if ( maxb < 0 ) {
        maxb = 0;
    }

    minc = z2voxel(bounds[MIN_Z], grid);
    if ( minc >= grid->zsize ) {
        minc = grid->zsize - 1;
    }
    if ( minc < 0 ) {
        minc = 0;
    }
    maxc = z2voxel(bounds[MAX_Z], grid);
    if ( maxc >= grid->zsize ) {
        maxc = grid->zsize - 1;
    }
    if ( maxc < 0 ) {
        maxc = 0;
    }

    for ( a = mina; a <= maxa; a++ ) {
        for ( b = minb; b <= maxb; b++ ) {
            for ( c = minc; c <= maxc; c++ ) {
                GRIDITEMLIST **items = &grid->items[CELLADDR(grid, a, b, c)];
                (*items) = GridItemListAdd(*items, item);
            }
        }
    }
}

static void EngridPatch(PATCH *patch, GRID *grid) {
    BOUNDINGBOX bounds;
    EngridItem(NewGridItem(patch, ISPATCH, grid), patch->bounds ? patch->bounds : PatchBounds(patch, bounds), grid);
}

static void EngridGeom(Geometry *geom, GRID *grid) {
    if ( IsSmall(geom->bounds, grid)) {
        if ( geom->tmp.i /*item count*/ < 10 ) {
            EngridItem(NewGridItem(geom, ISGEOM, grid), geom->bounds, grid);
        } else {
            GRID *subgrid = CreateGrid(geom);
            EngridItem(NewGridItem(subgrid, ISGRID, grid), subgrid->bounds, grid);
        }
    } else {
        if ( geomIsAggregate(geom)) {
            ForAllGeoms(child, geomPrimList(geom))
                        {
                            EngridGeom(child, grid);
                        }
            EndForAll;
        } else {
            ForAllPatches(patch, geomPatchList(geom))
                        {
                            EngridPatch(patch, grid);
                        }
            EndForAll;
        }
    }
}

static GRID *Engrid(Geometry *geom, short na, short nb, short nc, int istop) {
    GRID *grid;
    int i;
    float xext, yext, zext;

    if ( !geom ) {
        return (GRID *) nullptr;
    }

    if ( !geom->bounds ) {
        Error("Engrid", "Can't engrid an unbounded geom");
        return (GRID *) nullptr;
    }

    if ( na <= 0 || nb <= 0 || nc <= 0 ) {
        Error("Engrid", "Invalid grid dimensions");
        return (GRID *) nullptr;
    }

    grid = (GRID *)malloc(sizeof(GRID));

    /* enlarge the getBoundingBox by a small amount */
    xext = (geom->bounds[MAX_X] - geom->bounds[MIN_X]) * 1e-4;
    yext = (geom->bounds[MAX_Y] - geom->bounds[MIN_Y]) * 1e-4;
    zext = (geom->bounds[MAX_Z] - geom->bounds[MIN_Z]) * 1e-4;
    BoundsCopy(geom->bounds, grid->bounds);
    grid->bounds[MIN_X] -= xext;
    grid->bounds[MAX_X] += xext;
    grid->bounds[MIN_Y] -= yext;
    grid->bounds[MAX_Y] += yext;
    grid->bounds[MIN_Z] -= zext;
    grid->bounds[MAX_Z] += zext;

    grid->xsize = na;
    grid->ysize = nb;
    grid->zsize = nc;
    grid->voxsize.x = (grid->bounds[MAX_X] - grid->bounds[MIN_X]) / (float) na;
    grid->voxsize.y = (grid->bounds[MAX_Y] - grid->bounds[MIN_Y]) / (float) nb;
    grid->voxsize.z = (grid->bounds[MAX_Z] - grid->bounds[MIN_Z]) / (float) nc;
    grid->items = (GRIDITEMLIST **)malloc(na * nb * nc * sizeof(GRIDITEMLIST *));
    grid->gridItemPool = (void**) nullptr;
    for ( i = 0; i < na * nb * nc; i++ ) {
        grid->items[i] = GridItemListCreate();
    }
    EngridGeom(geom, grid);

    return grid;
}

static int GeomCountItems(Geometry *geom) {
    int count = 0;

    if ( geomIsAggregate(geom)) {
        GEOMLIST *primlist = geomPrimList(geom);
        ForAllGeoms(child, primlist)
                    {
                        count += GeomCountItems(child);
                    }
        EndForAll;
    } else {
        PATCHLIST *patches = geomPatchList(geom);
        count = PatchListCount(patches);
    }

    return geom->tmp.i = count;
}

/* Constructs a recursive grid structure containing the whole geom. */
GRID *CreateGrid(Geometry *geom) {
    static int level = 0;
    int gridsize;
    GRID *grid;

    if ( level == 0 ) {
        GeomCountItems(geom);
    }

    gridsize = pow((double) geom->tmp.i /*item count*/, 0.33333) + 1;
    fprintf(stderr, "Engridding %d items in %d^3 cells level %d voxel grid ... \n", geom->tmp.i, gridsize, level);
    level++;

    grid = Engrid(geom, gridsize, gridsize, gridsize, (level == 1) ? true : false);

    level--;

    return grid;
}

static void DestroyGridRecursive(GRID *grid) {
    int i;

    if ( !grid ) {
        return;
    }

    for ( i = 0; i < grid->xsize * grid->ysize * grid->zsize; i++ ) {
        ForAllGridItems(item, grid->items[i])
                    {
                        if ( IsGrid(item) && item->ptr ) {
                            /* destroy the subgrid */
                            DestroyGridRecursive((GRID *)item->ptr);
                            item->ptr = nullptr;
                        }
                    }
        EndForAll;
        GridItemListDestroy(grid->items[i]);
    }
    free((char *) grid);
}

void DestroyGrid(GRID *grid) {
    if ( !grid ) {
        return;
    }
    DestroyGridRecursive(grid);
    free((grid->gridItemPool));
}

/* ************************************************************************* */
/* ray-grid intersection: Snyder&Barr, SIGGRAPH'87, p123, with several
 * optimisations/enhancements from rayshade 4.0.6 by Graig Kolb, Stanford U. */

static int RandomRayId() {
    static int count = 0;
    count++;
    return (count & RAYCOUNT_MASK);
}

/* Compute t0, ray's minimal intersection with the whole grid and
 * position P of this intersection. Returns true if the grid getBoundingBox are
 * intersected and false if the ray passes along the voxel grid. */
static int GridBoundsIntersect(/*IN*/ GRID *grid, Ray *ray, float mindist, float maxdist,
        /*OUT*/ float *t0, Vector3D *P) {
    *t0 = mindist;
    VECTORSUMSCALED(ray->pos, *t0, ray->dir, *P);
    if ( OutOfBounds(P, grid->bounds)) {
        *t0 = maxdist;
        if ( !BoundsIntersect(ray, grid->bounds, mindist, t0)) {
            return false;
        }
        VECTORSUMSCALED(ray->pos, *t0, ray->dir, *P);
    } else {
    }

    return true;
}

/* initializes grid tracing */
static void GridTraceSetup(/*IN*/ GRID *grid, Ray *ray, float t0, Vector3D *P,
        /*OUT*/ int *g, Vector3D *tDelta, Vector3D *tNext, int *step, int *out) {
    /* Compute the grid cell g where this intersection occurs */
    g[X] = x2voxel(P->x, grid);
    if ( g[X] >= grid->xsize ) {
        g[X] = grid->xsize - 1;
    }
    g[Y] = y2voxel(P->y, grid);
    if ( g[Y] >= grid->ysize ) {
        g[Y] = grid->ysize - 1;
    }
    g[Z] = z2voxel(P->z, grid);
    if ( g[Z] >= grid->zsize ) {
        g[Z] = grid->zsize - 1;
    }

    /* Setup X: */
    /* tDelta->x is the distance increment along the ray to the adjacent
     * voxel in X direction.
     * tNext->x is the total distance from the ray origin to the next voxel
     * in X direction.
     * step[X] is either +1 or -1 accroding to the ray X direction.
     * out[X] is -1 or grid->xsize: the first x grid cell index outside the
     * grid. */
    if ( fabs(ray->dir.x) > EPSILON ) {
        if ( ray->dir.x > 0. ) {
            tDelta->x = grid->voxsize.x / ray->dir.x;
            tNext->x = t0 + (voxel2x(g[X] + 1, grid) - P->x) / ray->dir.x;
            step[X] = 1;
            out[X] = grid->xsize;
        } else {
            tDelta->x = grid->voxsize.x / -ray->dir.x;
            tNext->x = t0 + (voxel2x(g[X], grid) - P->x) / ray->dir.x;
            step[X] = out[X] = -1;
        }
    } else {
        tDelta->x = 0.;
        tNext->x = HUGE;
    }

    /* Setup Y: */
    if ( fabs(ray->dir.y) > EPSILON ) {
        if ( ray->dir.y > 0. ) {
            tDelta->y = grid->voxsize.y / ray->dir.y;
            tNext->y = t0 + (voxel2y(g[Y] + 1, grid) - P->y) / ray->dir.y;
            step[Y] = 1;
            out[Y] = grid->ysize;
        } else {
            tDelta->y = grid->voxsize.y / -ray->dir.y;
            tNext->y = t0 + (voxel2y(g[Y], grid) - P->y) / ray->dir.y;
            step[Y] = out[Y] = -1;
        }
    } else {
        tDelta->y = 0.;
        tNext->y = HUGE;
    }

    /* Setup Z: */
    if ( fabs(ray->dir.z) > EPSILON ) {
        if ( ray->dir.z > 0. ) {
            tDelta->z = grid->voxsize.z / ray->dir.z;
            tNext->z = t0 + (voxel2z(g[Z] + 1, grid) - P->z) / ray->dir.z;
            step[Z] = 1;
            out[Z] = grid->zsize;
        } else {
            tDelta->z = grid->voxsize.z / -ray->dir.z;
            tNext->z = t0 + (voxel2z(g[Z], grid) - P->z) / ray->dir.z;
            step[Z] = out[Z] = -1;
        }
    } else {
        tDelta->z = 0.;
        tNext->z = HUGE;
    }
}

/* Advances to the next grid cell. Assumes setup with GridTraceSetup(). 
 * returns false if the current voxel was the last voxel in the grid intersected 
 * by the ray */
static int NextVoxel(float *t0, int *g, Vector3D *tNext, Vector3D *tDelta, int *step, int *out) {
    int ingrid = true;

    if ( tNext->x <= tNext->y && tNext->x <= tNext->z ) {  /* tNext->x is smallest */
        g[X] += step[X];
        *t0 = tNext->x;
        tNext->x += tDelta->x;
        ingrid = g[X] - out[X];               /* false if g[X]==out[X] */
    } else if ( tNext->y <= tNext->z ) {             /* tNext->y is smallest */
        g[Y] += step[Y];
        *t0 = tNext->y;
        tNext->y += tDelta->y;
        ingrid = g[Y] - out[Y];
    } else {                         /* tNext->z is smallest */
        g[Z] += step[Z];
        *t0 = tNext->z;
        tNext->z += tDelta->z;
        ingrid = g[Z] - out[Z];
    }
    return ingrid;
}

/* finds the nearest intersection of the ray with an item (Geometry or PATCH) in
 * a voxel's item list. If there is an intersection, maxdist will contain
 * the distance to the intersection point measured from the ray origin
 * as usual. If there is no intersection, maxdist remains unmodified. */
static HITREC *VoxelIntersect(GRIDITEMLIST *items, Ray *ray, int counter,
                              float mindist, float *maxdist,
                              int hitflags,
                              HITREC *hitstore) {
    HITREC *hit = nullptr;

    ForAllGridItems(item, items)
                {
                    if ( LastRayId(item) != counter ) {
                        /* avoid testing objects multiple times */
                        HITREC *h = (HITREC *) nullptr;
                        if ( IsPatch(item)) {
                            h = PatchIntersect((PATCH *) item->ptr, ray, mindist, maxdist, hitflags, hitstore);
                        } else if ( IsGeom(item)) {
                            h = geomDiscretizationIntersect((Geometry *) item->ptr, ray, mindist, maxdist, hitflags,
                                                            hitstore);
                        } else if ( IsGrid(item)) {
                            h = GridIntersect((GRID *) item->ptr, ray, mindist, maxdist, hitflags, hitstore);
                        }
                        if ( h ) {
                            hit = h;
                        }

                        UpdateRayId(item, counter);
                    } else {
                    }
                }
    EndForAll;

    return hit;
}

/* traces a ray through a voxel grid. Returns nearest intersection or nullptr */
HITREC *GridIntersect(GRID *grid, Ray *ray, float mindist, float *maxdist, int hitflags, HITREC *hitstore) {
    Vector3D tNext, tDelta, P;
    int step[3], out[3], g[3];
    HITREC *hit = nullptr;
    float t0;
    int counter;

    if ( !grid || !GridBoundsIntersect(grid, ray, mindist, *maxdist, &t0, &P)) {
        return (HITREC *) nullptr;
    }

    GridTraceSetup(grid, ray, t0, &P, g, &tDelta, &tNext, step, out);


    /* ray counter in order to avoid testing objects spanning several
     * voxel grid cells multiple times. */
    counter = RandomRayId();

    do {
        GRIDITEMLIST *list = grid->items[CELLADDR(grid, g[X], g[Y], g[Z])];
        if ( list ) {
            HITREC *h;
            if ((h = VoxelIntersect(list, ray, counter, t0, maxdist, hitflags, hitstore))) {
                hit = h;
            }
        }
    } while ( NextVoxel(&t0, g, &tNext, &tDelta, step, out) && t0 <= *maxdist );

    return hit;
}

/* Prepends all hits in the voxel to the hitlist. The modified hitlist
 * is returned. */
static HITLIST *AllVoxelIntersections(HITLIST *hitlist,
                                      GRIDITEMLIST *items, Ray *ray, int counter,
                                      float mindist, float maxdist,
                                      int hitflags) {
    HITREC hitstore;
    ForAllGridItems(item, items)
                {
                    /* avoid testing objects multiple times */
                    if ( LastRayId(item) != counter ) {
                        if ( IsPatch(item)) {
                            float tmax = maxdist;
                            HITREC *h = PatchIntersect((PATCH *) item->ptr, ray, mindist, &tmax, hitflags, &hitstore);
                            if ( h ) {
                                hitlist = HitListAdd(hitlist, DuplicateHit(h));
                            }
                        } else if ( IsGeom(item)) {
                            hitlist = geomAllDiscretizationIntersections(hitlist, (Geometry *) item->ptr, ray, mindist,
                                                                         maxdist, hitflags);
                        } else if ( IsGrid(item)) {
                            hitlist = AllGridIntersections(hitlist, (GRID *) item->ptr, ray, mindist, maxdist,
                                                           hitflags);
                        }

                        UpdateRayId(item, counter);
                    }
                }
    EndForAll;

    return hitlist;
}

/* Traces a ray through a voxel grid. Adds all found intersections to the
 * passed 'hitlist' */
HITLIST *AllGridIntersections(HITLIST *hits, GRID *grid, Ray *ray, float mindist, float maxdist, int hitflags) {
    Vector3D tNext, tDelta, P;
    int step[3], out[3], g[3];
    float t0;
    int counter;

    if ( !grid || !GridBoundsIntersect(grid, ray, mindist, maxdist, &t0, &P)) {
        return hits;
    }
    GridTraceSetup(grid, ray, t0, &P, g, &tDelta, &tNext, step, out);

    /* ray counter in order to avoid testing objects spanning several
     * voxel grid cells multiple times. */
    counter = RandomRayId();

    do {
        GRIDITEMLIST *itemlist = grid->items[CELLADDR(grid, g[X], g[Y], g[Z])];
        if ( itemlist ) {
            hits = AllVoxelIntersections(hits, itemlist, ray, counter, t0, maxdist, hitflags);
        }
    } while ( NextVoxel(&t0, g, &tNext, &tDelta, step, out) && t0 <= maxdist );

    return hits;
}

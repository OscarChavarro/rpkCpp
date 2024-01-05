/*
Shaft culling ala Haines, E. A. and Wallace, J. R. "Shaft culling for
	     efficient ray-traced radiosity", 2nd Eurographics Workshop
	     on Rendering, Barcelona, Spain, may 1991
*/

#include "skin/patchlist_geom.h"
#include "skin/Geometry.h"
#include "skin/Patch.h"
#include "GALERKIN/shaftculling.h"

/* default strategy is "overlap open", which was
 * the most efficient strategy in the tests I did. */
static SHAFTCULLSTRATEGY strategy = OVERLAP_OPEN;

void ShaftOmit(SHAFT *shaft, Geometry *geom) {
    shaft->omit[shaft->nromit++] = geom;
}

void ShaftDontOpen(SHAFT *shaft, Geometry *geom) {
    shaft->dontopen[shaft->nrdontopen++] = geom;
}

/* constructs a shaft for two given bounding boxes */
SHAFT *ConstructShaft(float *ref1, float *ref2, SHAFT *shaft) {
    int i, j, hasminmax1[6], hasminmax2[6];
    SHAFTPLANE *plane;

    shaft->nromit = shaft->nrdontopen = 0;
    shaft->cut = false;

    shaft->ref1 = ref1;
    shaft->ref2 = ref2;

    /* midpoints of the reference boxes define a line that is guaranteed
     * to lay within the shaft. */
    shaft->center1.x = 0.5 * (ref1[MIN_X] + ref1[MAX_X]);
    shaft->center1.y = 0.5 * (ref1[MIN_Y] + ref1[MAX_Y]);
    shaft->center1.z = 0.5 * (ref1[MIN_Z] + ref1[MAX_Z]);

    shaft->center2.x = 0.5 * (ref2[MIN_X] + ref2[MAX_X]);
    shaft->center2.y = 0.5 * (ref2[MIN_Y] + ref2[MAX_Y]);
    shaft->center2.z = 0.5 * (ref2[MIN_Z] + ref2[MAX_Z]);

    for ( i = 0; i < 6; i++ ) {
        hasminmax1[i] = hasminmax2[i] = 0;
    }

    /* create extent box of both volumeListsOfItems and keep track which coordinates of which
     * box become the minimum or maximum */
    for ( i = MIN_X; i <= MIN_Z; i++ ) {
        if ( shaft->ref1[i] < shaft->ref2[i] ) {
            shaft->extent[i] = shaft->ref1[i];
            hasminmax1[i] = 1;
        } else {
            shaft->extent[i] = shaft->ref2[i];
            if ( !FLOATEQUAL(shaft->ref1[i], shaft->ref2[i], EPSILON)) {
                hasminmax2[i] = 1;
            }
        }
    }

    for ( i = MAX_X; i <= MAX_Z; i++ ) {
        if ( shaft->ref1[i] > shaft->ref2[i] ) {
            shaft->extent[i] = shaft->ref1[i];
            hasminmax1[i] = 1;
        } else {
            shaft->extent[i] = shaft->ref2[i];
            if ( !FLOATEQUAL(shaft->ref1[i], shaft->ref2[i], EPSILON)) {
                hasminmax2[i] = 1;
            }
        }
    }

    /* create plane set */
    plane = &shaft->plane[0];
    for ( i = 0; i < 6; i++ ) {
        if ( !hasminmax1[i] ) {
            continue;
        }
        for ( j = 0; j < 6; j++ ) {
            float u1, u2, v1, v2, du, dv;
            int a = (i % 3), b = (j % 3); /* directions */

            if ( !hasminmax2[j] ||
                 a == b ) { /*same direction*/ continue;
            }

            u1 = shaft->ref1[i];  /* coords. defining the plane */
            v1 = shaft->ref1[j];
            u2 = shaft->ref2[i];
            v2 = shaft->ref2[j];

            if ((i <= MIN_Z && j <= MIN_Z) || (i >= MAX_X && j >= MAX_X)) {
                du = v2 - v1;
                dv = u1 - u2;
            } else { /* normal must point outwards shaft */
                du = v1 - v2;
                dv = u2 - u1;
            }

            plane->n[a] = du;
            plane->n[b] = dv;
            plane->n[3 - a - b] = 0.;
            plane->d = -(du * u1 + dv * v1);

            plane->coord_offset[0] = plane->n[0] > 0. ? MIN_X : MAX_X;
            plane->coord_offset[1] = plane->n[1] > 0. ? MIN_Y : MAX_Y;
            plane->coord_offset[2] = plane->n[2] > 0. ? MIN_Z : MAX_Z;

            plane++;
        }
    }
    shaft->planes = plane - &shaft->plane[0];

    return shaft;
}

/* some constants describing the position of some item with respect to a plane
 * or a shaft. */
#define INSIDE -1
#define OVERLAP 0
#define OUTSIDE 1
#define COPLANAR 2

/* Tests a polygon w.r.t. the plane defined by the given normal and plane
 * constant. Returns INSIDE if the polygon is totally on the negative side of
 * the plane, OUTSIDE if the polygon on all on the positive side, OVERLAP
 * if the polygon is cut by the plane and COPLANAR if the polygon lays on the 
 * plane within tolerance distance d*EPSILON. */
static int TestPolygonWrtPlane(POLYGON *poly, Vector3Dd *normal, double d) {
    int i, out, in;    /* out = there are positions on the positive side of the plane */
    /* in  = there are positions on the negative side of the plane */

    out = in = false;
    for ( i = 0; i < poly->nrvertices; i++ ) {
        double e = VECTORDOTPRODUCT(*normal, poly->vertex[i]) + d,
                tolerance = fabs(d) * EPSILON + VECTORTOLERANCE(poly->vertex[i]);
        out |= (e > tolerance);
        in |= (e < -tolerance);
        if ( out && in ) {
            return OVERLAP;
        }
    }
    return (out ? OUTSIDE : (in ? INSIDE : COPLANAR));
}

/* Verifies whether the polygon is on the given side of the plane. Returns true if
 * so, and false if not. */
int VerifyPolygonWrtPlane(POLYGON *poly, Vector3Dd *normal, double d, int side) {
    int i, out, in;

    out = in = false;
    for ( i = 0; i < poly->nrvertices; i++ ) {
        double e = VECTORDOTPRODUCT(*normal, poly->vertex[i]) + d,
                tolerance = fabs(d) * EPSILON + VECTORTOLERANCE(poly->vertex[i]);
        out |= (e > tolerance);
        if ( out && (side == INSIDE || side == COPLANAR)) {
            return false;
        }
        in |= (e < -tolerance);
        if ( in && (side == OUTSIDE || side == COPLANAR || (out && side != OVERLAP))) {
            return false;
        }
    }

    if ( in ) {
        if ( out ) {
            if ( side == OVERLAP ) {
                return true;
            }
        } else {
            if ( side == INSIDE ) {
                return true;
            }
        }
    } else {
        if ( out ) {
            if ( side == OUTSIDE ) {
                return true;
            }
        } else {
            if ( side == COPLANAR ) {
                return true;
            }
        }
    }
    return false;
}

/* Tests the position of a point w.r.t. a plane. Returns OUTSIDE if the point is
 * on the positive side of the plane, INSIDE if on the negative side, and COPLANAR
 * if the point is on the plane within tolerance distance d*EPSILON. */
int TestPointWrtPlane(Vector3D *p, Vector3Dd *normal, double d) {
    double e, tolerance = fabs(d * EPSILON) + VECTORTOLERANCE(*p);
    e = VECTORDOTPRODUCT(*normal, *p) + d;
    if ( e < -tolerance ) {
        return INSIDE;
    }
    if ( e > +tolerance ) {
        return OUTSIDE;
    }
    return COPLANAR;
}

/* Compare to shaft planes. Returns 0 if they are the same and -1 or +1
 * if not (can be used for sorting the planes. It is assumed that the plane normals
 * are normalized! */
static int CompareShaftPlanes(SHAFTPLANE *p1, SHAFTPLANE *p2) {
    double tolerance;

    /* compare components of plane normal (normalized vector, so components
     * are in the range [-1,1]. */
    if ( p1->n[0] < p2->n[0] - EPSILON ) {
        return -1;
    } else if ( p1->n[0] > p2->n[0] + EPSILON ) {
        return +1;
    }

    if ( p1->n[1] < p2->n[1] - EPSILON ) {
        return -1;
    } else if ( p1->n[1] > p2->n[1] + EPSILON ) {
        return +1;
    }

    if ( p1->n[2] < p2->n[2] - EPSILON ) {
        return -1;
    } else if ( p1->n[2] > p2->n[2] + EPSILON ) {
        return +1;
    }

    /* compare plane constants. */
    tolerance = fabs(MAX(p1->d, p2->d) * EPSILON);
    if ( p1->d < p2->d - tolerance ) {
        return -1;
    } else if ( p1->d > p2->d + tolerance ) {
        return +1;
    }
    return 0;
}

/* Plane is a pointer to the shaft plane defined in shaft. This routine will return
 * true if the plane differs from all previous defined planes. */
static int UniqueShaftPlane(SHAFT *shaft, SHAFTPLANE *plane) {
    SHAFTPLANE *ref;

    for ( ref = &shaft->plane[0]; ref != plane; ref++ ) {
        if ( CompareShaftPlanes(ref, plane) == 0 ) {
            return false;
        }
    }
    return true;
}

/* fills in normal and plane constant, as will as the coord_offset arameters. */
static void FillInPlane(SHAFTPLANE *plane, double nx, double ny, double nz, double d) {
    plane->n[0] = nx;
    plane->n[1] = ny;
    plane->n[2] = nz;
    plane->d = d;

    plane->coord_offset[0] = plane->n[0] > 0. ? MIN_X : MAX_X;
    plane->coord_offset[1] = plane->n[1] > 0. ? MIN_Y : MAX_Y;
    plane->coord_offset[2] = plane->n[2] > 0. ? MIN_Z : MAX_Z;
}

/* Construct the planes determining the shaft that use edges of p1 and vertices 
 * of p2. */
static void ConstructPolygonToPolygonPlanes(POLYGON *p1, POLYGON *p2, SHAFT *shaft) {
    SHAFTPLANE *plane = &shaft->plane[shaft->planes];
    Vector3D *cur, *next, *other;
    Vector3Dd normal;
    double d, norm;
    int i, j, k, side, planes_found_for_edge, max_planes_per_edge;

    /* test p2 wrt plane of p1 */
    VECTORCOPY(p1->normal, normal);    /* convert to double precision */
    switch ( TestPolygonWrtPlane(p2, &normal, p1->plane_constant)) {
        case INSIDE:
            /* polygon p2 is on the negative side of the plane of p1. The plane of p1 is
             * a shaft plane and there will be at most one shaft plane per edge of p1. */
            FillInPlane(plane, p1->normal.x, p1->normal.y, p1->normal.z, p1->plane_constant);
            if ( UniqueShaftPlane(shaft, plane)) {
                plane++;
            }
            max_planes_per_edge = 1;
            break;
        case OUTSIDE:
            /* like above, except that p2 is on the positive side of the plane of p1, so
             * we have to invert normal and plane constant. */
            FillInPlane(plane, -p1->normal.x, -p1->normal.y, -p1->normal.z, -p1->plane_constant);
            if ( UniqueShaftPlane(shaft, plane)) {
                plane++;
            }
            max_planes_per_edge = 1;
            break;
        case OVERLAP:
            /* the plane of p1 cuts p2. It is not a shaft plane and there may be up to two
             * shaft planes for each edge of p1. */
            max_planes_per_edge = 2;
            break;
        default:    /* COPLANAR */
            /* degenerate case that should never happen. If it does, it will result
             * in a shaft with no planes and just a thin bounding box containing the coplanar
             * faces. */
            return;
    }

    for ( i = 0; i < p1->nrvertices; i++ ) {
        /* for each edge of p1 */
        cur = &p1->vertex[i];
        next = &p1->vertex[(i + 1) % p1->nrvertices];

        planes_found_for_edge = 0;
        for ( j = 0; j < p2->nrvertices && planes_found_for_edge < max_planes_per_edge; j++ ) {
            /* for each vertex of p2 */
            other = &p2->vertex[j];

            /* compute normal and plane constant of the plane formed by cur, next and
             * other */
            VECTORTRIPLECROSSPRODUCT(*cur, *next, *other, normal);
            norm = VECTORNORM(normal);
            if ( norm < EPSILON ) {
                continue;
            }    /* colinear vertices, try next vertex on p2 */
            VECTORSCALEINVERSE(norm, normal, normal);
            d = -VECTORDOTPRODUCT(normal, *cur);

            /* Test position of p1 w.r.t. the constructed plane. Skip the vertices
             * that were used to construct the plane. */
            k = (i + 2) % p1->nrvertices;
            side = TestPointWrtPlane(&p1->vertex[k], &normal, d);
            for ( k = (i + 3) % p1->nrvertices; k != i; k = (k + 1) % p1->nrvertices ) {
                int nside = TestPointWrtPlane(&p1->vertex[k], &normal, d);
                if ( side == COPLANAR ) {
                    side = nside;
                } else if ( nside != COPLANAR ) {
                    if ( side != nside ) {    /* side==INSIDE and nside==OUTSIDE or vice versa, */
                        side = OVERLAP;
                    }
                }    /* or side==OVERLAP already. May happen due to round */
            }                /* off error e.g. */
            if ( side != INSIDE && side != OUTSIDE ) {
                continue;
            }    /* not a valid candidate shaft plane */

            /* Verify whether p2 is on the same side of the constructed plane. If so,
             * the plane is a candidate shaft plane and will be added to the list if
             * it is unique. */
            if ( VerifyPolygonWrtPlane(p2, &normal, d, side)) {
                if ( side == INSIDE )    /* p1 and p2 are on the negative side as it should be */
                    FillInPlane(plane, normal.x, normal.y, normal.z, d);
                else
                    FillInPlane(plane, -normal.x, -normal.y, -normal.z, -d);
                if ( UniqueShaftPlane(shaft, plane))
                    plane++;
                planes_found_for_edge++;
            }
        }
    }

    shaft->planes = plane - &shaft->plane[0];
}

/* Constructs a shaft enclosing the two given polygons. */
SHAFT *ConstructPolygonToPolygonShaft(POLYGON *p1, POLYGON *p2, SHAFT *shaft) {
    int i;

    /* no "reference" bounding boxes to test with */
    shaft->ref1 = shaft->ref2 = (float *) nullptr;

    /* shaft extent = bounding box containing the bounding boxes of the
     * patches */
    BoundsCopy(p1->bounds, shaft->extent);
    BoundsEnlarge(shaft->extent, p2->bounds);

    /* nothing (yet) to omit */
    shaft->omit[0] = shaft->omit[1] = (Geometry *) nullptr;
    shaft->dontopen[0] = shaft->dontopen[1] = (Geometry *) nullptr;
    shaft->nromit = shaft->nrdontopen = 0;
    shaft->cut = false;

    /* center positions of polygons define a line that is guaranteed to lay
     * inside the shaft. */
    shaft->center1 = p1->vertex[0];
    for ( i = 1; i < p1->nrvertices; i++ ) {
        VECTORADD(shaft->center1, p1->vertex[i], shaft->center1);
    }
    VECTORSCALEINVERSE((float) p1->nrvertices, shaft->center1, shaft->center1);

    shaft->center2 = p2->vertex[0];
    for ( i = 1; i < p2->nrvertices; i++ ) {
        VECTORADD(shaft->center2, p2->vertex[i], shaft->center2);
    }
    VECTORSCALEINVERSE((float) p2->nrvertices, shaft->center2, shaft->center2);

    /* determine the shaft planes */
    shaft->planes = 0;
    ConstructPolygonToPolygonPlanes(p1, p2, shaft);
    ConstructPolygonToPolygonPlanes(p2, p1, shaft);

    return shaft;
}

/* Tests a bounding volume against the shaft: returns INSIDE if the bounding volume
 * is inside the shaft, OVERLAP if it overlaps, OUTSIDE if it is otside the shaft */
int ShaftBoxTest(float *bounds, SHAFT *shaft) {
    int i;
    SHAFTPLANE *plane;

    /* test against extent box */
    if ( DisjunctBounds(bounds, shaft->extent)) {
        return OUTSIDE;
    }

    /* test against plane set: if nearest corner of the bounding box is on or
     * outside any shaft plane, the object is outside the shaft */
    for ( i = 0, plane = &shaft->plane[0]; i < shaft->planes; i++, plane++ ) {
        if ( plane->n[0] * bounds[plane->coord_offset[0]] +
             plane->n[1] * bounds[plane->coord_offset[1]] +
             plane->n[2] * bounds[plane->coord_offset[2]] +
             plane->d > -fabs(plane->d * EPSILON)) {
            return OUTSIDE;
        }
    }

    /* test against reference volumeListsOfItems */
    if ((shaft->ref1 && !DisjunctBounds(bounds, shaft->ref1)) ||
        (shaft->ref2 && !DisjunctBounds(bounds, shaft->ref2))) {
        return OVERLAP;
    }

    /* if the bounding box survides all previous tests, it must overlap or be inside the
     * shaft. If the farest corner of the bounding box is outside any shaft-plane, it
     * overlaps the shaft, otherwise it is inside the shaft */
    for ( i = 0, plane = &shaft->plane[0]; i < shaft->planes; i++, plane++ ) {
        if ( plane->n[0] * bounds[(plane->coord_offset[0] + 3) % 6] +
             plane->n[1] * bounds[(plane->coord_offset[1] + 3) % 6] +
             plane->n[2] * bounds[(plane->coord_offset[2] + 3) % 6] +
             plane->d > fabs(plane->d * EPSILON)) {
            return OVERLAP;
        }
    }

    return INSIDE;
}

/* Tests the patch against the shaft: returns INSIDE, OVERLAP or OUTSIDE according
 * to whether the patch is fully inside the shaft, overlapping it, or fully outside. If it is detected that the patch fully occludes the shaft, shaft->cut is
 * set to true, indicating that there is full occlusion due to one patch and
 * that as such no further shaft culling is necessary. */
int ShaftPatchTest(PATCH *patch, SHAFT *shaft) {
    int i, j, someout, inall[PATCHMAXVERTICES];
    SHAFTPLANE *plane;
    double tmin[PATCHMAXVERTICES], tmax[PATCHMAXVERTICES],
            ptol[PATCHMAXVERTICES];
    Ray ray;
    float dist;
    HITREC hitstore;

    /* start by assuming that all vertices are on the negative side ("inside")
     * all shaft planes. */
    someout = false;
    for ( j = 0; j < patch->nrvertices; j++ ) {
        inall[j] = true;
        tmin[j] = 0.;  /* defines the segment of the edge that lays within the shaft */
        tmax[j] = 1.;
        ptol[j] = VECTORTOLERANCE(*patch->vertex[j]->point); /* vertex tolerance */
    }

    for ( i = 0, plane = &shaft->plane[0]; i < shaft->planes; i++, plane++ ) {
        /* test patch against i-th plane of the shaft */
        Vector3Dd plane_normal;
        double e[PATCHMAXVERTICES], tolerance;
        int in, out, side[PATCHMAXVERTICES];

        VECTORSET(plane_normal, plane->n[0], plane->n[1], plane->n[2]);

        in = out = false;
        for ( j = 0; j < patch->nrvertices; j++ ) {
            e[j] = VECTORDOTPRODUCT(plane_normal, *patch->vertex[j]->point) + plane->d;
            tolerance = fabs(plane->d) * EPSILON + ptol[j];
            side[j] = COPLANAR;
            if ( e[j] > tolerance ) {
                side[j] = OUTSIDE;
                out = true;
            } else if ( e[j] < -tolerance ) {
                side[j] = INSIDE;
                in = true;
            }
            if ( side[j] != INSIDE ) {
                inall[j] = false;
            }
        }

        if ( !in ) {        /* patch contains no vertices on the inside of the plane */
            return OUTSIDE;
        }

        if ( out ) {        /* patch contains at least one vertex inside and one */
            someout = true;    /* outside the plane. A point is inside the shaft
			 * if it is inside *all* planes, but it is outside
			 * as soon it is outside *one* plane. */

            for ( j = 0; j < patch->nrvertices; j++ ) {
                /* reduce segment of edge that can lay within the shaft. */
                int k = (j + 1) % patch->nrvertices;
                if ( side[j] != side[k] ) {
                    if ( side[k] == OUTSIDE ) {    /* decrease tmax[j] */
                        if ( side[j] == INSIDE ) {    /* find intersection */
                            if ( tmax[j] > tmin[j] ) {
                                double t = e[j] / (e[j] - e[k]);
                                if ( t < tmax[j] ) {
                                    tmax[j] = t;
                                }
                            }
                        } else /* if (side[j] == COPLANAR) */ { /* whole edge lays outside */
                            tmax[j] = -EPSILON;
                        }
                    } else if ( side[j] == OUTSIDE ) { /* increase tmin[j] */
                        if ( side[k] == INSIDE ) {    /* find intersection */
                            if ( tmin[j] < tmax[j] ) {
                                double t = e[j] / (e[j] - e[k]);
                                if ( t > tmin[j] ) {
                                    tmin[j] = t;
                                }
                            }
                        } else /* if (side[k] == COPLANAR) */ { /* whole edge lays outside */
                            tmin[j] = 1. + EPSILON;
                        }
                    }
                } else if ( side[j] == OUTSIDE ) {    /* whole edge lays outside */
                    tmax[j] = -EPSILON;
                }
            }
        }
    }

    /* The remaining tests only work if the shaft planes alone determine the
     * shaft. */
    if ( shaft->ref1 || shaft->ref2 ) {
        return OVERLAP;
    }

    if ( !someout ) {        /* no patch vertices are outside the shaft */
        return INSIDE;
    }

    for ( j = 0; j < patch->nrvertices; j++ ) {
        if ( inall[j] ) {    /* at least one patch vertex is really inside the shaft and */
            return OVERLAP;
        }
    }    /* at least one vertex is really outside => overlap. */

    /* All vertices are outside or on the shaft. Check whether there are edges
     * intersecting the shaft. */
    for ( j = 0; j < patch->nrvertices; j++ ) {
        if ( tmin[j] + EPSILON < tmax[j] - EPSILON ) {
            return OVERLAP;
        }
    }

    /* All vertices and edges of the patch are outside the shaft. Either the
     * patch is totally outside the shaft, either, the patch cuts the shaft.
     * If the line segment connecting the midpoints of the polygons defining
     * the shaft intersects the patch, the patch cuts the shaft. If not,
     * the patch lays fully outside. */
    ray.pos = shaft->center1;
    VECTORSUBTRACT(shaft->center2, shaft->center1, ray.dir);
    dist = 1. - EPSILON;
    if ( PatchIntersect(patch, &ray, EPSILON, &dist, HIT_FRONT | HIT_BACK, &hitstore)) {
        shaft->cut = true;
        return OVERLAP;
    }

    return OUTSIDE;
}

/* returns true if the geomerty is not to be enclosed in the shaft */
static int Omit(SHAFT *shaft, Geometry *geom) {
    int i;

    for ( i = 0; i < shaft->nromit; i++ ) {
        if ( shaft->omit[i] == geom ) {
            return true;
        }
    }
    return false;
}

/* returns true if the geomerty is not to be opened during shaft culling */
static int DontOpen(SHAFT *shaft, Geometry *geom) {
    int i;

    for ( i = 0; i < shaft->nrdontopen; i++ ) {
        if ( shaft->dontopen[i] == geom ) {
            return true;
        }
    }
    return false;
}

/* Given a PatchSet pl and a shaft. This routine will check every patch in pl to
 * see if it is inside, outside or overlapping the shaft. Inside or overlapping patches
 * are added to culledpatchlist. A pointer to the possibly enlonged culledpatchlist 
 * is returned. */
PatchSet *ShaftCullPatchlist(PatchSet *pl, SHAFT *shaft, PatchSet *culledpatchlist) {
    int total, kept, bbside; /* can be used for ratio-open strategies */

    for ( kept = total = 0; pl && !shaft->cut; pl = pl->next, total++ ) {
        if ( pl->patch->omit || Omit(shaft, (Geometry *) pl->patch)) {
            continue;
        }

        if ( !pl->patch->bounds ) { /* compute getBoundingBox */
            BOUNDINGBOX bounds;
            PatchBounds(pl->patch, bounds);
        }
        if ((bbside = ShaftBoxTest(pl->patch->bounds, shaft)) != OUTSIDE ) {
            /* patch bounding box is inside the shaft, or overlaps with it. If it
             * overlaps, do a more expensive, but definitive, test to see whether
             * the patch itself is inside, outside or overlapping the shaft. */
            if ( bbside == INSIDE || ShaftPatchTest(pl->patch, shaft) != OUTSIDE ) {
                culledpatchlist = PatchListAdd(culledpatchlist, pl->patch);
                kept++;
            }
        }
    }
    return culledpatchlist;
}

/* Adds the geom to the candlist, possibly duplicating it if the geom 
 * was created during previous shaftculling. */
static GeometryListNode *Keep(Geometry *geom, GeometryListNode *candlist) {
    if ( geom->omit ) {
        return candlist;
    }

    if ( geom->shaftCullGeometry ) {
        Geometry *newgeom = geomDuplicate(geom);
        newgeom->shaftCullGeometry = true;
        candlist = GeomListAdd(candlist, newgeom);
    } else {
        candlist = GeomListAdd(candlist, geom);
    }
    return candlist;
}

/* Breaks the geom into it's components and does shaft culling on
 * the components. */
static GeometryListNode *Open(Geometry *geom, SHAFT *shaft, GeometryListNode *candlist) {
    if ( geom->omit ) {
        return candlist;
    }

    if ( geomIsAggregate(geom)) {
        candlist = DoShaftCulling(geomPrimList(geom), shaft, candlist);
    } else {
        PatchSet *patchlist;
        patchlist = geomPatchList(geom);
        patchlist = ShaftCullPatchlist(patchlist, shaft, nullptr);
        if ( patchlist ) {
            Geometry *newgeom;
            newgeom = geomCreatePatchSet(patchlist, PatchListMethods());
            newgeom->shaftCullGeometry = true;
            candlist = GeomListAdd(candlist, newgeom);
        }
    }
    return candlist;
}

/* Tests the geom w.r.t. the shaft: if the geom is inside or overlaps
 * the shaft, it is copied to the shaft or broken open depending on
 * the current shaft culling strategy. */
GeometryListNode *ShaftCullGeom(Geometry *geom, SHAFT *shaft, GeometryListNode *candlist) {
    if ( geom->omit || Omit(shaft, geom)) {
        return candlist;
    }

    /* unbounded geoms always overlap the shaft */
    switch ( geom->bounded ? ShaftBoxTest(geom->bounds, shaft) : OVERLAP ) {
        case INSIDE:
            if ( strategy == ALWAYS_OPEN && !DontOpen(shaft, geom)) {
                candlist = Open(geom, shaft, candlist);
            } else {
                candlist = Keep(geom, candlist);
            }
            break;
        case OVERLAP:
            if ( strategy == KEEP_CLOSED || DontOpen(shaft, geom)) {
                candlist = Keep(geom, candlist);
            } else {
                candlist = Open(geom, shaft, candlist);
            }
            break;
        default:
            break;
    }

    return candlist;
}

/* adds all objects from world that overlap or lay inside the shaft to
 * candlist, returns the new candidate list */

/* During shaftculling getPatchList "geoms" are created - they (and only they)
 * need to be destroyed when destroying a geom candidate list created by 
 * DoShaftCulling - for other kinds of geoms, only a pointer is copied */
GeometryListNode *DoShaftCulling(GeometryListNode *world, SHAFT *shaft, GeometryListNode *candlist) {
    for ( ; world && !shaft->cut; world = world->next ) {
        candlist = ShaftCullGeom(world->geom, shaft, candlist);
    }

    return candlist;
}

void FreeCandidateList(GeometryListNode *candlist) {
    GeometryListNode *gl;

    /* only destroy geoms that were generated for shaft culling. */
    for ( gl = candlist; gl; gl = gl->next ) {
        if ( gl->geom->shaftCullGeometry ) {
            geomDestroy(gl->geom);
        }
    }

    GeomListDestroy(candlist);
}

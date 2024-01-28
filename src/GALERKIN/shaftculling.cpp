/**
Shaft culling ala Haines, E. A. and Wallace, J. R. "Shaft culling for
efficient ray-traced radiosity", 2nd Eurographics Workshop on Rendering, Barcelona, Spain, may 1991
*/

#include "java/util/ArrayList.txx"
#include "GALERKIN/shaftculling.h"

/**
Some constants describing the position of some item with respect to a plane or a shaft
*/
#define INSIDE (-1)
#define OVERLAP 0
#define OUTSIDE 1
#define COPLANAR 2

/**
Default strategy is "overlap open", which was
the most efficient strategy in the tests I did
*/
static SHAFTCULLSTRATEGY strategy = OVERLAP_OPEN;

/**
Marks a geometry as to be omitted during shaft culling: it will not be added to the
candidate list, even if the geometry overlaps or is inside the shaft
*/
void
setShaftOmit(SHAFT *shaft, Patch *geom) {
    shaft->omit[shaft->nromit++] = geom;
}

/**
Marks a geometry as one not to be opened during shaft culling
*/
void
setShaftDontOpen(SHAFT *shaft, Geometry *geom) {
    shaft->dontOpen[shaft->nrdontopen++] = geom;
}

/**
Constructs a shaft for two given bounding boxes. Supply a pointer to a SHAFT
structure. This structure will be filled in and the pointer returned if successful.
nullptr is returned if something goes wrong
*/
SHAFT *
constructShaft(float *ref1, float *ref2, SHAFT *shaft) {
    int i;
    int j;
    int hasMinMax1[6];
    int hasMinMax2[6];
    SHAFTPLANE *plane;

    shaft->nromit = shaft->nrdontopen = 0;
    shaft->cut = false;

    shaft->ref1 = ref1;
    shaft->ref2 = ref2;

    // Midpoints of the reference boxes define a line that is guaranteed
    // to lay within the shaft
    shaft->center1.x = 0.5f * (ref1[MIN_X] + ref1[MAX_X]);
    shaft->center1.y = 0.5f * (ref1[MIN_Y] + ref1[MAX_Y]);
    shaft->center1.z = 0.5f * (ref1[MIN_Z] + ref1[MAX_Z]);

    shaft->center2.x = 0.5f * (ref2[MIN_X] + ref2[MAX_X]);
    shaft->center2.y = 0.5f * (ref2[MIN_Y] + ref2[MAX_Y]);
    shaft->center2.z = 0.5f * (ref2[MIN_Z] + ref2[MAX_Z]);

    for ( i = 0; i < 6; i++ ) {
        hasMinMax1[i] = hasMinMax2[i] = 0;
    }

    // Create extent box of both volumeListsOfItems and keep track which coordinates of which
    // box become the minimum or maximum
    for ( i = MIN_X; i <= MIN_Z; i++ ) {
        if ( shaft->ref1[i] < shaft->ref2[i] ) {
            shaft->extent[i] = shaft->ref1[i];
            hasMinMax1[i] = 1;
        } else {
            shaft->extent[i] = shaft->ref2[i];
            if ( !FLOATEQUAL(shaft->ref1[i], shaft->ref2[i], EPSILON)) {
                hasMinMax2[i] = 1;
            }
        }
    }

    for ( i = MAX_X; i <= MAX_Z; i++ ) {
        if ( shaft->ref1[i] > shaft->ref2[i] ) {
            shaft->extent[i] = shaft->ref1[i];
            hasMinMax1[i] = 1;
        } else {
            shaft->extent[i] = shaft->ref2[i];
            if ( !FLOATEQUAL(shaft->ref1[i], shaft->ref2[i], EPSILON)) {
                hasMinMax2[i] = 1;
            }
        }
    }

    // Create plane set
    plane = &shaft->plane[0];
    for ( i = 0; i < 6; i++ ) {
        if ( !hasMinMax1[i] ) {
            continue;
        }
        for ( j = 0; j < 6; j++ ) {
            float u1, u2, v1, v2, du, dv;
            int a = (i % 3), b = (j % 3); // Directions

            if ( !hasMinMax2[j] || a == b ) {
                // Same direction
                continue;
            }

            u1 = shaft->ref1[i]; // Coords. defining the plane
            v1 = shaft->ref1[j];
            u2 = shaft->ref2[i];
            v2 = shaft->ref2[j];

            if ((i <= MIN_Z && j <= MIN_Z) || (i >= MAX_X && j >= MAX_X)) {
                du = v2 - v1;
                dv = u1 - u2;
            } else {
                // Normal must point outwards shaft
                du = v1 - v2;
                dv = u2 - u1;
            }

            plane->n[a] = du;
            plane->n[b] = dv;
            plane->n[3 - a - b] = 0.0;
            plane->d = -(du * u1 + dv * v1);

            plane->coord_offset[0] = plane->n[0] > 0. ? MIN_X : MAX_X;
            plane->coord_offset[1] = plane->n[1] > 0. ? MIN_Y : MAX_Y;
            plane->coord_offset[2] = plane->n[2] > 0. ? MIN_Z : MAX_Z;

            plane++;
        }
    }
    shaft->planes = (int)(plane - &shaft->plane[0]);

    return shaft;
}

/**
Tests a polygon w.r.t. the plane defined by the given normal and plane
constant. Returns INSIDE if the polygon is totally on the negative side of
the plane, OUTSIDE if the polygon on all on the positive side, OVERLAP
if the polygon is cut by the plane and COPLANAR if the polygon lays on the
plane within tolerance distance d*EPSILON
*/
static int
testPolygonWrtPlane(POLYGON *poly, Vector3D *normal, double d) {
    int i;
    int out; // out = there are positions on the positive side of the plane
    int in; // in  = there are positions on the negative side of the plane

    out = in = false;
    for ( i = 0; i < poly->nrvertices; i++ ) {
        double e = VECTORDOTPRODUCT(
                *normal,
                poly->vertex[i]) + d,
                tolerance = std::fabs(d) * EPSILON + VECTORTOLERANCE(poly->vertex[i]);
        out |= (e > tolerance);
        in |= (e < -tolerance);
        if ( out && in ) {
            return OVERLAP;
        }
    }
    return out ? OUTSIDE : (in ? INSIDE : COPLANAR);
}

/**
Verifies whether the polygon is on the given side of the plane. Returns true if
so, and false if not
*/
static bool
verifyPolygonWrtPlane(POLYGON *poly, Vector3D *normal, double d, int side) {
    bool out = false;
    bool in = false;

    for ( int i = 0; i < poly->nrvertices; i++ ) {
        double e = VECTORDOTPRODUCT(*normal, poly->vertex[i]) + d,
                tolerance = fabs(d) * EPSILON + VECTORTOLERANCE(poly->vertex[i]);
        out |= e > tolerance;
        if ( out && (side == INSIDE || side == COPLANAR) ) {
            return false;
        }
        in |= e < -tolerance;
        if ( in && (side == OUTSIDE || side == COPLANAR || (out && side != OVERLAP)) ) {
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

/**
Tests the position of a point w.r.t. a plane. Returns OUTSIDE if the point is
on the positive side of the plane, INSIDE if on the negative side, and COPLANAR
if the point is on the plane within tolerance distance d*EPSILON
*/
int
testPointWrtPlane(Vector3D *p, Vector3D *normal, double d) {
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

/**
Compare to shaft planes. Returns 0 if they are the same and -1 or +1
if not (can be used for sorting the planes. It is assumed that the plane normals
are normalized!
*/
static int
compareShaftPlanes(SHAFTPLANE *p1, SHAFTPLANE *p2) {
    double tolerance;

    // Compare components of plane normal (normalized vector, so components
    // are in the range [-1,1]
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

    // Compare plane constants
    tolerance = fabs(MAX(p1->d, p2->d) * EPSILON);
    if ( p1->d < p2->d - tolerance ) {
        return -1;
    } else if ( p1->d > p2->d + tolerance ) {
        return +1;
    }
    return 0;
}

/**
Plane is a pointer to the shaft plane defined in shaft. This routine will return
true if the plane differs from all previous defined planes
*/
static int
uniqueShaftPlane(SHAFT *shaft, SHAFTPLANE *plane) {
    SHAFTPLANE *ref;

    for ( ref = &shaft->plane[0]; ref != plane; ref++ ) {
        if ( compareShaftPlanes(ref, plane) == 0 ) {
            return false;
        }
    }
    return true;
}

/**
Fills in normal and plane constant, as will as the coord_offset parameters
*/
static void
fillInPlane(SHAFTPLANE *plane, float nx, float ny, float nz, float d) {
    plane->n[0] = nx;
    plane->n[1] = ny;
    plane->n[2] = nz;
    plane->d = d;

    plane->coord_offset[0] = plane->n[0] > 0. ? MIN_X : MAX_X;
    plane->coord_offset[1] = plane->n[1] > 0. ? MIN_Y : MAX_Y;
    plane->coord_offset[2] = plane->n[2] > 0. ? MIN_Z : MAX_Z;
}

/**
Construct the planes determining the shaft that use edges of p1 and vertices of p2
*/
static void
constructPolygonToPolygonPlanes(POLYGON *p1, POLYGON *p2, SHAFT *shaft) {
    SHAFTPLANE *plane = &shaft->plane[shaft->planes];
    Vector3D *cur, *next, *other;
    Vector3D normal;
    float d;
    float norm;
    int i;
    int j;
    int k;
    int side;
    int planesFoundForEdge;
    int maxPlanesPerEdge;

    // Test p2 wrt plane of p1
    VECTORCOPY(p1->normal, normal); // Convert to double precision
    switch ( testPolygonWrtPlane(p2, &normal, p1->plane_constant) ) {
        case INSIDE:
            // Polygon p2 is on the negative side of the plane of p1. The plane of p1 is
            // a shaft plane and there will be at most one shaft plane per edge of p1
            fillInPlane(plane, p1->normal.x, p1->normal.y, p1->normal.z, p1->plane_constant);
            if ( uniqueShaftPlane(shaft, plane) ) {
                plane++;
            }
            maxPlanesPerEdge = 1;
            break;
        case OUTSIDE:
            // Like above, except that p2 is on the positive side of the plane of p1, so
            // we have to invert normal and plane constant
            fillInPlane(plane, -p1->normal.x, -p1->normal.y, -p1->normal.z, -p1->plane_constant);
            if ( uniqueShaftPlane(shaft, plane) ) {
                plane++;
            }
            maxPlanesPerEdge = 1;
            break;
        case OVERLAP:
            // The plane of p1 cuts p2. It is not a shaft plane and there may be up to two
            // shaft planes for each edge of p1
            maxPlanesPerEdge = 2;
            break;
        default:
            // COPLANAR
            // Degenerate case that should never happen. If it does, it will result
            // in a shaft with no planes and just a thin bounding box containing the coplanar
            // faces
            return;
    }

    for ( i = 0; i < p1->nrvertices; i++ ) {
        // For each edge of p1
        cur = &p1->vertex[i];
        next = &p1->vertex[(i + 1) % p1->nrvertices];

        planesFoundForEdge = 0;
        for ( j = 0; j < p2->nrvertices && planesFoundForEdge < maxPlanesPerEdge; j++ ) {
            // For each vertex of p2
            other = &p2->vertex[j];

            // Compute normal and plane constant of the plane formed by cur, next and other
            VECTORTRIPLECROSSPRODUCT(*cur, *next, *other, normal);
            norm = VECTORNORM(normal);
            if ( norm < EPSILON ) {
                continue;
            }
            // Co-linear vertices, try next vertex on p2
            VECTORSCALEINVERSE(norm, normal, normal);
            d = -VECTORDOTPRODUCT(normal, *cur);

            // Test position of p1 w.r.t. the constructed plane. Skip the vertices
            // that were used to construct the plane
            k = (i + 2) % p1->nrvertices;
            side = testPointWrtPlane(&p1->vertex[k], &normal, d);
            for ( k = (i + 3) % p1->nrvertices; k != i; k = (k + 1) % p1->nrvertices ) {
                int nside = testPointWrtPlane(&p1->vertex[k], &normal, d);
                if ( side == COPLANAR ) {
                    side = nside;
                } else if ( nside != COPLANAR ) {
                    if ( side != nside ) {
                        // side==INSIDE and nside==OUTSIDE or vice versa
                        side = OVERLAP;
                    }
                }
                // Or side==OVERLAP already. May happen due to round
            } // Off error e.g.
            if ( side != INSIDE && side != OUTSIDE ) {
                continue;
            } // Not a valid candidate shaft plane

            // Verify whether p2 is on the same side of the constructed plane. If so,
            // the plane is a candidate shaft plane and will be added to the list if
            // it is unique
            if ( verifyPolygonWrtPlane(p2, &normal, d, side) ) {
                if ( side == INSIDE ) {
                    // p1 and p2 are on the negative side as it should be
                    fillInPlane(plane, normal.x, normal.y, normal.z, d);
                } else {
                    fillInPlane(plane, -normal.x, -normal.y, -normal.z, -d);
                }
                if ( uniqueShaftPlane(shaft, plane) ) {
                    plane++;
                }
                planesFoundForEdge++;
            }
        }
    }

    shaft->planes = (int)(plane - &shaft->plane[0]);
}

/**
Constructs a shaft enclosing the two given polygons
*/
SHAFT *
constructPolygonToPolygonShaft(POLYGON *p1, POLYGON *p2, SHAFT *shaft) {
    int i;

    // No "reference" bounding boxes to test with
    shaft->ref1 = shaft->ref2 = nullptr;

    // Shaft extent = bounding box containing the bounding boxes of the
    // patches
    boundsCopy(p1->bounds, shaft->extent);
    boundsEnlarge(shaft->extent, p2->bounds);

    // Nothing (yet) to omit
    shaft->omit[0] = nullptr;
    shaft->omit[1] = nullptr;
    shaft->dontOpen[0] = nullptr;
    shaft->dontOpen[1] = nullptr;
    shaft->nromit = 0;
    shaft->nrdontopen = 0;
    shaft->cut = false;

    // Center positions of polygons define a line that is guaranteed to lay inside the shaft
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

    // Determine the shaft planes
    shaft->planes = 0;
    constructPolygonToPolygonPlanes(p1, p2, shaft);
    constructPolygonToPolygonPlanes(p2, p1, shaft);

    return shaft;
}

/**
Tests a bounding volume against the shaft: returns INSIDE if the bounding volume
is inside the shaft, OVERLAP if it overlaps, OUTSIDE if it is outside the shaft
*/
int
shaftBoxTest(float *bounds, SHAFT *shaft) {
    int i;
    SHAFTPLANE *plane;

    // Test against extent box
    if ( disjunctBounds(bounds, shaft->extent)) {
        return OUTSIDE;
    }

    // Test against plane set: if nearest corner of the bounding box is on or
    // outside any shaft plane, the object is outside the shaft
    for ( i = 0, plane = &shaft->plane[0]; i < shaft->planes; i++, plane++ ) {
        if ( plane->n[0] * bounds[plane->coord_offset[0]] +
             plane->n[1] * bounds[plane->coord_offset[1]] +
             plane->n[2] * bounds[plane->coord_offset[2]] +
             plane->d > -fabs(plane->d * EPSILON)) {
            return OUTSIDE;
        }
    }

    // Test against reference volumeListsOfItems
    if ( (shaft->ref1 && !disjunctBounds(bounds, shaft->ref1)) ||
        (shaft->ref2 && !disjunctBounds(bounds, shaft->ref2)) ) {
        return OVERLAP;
    }

    // If the bounding box survives all previous tests, it must overlap or be inside the
    // shaft. If the farest corner of the bounding box is outside any shaft-plane, it
    // overlaps the shaft, otherwise it is inside the shaft */
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

/**
Tests the patch against the shaft: returns INSIDE, OVERLAP or OUTSIDE according
to whether the patch is fully inside the shaft, overlapping it, or fully outside. If it is detected that the patch fully occludes the shaft, shaft->cut is
set to true, indicating that there is full occlusion due to one patch and
that as such no further shaft culling is necessary
*/
int
shaftPatchTest(Patch *patch, SHAFT *shaft) {
    int i;
    int j;
    int someOut;
    int inAll[MAXIMUM_VERTICES_PER_PATCH];
    SHAFTPLANE *plane;
    double tMin[MAXIMUM_VERTICES_PER_PATCH];
    double tMax[MAXIMUM_VERTICES_PER_PATCH];
    double ptol[MAXIMUM_VERTICES_PER_PATCH];
    Ray ray;
    float dist;
    RayHit hitStore;

    // Start by assuming that all vertices are on the negative side ("inside") all shaft planes
    someOut = false;
    for ( j = 0; j < patch->numberOfVertices; j++ ) {
        inAll[j] = true;
        tMin[j] = 0.0;  // Defines the segment of the edge that lays within the shaft
        tMax[j] = 1.0;
        ptol[j] = VECTORTOLERANCE(*patch->vertex[j]->point); // Vertex tolerance
    }

    for ( i = 0, plane = &shaft->plane[0]; i < shaft->planes; i++, plane++ ) {
        // Test patch against i-th plane of the shaft
        Vector3Dd plane_normal;
        double e[MAXIMUM_VERTICES_PER_PATCH], tolerance;
        int in, out, side[MAXIMUM_VERTICES_PER_PATCH];

        VECTORSET(plane_normal, plane->n[0], plane->n[1], plane->n[2]);

        in = out = false;
        for ( j = 0; j < patch->numberOfVertices; j++ ) {
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
                inAll[j] = false;
            }
        }

        if ( !in ) {
            // Patch contains no vertices on the inside of the plane
            return OUTSIDE;
        }

        if ( out ) {
            // Patch contains at least one vertex inside and one
            someOut = true; // Outside the plane. A point is inside the shaft
			 // if it is inside *all* planes, but it is outside
			 // as soon it is outside *one* plane

            for ( j = 0; j < patch->numberOfVertices; j++ ) {
                // Reduce segment of edge that can lay within the shaft
                int k = (j + 1) % patch->numberOfVertices;
                if ( side[j] != side[k] ) {
                    if ( side[k] == OUTSIDE ) {
                        // Decrease tMax[j]
                        if ( side[j] == INSIDE ) {
                            // Find intersection
                            if ( tMax[j] > tMin[j] ) {
                                double t = e[j] / (e[j] - e[k]);
                                if ( t < tMax[j] ) {
                                    tMax[j] = t;
                                }
                            }
                        } else /* if (side[j] == COPLANAR) */ {
                            // Whole edge lays outside
                            tMax[j] = -EPSILON;
                        }
                    } else if ( side[j] == OUTSIDE ) {
                        // increase tMin[j]
                        if ( side[k] == INSIDE ) {
                            // Find intersection
                            if ( tMin[j] < tMax[j] ) {
                                double t = e[j] / (e[j] - e[k]);
                                if ( t > tMin[j] ) {
                                    tMin[j] = t;
                                }
                            }
                        } else /* if (side[k] == COPLANAR) */ {
                            // Whole edge lays outside
                            tMin[j] = 1. + EPSILON;
                        }
                    }
                } else if ( side[j] == OUTSIDE ) {
                    // Whole edge lays outside
                    tMax[j] = -EPSILON;
                }
            }
        }
    }

    // The remaining tests only work if the shaft planes alone determine the shaft
    if ( shaft->ref1 || shaft->ref2 ) {
        return OVERLAP;
    }

    if ( !someOut ) {
        // No patch vertices are outside the shaft
        return INSIDE;
    }

    for ( j = 0; j < patch->numberOfVertices; j++ ) {
        if ( inAll[j] ) {
            // At least one patch vertex is really inside the shaft and
            return OVERLAP;
        }
    }
    // At least one vertex is really outside => overlap

    // All vertices are outside or on the shaft. Check whether there are edges
    // intersecting the shaft
    for ( j = 0; j < patch->numberOfVertices; j++ ) {
        if ( tMin[j] + EPSILON < tMax[j] - EPSILON ) {
            return OVERLAP;
        }
    }

    // All vertices and edges of the patch are outside the shaft. Either the
    // patch is totally outside the shaft, either, the patch cuts the shaft.
    // If the line segment connecting the midpoints of the polygons defining
    // the shaft intersects the patch, the patch cuts the shaft. If not,
    // the patch lays fully outside
    ray.pos = shaft->center1;
    VECTORSUBTRACT(shaft->center2, shaft->center1, ray.dir);
    dist = 1.0 - EPSILON;
    if ( patchIntersect(patch, &ray, EPSILON, &dist, HIT_FRONT | HIT_BACK, &hitStore)) {
        shaft->cut = true;
        return OVERLAP;
    }

    return OUTSIDE;
}

/**
Returns true if the geometry is not to be enclosed in the shaft
*/
static int
patchIsOnOmitSet(SHAFT *shaft, Patch *geometry) {
    for ( int i = 0; i < shaft->nromit; i++ ) {
        if ( shaft->omit[i] == geometry ) {
            return true;
        }
    }
    return false;
}

/**
Returns true if the geometry is not to be opened during shaft culling
*/
static int
dontOpen(SHAFT *shaft, Geometry *geom) {
    for ( int i = 0; i < shaft->nrdontopen; i++ ) {
        if ( shaft->dontOpen[i] == geom ) {
            return true;
        }
    }
    return false;
}

/**
Given a PatchSet pl and a shaft. This routine will check every patch in pl to
see if it is inside, outside or overlapping the shaft. Inside or overlapping patches
are added to culledPatchList. A pointer to the possibly enlonged culledPatchList
is returned
*/
PatchSet *
shaftCullPatchList(PatchSet *patchList, SHAFT *shaft, PatchSet *culledPatchList) {
    int total; // Can be used for ratio-open strategies
    int boundingBoxSide;

    for ( total = 0; patchList && !shaft->cut; patchList = patchList->next, total++ ) {
        if ( patchList->patch->omit || patchIsOnOmitSet(shaft, patchList->patch)) {
            continue;
        }

        if ( patchList->patch->boundingBox == nullptr ) {
            // Compute getBoundingBox
            BOUNDINGBOX bounds;
            patchBounds(patchList->patch, bounds);
        }
        boundingBoxSide = shaftBoxTest(patchList->patch->boundingBox, shaft);
        if ( boundingBoxSide != OUTSIDE ) {
            // Patch bounding box is inside the shaft, or overlaps with it. If it
            // overlaps, do a more expensive, but definitive, test to see whether
            // the patch itself is inside, outside or overlapping the shaft
            if ( boundingBoxSide == INSIDE || shaftPatchTest(patchList->patch, shaft) != OUTSIDE ) {
                if ( patchList->patch != nullptr ) {
                    PatchSet *newListNode = (PatchSet *) malloc(sizeof(PatchSet));
                    newListNode->patch = patchList->patch;
                    newListNode->next = culledPatchList;
                    culledPatchList = newListNode;
                }
            }
        }
    }
    return culledPatchList;
}

/**
Adds the geom to the candidateList, possibly duplicating it if the geom
was created during previous shaft culling
*/
static GeometryListNode *
keep(Geometry *geom, GeometryListNode *candidateList) {
    if ( geom->omit ) {
        return candidateList;
    }

    if ( geom->shaftCullGeometry ) {
        Geometry *newGeometry = geomDuplicate(geom);
        newGeometry->shaftCullGeometry = true;
        candidateList = geometryListAdd(candidateList, newGeometry);
    } else {
        candidateList = geometryListAdd(candidateList, geom);
    }
    return candidateList;
}

/**
Breaks the geom into it's components and does shaft culling on
the components
*/
static GeometryListNode *
shaftCullOpen(Geometry *geom, SHAFT *shaft, GeometryListNode *candidateList) {
    if ( geom->omit ) {
        return candidateList;
    }

    if ( geomIsAggregate(geom) ) {
        candidateList = doShaftCulling(geomPrimList(geom), shaft, candidateList);
    } else {
        PatchSet *patchList = geomPatchList(geom);
        patchList = shaftCullPatchList(patchList, shaft, nullptr);
        if ( patchList != nullptr ) {
            Geometry *newGeometry;
            newGeometry = geomCreatePatchSet(patchList, &GLOBAL_skin_patchListGeometryMethods);
            newGeometry->shaftCullGeometry = true;
            candidateList = geometryListAdd(candidateList, newGeometry);
        }
    }
    return candidateList;
}

/**
Tests the geom w.r.t. the shaft: if the geom is inside or overlaps
the shaft, it is copied to the shaft or broken open depending on
the current shaft culling strategy
*/
GeometryListNode *
shaftCullGeom(Geometry *geometry, SHAFT *shaft, GeometryListNode *candidateList) {
    if ( geometry->className == GeometryClassId::PATCH_SET && (geometry->omit || patchIsOnOmitSet(shaft, (Patch *)geometry)) ) {
        return candidateList;
    }

    // Unbounded geoms always overlap the shaft
    switch ( geometry->bounded ? shaftBoxTest(geometry->bounds, shaft) : OVERLAP ) {
        case INSIDE:
            if ( strategy == ALWAYS_OPEN && !dontOpen(shaft, geometry) ) {
                candidateList = shaftCullOpen(geometry, shaft, candidateList);
            } else {
                candidateList = keep(geometry, candidateList);
            }
            break;
        case OVERLAP:
            if ( strategy == KEEP_CLOSED || dontOpen(shaft, geometry) ) {
                candidateList = keep(geometry, candidateList);
            } else {
                candidateList = shaftCullOpen(geometry, shaft, candidateList);
            }
            break;
        default:
            break;
    }

    return candidateList;
}

/**
Adds all objects from world that overlap or lay inside the shaft to
candidateList, returns the new candidate list

During shaft culling getPatchList "geoms" are created - they (and only they)
need to be destroyed when destroying a geom candidate list created by
doShaftCulling - for other kinds of geoms, only a pointer is copied
*/
GeometryListNode *
doShaftCulling(GeometryListNode *world, SHAFT *shaft, GeometryListNode *candidateList) {
    for ( ; world && !shaft->cut; world = world->next ) {
        candidateList = shaftCullGeom(world->geom, shaft, candidateList);
    }

    return candidateList;
}

/**
Frees the memory occupied by a candidate list produced by DoShaftCulling
*/
void
freeCandidateList(GeometryListNode *candidateList) {
    GeometryListNode *gl;

    // Only destroy geoms that were generated for shaft culling
    for ( gl = candidateList; gl; gl = gl->next ) {
        if ( gl->geom->shaftCullGeometry ) {
            geomDestroy(gl->geom);
        }
    }

    GeomListDestroy(candidateList);
}

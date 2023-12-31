#include <cstdlib>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "shared/render.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"

static ELEMENT *monteCarloRadiosityCreateClusterHierarchyRecursive(Geometry *world);

/**
Orientation and position of regular sub-elements is fully determined by the
following transformations. A uniform mapping of parameter domain to the
elements is supposed (i.o.w. use patchUniformPoint() to map (u,v) coordinates
on the toplevel element to a 3D point on the patch). The sub-elements
have equal area. No explicit Jacobian stuff needed to compute integrals etc..
etc.

Do not change the conventions below without knowing VERY well what
you are doing.
*/

/** Up-transforms for regular quadrilateral sub-elements:
 *
 *   (v)
 *
 *    1 +---------+---------+
 *      |         |         |
 *      |         |         |
 *      | 2       | 3       |
 *  0.5 +---------+---------+
 *      |         |         |
 *      |         |         |
 *      | 0       | 1       |
 *    0 +---------+---------+
 *      0        0.5        1   (u)
 */
Matrix2x2 GLOBAL_stochasticRaytracing_quadupxfm[4] = {
    // South-west [0,0.5] x [0,0.5]
    {{{0.5, 0.0}, {0.0, 0.5}}, {0.0, 0.0}},

    // South-east: [0.5,1] x [0,0.5]
    {{{0.5, 0.0}, {0.0, 0.5}}, {0.5, 0.0}},

    // North-west: [0,0.5] x [0.5,1]
    {{{0.5, 0.0}, {0.0, 0.5}}, {0.0, 0.5}},

    // North-east: [0.5,1] x [0.5,1]
    {{{0.5, 0.0}, {0.0, 0.5}}, {0.5, 0.5}}
};

/**
Up-transforms for triangular sub-elements:

 (v)

  1 +
    | \
    |   \
    |     \
    | 2     \
0.5 +---------+
    | \     3 | \
    |   \     |   \
    |     \   |     \
    | 0     \ | 1     \
  0 +---------+---------+
    0        0.5        1  (u)
*/
Matrix2x2 GLOBAL_stochasticRaytracing_triupxfm[4] = {
    // Left: (0,0),(0.5,0),(0,0.5)
    {{{0.5,  0.0}, {0.0, 0.5}},  {0.0, 0.0}},

    // Right: (0.5,0),(1,0),(0.5,0.5)
    {{{0.5,  0.0}, {0.0, 0.5}},  {0.5, 0.0}},

    // Top: (0,0.5),(0.5,0.5),(0,1)
    {{{0.5,  0.0}, {0.0, 0.5}},  {0.0, 0.5}},

    // Middle: (0.5,0.5),(0,0.5),(0.5,0)
    {{{-0.5, 0.0}, {0.0, -0.5}}, {0.5, 0.5}}
};

static ELEMENT *
createElement() {
    static long id = 1;
    ELEMENT *elem = (ELEMENT *)malloc(sizeof(ELEMENT));

    elem->pog.patch = (Patch *) nullptr;
    elem->id = id;
    id++;
    elem->area = 0.;
    initCoefficients(elem);    /* allocation of the coefficients is left
				 * until just before the first iteration
				 * in monteCarloRadiosityReInit() */

    colorClear(elem->Ed);
    colorClear(elem->Rd);

    elem->ray_index = 0;
    elem->quality = 0;
    elem->ng = 0.;

    elem->imp = elem->unshot_imp = elem->source_imp = 0.;
    elem->imp_ray_index = 0;

    VECTORSET(elem->midpoint, 0., 0., 0.);
    elem->vertex[0] = elem->vertex[1] = elem->vertex[2] = elem->vertex[3] = (Vertex *) nullptr;
    elem->parent = (ELEMENT *) nullptr;
    elem->regular_subelements = (ELEMENT **) nullptr;
    elem->irregular_subelements = (ELEMENTLIST *) nullptr;
    elem->uptrans = (Matrix2x2 *) nullptr;
    elem->child_nr = -1;
    elem->nrvertices = 0;
    elem->iscluster = false;
    elem->flags = 0;

    GLOBAL_stochasticRaytracing_hierarchy.nr_elements++;

    return elem;
}

static void
vertexAttachElement(Vertex *v, ELEMENT *elem) {
    v->radiance_data = ElementListAdd(v->radiance_data, elem);
}

ELEMENT *
monteCarloRadiosityCreateToplevelSurfaceElement(Patch *patch) {
    int i;
    ELEMENT *elem = createElement();
    elem->pog.patch = patch;
    elem->iscluster = false;
    elem->area = patch->area;
    elem->midpoint = patch->midpoint;
    elem->nrvertices = patch->numberOfVertices;
    for ( i = 0; i < elem->nrvertices; i++ ) {
        elem->vertex[i] = patch->vertex[i];
        vertexAttachElement(elem->vertex[i], elem);
    }

    allocCoefficients(elem);    /* may need reallocation before the start
				 * of the computations. */
    stochasticRadiosityClearCoefficients(elem->rad, elem->basis);
    stochasticRadiosityClearCoefficients(elem->unshot_rad, elem->basis);
    stochasticRadiosityClearCoefficients(elem->received_rad, elem->basis);

    elem->Ed = patchAverageEmittance(patch, DIFFUSE_COMPONENT);
    colorScaleInverse(M_PI, elem->Ed, elem->Ed);
    elem->Rd = patchAverageNormalAlbedo(patch, BRDF_DIFFUSE_COMPONENT);

    return elem;
}

static ELEMENT *
monteCarloRadiosityCreateCluster(Geometry *geom) {
    ELEMENT *elem = createElement();
    float *bounds = geom->bounds;

    elem->pog.geom = geom;
    elem->iscluster = true;

    colorSetMonochrome(elem->Rd, 1.);
    colorClear(elem->Ed);

    /* elem->area will be computed from the subelements in the cluster later */
    VECTORSET(elem->midpoint,
              (bounds[MIN_X] + bounds[MAX_X]) / 2.,
              (bounds[MIN_Y] + bounds[MAX_Y]) / 2.,
              (bounds[MIN_Z] + bounds[MAX_Z]) / 2.);

    allocCoefficients(elem);    /* always constant approx. so no need to
				 * delay allocating the coefficients. */
    stochasticRadiosityClearCoefficients(elem->rad, elem->basis);
    stochasticRadiosityClearCoefficients(elem->unshot_rad, elem->basis);
    stochasticRadiosityClearCoefficients(elem->received_rad, elem->basis);
    elem->imp = elem->unshot_imp = elem->received_imp = 0.;

    GLOBAL_stochasticRaytracing_hierarchy.nr_clusters++;

    return elem;
}

static void
monteCarloRadiosityCreateSurfaceElementChild(Patch *patch, ELEMENT *parent) {
    ELEMENT *elem;
    elem = (ELEMENT *)patch->radiance_data;    /* created before */
    elem->parent = (ELEMENT *)parent;
    parent->irregular_subelements = ElementListAdd(parent->irregular_subelements, elem);
}

static void
monteCarloRadiosityCreateClusterChild(Geometry *geom, ELEMENT *parent) {
    ELEMENT *subCluster = monteCarloRadiosityCreateClusterHierarchyRecursive(geom);
    subCluster->parent = parent;
    parent->irregular_subelements = ElementListAdd(parent->irregular_subelements, subCluster);
}

/**
Initialises parent cluster radiance/importance/area for child voxelData
*/
static void
monteCarloRadiosityInitClusterPull(ELEMENT *parent, ELEMENT *child) {
    parent->area += child->area;
    PullRadiance(parent, child, parent->rad, child->rad);
    PullRadiance(parent, child, parent->unshot_rad, child->unshot_rad);
    PullRadiance(parent, child, parent->received_rad, child->received_rad);
    PullImportance(parent, child, &parent->imp, &child->imp);
    PullImportance(parent, child, &parent->unshot_imp, &child->unshot_imp);
    PullImportance(parent, child, &parent->received_imp, &child->received_imp);

    /* needs division by parent->area once it is known after monteCarloRadiosityInitClusterPull for
     * all children elements. */
    colorAddScaled(parent->Ed, child->area, child->Ed, parent->Ed);
}

static void
monteCarloRadiosityCreateClusterChildren(ELEMENT *parent) {
    Geometry *geom = parent->pog.geom;

    if ( geomIsAggregate(geom)) {
        GeometryListNode *geometryList = geomPrimList(geom);
        if ( geometryList != nullptr ) {
            GeometryListNode *window;
            for ( window = geometryList; window; window = window->next ) {
                Geometry *geometry = window->geom;
                monteCarloRadiosityCreateClusterChild(geometry, parent);
            }
        }
    } else {
        for ( PatchSet *window = geomPatchList(geom); window != nullptr; window = window->next ) {
            monteCarloRadiosityCreateSurfaceElementChild(window->patch, parent);
        }
    }

    ForAllIrregularSubelements(child, parent)
                {
                    monteCarloRadiosityInitClusterPull(parent, child);
                }
    EndForAll;
    colorScaleInverse(parent->area, parent->Ed, parent->Ed);
}

static ELEMENT *
monteCarloRadiosityCreateClusterHierarchyRecursive(Geometry *world) {
    ELEMENT *topcluster;
    world->radiance_data = topcluster = (ELEMENT *) monteCarloRadiosityCreateCluster(world);
    monteCarloRadiosityCreateClusterChildren(topcluster);
    return topcluster;
}

ELEMENT *
monteCarloRadiosityCreateClusterHierarchy(Geometry *world) {
    if ( !world ) {
        return (ELEMENT *) nullptr;
    } else {
        return monteCarloRadiosityCreateClusterHierarchyRecursive(world);
    }
}

/**
Determine the (u,v) coordinate range of the element w.r.t. the patch to
which it belongs when using regular quadtree subdivision in
order to efficiently generate samples with NextNiedInRange()
in niederreiter.c. NextNiedInRange() creates a sample on a quadrilateral
subdomain, called a "dyadic box" in QMC literature. All samples in
such a dyadic box have the same most significant bits. This routine
basically computes what these most significant bits are. The computation
is based on the regular quadtree subdivision of a quadrilateral, as
shown above. If a triangular element is to be sampled, the quadrilateral
sample needs to be "folded" into the triangle. FoldSample() in sample4d.c
does this
*/
void
monteCarloRadiosityElementRange(ELEMENT *elem, int *nbits, niedindex *msb1, niedindex *rmsb2) {
    int nb;
    niedindex b1, b2;

    nb = b1 = b2 = 0;
    while ( elem->child_nr >= 0 ) {
        nb++;
        b1 = (b1 << 1) | (niedindex) (elem->child_nr & 1);
        b2 = (b2 >> 1) | ((niedindex) (elem->child_nr & 2) << (NBITS - 2));
        elem = elem->parent;
    }

    *nbits = nb;
    *msb1 = b1;
    *rmsb2 = b2;
}

/**
Determines the regular sub-element at point (u,v) of the given parent
element. Returns the parent element itself if there are no regular sub-elements.
The point is transformed to the corresponding point on the sub-element
*/
ELEMENT *
monteCarloRadiosityRegularSubElementAtPoint(ELEMENT *parent, double *u, double *v) {
    ELEMENT *child = (ELEMENT *) nullptr;
    double _u = *u, _v = *v;

    if ( parent->iscluster || !parent->regular_subelements ) {
        return (ELEMENT *) nullptr;
    }

    /* Have a look at the drawings above to understand what is done exactly. */
    switch ( parent->nrvertices ) {
        case 3:
            if ( _u + _v <= 0.5 ) {
                child = parent->regular_subelements[0];
                *u = _u * 2.;
                *v = _v * 2.;
            } else if ( _u > 0.5 ) {
                child = parent->regular_subelements[1];
                *u = (_u - 0.5) * 2.;
                *v = _v * 2.;
            } else if ( _v > 0.5 ) {
                child = parent->regular_subelements[2];
                *u = _u * 2.;
                *v = (_v - 0.5) * 2.;
            } else {
                child = parent->regular_subelements[3];
                *u = (0.5 - _u) * 2.;
                *v = (0.5 - _v) * 2.;
            }
            break;
        case 4:
            if ( _v <= 0.5 ) {
                if ( _u < 0.5 ) {
                    child = parent->regular_subelements[0];
                    *u = _u * 2.;
                } else {
                    child = parent->regular_subelements[1];
                    *u = (_u - 0.5) * 2.;
                }
                *v = _v * 2.;
            } else {
                if ( _u < 0.5 ) {
                    child = parent->regular_subelements[2];
                    *u = _u * 2.;
                } else {
                    child = parent->regular_subelements[3];
                    *u = (_u - 0.5) * 2.;
                }
                *v = (_v - 0.5) * 2.;
            }
            break;
        default:
            logFatal(-1, "galerkinRegularSubelementAtPoint", "Can handle only triangular or quadrilateral elements");
    }

    return child;
}

/**
Returns the leaf regular sub-element of 'top' at the point (u,v) (uniform
coordinates!). (u,v) is transformed to the coordinates of the corresponding
point on the leaf element. 'top' is a surface element, not a cluster
*/
ELEMENT *
monteCarloRadiosityRegularLeafElementAtPoint(ELEMENT *top, double *u, double *v) {
    ELEMENT *leaf;

    /* find leaf element of 'top' at (u,v) */
    leaf = top;
    while ( leaf->regular_subelements ) {
        leaf = monteCarloRadiosityRegularSubElementAtPoint(leaf, u, v);
    }

    return leaf;
}

static Vector3D *
monteCarloRadiosityInstallCoordinate(Vector3D *coord) {
    Vector3D *v = VectorCreate(coord->x, coord->y, coord->z);
    GLOBAL_stochasticRaytracing_hierarchy.coords = VectorListAdd(GLOBAL_stochasticRaytracing_hierarchy.coords, v);
    return v;
}

static Vector3D *
monteCarloRadiosityInstallNormal(Vector3D *norm) {
    Vector3D *v = VectorCreate(norm->x, norm->y, norm->z);
    GLOBAL_stochasticRaytracing_hierarchy.normals = VectorListAdd(GLOBAL_stochasticRaytracing_hierarchy.normals, v);
    return v;
}

static Vector3D *
monteCarloRadiosityInstallTexCoord(Vector3D *texCoord) {
    Vector3D *t = VectorCreate(texCoord->x, texCoord->y, texCoord->z);
    GLOBAL_stochasticRaytracing_hierarchy.texCoords = VectorListAdd(GLOBAL_stochasticRaytracing_hierarchy.texCoords, t);
    return t;
}

static Vertex *
monteCarloRadiosityInstallVertex(Vector3D *coord, Vector3D *norm, Vector3D *texCoord) {
    java::ArrayList<Patch *> *newPatchList = new java::ArrayList<Patch *>();
    Vertex *v = vertexCreate(coord, norm, texCoord, newPatchList);
    GLOBAL_stochasticRaytracing_hierarchy.vertices->add(0, v);
    return v;
}

static Vertex *
monteCarloRadiosityNewMidpointVertex(ELEMENT *elem, Vertex *v1, Vertex *v2) {
    Vector3D coord, norm, texCoord, *p, *n, *t;

    MIDPOINT(*(v1->point), *(v2->point), coord);
    p = monteCarloRadiosityInstallCoordinate(&coord);

    if ( v1->normal && v2->normal ) {
        MIDPOINT(*(v1->normal), *(v2->normal), norm);
        n = monteCarloRadiosityInstallNormal(&norm);
    } else {
        n = &elem->pog.patch->normal;
    }

    if ( v1->texCoord && v2->texCoord ) {
        MIDPOINT(*(v1->texCoord), *(v2->texCoord), texCoord);
        t = monteCarloRadiosityInstallTexCoord(&texCoord);
    } else {
        t = nullptr;
    }

    return monteCarloRadiosityInstallVertex(p, n, t);
}

/**
Find the neighbouring element of the given element at the edgenr-th edge.
The edgenr-th edge is the edge connecting the edgenr-th vertex to the
(edgenr+1 modulo GLOBAL_statistics_numberOfVertices)-th vertex
*/
static ELEMENT *
monteCarloRadiosityElementNeighbour(ELEMENT *elem, int edgenr) {
    Vertex *from = elem->vertex[edgenr],
            *to = elem->vertex[(edgenr + 1) % elem->nrvertices];

    ForAllElements(e, to->radiance_data)
                {
                    if ( e != elem &&
                         ((e->nrvertices == 3 &&
                           ((e->vertex[0] == to && e->vertex[1] == from) ||
                            (e->vertex[1] == to && e->vertex[2] == from) ||
                            (e->vertex[2] == to && e->vertex[0] == from)))
                          || (e->nrvertices == 4 &&
                              ((e->vertex[0] == to && e->vertex[1] == from) ||
                               (e->vertex[1] == to && e->vertex[2] == from) ||
                               (e->vertex[2] == to && e->vertex[3] == from) ||
                               (e->vertex[3] == to && e->vertex[0] == from))))) {
                        return e;
                    }
                }
    EndForAll;

    return (ELEMENT *) nullptr;
}

Vertex *
monteCarloRadiosityEdgeMidpointVertex(ELEMENT *elem, int edgenr) {
    Vertex *v = (Vertex *) nullptr,
            *to = elem->vertex[(edgenr + 1) % elem->nrvertices];
    ELEMENT *neighbour = monteCarloRadiosityElementNeighbour(elem, edgenr);

    if ( neighbour && neighbour->regular_subelements ) {
        /* elem has a neighbour at the edge from 'from' to 'to'. This neighbouring
         * element has been subdivided before, so the edge midpoint vertex already
         * exists: it is the midpoint of the neighbour's edge leading from 'to' to
         * 'from'. This midpoint is a vertex of a regular subelement of 'neighbour'.
         * Which regular subelement and which vertex is determined from the diagrams
         * above. */
        int index = (to == neighbour->vertex[0] ? 0 :
                     (to == neighbour->vertex[1] ? 1 :
                      (to == neighbour->vertex[2] ? 2 :
                       (to == neighbour->vertex[3] ? 3 : -1))));

        switch ( neighbour->nrvertices ) {
            case 3:
                switch ( index ) {
                    case 0:
                        v = neighbour->regular_subelements[0]->vertex[1];
                        break;
                    case 1:
                        v = neighbour->regular_subelements[1]->vertex[2];
                        break;

                    case 2:
                        v = neighbour->regular_subelements[2]->vertex[0];
                        break;
                    default:
                        logError("EdgeMidpointVertex", "Invalid vertex index %d", index);
                }
                break;
            case 4:
                switch ( index ) {
                    case 0:
                        v = neighbour->regular_subelements[0]->vertex[1];
                        break;
                    case 1:
                        v = neighbour->regular_subelements[1]->vertex[2];
                        break;
                    case 2:
                        v = neighbour->regular_subelements[3]->vertex[3];
                        break;
                    case 3:
                        v = neighbour->regular_subelements[2]->vertex[0];
                        break;
                    default:
                        logError("EdgeMidpointVertex", "Invalid vertex index %d", index);
                }
                break;
            default:
                logFatal(-1, "EdgeMidpointVertex", "only triangular and quadrilateral elements are supported");
        }
    }

    return v;
}

static Vertex *
monteCarloRadiosityNewEdgeMidpointVertex(ELEMENT *elem, int edgenr) {
    Vertex *v = monteCarloRadiosityEdgeMidpointVertex(elem, edgenr);
    if ( !v ) { /* first time we split the edge, create the midpoint vertex */
        Vertex *from = elem->vertex[edgenr],
                *to = elem->vertex[(edgenr + 1) % elem->nrvertices];
        v = monteCarloRadiosityNewMidpointVertex(elem, from, to);
    }
    return v;
}

static Vector3D
galerkinElementMidpoint(ELEMENT *elem) {
    int i;
    VECTORSET(elem->midpoint, 0., 0., 0.);
    for ( i = 0; i < elem->nrvertices; i++ ) {
        VECTORADD(elem->midpoint, *elem->vertex[i]->point, elem->midpoint);
    }
    VECTORSCALEINVERSE((float) elem->nrvertices, elem->midpoint, elem->midpoint);

    return elem->midpoint;
}

int
monteCarloRadiosityElementIsTextured(ELEMENT *elem) {
    Material *mat;
    if ( elem->iscluster ) {
        logFatal(-1, "monteCarloRadiosityElementIsTextured", "this routine should not be called for cluster elements");
        return false;
    }
    mat = elem->pog.patch->surface->material;
    return BsdfIsTextured(mat->bsdf) || EdfIsTextured(mat->edf);
}

float
monteCarloRadiosityElementScalarReflectance(ELEMENT *elem) {
    float rd;

    if ( elem->iscluster ) {
        return 1.0;
    }

    rd = colorMaximumComponent(elem->Rd);
    if ( rd < EPSILON ) {
        rd = EPSILON;
    }    /* avoid divisions by zero */
    return rd;
}

/**
Computes average reflectance and emittance of a surface sub-element
*/
static void
monteCarloRadiosityElementComputeAverageReflectanceAndEmittance(ELEMENT *elem) {
    Patch *patch = elem->pog.patch;
    int i;
    int numberOfSamples;
    int isTextured;
    int nbits;
    niedindex msb1;
    niedindex rmsb2;
    niedindex n;
    COLOR albedo, emittance;
    RayHit hit;
    InitHit(&hit, patch, nullptr, &patch->midpoint, &patch->normal, patch->surface->material, 0.);

    isTextured = monteCarloRadiosityElementIsTextured(elem);
    numberOfSamples = isTextured ? 100 : 1;
    colorClear(albedo);
    colorClear(emittance);
    monteCarloRadiosityElementRange(elem, &nbits, &msb1, &rmsb2);

    n = 1;
    for ( i = 0; i < numberOfSamples; i++, n++ ) {
        COLOR sample;
        niedindex *xi = NextNiedInRange(&n, +1, nbits, msb1, rmsb2);
        hit.uv.u = (double) xi[0] * RECIP;
        hit.uv.v = (double) xi[1] * RECIP;
        hit.flags |= HIT_UV;
        patchUniformPoint(patch, hit.uv.u, hit.uv.v, &hit.point);
        if ( patch->surface->material->bsdf ) {
            sample = BsdfScatteredPower(patch->surface->material->bsdf, &hit, &patch->normal, BRDF_DIFFUSE_COMPONENT);
            colorAdd(albedo, sample, albedo);
        }
        if ( patch->surface->material->edf ) {
            sample = EdfEmittance(patch->surface->material->edf, &hit, DIFFUSE_COMPONENT);
            colorAdd(emittance, sample, emittance);
        }
    }
    colorScaleInverse((float) numberOfSamples, albedo, elem->Rd);
    colorScaleInverse((float) numberOfSamples, emittance, elem->Ed);
}

/**
Initial push operation for surface sub-elements
*/
static void
monteCarloRadiosityInitSurfacePush(ELEMENT *parent, ELEMENT *child) {
    child->source_rad = parent->source_rad;
    PushRadiance(parent, child, parent->rad, child->rad);
    PushRadiance(parent, child, parent->unshot_rad, child->unshot_rad);
    PushImportance(parent, child, &parent->imp, &child->imp);
    PushImportance(parent, child, &parent->source_imp, &child->source_imp);
    PushImportance(parent, child, &parent->unshot_imp, &child->unshot_imp);
    child->ray_index = parent->ray_index;
    child->quality = parent->quality;
    child->prob = parent->prob * child->area / parent->area;

    child->Rd = parent->Rd;
    child->Ed = parent->Ed;
    monteCarloRadiosityElementComputeAverageReflectanceAndEmittance(child);
}

/**
Creates a sub-element of the element "*parent", stores it as the
sub-element number "childNumber". Tha value of "v3" is unused in the
process of triangle subdivision.
*/
static ELEMENT *
monteCarloRadiosityCreateSurfaceSubElement(
    ELEMENT *parent,
    int childNumber,
    Vertex *v0,
    Vertex *v1,
    Vertex *v2,
    Vertex *v3)
{
    int i;

    ELEMENT *elem = createElement();
    parent->regular_subelements[childNumber] = elem;

    elem->pog.patch = parent->pog.patch;
    elem->nrvertices = parent->nrvertices;
    elem->vertex[0] = v0;
    elem->vertex[1] = v1;
    elem->vertex[2] = v2;
    elem->vertex[3] = v3;
    for ( i = 0; i < elem->nrvertices; i++ ) {
        vertexAttachElement(elem->vertex[i], elem);
    }

    elem->area = 0.25 * parent->area; /* regular elements, regular subdivision */
    elem->midpoint = galerkinElementMidpoint(elem);

    elem->parent = parent;
    elem->child_nr = childNumber;
    elem->uptrans = elem->nrvertices == 3 ? &GLOBAL_stochasticRaytracing_triupxfm[childNumber] : &GLOBAL_stochasticRaytracing_quadupxfm[childNumber];

    allocCoefficients(elem);
    stochasticRadiosityClearCoefficients(elem->rad, elem->basis);
    stochasticRadiosityClearCoefficients(elem->unshot_rad, elem->basis);
    stochasticRadiosityClearCoefficients(elem->received_rad, elem->basis);
    elem->imp = elem->unshot_imp = elem->received_imp = 0.;
    monteCarloRadiosityInitSurfacePush(parent, elem);

    return elem;
}

/**
Create sub-elements: regular subdivision, see drawings above
*/
static ELEMENT **
monteCarloRadiosityRegularSubdivideTriangle(ELEMENT *element) {
    Vertex *v0, *v1, *v2, *m0, *m1, *m2;

    v0 = element->vertex[0];
    v1 = element->vertex[1];
    v2 = element->vertex[2];
    m0 = monteCarloRadiosityNewEdgeMidpointVertex(element, 0);
    m1 = monteCarloRadiosityNewEdgeMidpointVertex(element, 1);
    m2 = monteCarloRadiosityNewEdgeMidpointVertex(element, 2);

    monteCarloRadiosityCreateSurfaceSubElement(element, 0, v0, m0, m2, nullptr);
    monteCarloRadiosityCreateSurfaceSubElement(element, 1, m0, v1, m1, nullptr);
    monteCarloRadiosityCreateSurfaceSubElement(element, 2, m2, m1, v2, nullptr);
    monteCarloRadiosityCreateSurfaceSubElement(element, 3, m1, m2, m0, nullptr);

    renderSetColor(&renderopts.outline_color);
    renderLine(v0->point, v1->point);
    renderLine(v1->point, v2->point);
    renderLine(v2->point, v0->point);
    renderLine(m0->point, m1->point);
    renderLine(m1->point, m2->point);
    renderLine(m2->point, m0->point);

    return element->regular_subelements;
}

static ELEMENT **
monteCarloRadiosityRegularSubdivideQuad(ELEMENT *element) {
    Vertex *v0, *v1, *v2, *v3, *m0, *m1, *m2, *m3, *mm;

    v0 = element->vertex[0];
    v1 = element->vertex[1];
    v2 = element->vertex[2];
    v3 = element->vertex[3];
    m0 = monteCarloRadiosityNewEdgeMidpointVertex(element, 0);
    m1 = monteCarloRadiosityNewEdgeMidpointVertex(element, 1);
    m2 = monteCarloRadiosityNewEdgeMidpointVertex(element, 2);
    m3 = monteCarloRadiosityNewEdgeMidpointVertex(element, 3);
    mm = monteCarloRadiosityNewMidpointVertex(element, m0, m2);

    monteCarloRadiosityCreateSurfaceSubElement(element, 0, v0, m0, mm, m3);
    monteCarloRadiosityCreateSurfaceSubElement(element, 1, m0, v1, m1, mm);
    monteCarloRadiosityCreateSurfaceSubElement(element, 2, m3, mm, m2, v3);
    monteCarloRadiosityCreateSurfaceSubElement(element, 3, mm, m1, v2, m2);

    renderSetColor(&renderopts.outline_color);
    renderLine(v0->point, v1->point);
    renderLine(v1->point, v2->point);
    renderLine(v2->point, v3->point);
    renderLine(v3->point, v0->point);
    renderLine(m0->point, m2->point);
    renderLine(m1->point, m3->point);

    return element->regular_subelements;
}

/**
Subdivides given triangle or quadrangle into four sub-elements if not yet
done so before. Returns the list of created sub-elements
*/
ELEMENT **
monteCarloRadiosityRegularSubdivideElement(ELEMENT *element) {
    if ( element->regular_subelements ) {
        return element->regular_subelements;
    }

    if ( element->iscluster ) {
        logFatal(-1, "galerkinRegularSubdivideElement", "Cannot regularly subdivide cluster elements");
        return (ELEMENT **) nullptr;
    }

    if ( element->pog.patch->jacobian ) {
        static int wgiv = false;
        if ( !wgiv ) {
            logWarning("galerkinRegularSubdivideElement",
                       "irregular quadrilateral patches are not correctly handled (but you probably won't notice it)");
        }
        wgiv = true;
    }

    /* create the subelements */
    element->regular_subelements = (ELEMENT **)malloc(4*sizeof(ELEMENT *));
    switch ( element->nrvertices ) {
        case 3:
            monteCarloRadiosityRegularSubdivideTriangle(element);
            break;
        case 4:
            monteCarloRadiosityRegularSubdivideQuad(element);
            break;
        default:
            logFatal(-1, "galerkinRegularSubdivideElement", "invalid element: not 3 or 4 vertices");
    }
    return element->regular_subelements;
}

static void
monteCarloRadiosityDestroyElement(ELEMENT *elem) {
    int i;
    if ( elem == nullptr ) {
        return;
    }

    if ( elem->iscluster ) {
        GLOBAL_stochasticRaytracing_hierarchy.nr_clusters--;
    }
    GLOBAL_stochasticRaytracing_hierarchy.nr_elements--;

    if ( elem->irregular_subelements )
        ElementListDestroy(elem->irregular_subelements);
    if ( elem->regular_subelements )
        free(elem->regular_subelements);
    for ( i = 0; i < elem->nrvertices; i++ ) {
        ElementListDestroy(elem->vertex[i]->radiance_data);
        elem->vertex[i]->radiance_data = nullptr;
    }
    disposeCoefficients(elem);
    free(elem);
}

static void
monteCarloRadiosityDestroySurfaceElement(ELEMENT *elem) {
    if ( !elem ) {
        return;
    }
    ForAllRegularSubelements(child, elem)
                {
                    monteCarloRadiosityDestroySurfaceElement(child);
                }
    EndForAll;
    monteCarloRadiosityDestroyElement(elem);
}

void
monteCarloRadiosityDestroyToplevelSurfaceElement(ELEMENT *elem) {
    monteCarloRadiosityDestroySurfaceElement(elem);
}

void
monteCarloRadiosityDestroyClusterHierarchy(ELEMENT *top) {
    if ( !top || !top->iscluster ) {
        return;
    }
    ForAllIrregularSubelements(child, top)
                {
                    if ( child->iscluster )
                        monteCarloRadiosityDestroyClusterHierarchy(child);
                }
    EndForAll;
    monteCarloRadiosityDestroyElement(top);
}

static void
monteCarloRadiosityTestPrintVertex(FILE *out, int i, Vertex *v) {
    fprintf(out, "vertex[%d]: %s", i, v ? "" : "(nil)");
    if ( v ) {
        vertexPrint(out, v);
    }
    fprintf(out, "\n");
}

void
monteCarloRadiosityPrintElement(FILE *out, ELEMENT *elem) {
    fprintf(out, "Element id %ld:\n", elem->id);
    fprintf(out, "Vertices: ");
    monteCarloRadiosityTestPrintVertex(out, 0, elem->vertex[0]);
    monteCarloRadiosityTestPrintVertex(out, 1, elem->vertex[1]);
    monteCarloRadiosityTestPrintVertex(out, 2, elem->vertex[2]);
    monteCarloRadiosityTestPrintVertex(out, 3, elem->vertex[3]);
    PrintBasis(elem->basis);

    if ( !elem->iscluster ) {
        int nbits;
        niedindex msb1, rmsb2;
        fprintf(out, "Surface element: material '%s', reflectosity = %g, self-emitted luminsoity = %g\n",
                elem->pog.patch->surface->material->name,
                colorGray(elem->Rd),
                colorLuminance(elem->Ed));
        monteCarloRadiosityElementRange(elem, &nbits, &msb1, &rmsb2);
        fprintf(out, "Element range: %d bits, msb1 = %016llx, rmsb2 = %016llx\n", nbits, msb1, rmsb2);
    }
    fprintf(out, "rad = ");
    stochasticRaytracingPrintCoefficients(out, elem->rad, elem->basis);
    fprintf(out, ", luminosity = %g\n", colorLuminance(elem->rad[0]) * M_PI);

    fprintf(out, "unshot rad = ");
    stochasticRaytracingPrintCoefficients(out, elem->unshot_rad, elem->basis);
    fprintf(out, ", luminosity = %g\n", colorLuminance(elem->unshot_rad[0]) * M_PI);
    fprintf(out, "received rad = ");
    stochasticRaytracingPrintCoefficients(out, elem->received_rad, elem->basis);
    fprintf(out, ", luminosity = %g\n", colorLuminance(elem->received_rad[0]) * M_PI);
    fprintf(out, "source rad = ");
    elem->source_rad.print(out);
    fprintf(out, ", luminosity = %g\n", colorLuminance(elem->source_rad) * M_PI);
    fprintf(out, "ray index = %d\n", (unsigned) elem->ray_index);
    fprintf(out, "imp = %g, unshot_imp = %g, received_imp = %g, source_imp = %g\n",
            elem->imp, elem->unshot_imp, elem->received_imp, elem->source_imp);
    fprintf(out, "importance ray index = %d\n", (unsigned) elem->imp_ray_index);
    fprintf(out, "prob = %g, quality factor = %g\nng = %g\n",
            elem->prob, elem->quality, elem->ng);
}

/**
Returns true if there are children elements and false if top is nullptr or a leaf element
*/
int
monteCarloRadiosityForAllChildrenElements(ELEMENT *top, void (*func)(ELEMENT *)) {
    if ( !top ) {
        return false;
    }

    if ( top->iscluster ) {
        ForAllIrregularSubelements(p, top)
                    func(p);
        EndForAll;
        return true;
    } else if ( top->regular_subelements ) {
        ForAllRegularSubelements(p, top)
                    func(p);
        EndForAll;
        return true;
    } else {     /* leaf element */
        return false;
    }
}

void
monteCarloRadiosityForAllLeafElements(ELEMENT *top, void (*func)(ELEMENT *)) {
    if ( !top ) {
        return;
    }

    if ( top->iscluster ) {
        ForAllIrregularSubelements(p, top)
                    monteCarloRadiosityForAllLeafElements(p, func);
        EndForAll;
    } else if ( top->regular_subelements ) {
        ForAllRegularSubelements(p, top)
                    monteCarloRadiosityForAllLeafElements(p, func);
        EndForAll;
    } else {
        func(top);
    }
}

void
monteCarloRadiosityForAllSurfaceLeafs(ELEMENT *top,
                                      void (*func)(ELEMENT *)) {
    REC_ForAllSurfaceLeafs(leaf, top)
            {
                func(leaf);
            }
    REC_EndForAllSurfaceLeafs;
}

int
monteCarloRadiosityElementIsLeaf(ELEMENT *elem) {
    return (!elem->regular_subelements && !elem->irregular_subelements);
}

/**
Computes and fills in a bounding box for the element
*/
float *
monteCarloRadiosityElementBounds(ELEMENT *elem, float *bounds) {
    if ( elem->iscluster ) {
        boundsCopy(elem->pog.geom->bounds, bounds);
    } else if ( !elem->uptrans ) {
        patchBounds(elem->pog.patch, bounds);
    } else {
        boundsInit(bounds);
        ForAllVerticesOfElement(v, elem)
                    {
                        boundsEnlargePoint(bounds, v->point);
                    }
        EndForAll;
    }
    return bounds;
}

ELEMENT *
monteCarloRadiosityClusterChildContainingElement(ELEMENT *parent, ELEMENT *descendant) {
    while ( descendant && descendant->parent != parent ) {
        descendant = descendant->parent;
    }
    if ( !descendant ) {
        logFatal(-1, "monteCarloRadiosityClusterChildContainingElement", "descendant is not a descendant of parent");
    }
    return descendant;
}

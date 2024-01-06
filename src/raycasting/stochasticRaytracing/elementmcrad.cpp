#include <cstdlib>

#include "common/error.h"
#include "material/statistics.h"
#include "skin/Vertex.h"
#include "skin/Patch.h"
#include "skin/Geometry.h"
#include "skin/vertexlist.h"
#include "shared/render.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "raycasting/stochasticRaytracing/coefficientsmcrad.h"

static ELEMENT *CreateClusterHierarchyRecursive(Geometry *world);

/* Orientation and position of regular subelements is fully determined by the
 * following transformations. A uniform mapping of parameter domain to the
 * elements is supposed (i.o.w. use PatchUniformPoint() to map (u,v) coordinates
 * on the toplevel element to a 3D point on the patch). The subelements
 * have equal area. No explicit Jacobian stuff needed to compute integrals etc..
 * etc..
 *
 * Do not change the conventions below without knowing VERY well what
 * you are doing.
 */

/* up-transforms for regular quadrilateral subelements:
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
Matrix2x2 mcr_quadupxfm[4] = {
        /* south-west [0,0.5] x [0,0.5] */
        {{{0.5, 0.0}, {0.0, 0.5}}, {0.0, 0.0}},

        /* south-east: [0.5,1] x [0,0.5] */
        {{{0.5, 0.0}, {0.0, 0.5}}, {0.5, 0.0}},

        /* north-west: [0,0.5] x [0.5,1] */
        {{{0.5, 0.0}, {0.0, 0.5}}, {0.0, 0.5}},

        /* north-east: [0.5,1] x [0.5,1] */
        {{{0.5, 0.0}, {0.0, 0.5}}, {0.5, 0.5}},
};

/* up-transforms for triangular subelements:
 *
 *  (v)
 *
 *   1 +
 *     | \
 *     |   \
 *     |     \
 *     | 2     \
 * 0.5 +---------+
 *     | \     3 | \
 *     |   \     |   \
 *     |     \   |     \
 *     | 0     \ | 1     \
 *   0 +---------+---------+
 *     0        0.5        1  (u)
 */
Matrix2x2 mcr_triupxfm[4] = {
        /* left: (0,0),(0.5,0),(0,0.5) */
        {{{0.5,  0.0}, {0.0, 0.5}},  {0.0, 0.0}},

        /* right: (0.5,0),(1,0),(0.5,0.5) */
        {{{0.5,  0.0}, {0.0, 0.5}},  {0.5, 0.0}},

        /* top: (0,0.5),(0.5,0.5),(0,1) */
        {{{0.5,  0.0}, {0.0, 0.5}},  {0.0, 0.5}},

        /* middle: (0.5,0.5),(0,0.5),(0.5,0) */
        {{{-0.5, 0.0}, {0.0, -0.5}}, {0.5, 0.5}},
};

static ELEMENT *
CreateElement() {
    static long id = 1;
    ELEMENT *elem = (ELEMENT *)malloc(sizeof(ELEMENT));

    elem->pog.patch = (PATCH *) nullptr;
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
    elem->vertex[0] = elem->vertex[1] = elem->vertex[2] = elem->vertex[3] = (VERTEX *) nullptr;
    elem->parent = (ELEMENT *) nullptr;
    elem->regular_subelements = (ELEMENT **) nullptr;
    elem->irregular_subelements = (ELEMENTLIST *) nullptr;
    elem->uptrans = (Matrix2x2 *) nullptr;
    elem->child_nr = -1;
    elem->nrvertices = 0;
    elem->iscluster = false;
    elem->flags = 0;

    hierarchy.nr_elements++;

    return elem;
}

static void
VertexAttachElement(VERTEX *v, ELEMENT *elem) {
    v->radiance_data = ElementListAdd(v->radiance_data, elem);
}

ELEMENT *
CreateToplevelSurfaceElement(PATCH *patch) {
    int i;
    ELEMENT *elem = CreateElement();
    elem->pog.patch = patch;
    elem->iscluster = false;
    elem->area = patch->area;
    elem->midpoint = patch->midpoint;
    elem->nrvertices = patch->nrvertices;
    for ( i = 0; i < elem->nrvertices; i++ ) {
        elem->vertex[i] = patch->vertex[i];
        VertexAttachElement(elem->vertex[i], elem);
    }

    allocCoefficients(elem);    /* may need reallocation before the start
				 * of the computations. */
    stochasticRadiosityClearCoefficients(elem->rad, elem->basis);
    stochasticRadiosityClearCoefficients(elem->unshot_rad, elem->basis);
    stochasticRadiosityClearCoefficients(elem->received_rad, elem->basis);

    elem->Ed = PatchAverageEmittance(patch, DIFFUSE_COMPONENT);
    colorScaleInverse(M_PI, elem->Ed, elem->Ed);
    elem->Rd = PatchAverageNormalAlbedo(patch, BRDF_DIFFUSE_COMPONENT);

    return elem;
}

static ELEMENT *
CreateCluster(Geometry *geom) {
    ELEMENT *elem = CreateElement();
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

    hierarchy.nr_clusters++;

    return elem;
}

static void
CreateSurfaceElementChild(PATCH *patch, ELEMENT *parent) {
    ELEMENT *elem;
    elem = (ELEMENT *)patch->radiance_data;    /* created before */
    elem->parent = (ELEMENT *)parent;
    parent->irregular_subelements = ElementListAdd(parent->irregular_subelements, elem);
}

static void
CreateClusterChild(Geometry *geom, ELEMENT *parent) {
    ELEMENT *subcluster = CreateClusterHierarchyRecursive(geom);
    subcluster->parent = parent;
    parent->irregular_subelements = ElementListAdd(parent->irregular_subelements, subcluster);
}

/* initialises parent cluster radiance/importance/area for child elem. */
static void
InitClusterPull(ELEMENT *parent, ELEMENT *child) {
    parent->area += child->area;
    PullRadiance(parent, child, parent->rad, child->rad);
    PullRadiance(parent, child, parent->unshot_rad, child->unshot_rad);
    PullRadiance(parent, child, parent->received_rad, child->received_rad);
    PullImportance(parent, child, &parent->imp, &child->imp);
    PullImportance(parent, child, &parent->unshot_imp, &child->unshot_imp);
    PullImportance(parent, child, &parent->received_imp, &child->received_imp);

    /* needs division by parent->area once it is known after InitClusterPull for
     * all children elements. */
    colorAddScaled(parent->Ed, child->area, child->Ed, parent->Ed);
}

static void
CreateClusterChildren(ELEMENT *parent) {
    Geometry *geom = parent->pog.geom;

    if ( geomIsAggregate(geom)) {
        ForAllGeoms(childgeom, geomPrimList(geom))
                    {
                        CreateClusterChild(childgeom, parent);
                    }
        EndForAll;
    } else {
        ForAllPatches(childpatch, geomPatchList(geom))
                    {
                        CreateSurfaceElementChild(childpatch, parent);
                    }
        EndForAll;
    }

    ForAllIrregularSubelements(child, parent)
                {
                    InitClusterPull(parent, child);
                }
    EndForAll;
    colorScaleInverse(parent->area, parent->Ed, parent->Ed);
}

static ELEMENT *
CreateClusterHierarchyRecursive(Geometry *world) {
    ELEMENT *topcluster;
    world->radiance_data = topcluster = (ELEMENT *) CreateCluster(world);
    CreateClusterChildren(topcluster);
    return topcluster;
}

ELEMENT *
McrCreateClusterHierarchy(Geometry *world) {
    if ( !world ) {
        return (ELEMENT *) nullptr;
    } else {
        return CreateClusterHierarchyRecursive(world);
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
ElementRange(ELEMENT *elem, int *nbits, niedindex *msb1, niedindex *rmsb2) {
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
Determines the regular subelement at point (u,v) of the given parent
element. Returns the parent element itself if there are no regular subelements.
The point is transformed to the corresponding point on the subelement
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
            Fatal(-1, "RegularSubelementAtPoint", "Can handle only triangular or quadrilateral elements");
    }

    return child;
}

/* Returns the leaf regular subelement of 'top' at the point (u,v) (uniform
 * coordinates!). (u,v) is transformed to the coordinates of the corresponding
 * point on the leaf element. 'top' is a surface element, not a cluster. */
ELEMENT *
McrRegularLeafElementAtPoint(ELEMENT *top, double *u, double *v) {
    ELEMENT *leaf;

    /* find leaf element of 'top' at (u,v) */
    leaf = top;
    while ( leaf->regular_subelements ) {
        leaf = monteCarloRadiosityRegularSubElementAtPoint(leaf, u, v);
    }

    return leaf;
}

static Vector3D *
InstallCoordinate(Vector3D *coord) {
    Vector3D *v = VectorCreate(coord->x, coord->y, coord->z);
    hierarchy.coords = VectorListAdd(hierarchy.coords, v);
    return v;
}

static Vector3D *
InstallNormal(Vector3D *norm) {
    Vector3D *v = VectorCreate(norm->x, norm->y, norm->z);
    hierarchy.normals = VectorListAdd(hierarchy.normals, v);
    return v;
}

static Vector3D *
InstallTexCoord(Vector3D *texCoord) {
    Vector3D *t = VectorCreate(texCoord->x, texCoord->y, texCoord->z);
    hierarchy.texCoords = VectorListAdd(hierarchy.texCoords, t);
    return t;
}

static VERTEX *
InstallVertex(Vector3D *coord, Vector3D *norm, Vector3D *texCoord) {
    VERTEX *v = VertexCreate(coord, norm, texCoord, (PatchSet *) nullptr);
    hierarchy.vertices = VertexListAdd(hierarchy.vertices, v);
    return v;
}

static VERTEX *
NewMidpointVertex(ELEMENT *elem, VERTEX *v1, VERTEX *v2) {
    Vector3D coord, norm, texCoord, *p, *n, *t;

    MIDPOINT(*(v1->point), *(v2->point), coord);
    p = InstallCoordinate(&coord);

    if ( v1->normal && v2->normal ) {
        MIDPOINT(*(v1->normal), *(v2->normal), norm);
        n = InstallNormal(&norm);
    } else {
        n = &elem->pog.patch->normal;
    }

    if ( v1->texCoord && v2->texCoord ) {
        MIDPOINT(*(v1->texCoord), *(v2->texCoord), texCoord);
        t = InstallTexCoord(&texCoord);
    } else {
        t = nullptr;
    }

    return InstallVertex(p, n, t);
}

/* Find the neighbouring element of the given element at the edgenr-th edge.
 * The edgenr-th edge is the edge connecting the edgenr-th vertex to the
 * (edgenr+1 modulo GLOBAL_statistics_numberOfVertices)-th vertex. */
static ELEMENT *
ElementNeighbour(ELEMENT *elem, int edgenr) {
    VERTEX *from = elem->vertex[edgenr],
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

VERTEX *
McrEdgeMidpointVertex(ELEMENT *elem, int edgenr) {
    VERTEX *v = (VERTEX *) nullptr,
            *to = elem->vertex[(edgenr + 1) % elem->nrvertices];
    ELEMENT *neighbour = ElementNeighbour(elem, edgenr);

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
                        Error("EdgeMidpointVertex", "Invalid vertex index %d", index);
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
                        Error("EdgeMidpointVertex", "Invalid vertex index %d", index);
                }
                break;
            default:
                Fatal(-1, "EdgeMidpointVertex", "only triangular and quadrilateral elements are supported");
        }
    }

    return v;
}

static VERTEX *
NewEdgeMidpointVertex(ELEMENT *elem, int edgenr) {
    VERTEX *v = McrEdgeMidpointVertex(elem, edgenr);
    if ( !v ) { /* first time we split the edge, create the midpoint vertex */
        VERTEX *from = elem->vertex[edgenr],
                *to = elem->vertex[(edgenr + 1) % elem->nrvertices];
        v = NewMidpointVertex(elem, from, to);
    }
    return v;
}

static Vector3D
ElementMidpoint(ELEMENT *elem) {
    int i;
    VECTORSET(elem->midpoint, 0., 0., 0.);
    for ( i = 0; i < elem->nrvertices; i++ ) {
        VECTORADD(elem->midpoint, *elem->vertex[i]->point, elem->midpoint);
    }
    VECTORSCALEINVERSE((float) elem->nrvertices, elem->midpoint, elem->midpoint);

    return elem->midpoint;
}

int
ElementIsTextured(ELEMENT *elem) {
    MATERIAL *mat;
    if ( elem->iscluster ) {
        Fatal(-1, "ElementIsTextured", "this routine should not be called for cluster elements");
        return false;
    }
    mat = elem->pog.patch->surface->material;
    return BsdfIsTextured(mat->bsdf) || EdfIsTextured(mat->edf);
}

float
ElementScalarReflectance(ELEMENT *elem) {
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

/* computes average reflectance and emittance of a surface sub-element */
static void
ElementComputeAverageReflectanceAndEmittance(ELEMENT *elem) {
    PATCH *patch = elem->pog.patch;
    int i, nrsamples, istextured;
    int nbits;
    niedindex msb1, rmsb2, n;
    COLOR albedo, emittance;
    HITREC hit;
    InitHit(&hit, patch, nullptr, &patch->midpoint, &patch->normal, patch->surface->material, 0.);

    istextured = ElementIsTextured(elem);
    nrsamples = istextured ? 100 : 1;
    colorClear(albedo);
    colorClear(emittance);
    ElementRange(elem, &nbits, &msb1, &rmsb2);

    n = 1;
    for ( i = 0; i < nrsamples; i++, n++ ) {
        COLOR sample;
        niedindex *xi = NextNiedInRange(&n, +1, nbits, msb1, rmsb2);
        hit.uv.u = (double) xi[0] * RECIP;
        hit.uv.v = (double) xi[1] * RECIP;
        hit.flags |= HIT_UV;
        PatchUniformPoint(patch, hit.uv.u, hit.uv.v, &hit.point);
        if ( patch->surface->material->bsdf ) {
            sample = BsdfScatteredPower(patch->surface->material->bsdf, &hit, &patch->normal, BRDF_DIFFUSE_COMPONENT);
            colorAdd(albedo, sample, albedo);
        }
        if ( patch->surface->material->edf ) {
            sample = EdfEmittance(patch->surface->material->edf, &hit, DIFFUSE_COMPONENT);
            colorAdd(emittance, sample, emittance);
        }
    }
    colorScaleInverse((float) nrsamples, albedo, elem->Rd);
    colorScaleInverse((float) nrsamples, emittance, elem->Ed);
}

/* initial push operation for surface subelements */
static void
InitSurfacePush(ELEMENT *parent, ELEMENT *child) {
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
    ElementComputeAverageReflectanceAndEmittance(child);
}

/* ---------------------------------------------------------------------------
  'CreateSurfaceSubelement'

  Creates a subelement of the element "*parent", stores it as the
  subelement number "childnr". Tha value of "v3" is unused in the
  process of triangle subdivision.
  ------------------------------------------------------------------------- */
static ELEMENT *
CreateSurfaceSubelement(
    ELEMENT *parent,
    int childnr,
    VERTEX *v0,
    VERTEX *v1,
    VERTEX *v2,
    VERTEX *v3)
{
    int i;

    ELEMENT *elem = CreateElement();
    parent->regular_subelements[childnr] = elem;

    elem->pog.patch = parent->pog.patch;
    elem->nrvertices = parent->nrvertices;
    elem->vertex[0] = v0;
    elem->vertex[1] = v1;
    elem->vertex[2] = v2;
    elem->vertex[3] = v3;
    for ( i = 0; i < elem->nrvertices; i++ ) {
        VertexAttachElement(elem->vertex[i], elem);
    }

    elem->area = 0.25 * parent->area; /* regular elements, regular subdivision */
    elem->midpoint = ElementMidpoint(elem);

    elem->parent = parent;
    elem->child_nr = childnr;
    elem->uptrans = elem->nrvertices == 3 ? &mcr_triupxfm[childnr] : &mcr_quadupxfm[childnr];

    allocCoefficients(elem);
    stochasticRadiosityClearCoefficients(elem->rad, elem->basis);
    stochasticRadiosityClearCoefficients(elem->unshot_rad, elem->basis);
    stochasticRadiosityClearCoefficients(elem->received_rad, elem->basis);
    elem->imp = elem->unshot_imp = elem->received_imp = 0.;
    InitSurfacePush(parent, elem);

    return elem;
}

/* create subelements: regular subdivision, see drawings above. */
static ELEMENT **
RegularSubdivideTriangle(ELEMENT *element) {
    VERTEX *v0, *v1, *v2, *m0, *m1, *m2;

    v0 = element->vertex[0];
    v1 = element->vertex[1];
    v2 = element->vertex[2];
    m0 = NewEdgeMidpointVertex(element, 0);
    m1 = NewEdgeMidpointVertex(element, 1);
    m2 = NewEdgeMidpointVertex(element, 2);

    CreateSurfaceSubelement(element, 0, v0, m0, m2, nullptr);
    CreateSurfaceSubelement(element, 1, m0, v1, m1, nullptr);
    CreateSurfaceSubelement(element, 2, m2, m1, v2, nullptr);
    CreateSurfaceSubelement(element, 3, m1, m2, m0, nullptr);

#ifndef NO_SUBDIVISION_LINES
    RenderSetColor(&renderopts.outline_color);
    RenderLine(v0->point, v1->point);
    RenderLine(v1->point, v2->point);
    RenderLine(v2->point, v0->point);
    RenderLine(m0->point, m1->point);
    RenderLine(m1->point, m2->point);
    RenderLine(m2->point, m0->point);
#endif
    return element->regular_subelements;
}

static ELEMENT **
RegularSubdivideQuad(ELEMENT *element) {
    VERTEX *v0, *v1, *v2, *v3, *m0, *m1, *m2, *m3, *mm;

    v0 = element->vertex[0];
    v1 = element->vertex[1];
    v2 = element->vertex[2];
    v3 = element->vertex[3];
    m0 = NewEdgeMidpointVertex(element, 0);
    m1 = NewEdgeMidpointVertex(element, 1);
    m2 = NewEdgeMidpointVertex(element, 2);
    m3 = NewEdgeMidpointVertex(element, 3);
    mm = NewMidpointVertex(element, m0, m2);

    CreateSurfaceSubelement(element, 0, v0, m0, mm, m3);
    CreateSurfaceSubelement(element, 1, m0, v1, m1, mm);
    CreateSurfaceSubelement(element, 2, m3, mm, m2, v3);
    CreateSurfaceSubelement(element, 3, mm, m1, v2, m2);

#ifndef NO_SUBDIVISION_LINES
    RenderSetColor(&renderopts.outline_color);
    RenderLine(v0->point, v1->point);
    RenderLine(v1->point, v2->point);
    RenderLine(v2->point, v3->point);
    RenderLine(v3->point, v0->point);
    RenderLine(m0->point, m2->point);
    RenderLine(m1->point, m3->point);
#endif
    return element->regular_subelements;
}

/* ---------------------------------------------------------------------------
  'RegularSubdivideElement'

  Subdivides given triangle or quadrangle into four subelements if not yet
  done so before. Returns the list of created subelements.
  ------------------------------------------------------------------------- */
ELEMENT **
McrRegularSubdivideElement(ELEMENT *element) {
    if ( element->regular_subelements ) {
        return element->regular_subelements;
    }

    if ( element->iscluster ) {
        Fatal(-1, "RegularSubdivideElement", "Cannot regularly subdivide cluster elements");
        return (ELEMENT **) nullptr;
    }

    if ( element->pog.patch->jacobian ) {
        static int wgiv = false;
        if ( !wgiv ) {
            Warning("RegularSubdivideElement",
                    "irregular quadrilateral patches are not correctly handled (but you probably won't notice it)");
        }
        wgiv = true;
    }

    /* create the subelements */
    element->regular_subelements = (ELEMENT **)malloc(4*sizeof(ELEMENT *));
    switch ( element->nrvertices ) {
        case 3:
            RegularSubdivideTriangle(element);
            break;
        case 4:
            RegularSubdivideQuad(element);
            break;
        default:
            Fatal(-1, "RegularSubdivideElement", "invalid element: not 3 or 4 vertices");
    }
    return element->regular_subelements;
}

static void
DestroyElement(ELEMENT *elem) {
    int i;
    if ( !elem ) {
        return;
    }

    if ( elem->iscluster ) {
        hierarchy.nr_clusters--;
    }
    hierarchy.nr_elements--;

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
DestroySurfaceElement(ELEMENT *elem) {
    if ( !elem ) {
        return;
    }
    ForAllRegularSubelements(child, elem)
                {
                    DestroySurfaceElement(child);
                }
    EndForAll;
    DestroyElement(elem);
}

void
McrDestroyToplevelSurfaceElement(ELEMENT *elem) {
    DestroySurfaceElement(elem);
}

void
McrDestroyClusterHierarchy(ELEMENT *top) {
    if ( !top || !top->iscluster ) {
        return;
    }
    ForAllIrregularSubelements(child, top)
                {
                    if ( child->iscluster )
                        McrDestroyClusterHierarchy(child);
                }
    EndForAll;
    DestroyElement(top);
}

static void
TestPrintVertex(FILE *out, int i, VERTEX *v) {
    fprintf(out, "vertex[%d]: %s", i, v ? "" : "(nil)");
    if ( v ) {
        VertexPrint(out, v);
    }
    fprintf(out, "\n");
}

void
McrPrintElement(FILE *out, ELEMENT *elem) {
    fprintf(out, "Element id %ld:\n", elem->id);
    fprintf(out, "Vertices: ");
    TestPrintVertex(out, 0, elem->vertex[0]);
    TestPrintVertex(out, 1, elem->vertex[1]);
    TestPrintVertex(out, 2, elem->vertex[2]);
    TestPrintVertex(out, 3, elem->vertex[3]);
    PrintBasis(elem->basis);

    if ( !elem->iscluster ) {
        int nbits;
        niedindex msb1, rmsb2;
        fprintf(out, "Surface element: material '%s', reflectosity = %g, self-emitted luminsoity = %g\n",
                elem->pog.patch->surface->material->name,
                colorGray(elem->Rd),
                colorLuminance(elem->Ed));
        ElementRange(elem, &nbits, &msb1, &rmsb2);
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

/* returns true if there are children elements and false if top is nullptr or a leaf element */
int
McrForAllChildrenElements(ELEMENT *top, void (*func)(ELEMENT *)) {
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
McrForAllLeafElements(ELEMENT *top, void (*func)(ELEMENT *)) {
    if ( !top ) {
        return;
    }

    if ( top->iscluster ) {
        ForAllIrregularSubelements(p, top)
                    McrForAllLeafElements(p, func);
        EndForAll;
    } else if ( top->regular_subelements ) {
        ForAllRegularSubelements(p, top)
                    McrForAllLeafElements(p, func);
        EndForAll;
    } else {
        func(top);
    }
}

void
McrForAllSurfaceLeafs(ELEMENT *top,
                        void (*func)(ELEMENT *)) {
    REC_ForAllSurfaceLeafs(leaf, top)
            {
                func(leaf);
            }
    REC_EndForAllSurfaceLeafs;
}

int
ElementIsLeaf(ELEMENT *elem) {
    return (!elem->regular_subelements && !elem->irregular_subelements);
}

/**
Computes and fills in a bounding box for the element
*/
float *
McrElementBounds(ELEMENT *elem, float *bounds) {
    if ( elem->iscluster ) {
        BoundsCopy(elem->pog.geom->bounds, bounds);
    } else if ( !elem->uptrans ) {
        PatchBounds(elem->pog.patch, bounds);
    } else {
        BoundsInit(bounds);
        ForAllVerticesOfElement(v, elem)
                    {
                        BoundsEnlargePoint(bounds, v->point);
                    }
        EndForAll;
    }
    return bounds;
}

ELEMENT *
ClusterChildContainingElement(ELEMENT *parent, ELEMENT *descendant) {
    while ( descendant && descendant->parent != parent ) {
        descendant = descendant->parent;
    }
    if ( !descendant ) {
        Fatal(-1, "ClusterChildContainingElement", "descendant is not a descendant of parent");
    }
    return descendant;
}

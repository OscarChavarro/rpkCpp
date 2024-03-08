#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "render/render.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "render/opengl.h"

static StochasticRadiosityElement *monteCarloRadiosityCreateClusterHierarchyRecursive(Geometry *world);

/**
Orientation and position of regular sub-elements is fully determined by the
following transformations. A uniform mapping of parameter domain to the
elements is supposed (i.o.w. use uniformPoint() to map (u,v) coordinates
on the toplevel element to a 3D point on the patch). The sub-elements
have equal area. No explicit Jacobian stuff needed to compute integrals etc..
etc.

Do not change the conventions below without knowing VERY well what
you are doing.
*/

/**
Up-transforms for regular quadrilateral sub-elements:

  (v)

   1 +---------+---------+
     |         |         |
     |         |         |
     | 2       | 3       |
 0.5 +---------+---------+
     |         |         |
     |         |         |
     | 0       | 1       |
   0 +---------+---------+
     0        0.5        1   (u)
*/
Matrix2x2 GLOBAL_stochasticRaytracing_quadUpTransform[4] = {
    // South-west [0, 0.5] x [0, 0.5]
    {{{0.5, 0.0}, {0.0, 0.5}}, {0.0, 0.0}},

    // South-east: [0.5, 1] x [0, 0.5]
    {{{0.5, 0.0}, {0.0, 0.5}}, {0.5, 0.0}},

    // North-west: [0, 0.5] x [0.5, 1]
    {{{0.5, 0.0}, {0.0, 0.5}}, {0.0, 0.5}},

    // North-east: [0.5, 1] x [0.5, 1]
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
Matrix2x2 GLOBAL_stochasticRaytracing_triangleUpTransform[4] = {
    // Left: (0, 0), (0.5, 0), (0, 0.5)
    {{{0.5,  0.0}, {0.0, 0.5}},  {0.0, 0.0}},

    // Right: (0.5, 0), (1, 0), (0.5, 0.5)
    {{{0.5,  0.0}, {0.0, 0.5}},  {0.5, 0.0}},

    // Top: (0, 0.5), (0.5, 0.5), (0, 1)
    {{{0.5,  0.0}, {0.0, 0.5}},  {0.0, 0.5}},

    // Middle: (0.5, 0.5), (0, 0.5), (0.5, 0)
    {{{-0.5, 0.0}, {0.0, -0.5}}, {0.5, 0.5}}
};

static StochasticRadiosityElement *
createElement() {
    static long id = 1;
    StochasticRadiosityElement *elem = new StochasticRadiosityElement();

    elem->patch = nullptr;
    elem->id = (int)id;
    id++;
    elem->area = 0.;
    initCoefficients(elem); // Allocation of the coefficients is left until just before the first iteration
				            // in monteCarloRadiosityReInit()

    colorClear(elem->Ed);
    colorClear(elem->Rd);

    elem->ray_index = 0;
    elem->quality = 0;
    elem->ng = 0.;

    elem->imp = elem->unShotImp = elem->source_imp = 0.;
    elem->imp_ray_index = 0;

    vectorSet(elem->midpoint, 0., 0., 0.);
    elem->vertex[0] = elem->vertex[1] = elem->vertex[2] = elem->vertex[3] = nullptr;
    elem->parent = nullptr;
    elem->regularSubElements = nullptr;
    elem->irregularSubElements = nullptr;
    elem->upTrans = nullptr;
    elem->childNumber = -1;
    elem->numberOfVertices = 0;
    elem->isCluster = false;

    GLOBAL_stochasticRaytracing_hierarchy.nr_elements++;

    return elem;
}

static void
vertexAttachElement(Vertex *v, StochasticRadiosityElement *elem) {
    elem->className = ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY;
    if ( v->radiance_data == nullptr ) {
        v->radiance_data = new java::ArrayList<Element *>();
    }
    v->radiance_data->add(0, elem);
}

StochasticRadiosityElement *
monteCarloRadiosityCreateToplevelSurfaceElement(Patch *patch) {
    int i;
    StochasticRadiosityElement *elem = createElement();
    elem->patch = patch;
    elem->isCluster = false;
    elem->area = patch->area;
    elem->midpoint = patch->midpoint;
    elem->numberOfVertices = patch->numberOfVertices;
    for ( i = 0; i < elem->numberOfVertices; i++ ) {
        elem->vertex[i] = patch->vertex[i];
        vertexAttachElement(elem->vertex[i], elem);
    }

    allocCoefficients(elem); // May need reallocation before the start of the computations
    stochasticRadiosityClearCoefficients(elem->radiance, elem->basis);
    stochasticRadiosityClearCoefficients(elem->unShotRadiance, elem->basis);
    stochasticRadiosityClearCoefficients(elem->receivedRadiance, elem->basis);

    elem->Ed = patch->averageEmittance(DIFFUSE_COMPONENT);
    colorScaleInverse(M_PI, elem->Ed, elem->Ed);
    elem->Rd = patch->averageNormalAlbedo(BRDF_DIFFUSE_COMPONENT);

    return elem;
}

static StochasticRadiosityElement *
monteCarloRadiosityCreateCluster(Geometry *geometry) {
    StochasticRadiosityElement *elem = createElement();
    float *bounds = geometry->bounds;

    elem->geometry = geometry;
    elem->isCluster = true;

    colorSetMonochrome(elem->Rd, 1.0);
    colorClear(elem->Ed);

    // elem->area will be computed from the sub-elements in the cluster later
    vectorSet(elem->midpoint,
              (bounds[MIN_X] + bounds[MAX_X]) / 2.0f,
              (bounds[MIN_Y] + bounds[MAX_Y]) / 2.0f,
              (bounds[MIN_Z] + bounds[MAX_Z]) / 2.0f);

    allocCoefficients(elem); // Always constant approx. so no need to delay allocating the coefficients
    stochasticRadiosityClearCoefficients(elem->radiance, elem->basis);
    stochasticRadiosityClearCoefficients(elem->unShotRadiance, elem->basis);
    stochasticRadiosityClearCoefficients(elem->receivedRadiance, elem->basis);
    elem->imp = 0.0;
    elem->unShotImp = 0.0;
    elem->received_imp = 0.0;

    GLOBAL_stochasticRaytracing_hierarchy.nr_clusters++;

    return elem;
}

static void
monteCarloRadiosityCreateSurfaceElementChild(Patch *patch, StochasticRadiosityElement *parent) {
    StochasticRadiosityElement *elem = (StochasticRadiosityElement *)patch->radianceData; // Created before
    elem->parent = (StochasticRadiosityElement *)parent;

    elem->className = ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY;
    if ( parent->irregularSubElements == nullptr ) {
        parent->irregularSubElements = new java::ArrayList<StochasticRadiosityElement *>();
    }
    parent->irregularSubElements->add(0, elem);
}

static void
monteCarloRadiosityCreateClusterChild(Geometry *geom, StochasticRadiosityElement *parent) {
    StochasticRadiosityElement *subCluster = monteCarloRadiosityCreateClusterHierarchyRecursive(geom);
    subCluster->parent = parent;
    subCluster->className = ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY;
    if ( parent->irregularSubElements == nullptr ) {
        parent->irregularSubElements = new java::ArrayList<StochasticRadiosityElement *>();
    }
    parent->irregularSubElements->add(0, subCluster);
}

/**
Initialises parent cluster radiance/importance/area for child voxelData
*/
static void
monteCarloRadiosityInitClusterPull(StochasticRadiosityElement *parent, StochasticRadiosityElement *child) {
    parent->area += child->area;
    pullRadiance(parent, child, parent->radiance, child->radiance);
    pullRadiance(parent, child, parent->unShotRadiance, child->unShotRadiance);
    pullRadiance(parent, child, parent->receivedRadiance, child->receivedRadiance);
    pullImportance(parent, child, &parent->imp, &child->imp);
    pullImportance(parent, child, &parent->unShotImp, &child->unShotImp);
    pullImportance(parent, child, &parent->received_imp, &child->received_imp);

    // Needs division by parent->area once it is known after monteCarloRadiosityInitClusterPull for
    // all children elements
    colorAddScaled(parent->Ed, child->area, child->Ed, parent->Ed);
}

static void
monteCarloRadiosityCreateClusterChildren(StochasticRadiosityElement *parent) {
    Geometry *geometry = parent->geometry;

    if ( geomIsAggregate(geometry) ) {
        java::ArrayList<Geometry *> *geometryList = geomPrimListCopy(geometry);
        for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
            monteCarloRadiosityCreateClusterChild(geometryList->get(i), parent);
        }
        delete geometryList;
    } else {
        java::ArrayList<Patch *> *patchList = geomPatchArrayListReference(geometry);
        for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
            monteCarloRadiosityCreateSurfaceElementChild(patchList->get(i), parent);
        }
    }

    for ( int i = 0; parent->irregularSubElements != nullptr && i < parent->irregularSubElements->size(); i++ ) {
        monteCarloRadiosityInitClusterPull(parent, parent->irregularSubElements->get(i));
    }
    colorScaleInverse(parent->area, parent->Ed, parent->Ed);
}

static StochasticRadiosityElement *
monteCarloRadiosityCreateClusterHierarchyRecursive(Geometry *world) {
    StochasticRadiosityElement *topCluster = (StochasticRadiosityElement *) monteCarloRadiosityCreateCluster(world);
    world->radianceData = topCluster;
    monteCarloRadiosityCreateClusterChildren(topCluster);
    return topCluster;
}

StochasticRadiosityElement *
monteCarloRadiosityCreateClusterHierarchy(Geometry *world) {
    if ( !world ) {
        return nullptr;
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
monteCarloRadiosityElementRange(StochasticRadiosityElement *elem, int *nbits, niedindex *msb1, niedindex *rmsb2) {
    int nb;
    niedindex b1, b2;

    nb = 0;
    b1 = 0;
    b2 = 0;
    while ( elem->childNumber >= 0 ) {
        nb++;
        b1 = (b1 << 1) | (niedindex) (elem->childNumber & 1);
        b2 = (b2 >> 1) | ((niedindex) (elem->childNumber & 2) << (NBITS - 2));
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
StochasticRadiosityElement *
monteCarloRadiosityRegularSubElementAtPoint(StochasticRadiosityElement *parent, double *u, double *v) {
    StochasticRadiosityElement *child = nullptr;
    double _u = *u, _v = *v;

    if ( parent->isCluster || !parent->regularSubElements ) {
        return nullptr;
    }

    // Have a look at the drawings above to understand what is done exactly
    switch ( parent->numberOfVertices ) {
        case 3:
            if ( _u + _v <= 0.5 ) {
                child = parent->regularSubElements[0];
                *u = _u * 2.0;
                *v = _v * 2.0;
            } else if ( _u > 0.5 ) {
                child = parent->regularSubElements[1];
                *u = (_u - 0.5) * 2.0;
                *v = _v * 2.0;
            } else if ( _v > 0.5 ) {
                child = parent->regularSubElements[2];
                *u = _u * 2.;
                *v = (_v - 0.5) * 2.0;
            } else {
                child = parent->regularSubElements[3];
                *u = (0.5 - _u) * 2.0;
                *v = (0.5 - _v) * 2.0;
            }
            break;
        case 4:
            if ( _v <= 0.5 ) {
                if ( _u < 0.5 ) {
                    child = parent->regularSubElements[0];
                    *u = _u * 2.0;
                } else {
                    child = parent->regularSubElements[1];
                    *u = (_u - 0.5) * 2.0;
                }
                *v = _v * 2.0;
            } else {
                if ( _u < 0.5 ) {
                    child = parent->regularSubElements[2];
                    *u = _u * 2.0;
                } else {
                    child = parent->regularSubElements[3];
                    *u = (_u - 0.5) * 2.0;
                }
                *v = (_v - 0.5) * 2.0;
            }
            break;
        default:
            logFatal(-1, "galerkinElementRegularSubElementAtPoint", "Can handle only triangular or quadrilateral elements");
    }

    return child;
}

/**
Returns the leaf regular sub-element of 'top' at the point (u,v) (uniform
coordinates!). (u,v) is transformed to the coordinates of the corresponding
point on the leaf element. 'top' is a surface element, not a cluster
*/
StochasticRadiosityElement *
monteCarloRadiosityRegularLeafElementAtPoint(StochasticRadiosityElement *top, double *u, double *v) {
    StochasticRadiosityElement *leaf;

    // Find leaf element of 'top' at (u,v)
    leaf = top;
    while ( leaf->regularSubElements ) {
        leaf = monteCarloRadiosityRegularSubElementAtPoint(leaf, u, v);
    }

    return leaf;
}

static Vector3D *
monteCarloRadiosityInstallCoordinate(Vector3D *coord) {
    Vector3D *v = VectorCreate(coord->x, coord->y, coord->z);
    GLOBAL_stochasticRaytracing_hierarchy.coords->add(0, v);
    return v;
}

static Vector3D *
monteCarloRadiosityInstallNormal(Vector3D *norm) {
    Vector3D *v = VectorCreate(norm->x, norm->y, norm->z);
    GLOBAL_stochasticRaytracing_hierarchy.normals->add(0, v);
    return v;
}

static Vector3D *
monteCarloRadiosityInstallTexCoord(Vector3D *texCoord) {
    Vector3D *t = VectorCreate(texCoord->x, texCoord->y, texCoord->z);
    GLOBAL_stochasticRaytracing_hierarchy.texCoords->add(0, t);
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
monteCarloRadiosityNewMidpointVertex(StochasticRadiosityElement *elem, Vertex *v1, Vertex *v2) {
    Vector3D coord, norm, texCoord, *p, *n, *t;

    vectorMidPoint(*(v1->point), *(v2->point), coord);
    p = monteCarloRadiosityInstallCoordinate(&coord);

    if ( v1->normal && v2->normal ) {
        vectorMidPoint(*(v1->normal), *(v2->normal), norm);
        n = monteCarloRadiosityInstallNormal(&norm);
    } else {
        n = &elem->patch->normal;
    }

    if ( v1->texCoord && v2->texCoord ) {
        vectorMidPoint(*(v1->texCoord), *(v2->texCoord), texCoord);
        t = monteCarloRadiosityInstallTexCoord(&texCoord);
    } else {
        t = nullptr;
    }

    return monteCarloRadiosityInstallVertex(p, n, t);
}

/**
Find the neighbouring element of the given element at the edgeNumber-th edge.
The edgeNumber-th edge is the edge connecting the edgeNumber-th vertex to the
(edgeNumber + 1 modulo GLOBAL_statistics_numberOfVertices)-th vertex
*/
static StochasticRadiosityElement *
monteCarloRadiosityElementNeighbour(StochasticRadiosityElement *elem, int edgeNumber) {
    Vertex *from = elem->vertex[edgeNumber];
    Vertex *to = elem->vertex[(edgeNumber + 1) % elem->numberOfVertices];

    for ( int i = 0; to->radiance_data != nullptr && i < to->radiance_data->size(); i++ ) {
        Element *element = to->radiance_data->get(i);
        if ( element->className != ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY ) {
            continue;
        }
        StochasticRadiosityElement *e = (StochasticRadiosityElement *)element;
        if ( e != elem &&
             ((e->numberOfVertices == 3 &&
               ((e->vertex[0] == to && e->vertex[1] == from) ||
                (e->vertex[1] == to && e->vertex[2] == from) ||
                (e->vertex[2] == to && e->vertex[0] == from)))
              || (e->numberOfVertices == 4 &&
                  ((e->vertex[0] == to && e->vertex[1] == from) ||
                   (e->vertex[1] == to && e->vertex[2] == from) ||
                   (e->vertex[2] == to && e->vertex[3] == from) ||
                   (e->vertex[3] == to && e->vertex[0] == from))))) {
            return e;
        }
    }

    return nullptr;
}

Vertex *
monteCarloRadiosityEdgeMidpointVertex(StochasticRadiosityElement *elem, int edgeNumber) {
    Vertex *v = nullptr,
            *to = elem->vertex[(edgeNumber + 1) % elem->numberOfVertices];
    StochasticRadiosityElement *neighbour = monteCarloRadiosityElementNeighbour(elem, edgeNumber);

    if ( neighbour && neighbour->regularSubElements ) {
        // Elem has a neighbour at the edge from 'from' to 'to'. This neighbouring
        // element has been subdivided before, so the edge midpoint vertex already
        // exists: it is the midpoint of the neighbour's edge leading from 'to' to
        // 'from'. This midpoint is a vertex of a regular sub-element of 'neighbour'.
        // Which regular sub-element and which vertex is determined from the diagrams
        // above
        int index = (to == neighbour->vertex[0] ? 0 :
                     (to == neighbour->vertex[1] ? 1 :
                      (to == neighbour->vertex[2] ? 2 :
                       (to == neighbour->vertex[3] ? 3 : -1))));

        switch ( neighbour->numberOfVertices ) {
            case 3:
                switch ( index ) {
                    case 0:
                        v = neighbour->regularSubElements[0]->vertex[1];
                        break;
                    case 1:
                        v = neighbour->regularSubElements[1]->vertex[2];
                        break;

                    case 2:
                        v = neighbour->regularSubElements[2]->vertex[0];
                        break;
                    default:
                        logError("EdgeMidpointVertex", "Invalid vertex index %d", index);
                }
                break;
            case 4:
                switch ( index ) {
                    case 0:
                        v = neighbour->regularSubElements[0]->vertex[1];
                        break;
                    case 1:
                        v = neighbour->regularSubElements[1]->vertex[2];
                        break;
                    case 2:
                        v = neighbour->regularSubElements[3]->vertex[3];
                        break;
                    case 3:
                        v = neighbour->regularSubElements[2]->vertex[0];
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
monteCarloRadiosityNewEdgeMidpointVertex(StochasticRadiosityElement *elem, int edgenr) {
    Vertex *v = monteCarloRadiosityEdgeMidpointVertex(elem, edgenr);
    if ( !v ) {
        // First time we split the edge, create the midpoint vertex
        Vertex *from = elem->vertex[edgenr],
                *to = elem->vertex[(edgenr + 1) % elem->numberOfVertices];
        v = monteCarloRadiosityNewMidpointVertex(elem, from, to);
    }
    return v;
}

static Vector3D
galerkinElementMidpoint(StochasticRadiosityElement *elem) {
    int i;
    vectorSet(elem->midpoint, 0., 0., 0.);
    for ( i = 0; i < elem->numberOfVertices; i++ ) {
        vectorAdd(elem->midpoint, *elem->vertex[i]->point, elem->midpoint);
    }
    vectorScaleInverse((float) elem->numberOfVertices, elem->midpoint, elem->midpoint);

    return elem->midpoint;
}

/**
Only for surface elements
*/
int
monteCarloRadiosityElementIsTextured(StochasticRadiosityElement *elem) {
    Material *mat;
    if ( elem->isCluster ) {
        logFatal(-1, "monteCarloRadiosityElementIsTextured", "this routine should not be called for cluster elements");
        return false;
    }
    mat = elem->patch->surface->material;
    return bsdfIsTextured(mat->bsdf) || edfIsTextured(mat->edf);
}

/**
Uses elem->Rd for surface elements
*/
float
monteCarloRadiosityElementScalarReflectance(StochasticRadiosityElement *elem) {
    float rd;

    if ( elem->isCluster ) {
        return 1.0;
    }

    rd = colorMaximumComponent(elem->Rd);
    if ( rd < EPSILON ) {
        // Avoid divisions by zero
        rd = EPSILON;
    }
    return rd;
}

/**
Computes average reflectance and emittance of a surface sub-element
*/
static void
monteCarloRadiosityElementComputeAverageReflectanceAndEmittance(StochasticRadiosityElement *elem) {
    Patch *patch = elem->patch;
    int i;
    int numberOfSamples;
    int isTextured;
    int nbits;
    niedindex msb1;
    niedindex rmsb2;
    niedindex n;
    COLOR albedo, emittance;
    RayHit hit;
    hitInit(&hit, patch, nullptr, &patch->midpoint, &patch->normal, patch->surface->material, 0.);

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
        patch->uniformPoint(hit.uv.u, hit.uv.v, &hit.point);
        if ( patch->surface->material->bsdf ) {
            sample = bsdfScatteredPower(patch->surface->material->bsdf, &hit, &patch->normal, BRDF_DIFFUSE_COMPONENT);
            colorAdd(albedo, sample, albedo);
        }
        if ( patch->surface->material->edf ) {
            sample = edfEmittance(patch->surface->material->edf, &hit, DIFFUSE_COMPONENT);
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
monteCarloRadiosityInitSurfacePush(StochasticRadiosityElement *parent, StochasticRadiosityElement *child) {
    child->sourceRad = parent->sourceRad;
    pushRadiance(parent, child, parent->radiance, child->radiance);
    pushRadiance(parent, child, parent->unShotRadiance, child->unShotRadiance);
    pushImportance(parent, child, &parent->imp, &child->imp);
    pushImportance(parent, child, &parent->source_imp, &child->source_imp);
    pushImportance(parent, child, &parent->unShotImp, &child->unShotImp);
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
static StochasticRadiosityElement *
monteCarloRadiosityCreateSurfaceSubElement(
        StochasticRadiosityElement *parent,
        int childNumber,
        Vertex *v0,
        Vertex *v1,
        Vertex *v2,
        Vertex *v3)
{
    int i;

    StochasticRadiosityElement *elem = createElement();
    parent->regularSubElements[childNumber] = elem;

    elem->patch = parent->patch;
    elem->numberOfVertices = parent->numberOfVertices;
    elem->vertex[0] = v0;
    elem->vertex[1] = v1;
    elem->vertex[2] = v2;
    elem->vertex[3] = v3;
    for ( i = 0; i < elem->numberOfVertices; i++ ) {
        vertexAttachElement(elem->vertex[i], elem);
    }

    elem->area = 0.25f * parent->area; // Regular elements, regular subdivision
    elem->midpoint = galerkinElementMidpoint(elem);

    elem->parent = parent;
    elem->childNumber = (char)childNumber;
    elem->upTrans = elem->numberOfVertices == 3 ? &GLOBAL_stochasticRaytracing_triangleUpTransform[childNumber] : &GLOBAL_stochasticRaytracing_quadUpTransform[childNumber];

    allocCoefficients(elem);
    stochasticRadiosityClearCoefficients(elem->radiance, elem->basis);
    stochasticRadiosityClearCoefficients(elem->unShotRadiance, elem->basis);
    stochasticRadiosityClearCoefficients(elem->receivedRadiance, elem->basis);
    elem->imp = elem->unShotImp = elem->received_imp = 0.;
    monteCarloRadiosityInitSurfacePush(parent, elem);

    return elem;
}

/**
Create sub-elements: regular subdivision, see drawings above
*/
static StochasticRadiosityElement **
monteCarloRadiosityRegularSubdivideTriangle(StochasticRadiosityElement *element) {
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

    openGlRenderSetColor(&GLOBAL_render_renderOptions.outline_color);
    openGlRenderLine(v0->point, v1->point);
    openGlRenderLine(v1->point, v2->point);
    openGlRenderLine(v2->point, v0->point);
    openGlRenderLine(m0->point, m1->point);
    openGlRenderLine(m1->point, m2->point);
    openGlRenderLine(m2->point, m0->point);

    return element->regularSubElements;
}

static StochasticRadiosityElement **
monteCarloRadiosityRegularSubdivideQuad(StochasticRadiosityElement *element) {
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

    openGlRenderSetColor(&GLOBAL_render_renderOptions.outline_color);
    openGlRenderLine(v0->point, v1->point);
    openGlRenderLine(v1->point, v2->point);
    openGlRenderLine(v2->point, v3->point);
    openGlRenderLine(v3->point, v0->point);
    openGlRenderLine(m0->point, m2->point);
    openGlRenderLine(m1->point, m3->point);

    return element->regularSubElements;
}

/**
Subdivides given triangle or quadrangle into four sub-elements if not yet
done so before. Returns the list of created sub-elements
*/
StochasticRadiosityElement **
monteCarloRadiosityRegularSubdivideElement(StochasticRadiosityElement *element) {
    if ( element->regularSubElements ) {
        return element->regularSubElements;
    }

    if ( element->isCluster ) {
        logFatal(-1, "galerkinElementRegularSubDivide", "Cannot regularly subdivide cluster elements");
        return nullptr;
    }

    if ( element->patch->jacobian ) {
        static int wgiv = false;
        if ( !wgiv ) {
            logWarning("galerkinElementRegularSubDivide",
                       "irregular quadrilateral patches are not correctly handled (but you probably won't notice it)");
        }
        wgiv = true;
    }

    // Create the sub-elements
    element->regularSubElements = new StochasticRadiosityElement *[4];
    switch ( element->numberOfVertices ) {
        case 3:
            monteCarloRadiosityRegularSubdivideTriangle(element);
            break;
        case 4:
            monteCarloRadiosityRegularSubdivideQuad(element);
            break;
        default:
            logFatal(-1, "galerkinElementRegularSubDivide", "invalid element: not 3 or 4 vertices");
    }
    return element->regularSubElements;
}

static void
monteCarloRadiosityDestroyElement(StochasticRadiosityElement *elem) {
    int i;
    if ( elem == nullptr ) {
        return;
    }

    if ( elem->isCluster ) {
        GLOBAL_stochasticRaytracing_hierarchy.nr_clusters--;
    }
    GLOBAL_stochasticRaytracing_hierarchy.nr_elements--;

    if ( elem->irregularSubElements ) {
        for ( int j = 0; elem->irregularSubElements != nullptr && j < elem->irregularSubElements->size(); j++ ) {
            delete elem->irregularSubElements->get(j);
        }
        delete elem->irregularSubElements;
        elem->irregularSubElements = nullptr;
    }

    if ( elem->regularSubElements ) {
        delete[] elem->regularSubElements;
    }
    for ( i = 0; i < elem->numberOfVertices; i++ ) {
        for ( int j = 0; elem->vertex[i]->radiance_data != nullptr && j < elem->vertex[i]->radiance_data->size(); j++ ) {
            delete elem->vertex[i]->radiance_data->get(j);
        }
        delete elem->vertex[i]->radiance_data;
        elem->vertex[i]->radiance_data = nullptr;
    }
    disposeCoefficients(elem);
    delete elem;
}

static void
monteCarloRadiosityDestroySurfaceElement(StochasticRadiosityElement *elem) {
    if ( !elem ) {
        return;
    }
    if ( elem->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++ ) {
            monteCarloRadiosityDestroySurfaceElement(elem->regularSubElements[i]);
        }
    }
    monteCarloRadiosityDestroyElement(elem);
}

void
monteCarloRadiosityDestroyToplevelSurfaceElement(StochasticRadiosityElement *elem) {
    monteCarloRadiosityDestroySurfaceElement(elem);
}

void
monteCarloRadiosityDestroyClusterHierarchy(StochasticRadiosityElement *top) {
    if ( top == nullptr || !top->isCluster ) {
        return;
    }
    for ( int i = 0; top->irregularSubElements != nullptr && i < top->irregularSubElements->size(); i++ ) {
        StochasticRadiosityElement *element = top->irregularSubElements->get(i);
        if ( element->isCluster ) {
            monteCarloRadiosityDestroyClusterHierarchy(element);
        }
    }
    monteCarloRadiosityDestroyElement(top);
}

/**
Returns true if there are children elements and false if top is nullptr or a leaf element
*/
int
monteCarloRadiosityForAllChildrenElements(StochasticRadiosityElement *top, void (*func)(StochasticRadiosityElement *)) {
    if ( !top ) {
        return false;
    }

    if ( top->isCluster ) {
        for ( int i = 0; top->irregularSubElements != nullptr && i < top->irregularSubElements->size(); i++ ) {
            func(top->irregularSubElements->get(i));
        }
        return true;
    } else if ( top->regularSubElements ) {
        if ( top->regularSubElements != nullptr ) {
            for ( int i = 0; i < 4; i++ ) {
                func(top->regularSubElements[i]);
            }
        }
        return true;
    } else {
        // Leaf element
        return false;
    }
}

void
monteCarloRadiosityForAllLeafElements(StochasticRadiosityElement *top, void (*func)(StochasticRadiosityElement *)) {
    if ( !top ) {
        return;
    }

    if ( top->isCluster ) {
        for ( int i = 0; top->irregularSubElements != nullptr && i < top->irregularSubElements->size(); i++ ) {
            monteCarloRadiosityForAllLeafElements(top->irregularSubElements->get(i), func);
        }
    } else if ( top->regularSubElements ) {
        if ( top->regularSubElements != nullptr ) {
            for ( int i = 0; i < 4; i++ ) {
                monteCarloRadiosityForAllLeafElements(top->regularSubElements[i], func);
            }
        }
    } else {
        func(top);
    }
}

static void
monteCarloRadiosityForAllSurfaceLeafsRecursive(
    StochasticRadiosityElement *element,
    void (*func)(StochasticRadiosityElement *))
{
    if ( element->regularSubElements == nullptr ) {
        // Trivial case
        func(element);
    } else {
        // Recursive case
        for ( int i = 0; i < 4; i++ ) {
            monteCarloRadiosityForAllSurfaceLeafsRecursive(element->regularSubElements[i], func);
        }
    }
}

void
monteCarloRadiosityForAllSurfaceLeafs(
    StochasticRadiosityElement *top,
    void (*func)(StochasticRadiosityElement *))
{
    monteCarloRadiosityForAllSurfaceLeafsRecursive(top, func);
}

/**
Returns true if elem is a leaf element
*/
int
monteCarloRadiosityElementIsLeaf(StochasticRadiosityElement *elem) {
    return (!elem->regularSubElements && !elem->irregularSubElements);
}

/**
Computes and fills in a bounding box for the element
*/
float *
monteCarloRadiosityElementBounds(StochasticRadiosityElement *elem, float *bounds) {
    if ( elem->isCluster ) {
        boundsCopy(elem->geometry->bounds, bounds);
    } else if ( !elem->upTrans ) {
            elem->patch->patchBounds(bounds);
    } else {
        boundsInit(bounds);
        for ( int i = 0; i < elem->numberOfVertices; i++ ) {
            Vertex *v = elem->vertex[i];
            boundsEnlargePoint(bounds, v->point);
        }
    }
    return bounds;
}

StochasticRadiosityElement *
monteCarloRadiosityClusterChildContainingElement(StochasticRadiosityElement *parent, StochasticRadiosityElement *descendant) {
    while ( descendant && descendant->parent != parent ) {
        descendant = descendant->parent;
    }
    if ( !descendant ) {
        logFatal(-1, "monteCarloRadiosityClusterChildContainingElement", "descendant is not a descendant of parent");
    }
    return descendant;
}

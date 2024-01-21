#include "common/error.h"
#include "shared/render.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/clustergalerkincpp.h"
#include "GALERKIN/GalerkinElement.h"

// Element render modes, additive
#define OUTLINE 1
#define FLAT 2
#define GOURAUD 4
#define STRONG 8

static int globalNumberOfElements = 0;
static int globalNumberOfClusters = 0;

GalerkinElement::GalerkinElement(): regularSubElements() {

}

void
printPre(int level) {
    if ( level <= 0 ) {
        printf("- ");
    } else if ( level == 1 ) {
        printf(" . ");
    } else {
        for ( int i = 0; i < level; i++ ) {
            printf("  ");
        }
        printf("->");
    }
}

void
GalerkinElement::printRegularHierarchy(int level) {
    printPre(level);
    printf("%d\n", id);
    if ( regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++ ) {
            regularSubElements[i]->printRegularHierarchy(level + 1);
        }
    }
}

/**
Orientation and position of regular sub-elements is fully determined by the
following transformations. A uniform mapping of parameter domain to the
elements is supposed (i.o.w. use patchUniformPoint() to map (u,v) coordinates
on the toplevel element to a 3D point on the patch). The sub-elements
have equal area. No explicit Jacobian stuff needed to compute integrals etc..
etc
*/

/**
Up-transforms for regular quadrilateral sub-elements:

  (v)

   1 +---------+---------+
     |         |         |
     |         |         |
     | 3       | 4       |
 0.5 +---------+---------+
     |         |         |
     |         |         |
     | 1       | 2       |
   0 +---------+---------+
     0        0.5        1   (u)
*/
Matrix2x2 GLOBAL_galerkin_QuadUpTransformMatrix[4] = {
    // South-west [0, 0.5] x [0, 0.5]
    {{{0.5, 0.0}, {0.0, 0.5}}, {0.0, 0.0}},

    // South-east: [0.5, 1] x [0, 0.5]
    {{{0.5, 0.0}, {0.0, 0.5}}, {0.5, 0.0}},

    // North-west: [0, 0.5] x [0.5, 1]
    {{{0.5, 0.0}, {0.0, 0.5}}, {0.0, 0.5}},

    // North-east: [0.5, 1] x [0.5, 1]
    {{{0.5, 0.0}, {0.0, 0.5}}, {0.5, 0.5}},
};

/**
Up-transforms for regular triangular sub-elements:

 (v)

  1 +
    | \
    |   \
    |     \
    | 3     \
0.5 +---------+
    | \     4 | \
    |   \     |   \
    |     \   |     \
    | 1     \ | 2     \
  0 +---------+---------+
    0        0.5        1  (u)
*/
Matrix2x2 GLOBAL_galerkin_TriangularUpTransformMatrix[4] = {
    // Left: (0, 0), (0.5, 0), (0, 0.5)
    {{{0.5,  0.0}, {0.0, 0.5}},  {0.0, 0.0}},

    // Right: (0.5, 0), (1, 0), (0.5, 0.5)
    {{{0.5,  0.0}, {0.0, 0.5}},  {0.5, 0.0}},

    // Top: (0, 0.5), (0.5, 0.5), (0, 1)
    {{{0.5,  0.0}, {0.0, 0.5}},  {0.0, 0.5}},

    // Middle: (0.5, 0.5), (0, 0.5), (0.5, 0)
    {{{-0.5, 0.0}, {0.0, -0.5}}, {0.5, 0.5}},
};

/**
Returns the total number of elements in use
*/
int
galerkinElementGetNumberOfElements() {
    return globalNumberOfElements;
}

int
galerkinElementGetNumberOfClusters() {
    return globalNumberOfClusters;
}

int
galerkinElementGetNumberOfSurfaceElements() {
    return globalNumberOfElements - globalNumberOfClusters;
}

/**
Re-allocates storage for the coefficients to represent radiance, received radiance
and un-shot radiance on the element
*/
void
galerkinElementReAllocCoefficients(GalerkinElement *element) {
    COLOR *radiance;
    COLOR *receivedRadiance;
    COLOR *unShotRadiance;
    char basisSize = 0;

    if ( isCluster(element) ) {
        // We always use a constant basis on cluster elements
        basisSize = 1;
    } else {
        switch ( GLOBAL_galerkin_state.basis_type ) {
            case CONSTANT:
                basisSize = 1;
                break;
            case LINEAR:
                basisSize = 3;
                break;
            case QUADRATIC:
                basisSize = 6;
                break;
            case CUBIC:
                basisSize = 10;
                break;
            default:
                logFatal(-1, "galerkinElementReAllocCoefficients", "Invalid basis type %d", GLOBAL_galerkin_state.basis_type);
        }
    }

    radiance = new COLOR[basisSize];
    clusterGalerkinClearCoefficients(radiance, basisSize);
    if ( element->radiance ) {
        clusterGalerkinCopyCoefficients(radiance, element->radiance, MIN(element->basisSize, basisSize));
        free(element->radiance);
    }
    element->radiance = radiance;

    receivedRadiance = new COLOR[basisSize];
    clusterGalerkinClearCoefficients(receivedRadiance, basisSize);
    if ( element->receivedRadiance ) {
        clusterGalerkinCopyCoefficients(receivedRadiance, element->receivedRadiance, MIN(element->basisSize, basisSize));
        free(element->receivedRadiance);
    }
    element->receivedRadiance = receivedRadiance;

    if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL ) {
        unShotRadiance = new COLOR[basisSize];
        clusterGalerkinClearCoefficients(unShotRadiance, basisSize);
        if ( !isCluster(element)) {
            if ( element->unShotRadiance ) {
                clusterGalerkinCopyCoefficients(unShotRadiance, element->unShotRadiance,
                                                MIN(element->basisSize, basisSize));
                free(element->unShotRadiance);
            } else if ( element->patch->surface ) {
                unShotRadiance[0] = SELFEMITTED_RADIANCE(element->patch);
            }
        }
        element->unShotRadiance = unShotRadiance;
    } else {
        if ( element->unShotRadiance ) {
            free(element->unShotRadiance);
        }
        element->unShotRadiance = (COLOR *) nullptr;
    }

    element->basisSize = basisSize;
    if ( element->basisUsed > element->basisSize ) {
        element->basisUsed = element->basisSize;
    }
}

/**
Use either galerkinElementCreateTopLevel() or CreateRegularSubelement()
*/
static GalerkinElement *
galerkinElementCreate() {
    GalerkinElement *newElement = new GalerkinElement();

    colorClear(newElement->Ed);
    colorClear(newElement->Rd);
    newElement->id = globalNumberOfElements + 1; // Let the IDs start from 1, not 0
    newElement->radiance = newElement->receivedRadiance = newElement->unShotRadiance = (COLOR *) nullptr;
    newElement->potential.f = newElement->receivedPotential.f = newElement->unShotPotential.f = 0.;
    newElement->interactions = InteractionListCreate();
    newElement->patch = nullptr;
    newElement->geom = nullptr;
    newElement->parent = nullptr;
    newElement->regularSubElements = nullptr;
    newElement->irregularSubElements = ElementListCreate();
    newElement->uptrans = nullptr;
    newElement->area = 0.0;
    newElement->flags = 0x00;
    newElement->childnr = -1; // Means: "not a regular sub-element"
    newElement->basisSize = 0;
    newElement->basisUsed = 0;
    newElement->numberOfPatches = 1; // Correct for surface elements, it will be computed later for clusters
    newElement->minimumArea = HUGE;
    newElement->tmp = 0;
    newElement->bsize = 0.0; // Correct eq. blocker size will be computer later on

    globalNumberOfElements++;
    return newElement;
}

/**
Creates the toplevel element for the patch
*/
GalerkinElement *
galerkinElementCreateTopLevel(Patch *patch) {
    GalerkinElement *element = galerkinElementCreate();
    element->patch = patch;
    element->minimumArea = element->area = patch->area;
    element->bsize = 2.0f * (float)std::sqrt(element->area / M_PI);
    element->directPotential.f = patch->directPotential;

    element->Rd = patchAverageNormalAlbedo(patch, BRDF_DIFFUSE_COMPONENT);
    if ( patch->surface && patch->surface->material &&
         patch->surface->material->edf ) {
        element->flags |= IS_LIGHT_SOURCE;
        element->Ed = patchAverageEmittance(patch, DIFFUSE_COMPONENT);
        colorScaleInverse(M_PI, element->Ed, element->Ed);
    }

    patch->radianceData = element;
    galerkinElementReAllocCoefficients(element);

    return element;
}

/**
Creates a cluster element for the given geometry
The average projected area still needs to be determined
*/
GalerkinElement *
galerkinElementCreateCluster(Geometry *geometry) {
    GalerkinElement *element = galerkinElementCreate();
    element->geom = geometry;
    element->area = 0.0; // Needs to be computed after the whole cluster hierarchy has been constructed
    element->flags |= IS_CLUSTER;
    galerkinElementReAllocCoefficients(element);

    colorSetMonochrome(element->Rd, 1.);

    // Whether the cluster contains light sources or not is also determined after the hierarchy is constructed
    globalNumberOfClusters++;
    return element;
}

/**
Regularly subdivides the given element. A pointer to an array of
4 pointers to sub-elements is returned.

Only applicable to surface elements.
*/
GalerkinElement **
galerkinElementRegularSubDivide(GalerkinElement *element) {
    if ( isCluster(element) ) {
        logFatal(-1, "galerkinElementRegularSubDivide", "Cannot regularly subdivide cluster elements");
        return nullptr;
    }

    if ( element->regularSubElements != nullptr ) {
        return element->regularSubElements;
    }

    GalerkinElement **subElement = new GalerkinElement *[4];

    for ( int i = 0; i < 4; i++ ) {
        subElement[i] = galerkinElementCreate();
        subElement[i]->patch = element->patch;
        subElement[i]->parent = element;
        subElement[i]->uptrans =
                element->patch->numberOfVertices == 3 ? &GLOBAL_galerkin_TriangularUpTransformMatrix[i] : &GLOBAL_galerkin_QuadUpTransformMatrix[i];
        subElement[i]->area = 0.25f * element->area;  /* we always use a uniform mapping */
        subElement[i]->bsize = 2.0f * (float)std::sqrt(subElement[i]->area / M_PI);
        subElement[i]->childnr = (char)i;
        galerkinElementReAllocCoefficients(subElement[i]);

        basisGalerkinPush(element, element->radiance, subElement[i], subElement[i]->radiance);

        subElement[i]->potential.f = element->potential.f;
        subElement[i]->directPotential.f = element->directPotential.f;

        if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL ) {
            basisGalerkinPush(element, element->unShotRadiance, subElement[i], subElement[i]->unShotRadiance);
            subElement[i]->unShotPotential.f = element->unShotPotential.f;
        }

        subElement[i]->flags |= (element->flags & IS_LIGHT_SOURCE);

        subElement[i]->Rd = element->Rd;
        subElement[i]->Ed = element->Ed;

        renderSetColor(&GLOBAL_render_renderOptions.outline_color);
        galerkinElementDrawOutline(subElement[i]);
    }

    element->regularSubElements = subElement;
    return subElement;
}

static void
galerkinElementDestroy(GalerkinElement *element) {
    InteractionListIterate(element->interactions, interactionDestroy);
    InteractionListDestroy(element->interactions);

    if ( element->radiance ) {
        delete[] element->radiance;
    }
    if ( element->receivedRadiance ) {
        delete[] element->receivedRadiance;
    }
    if ( element->unShotRadiance ) {
        delete[] element->unShotRadiance;
    }
    delete element;
    globalNumberOfElements--;
}

/**
Destroys the toplevel surface element and it's sub-elements (recursive)
*/
void
galerkinElementDestroyTopLevel(GalerkinElement *element) {
    if ( element == nullptr ) {
        return;
    }

    if ( element->regularSubElements ) {
        ITERATE_REGULAR_SUBELEMENTS(element, galerkinElementDestroyTopLevel);
        free(element->regularSubElements);
    }

    if ( element->irregularSubElements ) {
        ITERATE_IRREGULAR_SUBELEMENTS(element, galerkinElementDestroyTopLevel);
        ElementListDestroy(element->irregularSubElements);
    }

    galerkinElementDestroy(element);
}

/**
Destroys the cluster element, not recursive
*/
void
galerkinElementDestroyCluster(GalerkinElement *element) {
    galerkinElementDestroy(element);
    globalNumberOfClusters--;
}

/**
Prints the element data to the file 'out'
*/
void
galerkinElementPrint(FILE *out, GalerkinElement *element) {
    fprintf(out, "Element %d: ", element->id);
    if ( isCluster(element)) {
        fprintf(out, "cluster element, ");
    } else {
        fprintf(out, "belongs to patch ID %d, ",
                element->patch ? element->patch->id : -1);
    }
    fprintf(out, "parent element ID = %d, child nr = %d\n",
            element->parent ? element->parent->id : -1,
            element->childnr);
    fprintf(out, "area = %g, bsize = %g, basis size = %d\n",
            element->area, element->bsize, element->basisSize);

    if ( element->uptrans ) {
        fprintf(out, "up-transform:\n");
        element->uptrans->print(out);
    } else {
        fprintf(out, "no up-transform.\n");
    }

    if ( element->radiance ) {
        fprintf(out, "radiance = ");
        printCoefficients(out, element->radiance, element->basisSize);
        fprintf(out, "\n");
    } else {
        fprintf(out, "No radiance coefficients.\n");
    }

    if ( element->receivedRadiance ) {
        fprintf(out, "received_radiance = ");
        printCoefficients(out, element->receivedRadiance, element->basisSize);
        fprintf(out, "\n");
    } else {
        fprintf(out, "No received_radiance coefficients.\n");
    }

    if ( element->unShotRadiance ) {
        fprintf(out, "unshot_radiance = ");
        printCoefficients(out, element->unShotRadiance, element->basisSize);
        fprintf(out, "\n");
    } else {
        fprintf(out, "No unshot_radiance coefficients.\n");
    }

    fprintf(out, "potential.f = %g, received_potential.f = %g, unshot_potential.f = %g, directPotential = %g\n",
            element->potential.f, element->receivedPotential.f, element->unShotPotential.f,
            element->directPotential.f);

    fprintf(out, "interactions_created = %s",
            element->flags & INTERACTIONS_CREATED ? "TRUE" : "FALSE");

    if ( element->interactions ) {
        fprintf(out, ", interactions:\n");
        InteractionListIterate1B(element->interactions, interactionPrint, out);
    } else {
        fprintf(out, ", no interactions.\n");
    }

    if ( element->regularSubElements ) {
        int i;
        fprintf(out, "regular subelements: ");
        for ( i = 0; i < 4; i++ ) {
            fprintf(out, "%d, ", element->regularSubElements[i]->id);
        }
        fprintf(out, "\n");
    } else {
        fprintf(out, "No regular subelements.\n");
    }

    if ( element->irregularSubElements ) {
        ELEMENTLIST *elist;
        fprintf(out, "irregular subelements: ");
        for ( elist = element->irregularSubElements; elist; elist = elist->next ) {
            fprintf(out, "%d, ", elist->element->id);
        }
        fprintf(out, "\n");
    } else {
        fprintf(out, "No irregular subelements.\n");
    }
}

/**
Prints the patch id and the child numbers of the element and its parents
*/
void
galerkinElementPrintId(FILE *out, GalerkinElement *elem) {
    if ( isCluster(elem)) {
        fprintf(out, "geom %d cluster", elem->geom->id);
    } else {
        if ( elem->uptrans ) {
            galerkinElementPrintId(out, elem->parent);
            fprintf(out, "%d", elem->childnr + 1);
        } else {
            fprintf(out, "patch %d element ", elem->patch->id);
        }
    }
}

/**
Computes the transform relating a surface element to the toplevel element
in the patch hierarchy by concatenating the up-transforms of the element
and all parent elements. If the element is a toplevel element,
(Matrix4x4 *)nullptr is
returned and nothing is filled in in xf (no transform is necessary
to transform positions on the element to the corresponding point on the toplevel
element). In the other case, the composed transform is filled in in xf and
xf (pointer to the transform) is returned
*/
Matrix2x2 *
galerkinElementToTopTransform(GalerkinElement *element, Matrix2x2 *xf) {
    // Top level element: no transform necessary to transform to top
    if ( !element->uptrans ) {
        return (Matrix2x2 *) nullptr;
    }

    *xf = *element->uptrans;
    while ( (element = element->parent) && element->uptrans ) {
        PRECONCAT_TRANSFORM2D(*element->uptrans, *xf, *xf);
    }

    return xf;
}

/**
Determines the regular sub-element at point (u,v) of the given parent
element. Returns the parent element itself if there are no regular sub-elements.
The point is transformed to the corresponding point on the sub-element
*/
GalerkinElement *
galerkinElementRegularSubElementAtPoint(GalerkinElement *parent, double *u, double *v) {
    GalerkinElement *child = nullptr;
    double _u = *u;
    double _v = *v;

    if ( isCluster(parent) || !parent->regularSubElements ) {
        return parent;
    }

    // Have a look at the drawings above to understand what is done exactly
    switch ( parent->patch->numberOfVertices ) {
        case 3:
            if ( _u + _v <= 0.5 ) {
                child = parent->regularSubElements[0];
                *u = _u * 2.;
                *v = _v * 2.;
            } else if ( _u > 0.5 ) {
                child = parent->regularSubElements[1];
                *u = (_u - 0.5) * 2.;
                *v = _v * 2.;
            } else if ( _v > 0.5 ) {
                child = parent->regularSubElements[2];
                *u = _u * 2.;
                *v = (_v - 0.5) * 2.;
            } else {
                child = parent->regularSubElements[3];
                *u = (0.5 - _u) * 2.;
                *v = (0.5 - _v) * 2.;
            }
            break;
        case 4:
            if ( _v <= 0.5 ) {
                if ( _u < 0.5 ) {
                    child = parent->regularSubElements[0];
                    *u = _u * 2.;
                } else {
                    child = parent->regularSubElements[1];
                    *u = (_u - 0.5) * 2.;
                }
                *v = _v * 2.;
            } else {
                if ( _u < 0.5 ) {
                    child = parent->regularSubElements[2];
                    *u = _u * 2.;
                } else {
                    child = parent->regularSubElements[3];
                    *u = (_u - 0.5) * 2.;
                }
                *v = (_v - 0.5) * 2.;
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
GalerkinElement *
galerkinElementRegularLeafAtPoint(GalerkinElement *top, double *u, double *v) {
    GalerkinElement *leaf;

    /* find leaf element of 'top' at (u,v) */
    leaf = top;
    while ( leaf->regularSubElements ) {
        leaf = galerkinElementRegularSubElementAtPoint(leaf, u, v);
    }

    return leaf;
}

/**
Computes the vertices of a surface element (3 or 4 vertices) or
cluster element (8 vertices). The number of vertices is returned
*/
int
galerkinElementVertices(GalerkinElement *elem, Vector3D *p) {
    if ( isCluster(elem)) {
        BOUNDINGBOX vol;

        galerkinElementBounds(elem, vol);

        VECTORSET(p[0], vol[MIN_X], vol[MIN_Y], vol[MIN_Z]);
        VECTORSET(p[1], vol[MIN_X], vol[MIN_Y], vol[MAX_Z]);
        VECTORSET(p[2], vol[MIN_X], vol[MAX_Y], vol[MIN_Z]);
        VECTORSET(p[3], vol[MIN_X], vol[MAX_Y], vol[MAX_Z]);
        VECTORSET(p[4], vol[MAX_X], vol[MIN_Y], vol[MIN_Z]);
        VECTORSET(p[5], vol[MAX_X], vol[MIN_Y], vol[MAX_Z]);
        VECTORSET(p[6], vol[MAX_X], vol[MAX_Y], vol[MIN_Z]);
        VECTORSET(p[7], vol[MAX_X], vol[MAX_Y], vol[MAX_Z]);

        return 8;
    } else {
        Matrix2x2 topTrans;
        Vector2D uv;

        if ( elem->uptrans ) {
            galerkinElementToTopTransform(elem, &topTrans);
        }

        uv.u = 0.;
        uv.v = 0.;
        if ( elem->uptrans ) {
            transformPoint2D(topTrans, uv, uv);
        }
        patchUniformPoint(elem->patch, uv.u, uv.v, &p[0]);

        uv.u = 1.;
        uv.v = 0.;
        if ( elem->uptrans ) transformPoint2D(topTrans, uv, uv);
        patchUniformPoint(elem->patch, uv.u, uv.v, &p[1]);

        if ( elem->patch->numberOfVertices == 4 ) {
            uv.u = 1.;
            uv.v = 1.;
            if ( elem->uptrans ) transformPoint2D(topTrans, uv, uv);
            patchUniformPoint(elem->patch, uv.u, uv.v, &p[2]);

            uv.u = 0.;
            uv.v = 1.;
            if ( elem->uptrans ) transformPoint2D(topTrans, uv, uv);
            patchUniformPoint(elem->patch, uv.u, uv.v, &p[3]);
        } else {
            uv.u = 0.;
            uv.v = 1.;
            if ( elem->uptrans ) transformPoint2D(topTrans, uv, uv);
            patchUniformPoint(elem->patch, uv.u, uv.v, &p[2]);

            VECTORSET(p[3], 0., 0., 0.);
        }

        return elem->patch->numberOfVertices;
    }
}

/**
Computes the midpoint of the element
*/
Vector3D
galerkinElementMidPoint(GalerkinElement *elem) {
    Vector3D c;

    if ( isCluster(elem)) {
        float *bbox = geomBounds(elem->geom);

        VECTORSET(c,
                  (bbox[MIN_X] + bbox[MAX_X]) / 2.0f,
                  (bbox[MIN_Y] + bbox[MAX_Y]) / 2.0f,
                  (bbox[MIN_Z] + bbox[MAX_Z]) / 2.0f);
    } else {
        Vector3D p[4];
        int i, nrverts;

        nrverts = galerkinElementVertices(elem, p);

        VECTORSET(c, 0., 0., 0.);
        for ( i = 0; i < nrverts; i++ )
                VECTORADD (c, p[i], c);
        VECTORSCALE((1.0f / (float) nrverts), c, c);
    }

    return c;
}

/**
Computes a bounding box for the element
*/
float *
galerkinElementBounds(GalerkinElement *elem, float *bounds) {
    if ( isCluster(elem)) {
        boundsCopy(geomBounds(elem->geom), bounds);
    } else {
        Vector3D p[4];
        int i, nrverts;

        nrverts = galerkinElementVertices(elem, p);

        boundsInit(bounds);
        for ( i = 0; i < nrverts; i++ ) {
            boundsEnlargePoint(bounds, &p[i]);
        }
    }

    return bounds;
}

/**
Computes a polygon description for shaft culling for the surface
element. Cannot be used for clusters
*/
POLYGON *
galerkinElementPolygon(GalerkinElement *elem, POLYGON *poly) {
    int i;

    if ( isCluster(elem)) {
        logFatal(-1, "galerkinElementPolygon", "Cannot use this function for cluster elements");
        return (POLYGON *) nullptr;
    }

    poly->normal = elem->patch->normal;
    poly->plane_constant = elem->patch->planeConstant;
    poly->index = elem->patch->index;
    poly->nrvertices = galerkinElementVertices(elem, poly->vertex);

    boundsInit(poly->bounds);
    for ( i = 0; i < poly->nrvertices; i++ ) {
        boundsEnlargePoint(poly->bounds, &poly->vertex[i]);
    }

    return poly;
}

static void
galerkinElementDraw(GalerkinElement *element, int mode) {
    Vector3D p[4];
    int numberOfVertices;

    if ( isCluster(element)) {
        if ( mode & OUTLINE || mode & STRONG ) {
            renderBounds(geomBounds(element->geom));
        }
        return;
    }

    numberOfVertices = galerkinElementVertices(element, p);

    if ( mode & FLAT ) {
        RGB color;
        COLOR rho = REFLECTIVITY(element->patch);

        if ( GLOBAL_galerkin_state.use_ambient_radiance ) {
            COLOR rad_vis;
            colorProduct(rho, GLOBAL_galerkin_state.ambient_radiance, rad_vis);
            colorAdd(rad_vis, element->radiance[0], rad_vis);
            radianceToRgb(rad_vis, &color);
        } else {
            radianceToRgb(element->radiance[0], &color);
        }
        renderSetColor(&color);
        renderPolygonFlat(numberOfVertices, p);
    } else if ( mode & GOURAUD ) {
        RGB vertcol[4];
        COLOR vertrad[4];
        int i;

        if ( numberOfVertices == 3 ) {
            vertrad[0] = basisGalerkinRadianceAtPoint(element, element->radiance, 0.0, 0.0);
            vertrad[1] = basisGalerkinRadianceAtPoint(element, element->radiance, 1.0, 0.0);
            vertrad[2] = basisGalerkinRadianceAtPoint(element, element->radiance, 0.0, 1.0);
        } else {
            vertrad[0] = basisGalerkinRadianceAtPoint(element, element->radiance, 0.0, 0.0);
            vertrad[1] = basisGalerkinRadianceAtPoint(element, element->radiance, 1.0, 0.0);
            vertrad[2] = basisGalerkinRadianceAtPoint(element, element->radiance, 1.0, 1.0);
            vertrad[3] = basisGalerkinRadianceAtPoint(element, element->radiance, 0.0, 1.0);
        }

        if ( GLOBAL_galerkin_state.use_ambient_radiance ) {
            COLOR rho = REFLECTIVITY(element->patch), ambient;

            colorProduct(rho, GLOBAL_galerkin_state.ambient_radiance, ambient);
            for ( i = 0; i < numberOfVertices; i++ ) {
                colorAdd(vertrad[i], ambient, vertrad[i]);
            }
        }

        for ( i = 0; i < numberOfVertices; i++ ) {
            radianceToRgb(vertrad[i], &vertcol[i]);
        }

        renderPolygonGouraud(numberOfVertices, p, vertcol);
    }

    // Modifies the positions, that's why it comes last
    if ( mode & OUTLINE ) {
        int i;

        for ( i = 0; i < numberOfVertices; i++ ) {
            // Move the point a bit closer the eye point to avoid aliasing
            Vector3D d;
            VECTORSUBTRACT(GLOBAL_camera_mainCamera.eyePosition, p[i], d);
            VECTORSUMSCALED(p[i], 0.01, d, p[i]);
        }

        renderSetColor(&GLOBAL_render_renderOptions.outline_color);
        renderLine(&p[0], &p[1]);
        renderLine(&p[1], &p[2]);
        if ( numberOfVertices == 3 ) {
            renderLine(&p[2], &p[0]);
        } else {
            renderLine(&p[2], &p[3]);
            renderLine(&p[3], &p[0]);
        }

        if ( mode & STRONG ) {
            if ( numberOfVertices == 3 ) {
                Vector3D d, pt;

                VECTORSUBTRACT(p[2], p[1], d);

                for ( i = 1; i < 4; i++ ) {
                    VECTORSUMSCALED(p[1], i * 0.25, d, pt);
                    renderLine(&p[0], &pt);
                }
            } else if ( numberOfVertices == 4 ) {
                Vector3D d1, d2, p1, p2;

                VECTORSUBTRACT(p[1], p[0], d1);
                VECTORSUBTRACT(p[3], p[2], d2);


                for ( i = 0; i < 5; i++ ) {
                    VECTORSUMSCALED(p[0], i * 0.25, d1, p1);
                    VECTORSUMSCALED(p[2], (1.0 - i * 0.25), d2, p2);
                    renderLine(&p1, &p2);
                }
            }
        }
    }
}

/**
Draws element outline in the current outline color
*/
void
galerkinElementDrawOutline(GalerkinElement *elem) {
    galerkinElementDraw(elem, OUTLINE);
}

/**
Renders a surface element flat shaded based on its radiance
*/
void
galerkinElementRender(GalerkinElement *elem) {
    int renderCode = 0;

    if ( GLOBAL_render_renderOptions.draw_outlines ) {
        renderCode |= OUTLINE;
    }

    if ( GLOBAL_render_renderOptions.smooth_shading ) {
        renderCode |= GOURAUD;
    } else {
        renderCode |= FLAT;
    }

    galerkinElementDraw(elem, renderCode);
}

/**
Call func for each leaf element of top
*/
void
forAllLeafElements(GalerkinElement *top, void (*func)(GalerkinElement *)) {
    ForAllIrregularSubelements(child, top)
                {
                    forAllLeafElements(child, func);
                }
    EndForAll;

    ForAllRegularSubelements(child, top)
                {
                    forAllLeafElements(child, func);
                }
    EndForAll;

    if ( !top->irregularSubElements && !top->regularSubElements ) {
        func(top);
    }
}

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "render/render.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/clustergalerkincpp.h"
#include "GALERKIN/GalerkinElement.h"
#include "render/opengl.h"

// Element render modes, additive
#define OUTLINE 1
#define FLAT 2
#define GOURAUD 4
#define STRONG 8

static int globalNumberOfElements = 0;
static int globalNumberOfClusters = 0;

GalerkinElement::GalerkinElement():
    radiance(),
    receivedRadiance(),
    unShotRadiance(),
    potential(),
    receivedPotential(),
    unShotPotential(),
    directPotential(),
    interactions(),
    parent(),
    regularSubElements(),
    irregularSubElements(),
    upTrans(),
    area(),
    minimumArea(),
    bsize(),
    numberOfPatches(),
    tmp(),
    childNumber(),
    basisSize(),
    basisUsed(),
    flags()
{
    className = ElementTypes::ELEMENT_GALERKIN;
    irregularSubElements = new java::ArrayList<GalerkinElement *>();
}

GalerkinElement::~GalerkinElement() {
    if ( irregularSubElements != nullptr ) {
        delete irregularSubElements;
    }
}

/**
Orientation and position of regular sub-elements is fully determined by the
following transformations. A uniform mapping of parameter domain to the
elements is supposed (i.o.w. use uniformPoint() to map (u,v) coordinates
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
        switch ( GLOBAL_galerkin_state.basisType ) {
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
                logFatal(-1, "galerkinElementReAllocCoefficients", "Invalid basis type %d", GLOBAL_galerkin_state.basisType);
        }
    }

    radiance = new COLOR[basisSize];
    clusterGalerkinClearCoefficients(radiance, basisSize);
    if ( element->radiance ) {
        clusterGalerkinCopyCoefficients(radiance, element->radiance, charMin(element->basisSize, basisSize));
        free(element->radiance);
    }
    element->radiance = radiance;

    receivedRadiance = new COLOR[basisSize];
    clusterGalerkinClearCoefficients(receivedRadiance, basisSize);
    if ( element->receivedRadiance ) {
        clusterGalerkinCopyCoefficients(receivedRadiance, element->receivedRadiance,
                                        charMin(element->basisSize, basisSize));
        free(element->receivedRadiance);
    }
    element->receivedRadiance = receivedRadiance;

    if ( GLOBAL_galerkin_state.iteration_method == SOUTH_WELL ) {
        unShotRadiance = new COLOR[basisSize];
        clusterGalerkinClearCoefficients(unShotRadiance, basisSize);
        if ( !isCluster(element)) {
            if ( element->unShotRadiance ) {
                clusterGalerkinCopyCoefficients(unShotRadiance, element->unShotRadiance,
                                                charMin(element->basisSize, basisSize));
                free(element->unShotRadiance);
            } else if ( element->patch->surface ) {
                unShotRadiance[0] = element->patch->radianceData->Ed;
            }
        }
        element->unShotRadiance = unShotRadiance;
    } else {
        if ( element->unShotRadiance ) {
            free(element->unShotRadiance);
        }
        element->unShotRadiance = nullptr;
    }

    element->basisSize = basisSize;
    if ( element->basisUsed > element->basisSize ) {
        element->basisUsed = element->basisSize;
    }
}

/**
Use either galerkinElementCreateTopLevel() or CreateRegularSubElement()
*/
static GalerkinElement *
galerkinElementCreate() {
    GalerkinElement *newElement = new GalerkinElement();

    colorClear(newElement->Ed);
    colorClear(newElement->Rd);
    newElement->id = globalNumberOfElements + 1; // Let the IDs start from 1, not 0
    newElement->radiance = nullptr;
    newElement->receivedRadiance = nullptr;
    newElement->unShotRadiance = nullptr;
    newElement->potential = 0.0f;
    newElement->receivedPotential = 0.0f;
    newElement->unShotPotential = 0.0f;
    newElement->interactions = nullptr; // New list
    newElement->patch = nullptr;
    newElement->geom = nullptr;
    newElement->parent = nullptr;
    newElement->regularSubElements = nullptr;
    newElement->irregularSubElements = nullptr; // New list
    newElement->upTrans = nullptr;
    newElement->area = 0.0;
    newElement->flags = 0x00;
    newElement->childNumber = -1; // Means: "not a regular sub-element"
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
    element->directPotential = patch->directPotential;

    element->Rd = patch->averageNormalAlbedo(BRDF_DIFFUSE_COMPONENT);
    if ( patch->surface && patch->surface->material &&
         patch->surface->material->edf ) {
        element->flags |= IS_LIGHT_SOURCE;
        element->Ed = patch->averageEmittance(DIFFUSE_COMPONENT);
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
        subElement[i]->upTrans =
                element->patch->numberOfVertices == 3 ? &GLOBAL_galerkin_TriangularUpTransformMatrix[i] : &GLOBAL_galerkin_QuadUpTransformMatrix[i];
        subElement[i]->area = 0.25f * element->area;  /* we always use a uniform mapping */
        subElement[i]->bsize = 2.0f * (float)std::sqrt(subElement[i]->area / M_PI);
        subElement[i]->childNumber = (char)i;
        galerkinElementReAllocCoefficients(subElement[i]);

        basisGalerkinPush(element, element->radiance, subElement[i], subElement[i]->radiance);

        subElement[i]->potential = element->potential;
        subElement[i]->directPotential = element->directPotential;

        if ( GLOBAL_galerkin_state.iteration_method == SOUTH_WELL ) {
            basisGalerkinPush(element, element->unShotRadiance, subElement[i], subElement[i]->unShotRadiance);
            subElement[i]->unShotPotential = element->unShotPotential;
        }

        subElement[i]->flags |= (element->flags & IS_LIGHT_SOURCE);

        subElement[i]->Rd = element->Rd;
        subElement[i]->Ed = element->Ed;

        openGlRenderSetColor(&GLOBAL_render_renderOptions.outline_color);
        galerkinElementDrawOutline(subElement[i]);
    }

    element->regularSubElements = subElement;
    return subElement;
}

static void
galerkinElementDestroy(GalerkinElement *element) {
    for ( InteractionListNode *window = element->interactions; window != nullptr; window = window->next ) {
        interactionDestroy(window->interaction);
    }

    listDestroy((LIST *)element->interactions);

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

    if ( element->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++) {
            galerkinElementDestroyTopLevel((element)->regularSubElements[i]);
        }
        free(element->regularSubElements);
    }

    if ( element->irregularSubElements != nullptr ) {
        for ( int i = 0; i < element->irregularSubElements->size(); i++ ) {
            galerkinElementDestroyTopLevel(element->irregularSubElements->get(i));
        }
        delete element->irregularSubElements;
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
Prints the patch id and the child numbers of the element and its parents
*/
void
galerkinElementPrintId(FILE *out, GalerkinElement *elem) {
    if ( isCluster(elem)) {
        fprintf(out, "geom %d cluster", elem->geom->id);
    } else {
        if ( elem->upTrans ) {
            galerkinElementPrintId(out, elem->parent);
            fprintf(out, "%d", elem->childNumber + 1);
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
    if ( !element->upTrans ) {
        return nullptr;
    }

    *xf = *element->upTrans;
    while ( (element = element->parent) && element->upTrans ) {
        matrix2DPreConcatTransform(*element->upTrans, *xf, *xf);
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
galerkinElementVertices(GalerkinElement *elem, Vector3D *p, int n) {
    if ( isCluster(elem) ) {
        BoundingBox vol;

        galerkinElementBounds(elem, vol);

        for ( int i = 0; i < n; i++ ) {
            vectorSet(p[i], vol[MIN_X], vol[MIN_Y], vol[MIN_Z]);
        }

        return 8;
    } else {
        Matrix2x2 topTrans{};
        Vector2D uv;

        if ( elem->upTrans ) {
            galerkinElementToTopTransform(elem, &topTrans);
        }

        uv.u = 0.0;
        uv.v = 0.0;
        if ( elem->upTrans ) {
            transformPoint2D(topTrans, uv, uv);
        }
        elem->patch->uniformPoint(uv.u, uv.v, &p[0]);

        uv.u = 1.0;
        uv.v = 0.0;
        if ( elem->upTrans ) transformPoint2D(topTrans, uv, uv);
        elem->patch->uniformPoint(uv.u, uv.v, &p[1]);

        if ( elem->patch->numberOfVertices == 4 ) {
            uv.u = 1.0;
            uv.v = 1.0;
            if ( elem->upTrans ) transformPoint2D(topTrans, uv, uv);
            elem->patch->uniformPoint(uv.u, uv.v, &p[2]);

            uv.u = 0.0;
            uv.v = 1.0;
            if ( elem->upTrans ) transformPoint2D(topTrans, uv, uv);
            elem->patch->uniformPoint(uv.u, uv.v, &p[3]);
        } else {
            uv.u = 0.0;
            uv.v = 1.0;
            if ( elem->upTrans ) transformPoint2D(topTrans, uv, uv);
            elem->patch->uniformPoint(uv.u, uv.v, &p[2]);

            vectorSet(p[3], 0.0, 0.0, 0.0);
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

        vectorSet(c,
                  (bbox[MIN_X] + bbox[MAX_X]) / 2.0f,
                  (bbox[MIN_Y] + bbox[MAX_Y]) / 2.0f,
                  (bbox[MIN_Z] + bbox[MAX_Z]) / 2.0f);
    } else {
        Vector3D p[8];
        int i;
        int numberOfVertices;

        numberOfVertices = galerkinElementVertices(elem, p, 4);

        vectorSet(c, 0.0, 0.0, 0.0);
        for ( i = 0; i < numberOfVertices; i++ ) {
            vectorAdd(c, p[i], c);
        }
        vectorScale((1.0f / (float) numberOfVertices), c, c);
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
        int i;
        int numberOfVertices;

        numberOfVertices = galerkinElementVertices(elem, p, 4);

        boundsInit(bounds);
        for ( i = 0; i < numberOfVertices; i++ ) {
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
galerkinElementPolygon(GalerkinElement *elem, POLYGON *polygon) {
    if ( isCluster(elem) ) {
        logFatal(-1, "galerkinElementPolygon", "Cannot use this function for cluster elements");
        return nullptr;
    }

    polygon->normal = elem->patch->normal;
    polygon->planeConstant = elem->patch->planeConstant;
    polygon->index = (unsigned char)elem->patch->index;
    polygon->numberOfVertices = galerkinElementVertices(elem, polygon->vertex, polygon->numberOfVertices);

    boundsInit(polygon->bounds);
    for ( int i = 0; i < polygon->numberOfVertices; i++ ) {
        boundsEnlargePoint(polygon->bounds, &polygon->vertex[i]);
    }

    return polygon;
}

static void
galerkinElementDraw(GalerkinElement *element, int mode) {
    Vector3D p[4];
    int numberOfVertices;

    if ( isCluster(element) ) {
        if ( mode & OUTLINE || mode & STRONG ) {
            renderBounds(geomBounds(element->geom));
        }
        return;
    }

    numberOfVertices = galerkinElementVertices(element, p, 4);

    if ( mode & FLAT ) {
        RGB color{};
        COLOR rho = element->patch->radianceData->Rd;

        if ( GLOBAL_galerkin_state.use_ambient_radiance ) {
            COLOR rad_vis;
            colorProduct(rho, GLOBAL_galerkin_state.ambient_radiance, rad_vis);
            colorAdd(rad_vis, element->radiance[0], rad_vis);
            radianceToRgb(rad_vis, &color);
        } else {
            radianceToRgb(element->radiance[0], &color);
        }
        openGlRenderSetColor(&color);
        openGlRenderPolygonFlat(numberOfVertices, p);
    } else if ( mode & GOURAUD ) {
        RGB vertColor[4];
        COLOR vertRadiosity[4];
        int i;

        if ( numberOfVertices == 3 ) {
            vertRadiosity[0] = basisGalerkinRadianceAtPoint(element, element->radiance, 0.0, 0.0);
            vertRadiosity[1] = basisGalerkinRadianceAtPoint(element, element->radiance, 1.0, 0.0);
            vertRadiosity[2] = basisGalerkinRadianceAtPoint(element, element->radiance, 0.0, 1.0);
        } else {
            vertRadiosity[0] = basisGalerkinRadianceAtPoint(element, element->radiance, 0.0, 0.0);
            vertRadiosity[1] = basisGalerkinRadianceAtPoint(element, element->radiance, 1.0, 0.0);
            vertRadiosity[2] = basisGalerkinRadianceAtPoint(element, element->radiance, 1.0, 1.0);
            vertRadiosity[3] = basisGalerkinRadianceAtPoint(element, element->radiance, 0.0, 1.0);
        }

        if ( GLOBAL_galerkin_state.use_ambient_radiance ) {
            COLOR reflectivity = element->patch->radianceData->Rd;
            COLOR ambient;

            colorProduct(reflectivity, GLOBAL_galerkin_state.ambient_radiance, ambient);
            for ( i = 0; i < numberOfVertices; i++ ) {
                colorAdd(vertRadiosity[i], ambient, vertRadiosity[i]);
            }
        }

        for ( i = 0; i < numberOfVertices; i++ ) {
            radianceToRgb(vertRadiosity[i], &vertColor[i]);
        }

            openGlRenderPolygonGouraud(numberOfVertices, p, vertColor);
    }

    // Modifies the positions, that's why it comes last
    if ( mode & OUTLINE ) {
        int i;

        for ( i = 0; i < numberOfVertices; i++ ) {
            // Move the point a bit closer the eye point to avoid aliasing
            Vector3D d;
            vectorSubtract(GLOBAL_camera_mainCamera.eyePosition, p[i], d);
            vectorSumScaled(p[i], 0.01, d, p[i]);
        }

        openGlRenderSetColor(&GLOBAL_render_renderOptions.outline_color);
        openGlRenderLine(&p[0], &p[1]);
        openGlRenderLine(&p[1], &p[2]);
        if ( numberOfVertices == 3 ) {
            openGlRenderLine(&p[2], &p[0]);
        } else {
            openGlRenderLine(&p[2], &p[3]);
            openGlRenderLine(&p[3], &p[0]);
        }

        if ( mode & STRONG ) {
            if ( numberOfVertices == 3 ) {
                Vector3D d, pt;

                vectorSubtract(p[2], p[1], d);

                for ( i = 1; i < 4; i++ ) {
                    vectorSumScaled(p[1], i * 0.25, d, pt);
                    openGlRenderLine(&p[0], &pt);
                }
            } else if ( numberOfVertices == 4 ) {
                Vector3D d1, d2, p1, p2;

                    vectorSubtract(p[1], p[0], d1);
                    vectorSubtract(p[3], p[2], d2);


                for ( i = 0; i < 5; i++ ) {
                    vectorSumScaled(p[0], i * 0.25, d1, p1);
                    vectorSumScaled(p[2], (1.0 - i * 0.25), d2, p2);
                    openGlRenderLine(&p1, &p2);
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

    if ( GLOBAL_render_renderOptions.drawOutlines ) {
        renderCode |= OUTLINE;
    }

    if ( GLOBAL_render_renderOptions.smoothShading ) {
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
    for ( int i = 0; top->irregularSubElements != nullptr && i < top->irregularSubElements->size(); i++ ) {
        forAllLeafElements(top->irregularSubElements->get(i), func);
    }

    if ( top->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++ ) {
            forAllLeafElements(top->regularSubElements[i], func);
        }
    }

    if ( !top->irregularSubElements && !top->regularSubElements ) {
        func(top);
    }
}

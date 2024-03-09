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
Private inner constructor, Use either galerkinElementCreateTopLevel() or CreateRegularSubElement()
*/
GalerkinElement::GalerkinElement():
    potential(),
    receivedPotential(),
    unShotPotential(),
    directPotential(),
    minimumArea(),
    blockerSize(),
    numberOfPatches(),
    tmp(),
    childNumber(),
    basisSize(),
    basisUsed()
{
    className = ElementTypes::ELEMENT_GALERKIN;
    irregularSubElements = new java::ArrayList<Element *>();
    interactions = new java::ArrayList<Interaction *>();

    colorClear(Ed);
    colorClear(Rd);
    id = globalNumberOfElements + 1; // Let the IDs start from 1, not 0
    radiance = nullptr;
    receivedRadiance = nullptr;
    unShotRadiance = nullptr;
    potential = 0.0f;
    receivedPotential = 0.0f;
    unShotPotential = 0.0f;
    patch = nullptr;
    geometry = nullptr;
    parent = nullptr;
    regularSubElements = nullptr;
    irregularSubElements = nullptr; // New list
    upTrans = nullptr;
    area = 0.0;
    childNumber = -1; // Means: "not a regular sub-element"
    basisSize = 0;
    basisUsed = 0;
    numberOfPatches = 1; // Correct for surface elements, it will be computed later for clusters
    minimumArea = HUGE;
    tmp = 0;
    blockerSize = 0.0; // Correct eq. blocker size will be computer later on

    globalNumberOfElements++;
}

/**
Creates the toplevel element for the patch
*/
GalerkinElement::GalerkinElement(Patch *parameterPatch): GalerkinElement() {
    patch = parameterPatch;
    minimumArea = area = patch->area;
    blockerSize = 2.0f * (float)std::sqrt(area / M_PI);
    directPotential = patch->directPotential;

    Rd = patch->averageNormalAlbedo(BRDF_DIFFUSE_COMPONENT);
    if ( patch->surface && patch->surface->material &&
         patch->surface->material->edf ) {
        flags |= IS_LIGHT_SOURCE_MASK;
        Ed = patch->averageEmittance(DIFFUSE_COMPONENT);
        colorScaleInverse(M_PI, Ed, Ed);
    }

    patch->radianceData = this;
    reAllocCoefficients();
}

/**
Creates a cluster element for the given geometry
The average projected area still needs to be determined
*/
GalerkinElement::GalerkinElement(Geometry *parameterGeometry): GalerkinElement() {
    geometry = parameterGeometry;
    area = 0.0; // Needs to be computed after the whole cluster hierarchy has been constructed
    flags |= IS_CLUSTER_MASK;
    reAllocCoefficients();

    colorSetMonochrome(Rd, 1.0);

    // Whether the cluster contains light sources or not is also determined after the hierarchy is constructed
    globalNumberOfClusters++;
}

GalerkinElement::~GalerkinElement() {
    if ( regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++) {
            delete regularSubElements[i];
        }
        delete regularSubElements;
    }

    if ( irregularSubElements != nullptr ) {
        for ( int i = 0; i < irregularSubElements->size(); i++ ) {
            delete irregularSubElements->get(i);
        }
        delete irregularSubElements;
    }

    for ( int i = 0; interactions != nullptr && i < interactions->size(); i++ ) {
        interactionDestroy(interactions->get(i));
    }
    delete interactions;

    if ( radiance ) {
        delete[] radiance;
    }
    if ( receivedRadiance ) {
        delete[] receivedRadiance;
    }
    if ( unShotRadiance ) {
        delete[] unShotRadiance;
    }
    globalNumberOfElements--;
    if ( isCluster() ) {
        globalNumberOfClusters--;
    }
}

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
GalerkinElement::reAllocCoefficients() {
    char localBasisSize = 0;

    if ( isCluster() ) {
        // We always use a constant basis on cluster elements
        localBasisSize = 1;
    } else {
        switch ( GLOBAL_galerkin_state.basisType ) {
            case CONSTANT:
                localBasisSize = 1;
                break;
            case LINEAR:
                localBasisSize = 3;
                break;
            case QUADRATIC:
                localBasisSize = 6;
                break;
            case CUBIC:
                localBasisSize = 10;
                break;
            default:
                logFatal(-1, "galerkinElementReAllocCoefficients", "Invalid basis type %d", GLOBAL_galerkin_state.basisType);
        }
    }

    COLOR *defaultRadiance = new COLOR[localBasisSize];
    clusterGalerkinClearCoefficients(defaultRadiance, localBasisSize);
    if ( radiance ) {
        clusterGalerkinCopyCoefficients(defaultRadiance, radiance, charMin(basisSize, localBasisSize));
        delete radiance;
    }
    radiance = defaultRadiance;

    COLOR *defaultReceivedRadiance = new COLOR[localBasisSize];
    clusterGalerkinClearCoefficients(defaultReceivedRadiance, localBasisSize);
    if ( receivedRadiance ) {
        clusterGalerkinCopyCoefficients(defaultReceivedRadiance, receivedRadiance,
                                        charMin(basisSize, localBasisSize));
        delete receivedRadiance;
    }
    receivedRadiance = defaultReceivedRadiance;

    if ( GLOBAL_galerkin_state.iteration_method == SOUTH_WELL ) {
        COLOR *defaultUnShotRadiance = new COLOR[localBasisSize];
        clusterGalerkinClearCoefficients(defaultUnShotRadiance, localBasisSize);
        if ( !isCluster() ) {
            if ( unShotRadiance ) {
                clusterGalerkinCopyCoefficients(defaultUnShotRadiance, unShotRadiance,
                                                charMin(basisSize, localBasisSize));
                delete unShotRadiance;
            } else if ( patch->surface ) {
                    defaultUnShotRadiance[0] = patch->radianceData->Ed;
            }
        }
        unShotRadiance = defaultUnShotRadiance;
    } else {
        if ( unShotRadiance ) {
            delete unShotRadiance;
        }
        unShotRadiance = nullptr;
    }

    basisSize = localBasisSize;
    if ( basisUsed > basisSize ) {
        basisUsed = basisSize;
    }
}

/**
Regularly subdivides the given element. A pointer to an array of
4 pointers to sub-elements is returned.

Only applicable to surface elements.
*/
GalerkinElement **
GalerkinElement::regularSubDivide() {
    if ( isCluster() ) {
        logFatal(-1, "galerkinElementRegularSubDivide", "Cannot regularly subdivide cluster elements");
        return nullptr;
    }

    if ( regularSubElements != nullptr ) {
        return (GalerkinElement **)regularSubElements;
    }

    GalerkinElement **subElement = new GalerkinElement *[4];

    for ( int i = 0; i < 4; i++ ) {
        subElement[i] = new GalerkinElement();
        subElement[i]->patch = patch;
        subElement[i]->parent = this;
        subElement[i]->upTrans =
                patch->numberOfVertices == 3 ? &GLOBAL_galerkin_TriangularUpTransformMatrix[i] : &GLOBAL_galerkin_QuadUpTransformMatrix[i];
        subElement[i]->area = 0.25f * area;  /* we always use a uniform mapping */
        subElement[i]->blockerSize = 2.0f * (float)std::sqrt(subElement[i]->area / M_PI);
        subElement[i]->childNumber = (char)i;
        subElement[i]->reAllocCoefficients();

        basisGalerkinPush(this, radiance, subElement[i], subElement[i]->radiance);

        subElement[i]->potential = potential;
        subElement[i]->directPotential = directPotential;

        if ( GLOBAL_galerkin_state.iteration_method == SOUTH_WELL ) {
            basisGalerkinPush(this, unShotRadiance, subElement[i], subElement[i]->unShotRadiance);
            subElement[i]->unShotPotential = unShotPotential;
        }

        subElement[i]->flags |= (flags & IS_LIGHT_SOURCE_MASK);

        subElement[i]->Rd = Rd;
        subElement[i]->Ed = Ed;

        openGlRenderSetColor(&GLOBAL_render_renderOptions.outline_color);
        subElement[i]->drawOutline();
    }

    regularSubElements = (Element **)subElement;
    return subElement;
}

/**
Determines the regular sub-element at point (u,v) of the given element
element. Returns the element element itself if there are no regular sub-elements.
The point is transformed to the corresponding point on the sub-element
*/
GalerkinElement *
GalerkinElement::regularSubElementAtPoint(double *u, double *v) {
    Element *childElement = nullptr;
    double _u = *u;
    double _v = *v;

    if ( isCluster() || !regularSubElements ) {
        return this;
    }

    // Have a look at the drawings above to understand what is done exactly
    switch ( patch->numberOfVertices ) {
        case 3:
            if ( _u + _v <= 0.5 ) {
                childElement = regularSubElements[0];
                *u = _u * 2.0;
                *v = _v * 2.0;
            } else if ( _u > 0.5 ) {
                childElement = regularSubElements[1];
                *u = (_u - 0.5) * 2.0;
                *v = _v * 2.0;
            } else if ( _v > 0.5 ) {
                childElement = regularSubElements[2];
                *u = _u * 2.0;
                *v = (_v - 0.5) * 2.0;
            } else {
                childElement = regularSubElements[3];
                *u = (0.5 - _u) * 2.0;
                *v = (0.5 - _v) * 2.0;
            }
            break;
        case 4:
            if ( _v <= 0.5 ) {
                if ( _u < 0.5 ) {
                    childElement = regularSubElements[0];
                    *u = _u * 2.0;
                } else {
                    childElement = regularSubElements[1];
                    *u = (_u - 0.5) * 2.0;
                }
                *v = _v * 2.0;
            } else {
                if ( _u < 0.5 ) {
                    childElement = regularSubElements[2];
                    *u = _u * 2.0;
                } else {
                    childElement = regularSubElements[3];
                    *u = (_u - 0.5) * 2.0;
                }
                *v = (_v - 0.5) * 2.0;
            }
            break;
        default:
            logFatal(-1, "galerkinElementRegularSubElementAtPoint", "Can handle only triangular or quadrilateral elements");
    }

    return (GalerkinElement *)childElement;
}

/**
Returns the leaf regular sub-element of 'element' at the point (u,v) (uniform
coordinates!). (u,v) is transformed to the coordinates of the corresponding
point on the leaf element. 'element' is a surface element, not a cluster
*/
GalerkinElement *
GalerkinElement::regularLeafAtPoint(double *u, double *v) {
    // Find leaf element of 'element' at (u, v)
    GalerkinElement *leaf = this;
    while ( leaf->regularSubElements ) {
        leaf = leaf->regularSubElementAtPoint(u, v);
    }
    return leaf;
}

/**
Computes the vertices of a surface element (3 or 4 vertices) or
cluster element (8 vertices). The number of vertices is returned
*/
int
GalerkinElement::vertices(Vector3D *p, int n) {
    if ( isCluster() ) {
        BoundingBox boundingBox;

        bounds(&boundingBox);

        for ( int i = 0; i < n; i++ ) {
            vectorSet(p[i], boundingBox.coordinates[MIN_X], boundingBox.coordinates[MIN_Y], boundingBox.coordinates[MIN_Z]);
        }

        return 8;
    } else {
        Matrix2x2 topTrans{};
        Vector2D uv;

        if ( upTrans ) {
            topTransform(&topTrans);
        }

        uv.u = 0.0;
        uv.v = 0.0;
        if ( upTrans ) {
            transformPoint2D(topTrans, uv, uv);
        }
        patch->uniformPoint(uv.u, uv.v, &p[0]);

        uv.u = 1.0;
        uv.v = 0.0;
        if ( upTrans ) transformPoint2D(topTrans, uv, uv);
        patch->uniformPoint(uv.u, uv.v, &p[1]);

        if ( patch->numberOfVertices == 4 ) {
            uv.u = 1.0;
            uv.v = 1.0;
            if ( upTrans ) transformPoint2D(topTrans, uv, uv);
            patch->uniformPoint(uv.u, uv.v, &p[2]);

            uv.u = 0.0;
            uv.v = 1.0;
            if ( upTrans ) {
                transformPoint2D(topTrans, uv, uv);
            }
            patch->uniformPoint(uv.u, uv.v, &p[3]);
        } else {
            uv.u = 0.0;
            uv.v = 1.0;
            if ( upTrans ) {
                transformPoint2D(topTrans, uv, uv);
            }
            patch->uniformPoint(uv.u, uv.v, &p[2]);

            vectorSet(p[3], 0.0, 0.0, 0.0);
        }

        return patch->numberOfVertices;
    }
}

/**
Computes the midpoint of the element
*/
Vector3D
GalerkinElement::midPoint() {
    Vector3D c;

    if ( isCluster() ) {
        vectorSet(c,
                  (geomBounds(geometry).coordinates[MIN_X] + geomBounds(geometry).coordinates[MAX_X]) / 2.0f,
                  (geomBounds(geometry).coordinates[MIN_Y] + geomBounds(geometry).coordinates[MAX_Y]) / 2.0f,
                  (geomBounds(geometry).coordinates[MIN_Z] + geomBounds(geometry).coordinates[MAX_Z]) / 2.0f);
    } else {
        Vector3D p[8];
        int numberOfVertices = vertices(p, 4);

        vectorSet(c, 0.0, 0.0, 0.0);
        for ( int i = 0; i < numberOfVertices; i++ ) {
            vectorAdd(c, p[i], c);
        }
        vectorScale((1.0f / (float) numberOfVertices), c, c);
    }

    return c;
}

/**
Computes a bounding box for the element
*/
BoundingBox *
GalerkinElement::bounds(BoundingBox *boundingBox) {
    if ( isCluster() ) {
        boundsCopy(geomBounds(geometry).coordinates, boundingBox->coordinates);
    } else {
        Vector3D p[4];
        int i;
        int numberOfVertices;

        numberOfVertices = vertices(p, 4);

        for ( i = 0; i < numberOfVertices; i++ ) {
            boundsEnlargePoint(boundingBox->coordinates, &p[i]);
        }
    }

    return boundingBox;
}

/**
Computes a polygon description for shaft culling for the surface
element. Cannot be used for clusters
*/
POLYGON *
GalerkinElement::polygon(POLYGON *polygon) {
    if ( isCluster() ) {
        logFatal(-1, "galerkinElementPolygon", "Cannot use this function for cluster elements");
        return nullptr;
    }

    polygon->normal = patch->normal;
    polygon->planeConstant = patch->planeConstant;
    polygon->index = (unsigned char)patch->index;
    polygon->numberOfVertices = vertices(polygon->vertex, polygon->numberOfVertices);

    for ( int i = 0; i < polygon->numberOfVertices; i++ ) {
        boundsEnlargePoint(polygon->bounds.coordinates, &polygon->vertex[i]);
    }

    return polygon;
}

void
GalerkinElement::draw(int mode) {
    Vector3D p[4];
    int numberOfVertices;

    if ( isCluster() ) {
        if ( mode & OUTLINE || mode & STRONG ) {
            renderBounds(geomBounds(geometry));
        }
        return;
    }

    numberOfVertices = vertices(p, 4);

    if ( mode & FLAT ) {
        RGB color{};
        COLOR rho = patch->radianceData->Rd;

        if ( GLOBAL_galerkin_state.use_ambient_radiance ) {
            COLOR rad_vis;
            colorProduct(rho, GLOBAL_galerkin_state.ambient_radiance, rad_vis);
            colorAdd(rad_vis, radiance[0], rad_vis);
            radianceToRgb(rad_vis, &color);
        } else {
            radianceToRgb(radiance[0], &color);
        }
        openGlRenderSetColor(&color);
        openGlRenderPolygonFlat(numberOfVertices, p);
    } else if ( mode & GOURAUD ) {
        RGB vertColor[4];
        COLOR vertRadiosity[4];
        int i;

        if ( numberOfVertices == 3 ) {
            vertRadiosity[0] = basisGalerkinRadianceAtPoint(this, radiance, 0.0, 0.0);
            vertRadiosity[1] = basisGalerkinRadianceAtPoint(this, radiance, 1.0, 0.0);
            vertRadiosity[2] = basisGalerkinRadianceAtPoint(this, radiance, 0.0, 1.0);
        } else {
            vertRadiosity[0] = basisGalerkinRadianceAtPoint(this, radiance, 0.0, 0.0);
            vertRadiosity[1] = basisGalerkinRadianceAtPoint(this, radiance, 1.0, 0.0);
            vertRadiosity[2] = basisGalerkinRadianceAtPoint(this, radiance, 1.0, 1.0);
            vertRadiosity[3] = basisGalerkinRadianceAtPoint(this, radiance, 0.0, 1.0);
        }

        if ( GLOBAL_galerkin_state.use_ambient_radiance ) {
            COLOR reflectivity = patch->radianceData->Rd;
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
                Vector3D d;
                Vector3D pt;

                vectorSubtract(p[2], p[1], d);

                for ( i = 1; i < 4; i++ ) {
                    vectorSumScaled(p[1], i * 0.25, d, pt);
                    openGlRenderLine(&p[0], &pt);
                }
            } else if ( numberOfVertices == 4 ) {
                Vector3D d1;
                Vector3D d2;
                Vector3D p1;
                Vector3D p2;

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
GalerkinElement::drawOutline() {
    draw(OUTLINE);
}

/**
Renders a surface element flat shaded based on its radiance
*/
void
GalerkinElement::render() {
    int renderCode = 0;

    if ( GLOBAL_render_renderOptions.drawOutlines ) {
        renderCode |= OUTLINE;
    }

    if ( GLOBAL_render_renderOptions.smoothShading ) {
        renderCode |= GOURAUD;
    } else {
        renderCode |= FLAT;
    }

    draw(renderCode);
}

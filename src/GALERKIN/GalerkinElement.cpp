#include "java/lang/Character.h"
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/numericalAnalysis/QuadCubatureRule.h"
#include "render/render.h"
#include "render/opengl.h"
#include "tonemap/ToneMap.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/GalerkinElement.h"

static int globalNumberOfElements = 0;
static int globalNumberOfClusters = 0;

/**
Orientation and position of regular sub-elements is fully determined by the following transformations.
A uniform mapping of parameter domain to the elements is supposed (in other words use uniformPoint() to map
(u,v) coordinates on the toplevel element to a 3D point on the patch). The sub-elements
have equal area. No explicit Jacobian stuff needed to compute integrals etc.
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
Matrix2x2 globalQuadToParentTransformMatrix[4] = {
    // 1: South-west [0, 0.5] x [0, 0.5]
    {
        {
            {0.5, 0.0},
            {0.0, 0.5}
        },
        {0.0, 0.0}
    },

    // 2: South-east [0.5, 1] x [0, 0.5]
    {
        {
            {0.5, 0.0},
            {0.0, 0.5}
        },
        {0.5, 0.0}
    },

    // 3: North-west [0, 0.5] x [0.5, 1]
    {
        {
            {0.5, 0.0},
            {0.0, 0.5}
        },
        {0.0, 0.5}
    },

    // 4: North-east [0.5, 1] x [0.5, 1]
    {
        {
            {0.5, 0.0},
            {0.0, 0.5}
        },
        {0.5, 0.5}
    }
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
Matrix2x2 globalTriangleToParentTransformMatrix[4] = {
    // 1: Left [0, 0], [0.5, 0], [0, 0.5]
    {
        {
            {0.5,  0.0},
            {0.0, 0.5}
        },
        {0.0, 0.0}
    },

    // 2: Right [0.5, 0], [1, 0], [0.5, 0.5]
    {
        {
            {0.5,  0.0},
            {0.0, 0.5}
        },
        {0.5, 0.0}
    },

    // 3: Top [0, 0.5], [0.5, 0.5], [0, 1]
    {
        {
            {0.5, 0.0},
            {0.0, 0.5}
        },
        {0.0, 0.5}
    },

    // 4: Middle [0.5, 0.5], [0, 0.5], [0.5, 0]
    {
        {
            {-0.5, 0.0},
            {0.0, -0.5}
        },
        {0.5, 0.5}
    }
};

/**
Private inner constructor, Use either galerkinElementCreateTopLevel() or CreateRegularSubElement()
*/
GalerkinElement::GalerkinElement(GalerkinState *inGalerkinState):
    Element()
{
    className = ElementTypes::ELEMENT_GALERKIN;

    interactions = new java::ArrayList<Interaction *>();
    galerkinState = inGalerkinState;

    id = globalNumberOfElements + 1; // Let the IDs start from 1, not 0
    radiance = nullptr;
    receivedRadiance = nullptr;
    unShotRadiance = nullptr;
    potential = 0.0f;
    receivedPotential = 0.0f;
    unShotPotential = 0.0f;
    directPotential = 0.0f;
    patch = nullptr;
    geometry = nullptr;
    parent = nullptr;
    regularSubElements = nullptr;
    irregularSubElements = nullptr; // New list
    transformToParent = nullptr;
    area = 0.0;
    childNumber = GalerkinElementRenderMode::NOT_A_REGULAR_SUB_ELEMENT;
    basisSize = 0;
    basisUsed = 0;
    numberOfPatches = 1; // Correct for surface elements, it will be computed later for clusters
    minimumArea = Numeric::HUGE_FLOAT_VALUE;
    scratchVisibilityUsageCounter = 0;
    blockerSize = 0.0; // Correct eq. blocker size will be computer later on

    globalNumberOfElements++;
}

/**
Creates the toplevel element for the patch
*/
GalerkinElement::GalerkinElement(Patch *parameterPatch, GalerkinState *inGalerkinState):
    GalerkinElement(inGalerkinState)
{
    patch = parameterPatch;
    minimumArea = area = patch->area;
    blockerSize = 2.0f * (float)java::Math::sqrt(area / M_PI);
    directPotential = patch->directPotential;

    Rd = patch->averageNormalAlbedo(BRDF_DIFFUSE_COMPONENT);
    if ( patch->material != nullptr && patch->material->getEdf() != nullptr ) {
        flags |= ElementFlags::IS_LIGHT_SOURCE_MASK;
        Ed = patch->averageEmittance(DIFFUSE_COMPONENT);
        Ed.scaleInverse((float)M_PI, Ed);
    }

    patch->radianceData = this;
    reAllocCoefficients();
}

/**
Creates a cluster element for the given geometry
The average projected area still needs to be determined
*/
GalerkinElement::GalerkinElement(Geometry *inGeometry, GalerkinState *inGalerkinState):
    GalerkinElement(inGalerkinState)
{
    geometry = inGeometry;
    area = 0.0; // Needs to be computed after the whole cluster hierarchy has been constructed
    flags |= ElementFlags::IS_CLUSTER_MASK;
    reAllocCoefficients();

    Rd.setMonochrome(1.0);

    // Whether the cluster contains light sources or not is also determined after the hierarchy is constructed
    globalNumberOfClusters++;
}

GalerkinElement::~GalerkinElement() {
    for ( int i = 0; interactions != nullptr && i < interactions->size(); i++ ) {
        delete interactions->get(i);
    }
    delete interactions;

    if ( regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++) {
            delete regularSubElements[i];
        }
        delete[] regularSubElements;
    }

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
GalerkinElement::getNumberOfElements() {
    return globalNumberOfElements;
}

int
GalerkinElement::getNumberOfClusters() {
    return globalNumberOfClusters;
}

int
GalerkinElement::getNumberOfSurfaceElements() {
    return globalNumberOfElements - globalNumberOfClusters;
}

/**
Re-allocates storage for the coefficients to represent radiance, received radiance
and un-shot radiance on the element
*/
void
GalerkinElement::reAllocCoefficients() {
    char localBasisSize;

    if ( isCluster() ) {
        // We always use a constant basis on cluster elements
        localBasisSize = 1;
    } else {
        switch ( galerkinState->basisType ) {
            case GalerkinBasisType::GALERKIN_CONSTANT:
                localBasisSize = 1;
                break;
            case GalerkinBasisType::GALERKIN_LINEAR:
                localBasisSize = 3;
                break;
            case GalerkinBasisType::GALERKIN_QUADRATIC:
                localBasisSize = 6;
                break;
            case GalerkinBasisType::GALERKIN_CUBIC:
                localBasisSize = 10;
                break;
            default:
                logFatal(-1, "galerkinElementReAllocCoefficients", "Invalid basis type %d", galerkinState->basisType);
        }
    }

    ColorRgb *defaultRadiance = new ColorRgb[localBasisSize];
    colorsArrayClear(defaultRadiance, localBasisSize);
    if ( radiance != nullptr ) {
        colorsArrayCopy(defaultRadiance, radiance, java::Character::min(basisSize, localBasisSize));
        delete radiance;
    }
    radiance = defaultRadiance;

    ColorRgb *defaultReceivedRadiance = new ColorRgb[localBasisSize];
    colorsArrayClear(defaultReceivedRadiance, localBasisSize);
    if ( receivedRadiance != nullptr ) {
        colorsArrayCopy(defaultReceivedRadiance, receivedRadiance, java::Character::min(basisSize, localBasisSize));
        delete receivedRadiance;
    }
    receivedRadiance = defaultReceivedRadiance;

    if ( galerkinState->galerkinIterationMethod == GalerkinIterationMethod::SOUTH_WELL ) {
        ColorRgb *defaultUnShotRadiance = new ColorRgb[localBasisSize];
        colorsArrayClear(defaultUnShotRadiance, localBasisSize);
        if ( !isCluster() ) {
            if ( unShotRadiance ) {
                colorsArrayCopy(defaultUnShotRadiance, unShotRadiance, java::Character::min(basisSize, localBasisSize));
                delete unShotRadiance;
            } else if ( patch->material != nullptr ) {
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
Regularly subdivides the given element. A pointer to an array of 4 pointers to sub-elements is returned.

Only applicable to surface elements.
*/
void
GalerkinElement::regularSubDivide() {
    if ( isCluster() ) {
        logFatal(-1, "galerkinElementRegularSubDivide", "Cannot regularly subdivide cluster elements");
    }

    if ( regularSubElements != nullptr ) {
        return;
    }

    GalerkinElement **new4ChildrenSet = new GalerkinElement *[4];

    for ( int i = 0; i < 4; i++ ) {
        GalerkinElement *child = new GalerkinElement(galerkinState);
        child->patch = patch;
        child->parent = this;
        child->transformToParent =
            patch->numberOfVertices == 3 ?
            &globalTriangleToParentTransformMatrix[i] :
            &globalQuadToParentTransformMatrix[i];
        child->area = 0.25f * area;  // Uniform mapping is always used
        child->blockerSize = 2.0f * (float)java::Math::sqrt(child->area / M_PI);
        child->childNumber = (GalerkinElementRenderMode)i;
        child->reAllocCoefficients();

        basisGalerkinPush(this, radiance, child, child->radiance);

        child->potential = potential;
        child->directPotential = directPotential;

        if ( galerkinState->galerkinIterationMethod == SOUTH_WELL ) {
            basisGalerkinPush(this, unShotRadiance, child, child->unShotRadiance);
            child->unShotPotential = unShotPotential;
        }

        child->flags |= (flags & ElementFlags::IS_LIGHT_SOURCE_MASK);

        child->Rd = Rd;
        child->Ed = Ed;

        new4ChildrenSet[i] = child;
    }

    regularSubElements = (Element **)new4ChildrenSet;
}

/**
Determines the regular sub-element at point (u,v) of the given element
element. Returns the element element itself if there are no regular sub-elements.
The point is transformed to the corresponding point on the sub-element
*/
GalerkinElement *
GalerkinElement::regularSubElementAtPoint(double *u, double *v) {
    if ( isCluster() || !regularSubElements ) {
        return this;
    }

    // Have a look at the drawings above to understand what is done exactly
    Element *childElement;
    double _u = *u;
    double _v = *v;
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
GalerkinElement::vertices(Vector3D *p, int n) const {
    if ( isCluster() ) {
        BoundingBox boundingBox;

        bounds(&boundingBox);

        for ( int i = 0; i < n; i++ ) {
            p[i].set(boundingBox.coordinates[MIN_X], boundingBox.coordinates[MIN_Y], boundingBox.coordinates[MIN_Z]);
        }

        return 8;
    } else {
        Matrix2x2 topTrans{};
        Vector2D uv;

        if ( transformToParent != nullptr ) {
            topTransform(&topTrans);
        }

        uv.u = 0.0;
        uv.v = 0.0;
        if ( transformToParent != nullptr ) {
            topTrans.transformPoint2D(uv, uv);
        }
        patch->uniformPoint(uv.u, uv.v, &p[0]);

        uv.u = 1.0;
        uv.v = 0.0;
        if ( transformToParent != nullptr ) {
            topTrans.transformPoint2D(uv, uv);
        }
        patch->uniformPoint(uv.u, uv.v, &p[1]);

        if ( patch->numberOfVertices == 4 ) {
            uv.u = 1.0;
            uv.v = 1.0;
            if ( transformToParent != nullptr ) {
                topTrans.transformPoint2D(uv, uv);
            }
            patch->uniformPoint(uv.u, uv.v, &p[2]);

            uv.u = 0.0;
            uv.v = 1.0;
            if ( transformToParent != nullptr ) {
                topTrans.transformPoint2D(uv, uv);
            }
            patch->uniformPoint(uv.u, uv.v, &p[3]);
        } else {
            uv.u = 0.0;
            uv.v = 1.0;
            if ( transformToParent != nullptr ) {
                topTrans.transformPoint2D(uv, uv);
            }
            patch->uniformPoint(uv.u, uv.v, &p[2]);

            p[3].set(0.0, 0.0, 0.0);
        }

        return patch->numberOfVertices;
    }
}

/**
Computes the midpoint of the element
*/
Vector3D
GalerkinElement::midPoint() const {
    Vector3D c;

    if ( isCluster() ) {
        c.set((geometry->getBoundingBox().coordinates[MIN_X] + geometry->getBoundingBox().coordinates[MAX_X]) / 2.0f,
              (geometry->getBoundingBox().coordinates[MIN_Y] + geometry->getBoundingBox().coordinates[MAX_Y]) / 2.0f,
              (geometry->getBoundingBox().coordinates[MIN_Z] + geometry->getBoundingBox().coordinates[MAX_Z]) / 2.0f);
    } else {
        Vector3D p[8];
        int numberOfVertices = vertices(p, 4);

        c.set(0.0, 0.0, 0.0);
        for ( int i = 0; i < numberOfVertices; i++ ) {
            c.addition(c, p[i]);
        }
        c.scaledCopy((1.0f / (float) numberOfVertices), c);
    }

    return c;
}

/**
Computes a bounding box for the element
*/
BoundingBox *
GalerkinElement::bounds(BoundingBox *boundingBox) const {
    if ( isCluster() ) {
        BoundingBox copy = geometry->getBoundingBox();
        boundingBox->copyFrom(&copy);
    } else {
        Vector3D p[4];
        int numberOfVertices;

        numberOfVertices = vertices(p, 4);

        for ( int i = 0; i < numberOfVertices; i++ ) {
            boundingBox->enlargeToIncludePoint(&p[i]);
        }
    }

    return boundingBox;
}

/**
Computes a polygon description for shaft culling for the surface
element. Cannot be used for clusters
*/
void
GalerkinElement::initPolygon(Polygon *polygon) const {
    if ( isCluster() ) {
        logFatal(-1, "galerkinElementPolygon", "Cannot use this function for cluster elements");
    }

    polygon->normal = patch->normal;
    polygon->planeConstant = patch->planeConstant;
    polygon->index = patch->index;
    polygon->numberOfVertices = vertices(polygon->vertex, polygon->numberOfVertices);

    for ( int i = 0; i < polygon->numberOfVertices; i++ ) {
        polygon->bounds.enlargeToIncludePoint(&polygon->vertex[i]);
    }
}

void
GalerkinElement::draw(int mode, const RenderOptions *renderOptions) const {
    if ( isCluster() ) {
        if ( mode & GalerkinElementRenderMode::OUTLINE ) {
            renderBoundingBox(geometry->getBoundingBox());
        }
        return;
    }

    Vector3D p[4];
    int numberOfVertices = vertices(p, 4);

    // Draw surfaces
    if ( renderOptions->drawSurfaces ) {
        if ( mode & GalerkinElementRenderMode::FLAT ) {
            ColorRgb color{};
            ColorRgb rho = patch->radianceData->Rd;

            if ( galerkinState->useAmbientRadiance ) {
                ColorRgb radVis;
                radVis.scalarProduct(rho, galerkinState->ambientRadiance);
                radVis.add(radVis, radiance[0]);
                radianceToRgb(radVis, &color);
            } else {
                radianceToRgb(radiance[0], &color);
            }
            openGlRenderSetColor(&color);
            openGlRenderPolygonFlat(numberOfVertices, p);
        } else if ( mode & GalerkinElementRenderMode::GOURAUD ) {
            ColorRgb vertRadiosity[4];

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

            if ( galerkinState->useAmbientRadiance ) {
                ColorRgb reflectivity = patch->radianceData->Rd;
                ColorRgb ambient;

                ambient.scalarProduct(reflectivity, galerkinState->ambientRadiance);
                for ( int i = 0; i < numberOfVertices; i++ ) {
                    vertRadiosity[i].add(vertRadiosity[i], ambient);
                }
            }

            ColorRgb vertexColors[4];
            for ( int i = 0; i < numberOfVertices; i++ ) {
                radianceToRgb(vertRadiosity[i], &vertexColors[i]);
            }

            openGlRenderPolygonGouraud(numberOfVertices, p, vertexColors);
        }
    }

    // Draw outlines
    if ( mode & GalerkinElementRenderMode::OUTLINE ) {
        openGlRenderSetColor(&renderOptions->outlineColor);
        if ( numberOfVertices == 3 ) {
            openGlRenderSetColor(&renderOptions->outlineColor);
            openGlRenderLine(&p[0], &p[1]);
            openGlRenderLine(&p[1], &p[2]);
            openGlRenderLine(&p[2], &p[0]);
        } else {
            ColorRgb green = {0.0, 1.0, 0.0};

            openGlRenderSetColor(&green);
            openGlRenderLine(&p[0], &p[1]);
            openGlRenderLine(&p[1], &p[2]);
            openGlRenderLine(&p[2], &p[3]);
            openGlRenderLine(&p[3], &p[0]);
        }
    }
}

/**
Renders a surface element flat shaded based on its radiance
*/
void
GalerkinElement::render(const RenderOptions *renderOptions) const {
    int renderCode = 0;

    if ( renderOptions->drawOutlines ) {
        renderCode |= GalerkinElementRenderMode::OUTLINE;
    }

    if ( renderOptions->smoothShading ) {
        renderCode |= GalerkinElementRenderMode::GOURAUD;
    } else {
        renderCode |= GalerkinElementRenderMode::FLAT;
    }

    draw(renderCode, renderOptions);
}

void
basisGalerkinInitBasis() {
    basisGalerkinComputeRegularFilterCoefficients(&GLOBAL_galerkin_quadBasis, globalQuadToParentTransformMatrix, &GLOBAL_crq8);
    basisGalerkinComputeRegularFilterCoefficients(&GLOBAL_galerkin_triBasis, globalTriangleToParentTransformMatrix, &GLOBAL_crt8);
}

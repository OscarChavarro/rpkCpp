#include "java/util/ArrayList.txx"
#include "common/Error.h"
#include "numericalAnalysis/PatchVisitor.h"
#include "numericalAnalysis/QuadCubatureRule.h"
#include "numericalAnalysis/TriangleCubatureRule.h"
#include "galerkin/GalerkinBasis.h"
#include "galerkin/GalerkinElement.h"

int GalerkinElement::numberOfElements = 0;
int GalerkinElement::numberOfClusters = 0;

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
const Matrix2x2 GalerkinElement::quadToParentTransformMatrix[4] = {
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
const Matrix2x2 GalerkinElement::triangleToParentTransformMatrix[4] = {
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
GalerkinElement::GalerkinElement(GalerkinState *inGalerkinState):Element()
{
    className = ElementTypes::ELEMENT_GALERKIN;

    interactions = new java::ArrayList<Interaction *>();
    galerkinState = inGalerkinState;

    id = numberOfElements + 1; // Let the IDs start from 1, not 0
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

    numberOfElements++;
}

/**
Creates the toplevel element for the patch
*/
GalerkinElement::GalerkinElement(Patch *parameterPatch, GalerkinState *inGalerkinState):
    GalerkinElement(inGalerkinState)
{
    patch = parameterPatch;
    minimumArea = area = patch->getArea();
    blockerSize = 2.0f * static_cast<float>(java::Math::sqrt(area / M_PI));
    directPotential = patch->getDirectPotential();

    Rd = PatchVisitor::averageNormalAlbedo(patch, BRDF_DIFFUSE_COMPONENT);
    if ( patch->getMaterial() != nullptr && patch->getMaterial()->getEdf() != nullptr ) {
        flags |= ElementFlags::IS_LIGHT_SOURCE_MASK;
        Ed = PatchVisitor::averageEmittance(patch, DIFFUSE_COMPONENT);
        Ed.scaleInverse(M_PI, Ed);
    }

    patch->setRadianceData(this);
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
    numberOfClusters++;
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
    numberOfElements--;
    if ( isCluster() ) {
        numberOfClusters--;
    }
}

/**
Returns the total number of elements in use
*/
int
GalerkinElement::getNumberOfElements() {
    return numberOfElements;
}

int
GalerkinElement::getNumberOfClusters() {
    return numberOfClusters;
}

int
GalerkinElement::getNumberOfSurfaceElements() {
    return numberOfElements - numberOfClusters;
}

GalerkinElement *
GalerkinElement::fromPatch(const Patch *patch) {
    if ( patch == nullptr ) {
        java::System::err.printf("Fatal: Trying to access as GalerkinElement on a null Patch\n");
        java::System::exit(1);
    }
    if ( patch->getRadianceData() == nullptr ) {
        java::System::err.printf("Fatal: Trying to access as GalerkinElement on a Patch with null radianceData\n");
        java::System::exit(1);
    }
    if ( patch->getRadianceData()->className != ElementTypes::ELEMENT_GALERKIN ) {
        java::System::err.printf("Fatal: Trying to access as GalerkinElement a different type of element\n");
        java::System::exit(1);
    }
    return static_cast<GalerkinElement *>(patch->getRadianceData());
}

int
GalerkinElement::renderMode(const RenderOptions *renderOptions) {
    if ( renderOptions == nullptr ) {
        return GalerkinElementRenderMode::FLAT;
    }

    int renderCode = 0;
    if ( renderOptions->drawOutlines ) {
        renderCode |= GalerkinElementRenderMode::OUTLINE;
    }
    if ( renderOptions->smoothShading ) {
        renderCode |= GalerkinElementRenderMode::GOURAUD;
    } else {
        renderCode |= GalerkinElementRenderMode::FLAT;
    }

    return renderCode;
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
                Error::fatal(-1, "galerkinElementReAllocCoefficients", "Invalid basis type %d", galerkinState->basisType);
        }
    }

    ColorRgb *defaultRadiance = new ColorRgb[localBasisSize];
    ColorRgb::arrayClear(defaultRadiance, localBasisSize);
    if ( radiance != nullptr ) {
        ColorRgb::arrayCopy(defaultRadiance, radiance, java::Math::min(basisSize, localBasisSize));
        delete radiance;
    }
    radiance = defaultRadiance;

    ColorRgb *defaultReceivedRadiance = new ColorRgb[localBasisSize];
    ColorRgb::arrayClear(defaultReceivedRadiance, localBasisSize);
    if ( receivedRadiance != nullptr ) {
        ColorRgb::arrayCopy(defaultReceivedRadiance, receivedRadiance, java::Math::min(basisSize, localBasisSize));
        delete receivedRadiance;
    }
    receivedRadiance = defaultReceivedRadiance;

    if ( galerkinState->galerkinIterationMethod == GalerkinIterationMethod::SOUTH_WELL ) {
        ColorRgb *defaultUnShotRadiance = new ColorRgb[localBasisSize];
        ColorRgb::arrayClear(defaultUnShotRadiance, localBasisSize);
        if ( !isCluster() ) {
            if ( unShotRadiance ) {
                ColorRgb::arrayCopy(defaultUnShotRadiance, unShotRadiance, java::Math::min(basisSize, localBasisSize));
                delete unShotRadiance;
            } else if ( patch->getMaterial() != nullptr ) {
                defaultUnShotRadiance[0] = patch->getRadianceData()->Ed;
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
        Error::fatal(-1, "galerkinElementRegularSubDivide", "Cannot regularly subdivide cluster elements");
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
            patch->getNumberOfVertices() == 3 ?
            &triangleToParentTransformMatrix[i] :
            &quadToParentTransformMatrix[i];
        child->area = 0.25f * area;  // Uniform mapping is always used
        child->blockerSize = 2.0f * static_cast<float>(java::Math::sqrt(child->area / M_PI));
        child->childNumber = static_cast<GalerkinElementRenderMode>(i);
        child->reAllocCoefficients();

        GalerkinBasis::push(this, radiance, child, child->radiance);

        child->potential = potential;
        child->directPotential = directPotential;

        if ( galerkinState->galerkinIterationMethod == SOUTH_WELL ) {
            GalerkinBasis::push(this, unShotRadiance, child, child->unShotRadiance);
            child->unShotPotential = unShotPotential;
        }

        child->flags |= (flags & ElementFlags::IS_LIGHT_SOURCE_MASK);

        child->Rd = Rd;
        child->Ed = Ed;

        new4ChildrenSet[i] = child;
    }

    regularSubElements = reinterpret_cast<Element **>(new4ChildrenSet);
}

/**
Determines the regular sub-element at point (u,v) of the given element.
Returns the element itself if there are no regular sub-elements.
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
    switch ( patch->getNumberOfVertices() ) {
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
            Error::fatal(-1, "galerkinElementRegularSubElementAtPoint", "Can handle only triangular or quadrilateral elements");
    }

    return dynamic_cast<GalerkinElement *>(childElement);
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
GalerkinElement::vertices(Vector3D *p) const {
    if ( isCluster() ) {
        BoundingBox boundingBox;
        bounds(&boundingBox);

        boundingBox.corners(p);

        return 8;
    } else {
        Matrix2x2 topTrans{};
        Vector2D uv;

        if ( transformToParent != nullptr ) {
            topTransform(&topTrans);
        }

        uv.u = 0.0f;
        uv.v = 0.0f;
        if ( transformToParent != nullptr ) {
            topTrans.transformPoint2D(uv, uv);
        }
        patch->uniformPoint(uv.u, uv.v, &p[0]);

        uv.u = 1.0f;
        uv.v = 0.0f;
        if ( transformToParent != nullptr ) {
            topTrans.transformPoint2D(uv, uv);
        }
        patch->uniformPoint(uv.u, uv.v, &p[1]);

        if ( patch->getNumberOfVertices() == 4 ) {
            uv.u = 1.0f;
            uv.v = 1.0f;
            if ( transformToParent != nullptr ) {
                topTrans.transformPoint2D(uv, uv);
            }
            patch->uniformPoint(uv.u, uv.v, &p[2]);

            uv.u = 0.0f;
            uv.v = 1.0f;
            if ( transformToParent != nullptr ) {
                topTrans.transformPoint2D(uv, uv);
            }
            patch->uniformPoint(uv.u, uv.v, &p[3]);
        } else {
            uv.u = 0.0f;
            uv.v = 1.0f;
            if ( transformToParent != nullptr ) {
                topTrans.transformPoint2D(uv, uv);
            }
            patch->uniformPoint(uv.u, uv.v, &p[2]);
            p[3].set(0.0f, 0.0f, 0.0f);
        }

        return patch->getNumberOfVertices();
    }
}

/**
Computes the midpoint of the element
*/
Vector3D
GalerkinElement::midPoint() const {
    if ( isCluster() ) {
        return geometry->getBoundingBox().center();
    } else {
        Vector3D p[8];
        int numberOfVertices = vertices(p);

        Vector3D c(0.0f, 0.0f, 0.0f);

        for ( int i = 0; i < numberOfVertices; i++ ) {
            c.addition(c, p[i]);
        }

        c.scaledCopy(1.0f / static_cast<float>(numberOfVertices), c);
        return c;
    }
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

        const int numberOfVertices = vertices(p);

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
        Error::fatal(-1, "galerkinElementPolygon", "Cannot use this function for cluster elements");
    }

    polygon->normal = patch->getNormal();
    polygon->planeConstant = patch->getPlaneConstant();
    polygon->index = patch->getDominantAxisIndex();
    polygon->numberOfVertices = vertices(polygon->vertex);

    for ( int i = 0; i < polygon->numberOfVertices; i++ ) {
        polygon->bounds.enlargeToIncludePoint(&polygon->vertex[i]);
    }
}

void
GalerkinElement::initializeBasis() {
    GalerkinBasis::computeRegularFilterCoefficients(
        GalerkinBasis::mutableBasisForVertexCount(4),
        quadToParentTransformMatrix,
        QuadCubatureRule::degree8QuadrilateralRule());
    GalerkinBasis::computeRegularFilterCoefficients(
        GalerkinBasis::mutableBasisForVertexCount(3),
        triangleToParentTransformMatrix,
        TriangleCubatureRule::degree8Rule());
}

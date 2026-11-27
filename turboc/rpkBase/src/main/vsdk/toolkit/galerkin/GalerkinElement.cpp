#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/logging/LoggerFormat.h"
#include "vsdk/toolkit/numericalAnalysis/PatchVisitor.h"
#include "vsdk/toolkit/numericalAnalysis/QuadCubatureRule.h"
#include "vsdk/toolkit/numericalAnalysis/TriangleCubatureRule.h"
#include "vsdk/toolkit/galerkin/GalerkinBasis.h"
#include "vsdk/toolkit/galerkin/GalerkinElement.h"

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
    Matrix2x2(0.5, 0.0, 0.0, 0.5, 0.0, 0.0),

    // 2: South-east [0.5, 1] x [0, 0.5]
    Matrix2x2(0.5, 0.0, 0.0, 0.5, 0.5, 0.0),

    // 3: North-west [0, 0.5] x [0.5, 1]
    Matrix2x2(0.5, 0.0, 0.0, 0.5, 0.0, 0.5),

    // 4: North-east [0.5, 1] x [0.5, 1]
    Matrix2x2(0.5, 0.0, 0.0, 0.5, 0.5, 0.5)
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
    Matrix2x2(0.5, 0.0, 0.0, 0.5, 0.0, 0.0),

    // 2: Right [0.5, 0], [1, 0], [0.5, 0.5]
    Matrix2x2(0.5, 0.0, 0.0, 0.5, 0.5, 0.0),

    // 3: Top [0, 0.5], [0.5, 0.5], [0, 1]
    Matrix2x2(0.5, 0.0, 0.0, 0.5, 0.0, 0.5),

    // 4: Middle [0.5, 0.5], [0, 0.5], [0.5, 0]
    Matrix2x2(-0.5, 0.0, 0.0, -0.5, 0.5, 0.5)
};

/**
Private inner constructor, Use either galerkinElementCreateTopLevel() or CreateRegularSubElement()
*/
void
GalerkinElement::initializeCommon(GalerkinState *inGalerkinState) {
    className = ELEMENT_GALERKIN;

    interactions = new ArrayList<Interaction *>();
    galerkinState = inGalerkinState;

    id = numberOfElements + 1; // Let the IDs start from 1, not 0
    radiance = NULL;
    receivedRadiance = NULL;
    unShotRadiance = NULL;
    potential = 0.0;
    receivedPotential = 0.0;
    unShotPotential = 0.0;
    directPotential = 0.0;
    patch = NULL;
    geometry = NULL;
    parent = NULL;
    regularSubElements = NULL;
    irregularSubElements = NULL; // New list
    transformToParent = NULL;
    area = 0.0;
    childNumber = NOT_A_REGULAR_SUB_ELEMENT;
    basisSize = 0;
    basisUsed = 0;
    numberOfPatches = 1; // Correct for surface elements, it will be computed later for clusters
    minimumArea = Double::MAX_VALUE;
    scratchVisibilityUsageCounter = 0;
    blockerSize = 0.0; // Correct eq. blocker size will be computer later on

    numberOfElements++;
}

GalerkinElement::GalerkinElement(GalerkinState *inGalerkinState):Element()
{
    initializeCommon(inGalerkinState);
}

/**
Creates the toplevel element for the patch
*/
GalerkinElement::GalerkinElement(Patch *parameterPatch, GalerkinState *inGalerkinState):Element()
{
    initializeCommon(inGalerkinState);

    patch = parameterPatch;
    minimumArea = area = patch->area;
    blockerSize = 2.0 * ((double)(Math::sqrt(area / M_PI)));
    directPotential = patch->directPotential;

    Rd = PatchVisitor::averageNormalAlbedo(patch, BRDF_DIFFUSE_COMPONENT);
    if ( patch->material != NULL && patch->material->getEdf() != NULL ) {
        flags |= IS_LIGHT_SOURCE_MASK;
        Ed = PatchVisitor::averageEmittance(patch, DIFFUSE_COMPONENT);
        Ed.scaleInverse(M_PI, Ed);
    }

    patch->radianceData = this;
    reAllocCoefficients();
}

/**
Creates a cluster element for the given geometry
The average projected area still needs to be determined
*/
GalerkinElement::GalerkinElement(RpkGeometry *inGeometry, GalerkinState *inGalerkinState):Element()
{
    initializeCommon(inGalerkinState);

    geometry = inGeometry;
    area = 0.0; // Needs to be computed after the whole cluster hierarchy has been constructed
    flags |= IS_CLUSTER_MASK;
    reAllocCoefficients();

    Rd = ColorRgbMutable(1.0, 1.0, 1.0);

    // Whether the cluster contains light sources or not is also determined after the hierarchy is constructed
    numberOfClusters++;
}

GalerkinElement::~GalerkinElement() {
    for ( int i = 0; interactions != NULL && i < interactions->size(); i++ ) {
        delete interactions->get(i);
    }
    delete interactions;

    if ( regularSubElements != NULL ) {
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
    if ( patch == NULL ) {
        System::err.printf("Fatal: Trying to access as GalerkinElement on a null Patch\n");
        System::exit(1);
    }
    if ( patch->radianceData == NULL ) {
        System::err.printf("Fatal: Trying to access as GalerkinElement on a Patch with null radianceData\n");
        System::exit(1);
    }
    if ( patch->radianceData->className != ELEMENT_GALERKIN ) {
        System::err.printf("Fatal: Trying to access as GalerkinElement a different type of element\n");
        System::exit(1);
    }
    return ((GalerkinElement *)(patch->radianceData));
}

int
GalerkinElement::renderMode(const RenderOptions *renderOptions) {
    if ( renderOptions == NULL ) {
        return FLAT;
    }

    int renderCode = 0;
    if ( renderOptions->isDrawOutlines() ) {
        renderCode |= OUTLINE;
    }
    if ( renderOptions->isSmoothShading() ) {
        renderCode |= GOURAUD;
    } else {
        renderCode |= FLAT;
    }

    return renderCode;
}

/**
Re-allocates storage for the coefficients to represent radiance, received radiance
and un-shot radiance on the element
*/
void
GalerkinElement::reAllocCoefficients() {
    char localBasisSize = 1;

    if ( isCluster() ) {
        // We always use a constant basis on cluster elements
        localBasisSize = 1;
    } else {
        switch ( galerkinState->basisType ) {
            case GALERKIN_CONSTANT:
                localBasisSize = 1;
                break;
            case GALERKIN_LINEAR:
                localBasisSize = 3;
                break;
            case GALERKIN_QUADRATIC:
                localBasisSize = 6;
                break;
            case GALERKIN_CUBIC:
                localBasisSize = 10;
                break;
            default:
                Logger::reportMessage("GalerkinElement", Logger::FATAL_ERROR, "glrknElemReAllcCoeff", LoggerFormat::format("Invalid basis type %d", galerkinState->basisType));
        }
    }

    ColorRgbMutable *defaultRadiance = new ColorRgbMutable[localBasisSize];
    ColorRgbMutable::arrayClear(defaultRadiance, localBasisSize);
    if ( radiance != NULL ) {
        ColorRgbMutable::arrayCopy(defaultRadiance, radiance, Math::min(basisSize, localBasisSize));
        delete radiance;
    }
    radiance = defaultRadiance;

    ColorRgbMutable *defaultReceivedRadiance = new ColorRgbMutable[localBasisSize];
    ColorRgbMutable::arrayClear(defaultReceivedRadiance, localBasisSize);
    if ( receivedRadiance != NULL ) {
        ColorRgbMutable::arrayCopy(defaultReceivedRadiance, receivedRadiance, Math::min(basisSize, localBasisSize));
        delete receivedRadiance;
    }
    receivedRadiance = defaultReceivedRadiance;

    if ( galerkinState->galerkinIterationMethod == SOUTH_WELL ) {
        ColorRgbMutable *defaultUnShotRadiance = new ColorRgbMutable[localBasisSize];
        ColorRgbMutable::arrayClear(defaultUnShotRadiance, localBasisSize);
        if ( !isCluster() ) {
            if ( unShotRadiance ) {
                ColorRgbMutable::arrayCopy(defaultUnShotRadiance, unShotRadiance, Math::min(basisSize, localBasisSize));
                delete unShotRadiance;
            } else if ( patch->material != NULL ) {
                defaultUnShotRadiance[0] = patch->radianceData->Ed;
            }
        }
        unShotRadiance = defaultUnShotRadiance;
    } else {
        if ( unShotRadiance ) {
            delete unShotRadiance;
        }
        unShotRadiance = NULL;
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
        Logger::reportMessage("GalerkinElement", Logger::FATAL_ERROR, "galerkinElementRegularSubDivide", "Cannot regularly subdivide cluster elements");
    }

    if ( regularSubElements != NULL ) {
        return;
    }

    GalerkinElement **new4ChildrenSet = new GalerkinElement *[4];

    for ( int i = 0; i < 4; i++ ) {
        GalerkinElement *child = new GalerkinElement(galerkinState);
        child->patch = patch;
        child->parent = this;
        child->transformToParent =
            patch->numberOfVertices == 3 ?
            &triangleToParentTransformMatrix[i] :
            &quadToParentTransformMatrix[i];
        child->area = 0.25 * area;  // Uniform mapping is always used
        child->blockerSize = 2.0 * ((double)(Math::sqrt(child->area / M_PI)));
        child->childNumber = ((GalerkinElementRenderMode)(i));
        child->reAllocCoefficients();

        GalerkinBasis::push(this, radiance, child, child->radiance);

        child->potential = potential;
        child->directPotential = directPotential;

        if ( galerkinState->galerkinIterationMethod == SOUTH_WELL ) {
            GalerkinBasis::push(this, unShotRadiance, child, child->unShotRadiance);
            child->unShotPotential = unShotPotential;
        }

        child->flags |= (flags & IS_LIGHT_SOURCE_MASK);

        child->Rd = Rd;
        child->Ed = Ed;

        new4ChildrenSet[i] = child;
    }

    regularSubElements = ((Element **)(new4ChildrenSet));
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
    Element *childElement = this;
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
            Logger::reportMessage("GalerkinElement", Logger::FATAL_ERROR, "glrknElemRegSubElemAPnt", "Can handle only triangular or quadrilateral elements");
    }

    return ((GalerkinElement *)(childElement));
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
GalerkinElement::vertices(Vector3Dd *p) const {
    if ( isCluster() ) {
        RpkAxisAlignedBoundingBox boundingBox;
        bounds(&boundingBox);

        boundingBox.corners(p);

        return 8;
    } else {
        Matrix2x2 topTrans = Matrix2x2();
        Vector2Dd uv;

        if ( transformToParent != NULL ) {
            topTransform(&topTrans);
        }

        uv = Vector2Dd(0.0, 0.0);
        if ( transformToParent != NULL ) {
            uv = topTrans.transformPoint(uv);
        }
        patch->uniformPoint(uv.x, uv.y, &p[0]);

        uv = Vector2Dd(1.0, 0.0);
        if ( transformToParent != NULL ) {
            uv = topTrans.transformPoint(uv);
        }
        patch->uniformPoint(uv.x, uv.y, &p[1]);

        if ( patch->numberOfVertices == 4 ) {
            uv = Vector2Dd(1.0, 1.0);
            if ( transformToParent != NULL ) {
                uv = topTrans.transformPoint(uv);
            }
            patch->uniformPoint(uv.x, uv.y, &p[2]);

            uv = Vector2Dd(0.0, 1.0);
            if ( transformToParent != NULL ) {
                uv = topTrans.transformPoint(uv);
            }
            patch->uniformPoint(uv.x, uv.y, &p[3]);
        } else {
            uv = Vector2Dd(0.0, 1.0);
            if ( transformToParent != NULL ) {
                uv = topTrans.transformPoint(uv);
            }
            patch->uniformPoint(uv.x, uv.y, &p[2]);
            p[3] = Vector3Dd(0.0, 0.0, 0.0);
        }

        return patch->numberOfVertices;
    }
}

/**
Computes the midpoint of the element
*/
Vector3Dd
GalerkinElement::midPoint() const {
    if ( isCluster() ) {
        return geometry->getBoundingBox().center();
    } else {
        Vector3Dd p[8];
        int numberOfVertices = vertices(p);

        Vector3Dd c(0.0, 0.0, 0.0);

        for ( int i = 0; i < numberOfVertices; i++ ) {
            c = c.add(p[i]);
        }

        c = c.multiply(1.0 / ((double)(numberOfVertices)));
        return c;
    }
}

/**
Computes a bounding box for the element
*/
RpkAxisAlignedBoundingBox *
GalerkinElement::bounds(RpkAxisAlignedBoundingBox *boundingBox) const {
    if ( isCluster() ) {
        RpkAxisAlignedBoundingBox copy = geometry->getBoundingBox();
        boundingBox->copyFrom(&copy);
    } else {
        Vector3Dd p[4];

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
        Logger::reportMessage("GalerkinElement", Logger::FATAL_ERROR, "galerkinElementPolygon", "Cannot use this function for cluster elements");
    }

    polygon->normal = patch->normal;
    polygon->planeConstant = patch->planeConstant;
    polygon->index = patch->index;
    polygon->numberOfVertices = vertices(polygon->vertex);

    for ( int i = 0; i < polygon->numberOfVertices; i++ ) {
        polygon->bounds.enlargeToIncludePoint(&polygon->vertex[i]);
    }
}

void
GalerkinElement::initializeBasis() {
    GalerkinBasis::cmptRegFiltCoeff(
        GalerkinBasis::mutableBasisForVertexCount(4),
        quadToParentTransformMatrix,
        QuadCubatureRule::degree8QuadrilateralRule());
    GalerkinBasis::cmptRegFiltCoeff(
        GalerkinBasis::mutableBasisForVertexCount(3),
        triangleToParentTransformMatrix,
        TriangleCubatureRule::degree8Rule());
}

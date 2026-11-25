#include "vsdk/toolkit/common/color/ColorRgbMath.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/galerkin/processing/FormFactorClusteredStrategy.h"
#include "vsdk/toolkit/galerkin/processing/FormFactorStrategy.h"

#ifdef RPK_RAY_CAPTURE
    #include "vsdk/toolkit/scene/VisibilityQueryRecorder.h"
#endif

/**
References:
- [BEKA1996] Ph. Bekaert, Y. D. Willems, "Error Control for Radiosity", Euro-graphics
  Rendering Workshop, Porto, Portugal, June 1996, pp 153 -164.

- [SILL1995b] F. Sillion, "A Unified Hierarchical Algorithm for Global Illumination
  with Scattering Volumes and Object Clusters", IEEE TVCG Vol 1 Nr 3, September 1995
*/

/**
Tests whether the ray intersects a geometry in the geometrySceneList. Returns
nullptr if the ray hits no geometries. Returns an arbitrary hit
patch if the ray does intersect one or more geometries. Intersections
further away than minimumDistance are ignored.
*/
RpkRayHit *
FormFactorStrategy::shadowTestDiscretization(
    RpkRay *ray,
    const java::ArrayList<RpkGeometry *> *geometrySceneList,
    const SceneQuery *sceneQuery,
    SceneQueryWorkspace *sceneQueryWorkspace,
    FormFactorWorkspace *formFactorWorkspace,
    ShadowCache *shadowCache,
    float minimumDistance,
    RpkRayHit *hitStore,
    bool isSceneGeometry,
    bool isClusteredGeometry,
    const RayExclusionSet &exclusions)
{
    ShadowStatistics *shadowStatistics = formFactorWorkspace->getShadowStatistics();
    shadowStatistics->numberOfShadowRays++;
    RpkRayHit *hit = shadowCache->cacheHit(ray, &minimumDistance, hitStore, exclusions);
    if ( hit != nullptr ) {
        shadowStatistics->numberOfShadowCacheHits++;
#ifdef RPK_RAY_CAPTURE
        if ( sceneQueryWorkspace != nullptr && sceneQueryWorkspace->getRecorder() != nullptr ) {
            sceneQueryWorkspace->getRecorder()->countCacheResolved();
        }
#endif
    } else {
#ifdef RPK_RAY_CAPTURE
        const float queryMaximumDistance = minimumDistance;
#endif
        if ( !isClusteredGeometry && !isSceneGeometry ) {
            hit = RpkGeometry::listDiscretizationIntersect(
                geometrySceneList,
                ray,
                Numeric::EPSILON_FLOAT * minimumDistance,
                &minimumDistance,
                RayHitFlag::FRONT | RayHitFlag::ANY,
                hitStore,
                exclusions);
        } else {
            hit = sceneQuery->intersect(
                ray,
                Numeric::EPSILON_FLOAT * minimumDistance,
                &minimumDistance,
                RayHitFlag::FRONT | RayHitFlag::ANY,
                hitStore,
                exclusions,
                sceneQueryWorkspace);
        }
#ifdef RPK_RAY_CAPTURE
        if ( sceneQueryWorkspace != nullptr && sceneQueryWorkspace->getRecorder() != nullptr ) {
            sceneQueryWorkspace->getRecorder()->recordQuery(
                (!isClusteredGeometry && !isSceneGeometry) ?
                    VisibilityQueryRecorder::GALERKIN_RESTRICTED_LIST : VisibilityQueryRecorder::GALERKIN_FULL_SCENE,
                ray,
                Numeric::EPSILON_FLOAT * queryMaximumDistance,
                queryMaximumDistance,
                RayHitFlag::FRONT | RayHitFlag::ANY,
                exclusions,
                hit,
                minimumDistance);
        }
#endif
        if ( hit != nullptr ) {
            shadowCache->addToShadowCache(hit->getPatch());
        }
    }

    return hit;
}

void
FormFactorStrategy::reportNodesTooClose(const GalerkinElement *receiverElement, const GalerkinElement *sourceElement) {
    Logger::reportMessage("FormFactorStrategy", Logger::WARNING, "evaluatePointsPairKernel", java::String::format("Nodes too close too each other (receiver id %d, source id %d)", receiverElement->id, sourceElement->id));
}

/**
This routine determines the cubature rule to be used on the given element
and computes the positions on the elements patch that correspond to the nodes
of the cubature rule on the element. The role (RECEIVER or SOURCE) is only
relevant for surface elements
*/
void
FormFactorStrategy::determineNodes(
        const GalerkinElement *element,
        const GalerkinRole role,
        const GalerkinState *galerkinState,
        CubatureRule **cr,
        Vector3Df x[CubatureRule::MAXIMUM_NODES])
{
    Matrix2x2 topTransform{};

    if ( element->isCluster() ) {
        *cr = galerkinState->clusterRule;

        RpkAxisAlignedBoundingBox boundingBox;
        element->bounds(&boundingBox);

        double dx = boundingBox.dx();
        double dy = boundingBox.dy();
        double dz = boundingBox.dz();

        const float minX = boundingBox.minX();
        const float minY = boundingBox.minY();
        const float minZ = boundingBox.minZ();

        for ( int k = 0; k < (*cr)->numberOfNodes; k++ ) {
            x[k] = Vector3Df(static_cast<float>(minX + (*cr)->u[k] * dx), static_cast<float>(minY + (*cr)->v[k] * dy), static_cast<float>(minZ + (*cr)->t[k] * dz));
        }
    } else {
        // What cubature rule should be used over the element
        switch ( element->patch->getNumberOfVertices() ) {
            case 3:
                *cr = role == GalerkinRole::RECEIVER ? galerkinState->receiverTriangleCubatureRule : galerkinState->sourceTriangleCubatureRule;
                break;
            case 4:
                *cr = role == GalerkinRole::RECEIVER ? galerkinState->receiverQuadCubatureRule : galerkinState->sourceQuadCubatureRule;
                break;
            default:
                Logger::reportMessage("FormFactorStrategy", Logger::FATAL_ERROR, "determineNodes", "Can only handle triangular and quadrilateral patches");
        }

        // Compute the transform relating positions on the element to positions on
        // the patch to which it belongs
        if ( element->transformToParent != nullptr ) {
            element->topTransform(&topTransform);
        }

        // Compute the positions x[k] corresponding to the nodes of the cubature rule
        // in the unit square or triangle used to parametrise the element
        for ( int k = 0; *cr != nullptr && k < (*cr)->numberOfNodes; k++ ) {
            Vector2Df node;
            node = Vector2Df(static_cast<float>((*cr)->u[k]), static_cast<float>((*cr)->v[k]));
            if ( element->transformToParent != nullptr ) {
                node = topTransform.transformPoint(node);
            }
            element->patch->uniformPoint(node.x(), node.y(), &x[k]);
        }
    }
}

/**
Evaluates the radiosity kernel (*not* taking into account the reflectivity of
the receiver) at positions x on the receiverElement and y on
sourceElement. The shadowGeometryList contains potential occluders.
Shadow caching is used to speed up the visibility detection
between x and y. The shadow cache is initialized before doing the first
evaluation of the kernel between each pair of patches. The un-occluded
point-to-point form factor is returned. Visibility is computed and returned
as part of the kernel.
*/
double
FormFactorStrategy::evaluatePointsPairKernel(
    ShadowCache *shadowCache,
    const SceneQuery *sceneQuery,
    SceneQueryWorkspace *sceneQueryWorkspace,
    FormFactorWorkspace *formFactorWorkspace,
    const Vector3Df *x,
    const Vector3Df *y,
    const GalerkinElement *receiverElement,
    const GalerkinElement *sourceElement,
    const java::ArrayList<RpkGeometry *> *shadowGeometryList,
    const bool isSceneGeometry,
    const bool isClusteredGeometry,
    const GalerkinState *galerkinState,
    const RayExclusionSet &exclusions)
{
    // Trace the ray from source to receiver (y to x) to handle one-sided surfaces correctly
    RpkRay ray;
    ray.position = *y;
    ray.direction = (*x).subtract(*y);
    double distance = ray.direction.length();
    ray.direction = ray.direction.multiplyByInverse(static_cast<float>(distance), Numeric::EPSILON_FLOAT);

    // Don't allow too nearby nodes to interact
    if ( distance < VSDK::EPSILON ) {
        if ( !formFactorWorkspace->deferNodesTooClose(receiverElement, sourceElement) ) {
            reportNodesTooClose(receiverElement, sourceElement);
        }
        return 0.0;
    }

    // Emitter factor times scale, see [SILL1995b]
    double cosThetaY;
    if ( sourceElement->isCluster() ) {
        cosThetaY = 0.25;
    } else {
        cosThetaY = ray.direction.dotProduct(sourceElement->patch->getNormal());
        if ( cosThetaY <= 0.0 ) {
            // Ray leaves behind the source
            return 0.0;
        }
    }

    // Receiver factor times scale
    double cosThetaX;
    if ( receiverElement->isCluster() ) {
        cosThetaX = 0.25;
    } else {
        cosThetaX = -ray.direction.dotProduct(receiverElement->patch->getNormal());
        if ( cosThetaX <= 0.0 ) {
            // Ray hits receiver from the back
            return 0.0;
        }
    }

    // Equation [BEKA1996](1): unoccluded transport kernel (without reflectivity term)
    double formFactorKernelTerm = cosThetaX * cosThetaY / (M_PI * distance * distance);
    float shortenedDistance = static_cast<float>(distance * (1.0F - VSDK::EPSILON));

    // Determine transmissivity (visibility)
    RpkRayHit hitStore;
    double visibilityFactor; // Will be 0.0 or 1.0

    if ( !shadowGeometryList || shadowGeometryList->size() == 0 ) {
        visibilityFactor = 1.0;
    } else if ( !galerkinState->multiResolutionVisibility ) {
        if ( shadowTestDiscretization(
                &ray,
                shadowGeometryList,
                sceneQuery,
                sceneQueryWorkspace,
                formFactorWorkspace,
                shadowCache,
                shortenedDistance,
                &hitStore,
                isSceneGeometry,
                isClusteredGeometry,
                exclusions) == nullptr ) {
            // No intersection with occluders means no shadow, so full visibility
            visibilityFactor = 1.0;
        } else {
            // If intersection with occluders found, there is shadow, so no visibility
            visibilityFactor = 0.0;
        }
    } else if ( shadowCache->cacheHit(&ray, &shortenedDistance, &hitStore, exclusions) ) {
#ifdef RPK_RAY_CAPTURE
        if ( sceneQueryWorkspace != nullptr && sceneQueryWorkspace->getRecorder() != nullptr ) {
            sceneQueryWorkspace->getRecorder()->countCacheResolved();
        }
#endif
        visibilityFactor = 0.0;
    } else {
        // Case never used if clustering disabled
        float minimumFeatureSize = 2.0F
            * static_cast<float>(java::Math::sqrt(Statistics::instance().radiance.totalArea * galerkinState->relMinElemArea / M_PI));
        visibilityFactor = FormFactorClusteredStrategy::geomListMultiResolutionVisibility(
            shadowGeometryList, shadowCache, &ray, shortenedDistance, sourceElement->blockerSize, minimumFeatureSize, exclusions);
    }

    return formFactorKernelTerm * visibilityFactor;
}

inline void
FormFactorStrategy::computeInteractionFormFactor(
    const CubatureRule *receiverCubatureRule,
    const CubatureRule *sourceCubatureRule,
    const double Gxy[CubatureRule::MAXIMUM_NODES][CubatureRule::MAXIMUM_NODES],
    const GalerkinElement *sourceElement,
    const GalerkinElement *receiverElement,
    const GalerkinBasis *sourceBasis,
    const GalerkinBasis *receiverBasis,
    const ColorRgb *sourceRadiance,
    double *gMin,
    double *gMax,
    double deltaRadiance[][3],
    Interaction *twoPatchesInteraction)
{
    // 1. Determine basis function values \phi_{i,\alpha}(x_k) at sample positions on the
    //    receiver patch for all basis functions \alpha
    double receiverPhi[GalerkinBasis::MAX_BASIS_SIZE][CubatureRule::MAXIMUM_NODES]{};

    for ( int k = 0; k < receiverCubatureRule->numberOfNodes; k++ ) {
        if ( receiverElement->isCluster() ) {
            // Constant approximation on clusters
            if ( twoPatchesInteraction->numberOfBasisFunctionsOnReceiver != 1 ) {
                Logger::reportMessage("FormFactorStrategy", Logger::FATAL_ERROR, "doHigherOrderAreaToAreaFormFactor", "non-constant approximation on receiver cluster is not possible");
            }
            receiverPhi[0][k] = 1.0;
        } else {
            for ( int alpha = 0; alpha < twoPatchesInteraction->numberOfBasisFunctionsOnReceiver && receiverBasis != nullptr; alpha++ ) {
                receiverPhi[alpha][k] = receiverBasis->function[alpha](receiverCubatureRule->u[k], receiverCubatureRule->v[k]);
            }
        }
    }

    // 2. Start with clean deltaRadiance
    double sourcePhi[CubatureRule::MAXIMUM_NODES]{};

    for ( int i = 0; i < CubatureRule::MAXIMUM_NODES; i++ ) {
        sourcePhi[i] = 0.0;
    }

    // 3. Compute form factor and write it to interaction K
    for ( int beta = 0; beta < twoPatchesInteraction->numberOfBasisFunctionsOnSource; beta++ ) {
        // Determine basis function values \phi_{j,\beta}(x_l) at sample positions on the source patch
        if ( sourceElement->isCluster() ) {
            if ( beta > 0 ) {
                Logger::reportMessage("FormFactorStrategy", Logger::FATAL_ERROR, "doHigherOrderAreaToAreaFormFactor", "non-constant approximation on source cluster is not possible");
            }
            for ( int l = 0; l < sourceCubatureRule->numberOfNodes; l++ ) {
                sourcePhi[l] = 1.0;
            }
        } else {
            for ( int l = 0; l < sourceCubatureRule->numberOfNodes && sourceBasis != nullptr; l++ ) {
                sourcePhi[l] = sourceBasis->function[beta](sourceCubatureRule->u[l], sourceCubatureRule->v[l]);
            }
        }

        double gBeta[CubatureRule::MAXIMUM_NODES]; // G_beta[k] = G_{j,\beta}(x_k)
        double deltaBeta[CubatureRule::MAXIMUM_NODES]; // delta_beta[k] = \delta_{j,\beta}(x_k)

        for ( int k = 0; k < receiverCubatureRule->numberOfNodes; k++ ) {
            // Compute point-to-patch form factors for positions x_k on receiver and
            // basis function \beta on the source
            gBeta[k] = 0.0;
            for ( int l = 0; l < sourceCubatureRule->numberOfNodes; l++ ) {
                gBeta[k] += sourceCubatureRule->w[l] * Gxy[k][l] * sourcePhi[l];
            }
            gBeta[k] *= sourceElement->area;

            // First part of error estimate at receiver node x_k
            deltaBeta[k] = -gBeta[k];
        }

        for ( int alpha = 0; alpha < twoPatchesInteraction->numberOfBasisFunctionsOnReceiver; alpha++ ) {
            // Compute patch-to-patch form factor for basis function alpha on the
            // receiver and beta on the source
            double gAlphaBeta = 0.0;
            for ( int k = 0; k < receiverCubatureRule->numberOfNodes; k++ ) {
                gAlphaBeta += receiverCubatureRule->w[k] * receiverPhi[alpha][k] * gBeta[k];
            }
            twoPatchesInteraction->K[alpha * twoPatchesInteraction->numberOfBasisFunctionsOnSource + beta] = static_cast<float>(receiverElement->area * gAlphaBeta);

            // Second part of error estimate at receiver node x_k
            for ( int k = 0; k < receiverCubatureRule->numberOfNodes; k++ ) {
                deltaBeta[k] += gAlphaBeta * receiverPhi[alpha][k];
            }
        }

        const double source[3] = {sourceRadiance[beta].getR(), sourceRadiance[beta].getG(), sourceRadiance[beta].getB()};
        for ( int k = 0; k < receiverCubatureRule->numberOfNodes; k++ ) {
            const double weight = static_cast<float>(deltaBeta[k]);
            for ( int channel = 0; channel < 3; channel++ ) {
                deltaRadiance[k][channel] += weight * source[channel];
            }
        }

        if ( beta == 0 ) {
            // Determine minimum and maximum point-to-patch form factor
            for ( int k = 0; k < receiverCubatureRule->numberOfNodes; k++ ) {
                if ( gBeta[k] < *gMin ) {
                    *gMin = gBeta[k];
                }
                if ( gBeta[k] > *gMax ) {
                    *gMax = gBeta[k];
                }
            }
        }
    }
}

inline void
FormFactorStrategy::computeInteractionError(
    const CubatureRule *receiverCubatureRule,
    const GalerkinElement *receiverElement,
    const double gMin,
    const double gMax,
    const ColorRgb *sourceRadiance,
    double deltaRadiance[][3],
    Interaction *link)
{
    // Compute error and write it to interaction deltaK
    if ( ColorRgbMath::isBlack(sourceRadiance[0]) ) {
        // No source radiance: use constant radiance error approximation
        double gav = link->K[0] / receiverElement->area;
        link->deltaK = static_cast<float>(gMax - gav);
        if ( gav - gMin > link->deltaK ) {
            link->deltaK = static_cast<float>(gav - gMin);
        }
    } else {
        link->deltaK = 0.0;
        for ( int k = 0; k < receiverCubatureRule->numberOfNodes; k++ ) {
            const ColorRgb difference(deltaRadiance[k][0], deltaRadiance[k][1], deltaRadiance[k][2]);
            const ColorRgb relativeError = ColorRgbMath::divide(difference, sourceRadiance[0]);
            double delta = java::Math::abs(ColorRgbMath::maximumComponent(relativeError));
            if ( delta > link->deltaK ) {
                link->deltaK = static_cast<float>(delta);
            }
        }
    }
    link->numberOfReceiverCubaturePositions = 1; // One error estimation coefficient
}

/**
Higher order area to area form factor computation. See [BEKA1996].
*/
void
FormFactorStrategy::doHigherOrderAreaToAreaFormFactor(
    Interaction *twoPatchesInteraction,
    const CubatureRule *receiverCubatureRule,
    const CubatureRule *sourceCubatureRule,
    const double Gxy[CubatureRule::MAXIMUM_NODES][CubatureRule::MAXIMUM_NODES],
    const GalerkinState *galerkinState)
{
    // 1. Receiver and source basis description
    const GalerkinElement *receiverElement = twoPatchesInteraction->receiverElement;
    const GalerkinElement *sourceElement = twoPatchesInteraction->sourceElement;
    const GalerkinBasis *receiverBasis;
    const GalerkinBasis *sourceBasis;

    if ( receiverElement->isCluster() ) {
        // No basis description for clusters: we always use a constant approximation on clusters
        receiverBasis = nullptr;
    } else {
        receiverBasis = GalerkinBasis::basisForVertexCount(receiverElement->patch->getNumberOfVertices());
    }

    if ( sourceElement->isCluster() ) {
        sourceBasis = nullptr;
    } else {
        sourceBasis = GalerkinBasis::basisForVertexCount(sourceElement->patch->getNumberOfVertices());
    }

    // 2. Compute form factor (sets K)
    const ColorRgb *sourceRadiance = (galerkinState->galerkinIterationMethod == SOUTH_WELL) ?
         sourceElement->unShotRadiance : sourceElement->radiance;
    double deltaRadiance[CubatureRule::MAXIMUM_NODES][3]{}; // See Bekaert & Willems, p159 bottom
    double gMin = Numeric::HUGE_DOUBLE_VALUE;
    double gMax = -Numeric::HUGE_DOUBLE_VALUE;

    computeInteractionFormFactor(
        receiverCubatureRule,
        sourceCubatureRule,
        Gxy,
        sourceElement,
        receiverElement,
        sourceBasis,
        receiverBasis,
        sourceRadiance,
        &gMin,
        &gMax,
        deltaRadiance,
        twoPatchesInteraction);

    // 3. Compute error (sets deltaK)
    computeInteractionError(
        receiverCubatureRule, // Inputs
        receiverElement,
        gMin,
        gMax,
        sourceRadiance,
        deltaRadiance,
        twoPatchesInteraction);
}

/**
Area (or volume) to area (or volume) form factor:

IN:
  - link->receiverElement
  - link->sourceElement
  - link->numberOfBasisFunctionsOnReceiver
  - link->numberOfBasisFunctionsOnSource: receiver and source element
    and the number of basis functions to consider on them.
  - geometryShadowList: a list of possible occluders

OUT:
 - link->K: generalized form factor / form factors
 - link->deltaK: error estimation coefficients (to be used in the refinement oracle
   hierarchicRefinementEvaluateInteraction)
 - link->numberOfReceiverCubaturePositions: number of error estimation coefficients (only 1 for the moment)
 - link->visibility: visibility factor from 0 (totally occluded) to 255 (total visibility)

Assumptions:
- The first basis function on the elements is constant and equal to 1
- The basis functions are orthonormal on their reference domain (unit square or standard triangle)
- The parameter mapping on the elements is uniform
- The cubature rules defined for triangles, quads and boxes provides sample points in 2D (z = 0) or 3D,
  allowing area to area, area to volume and volume to volume computations

With these assumptions, the jacobians are all constant and equal to the area
of the elements and the basis overlap matrices are the area of the element times
the unit matrix, see [BEKA1996].

We always use a constant approximation on clusters. For the form factor
computations, a cluster area of one fourth of it's total surface area
is used, see [SILL1995b].
*/
void
FormFactorStrategy::computeAreaToAreaFormFactorVisibility(
    const SceneQuery *sceneQuery,
    SceneQueryWorkspace *sceneQueryWorkspace,
    FormFactorWorkspace *formFactorWorkspace,
    const java::ArrayList<RpkGeometry *> *geometryShadowList,
    const bool isSceneGeometry,
    const bool isClusteredGeometry,
    Interaction *link,
    const GalerkinState *galerkinState)
{
    GalerkinElement *receiverElement = link->receiverElement;
    GalerkinElement *sourceElement = link->sourceElement;

    if ( receiverElement->isCluster() || sourceElement->isCluster() ) {
        RpkAxisAlignedBoundingBox sourceBoundingBox;
        RpkAxisAlignedBoundingBox receiverBoundingBox;
        receiverElement->bounds(&receiverBoundingBox);
        sourceElement->bounds(&sourceBoundingBox);

        // Do not allow interactions between a pair of overlapping source and receiver
        if ( !receiverBoundingBox.disjointToOtherBoundingBox(&sourceBoundingBox) ) {
            // Take 0 as form factor
            if ( link->numberOfBasisFunctionsOnReceiver == 1 && link->numberOfBasisFunctionsOnSource == 1 ) {
                link->K[0] = 0.0F;
            } else {
                for ( int i = 0;
                      i < link->numberOfBasisFunctionsOnReceiver * link->numberOfBasisFunctionsOnSource;
                      i++ ) {
                    link->K[i] = 0.0F;
                }
            }

            // And a large error on the form factor
            link->deltaK = 1.0F;
            link->numberOfReceiverCubaturePositions = 1;

            // And half visibility
            link->visibility = 128;
            return;
        }
    } else {
        // We assume that no light transport can take place between a surface element
        // and itself. It would cause trouble if we would go on b.t.w.
        if ( receiverElement == sourceElement ) {
            // Take 0. as form factor
            if ( link->numberOfBasisFunctionsOnReceiver == 1 && link->numberOfBasisFunctionsOnSource == 1 ) {
                link->K[0] = 0.0F;
            } else {
                for ( int i = 0;
                      i < link->numberOfBasisFunctionsOnReceiver * link->numberOfBasisFunctionsOnSource;
                      i++ ) {
                    link->K[i] = 0.0F;
                }
            }

            // And a 0 error on the form factor
            link->deltaK = 0.0F;
            link->numberOfReceiverCubaturePositions = 1;

            // And full occlusion
            link->visibility = 0;
            return;
        }
    }

    // If the receiver is another one than before, determine the cubature
    // rule to be used on it and the nodes (positions on the patch)
    // Very often, the source or receiver element is the same as the one in
    // the previous call of the function. The cubature rules, nodes and kernel
    // values of the last pair are kept in the form factor workspace
    Vector3Df *x = formFactorWorkspace->getReceiverNodes();
    Vector3Df *y = formFactorWorkspace->getSourceNodes();
    double (*Gxy)[CubatureRule::MAXIMUM_NODES] = formFactorWorkspace->getKernel();
    const bool sameReceiver = receiverElement == formFactorWorkspace->getLastReceiver();
    const bool sameSource = sourceElement == formFactorWorkspace->getLastSource();

    if ( !sameReceiver ) {
        CubatureRule *rule = formFactorWorkspace->getReceiverCubatureRule();
        determineNodes(receiverElement, GalerkinRole::RECEIVER, galerkinState, &rule, x);
        formFactorWorkspace->setReceiverCubatureRule(rule);
    }

    // Same for the source element
    if ( !sameSource ) {
        CubatureRule *rule = formFactorWorkspace->getSourceCubatureRule();
        determineNodes(sourceElement, GalerkinRole::SOURCE, galerkinState, &rule, y);
        formFactorWorkspace->setSourceCubatureRule(rule);
    }
    const CubatureRule *receiveCubatureRule = formFactorWorkspace->getReceiverCubatureRule();
    const CubatureRule *sourceCubatureRule = formFactorWorkspace->getSourceCubatureRule();

    // Evaluate the radiosity kernel between each pair of nodes on the source
    // and the receiver element if at least receiver or source changed since
    // last time
    if ( !sameReceiver || !sameSource ) {
        // Use shadow caching for accelerating occlusion detection
        ShadowCache shadowCache;

        // Exclude the patches in order to avoid immediate self-intersections
        const RayExclusionSet exclusions(
            receiverElement->isCluster() ? nullptr : receiverElement->patch,
            receiverElement->isCluster() ? nullptr : receiverElement->patch->getTwin(),
            sourceElement->isCluster() ? nullptr : sourceElement->patch,
            sourceElement->isCluster() ? nullptr : sourceElement->patch->getTwin(),
            receiverElement->isCluster() ? receiverElement->geometry : nullptr,
            sourceElement->isCluster() ? sourceElement->geometry : nullptr);

#ifdef RPK_RAY_CAPTURE
        if ( sceneQueryWorkspace != nullptr && sceneQueryWorkspace->getRecorder() != nullptr
          && geometryShadowList != nullptr && geometryShadowList->size() > 0
          && !isClusteredGeometry && !isSceneGeometry ) {
            sceneQueryWorkspace->getRecorder()->recordGeometryList(geometryShadowList);
        }
#endif

        double maximumKernelValue = 0.0; // Compute maximum un-occluded kernel value
        unsigned visibilityCount = 0; // Count the number of rays that "pass" occluders
        for ( int r = 0; receiveCubatureRule != nullptr && r < receiveCubatureRule->numberOfNodes; r++ ) {
            for ( int s = 0; sourceCubatureRule != nullptr && s < sourceCubatureRule->numberOfNodes; s++ ) {
                Gxy[r][s] = evaluatePointsPairKernel(
                    &shadowCache,
                    sceneQuery,
                    sceneQueryWorkspace,
                    formFactorWorkspace,
                    &x[r],
                    &y[s],
                    receiverElement,
                    sourceElement,
                    geometryShadowList,
                    isSceneGeometry,
                    isClusteredGeometry,
                    galerkinState,
                    exclusions);

                if ( Gxy[r][s] > maximumKernelValue ) {
                    maximumKernelValue = Gxy[r][s];
                }
                if ( java::Math::abs(Gxy[r][s]) > VSDK::EPSILON ) {
                    visibilityCount++;
                }
            }
        }

        formFactorWorkspace->setMaximumKernelValue(maximumKernelValue);
        formFactorWorkspace->setVisibilityCount(visibilityCount);
    }

    const double maximumKernelValue = formFactorWorkspace->getMaximumKernelValue();
    const unsigned visibilityCount = formFactorWorkspace->getVisibilityCount();

    if ( visibilityCount != 0 ) {
        // Actually compute the form factors
        if ( link->numberOfBasisFunctionsOnReceiver == 1 && link->numberOfBasisFunctionsOnSource == 1 ) {
            // Constant (1) basis functions is used on clusters
            FormFactorClusteredStrategy::doConstantAreaToAreaFormFactor(link, receiveCubatureRule, sourceCubatureRule, Gxy);
        } else {
            // So far only linear (3) basis functions being used. Quadratic and cubic not being selected
            doHigherOrderAreaToAreaFormFactor(link, receiveCubatureRule, sourceCubatureRule, Gxy, galerkinState);
        }
    }

    // Remember receiver and source for next time
    formFactorWorkspace->setLastPair(receiverElement, sourceElement);

    if ( galerkinState->clusteringStrategy == GalerkinClusteringStrategy::ISOTROPIC
        && (receiverElement->isCluster() || sourceElement->isCluster()) ) {
        link->deltaK = static_cast<float>(maximumKernelValue * sourceElement->area);
    }

    // Returns the visibility: basically the fraction of rays that did not hit an occluder
    if ( sourceCubatureRule != nullptr && receiveCubatureRule != nullptr ) {
        link->visibility = static_cast<unsigned char>(static_cast<unsigned>(255.0 * static_cast<double>(visibilityCount) /
                                                                  static_cast<double>(receiveCubatureRule->numberOfNodes * sourceCubatureRule->
                                                                            numberOfNodes)));
    }

    if ( galerkinState->exactVisibility && geometryShadowList != nullptr && link->visibility == 255 ) {
        // Not full visibility, we missed the shadow!
        link->visibility = 254;
    }
}

#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "common/Statistics.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/ShadowCache.h"
#include "GALERKIN/processing/FormFactorClusteredStrategy.h"
#include "GALERKIN/processing/FormFactorStrategy.h"

/**
References:
- [BEKA1996] Ph. Bekaert, Y. D. Willems, "Error Control for Radiosity", Euro-graphics
  Rendering Workshop, Porto, Portugal, June 1996, pp 153 -164.

- [SILL1995b] F. Sillion, "A Unified Hierarchical Algorithm for Global Illumination
  with Scattering Volumes and Object Clusters", IEEE TVCG Vol 1 Nr 3, September 1995
*/

GalerkinElement *FormFactorStrategy::formFactorLastReceived;
GalerkinElement *FormFactorStrategy::formFactorLastSource;

/**
Tests whether the ray intersects the a geometry in the geometrySceneList. Returns
nullptr if the ray hits no geometries. Returns an arbitrary hit
patch if the ray does intersect one or more geometries. Intersections
further away than minimumDistance are ignored.
*/
RayHit *
FormFactorStrategy::shadowTestDiscretization(
    Ray *ray,
    const java::ArrayList<Geometry *> *geometrySceneList,
    const VoxelGrid *voxelGrid,
    ShadowCache *shadowCache,
    float minimumDistance,
    RayHit *hitStore,
    bool isSceneGeometry,
    bool isClusteredGeometry)
{
    GLOBAL_statistics.numberOfShadowRays++;
    RayHit *hit = shadowCache->cacheHit(ray, &minimumDistance, hitStore);
    if ( hit != nullptr ) {
        GLOBAL_statistics.numberOfShadowCacheHits++;
    } else {
        if ( !isClusteredGeometry && !isSceneGeometry ) {
            hit = geometryListDiscretizationIntersect(
                geometrySceneList,
                ray,
                Numeric::EPSILON_FLOAT * minimumDistance,
                &minimumDistance,
                RayHitFlag::FRONT | RayHitFlag::ANY,
                hitStore);
        } else {
            hit = voxelGrid->gridIntersect(
                ray,
                Numeric::EPSILON_FLOAT * minimumDistance,
                &minimumDistance,
                RayHitFlag::FRONT | RayHitFlag::ANY,
                hitStore);
        }
        if ( hit != nullptr ) {
            shadowCache->addToShadowCache(hit->getPatch());
        }
    }

    return hit;
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
        Vector3D x[CUBATURE_MAXIMUM_NODES])
{
    Matrix2x2 topTransform{};

    if ( element->isCluster() ) {
        *cr = galerkinState->clusterRule;

        BoundingBox boundingBox;
        element->bounds(&boundingBox);
        double dx = boundingBox.coordinates[MAX_X] - boundingBox.coordinates[MIN_X];
        double dy = boundingBox.coordinates[MAX_Y] - boundingBox.coordinates[MIN_Y];
        double dz = boundingBox.coordinates[MAX_Z] - boundingBox.coordinates[MIN_Z];
        for ( int k = 0; k < (*cr)->numberOfNodes; k++ ) {
            x[k].set(
                (float)(boundingBox.coordinates[MIN_X] + (*cr)->u[k] * dx),
                (float)(boundingBox.coordinates[MIN_Y] + (*cr)->v[k] * dy),
                (float)(boundingBox.coordinates[MIN_Z] + (*cr)->t[k] * dz));
        }
    } else {
        // What cubature rule should be used over the element
        switch ( element->patch->numberOfVertices ) {
            case 3:
                *cr = role == GalerkinRole::RECEIVER ? galerkinState->receiverTriangleCubatureRule : galerkinState->sourceTriangleCubatureRule;
                break;
            case 4:
                *cr = role == GalerkinRole::RECEIVER ? galerkinState->receiverQuadCubatureRule : galerkinState->sourceQuadCubatureRule;
                break;
            default:
                logFatal(4, "determineNodes", "Can only handle triangular and quadrilateral patches");
        }

        // Compute the transform relating positions on the element to positions on
        // the patch to which it belongs
        if ( element->transformToParent != nullptr ) {
            element->topTransform(&topTransform);
        }

        // Compute the positions x[k] corresponding to the nodes of the cubature rule
        // in the unit square or triangle used to parametrise the element
        for ( int k = 0; *cr != nullptr && k < (*cr)->numberOfNodes; k++ ) {
            Vector2D node;
            node.u = (float)(*cr)->u[k];
            node.v = (float)(*cr)->v[k];
            if ( element->transformToParent != nullptr ) {
                topTransform.transformPoint2D(node, node);
            }
            element->patch->uniformPoint(node.u, node.v, &x[k]);
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
    const VoxelGrid *sceneWorldVoxelGrid,
    const Vector3D *x,
    const Vector3D *y,
    const GalerkinElement *receiverElement,
    const GalerkinElement *sourceElement,
    const java::ArrayList<Geometry *> *shadowGeometryList,
    const bool isSceneGeometry,
    const bool isClusteredGeometry,
    const GalerkinState *galerkinState)
{
    // Trace the ray from source to receiver (y to x) to handle one-sided surfaces correctly
    Ray ray;
    ray.pos = *y;
    ray.dir.subtraction(*x, *y);
    double distance = ray.dir.norm();
    ray.dir.inverseScaledCopy((float) distance, ray.dir, Numeric::EPSILON_FLOAT);

    // Don't allow too nearby nodes to interact
    if ( distance < Numeric::EPSILON ) {
        logWarning("evaluatePointsPairKernel", "Nodes too close too each other (receiver id %d, source id %d)",
            receiverElement->id, sourceElement->id);
        return 0.0;
    }

    // Emitter factor times scale, see [SILL1995b]
    double cosThetaY;
    if ( sourceElement->isCluster() ) {
        cosThetaY = 0.25;
    } else {
        cosThetaY = ray.dir.dotProduct(sourceElement->patch->normal);
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
        cosThetaX = -ray.dir.dotProduct(receiverElement->patch->normal);
        if ( cosThetaX <= 0.0 ) {
            // Ray hits receiver from the back
            return 0.0;
        }
    }

    // Un-occluded kernel value (without reflectivity term) - see equation (1) from [BEKA1996]
    double formFactorKernelTerm = cosThetaX * cosThetaY / (M_PI * distance * distance);
    float shortenedDistance = (float)(distance * (1.0f - Numeric::EPSILON));

    // Determine transmissivity (visibility)
    RayHit hitStore;
    double visibilityFactor; // Will be 0.0 or 1.0

    if ( !shadowGeometryList || shadowGeometryList->size() == 0 ) {
        visibilityFactor = 1.0;
    } else if ( !galerkinState->multiResolutionVisibility ) {
        if ( shadowTestDiscretization(
                &ray,
                shadowGeometryList,
                sceneWorldVoxelGrid,
                shadowCache,
                shortenedDistance,
                &hitStore,
                isSceneGeometry,
                isClusteredGeometry) == nullptr ) {
            // No intersection with occluders means no shadow, so full visibility
            visibilityFactor = 1.0;
        } else {
            // If intersection with occluders found, there is shadow, so no visibility
            visibilityFactor = 0.0;
        }
    } else if ( shadowCache->cacheHit(&ray, &shortenedDistance, &hitStore) ) {
        visibilityFactor = 0.0;
    } else {
        // Case never used if clustering disabled
        float minimumFeatureSize = 2.0f
            * (float)java::Math::sqrt(GLOBAL_statistics.totalArea * galerkinState->relMinElemArea / M_PI);
        visibilityFactor = FormFactorClusteredStrategy::geomListMultiResolutionVisibility(
            shadowGeometryList, shadowCache, &ray, shortenedDistance, sourceElement->blockerSize, minimumFeatureSize);
    }

    return formFactorKernelTerm * visibilityFactor;
}

inline void
FormFactorStrategy::computeInteractionFormFactor(
    const CubatureRule *receiverCubatureRule,
    const CubatureRule *sourceCubatureRule,
    const double Gxy[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES],
    const GalerkinElement *sourceElement,
    const GalerkinElement *receiverElement,
    const GalerkinBasis *sourceBasis,
    const GalerkinBasis *receiverBasis,
    const ColorRgb *sourceRadiance,
    double *gMin,
    double *gMax,
    ColorRgb *deltaRadiance,
    Interaction *twoPatchesInteraction)
{
    // 1. Determine basis function values \phi_{i,\alpha}(x_k) at sample positions on the
    //    receiver patch for all basis functions \alpha
    double receiverPhi[MAX_BASIS_SIZE][CUBATURE_MAXIMUM_NODES]{};

    for ( int k = 0; k < receiverCubatureRule->numberOfNodes; k++ ) {
        if ( receiverElement->isCluster() ) {
            // Constant approximation on clusters
            if ( twoPatchesInteraction->numberOfBasisFunctionsOnReceiver != 1 ) {
                logFatal(-1, "doHigherOrderAreaToAreaFormFactor",
                         "non-constant approximation on receiver cluster is not possible");
            }
            receiverPhi[0][k] = 1.0;
        } else {
            for ( int alpha = 0; alpha < twoPatchesInteraction->numberOfBasisFunctionsOnReceiver && receiverBasis != nullptr; alpha++ ) {
                receiverPhi[alpha][k] = receiverBasis->function[alpha](receiverCubatureRule->u[k], receiverCubatureRule->v[k]);
            }
        }
    }

    // 2. Start with clean deltaRadiance
    double sourcePhi[CUBATURE_MAXIMUM_NODES]{};

    for ( int i = 0; i < CUBATURE_MAXIMUM_NODES; i++ ) {
        sourcePhi[i] = 0.0;
    }

    for ( int k = 0; k < receiverCubatureRule->numberOfNodes; k++ ) {
        deltaRadiance[k].clear();
    }

    // 3. Compute form factor and write it to interaction K
    for ( int beta = 0; beta < twoPatchesInteraction->numberOfBasisFunctionsOnSource; beta++ ) {
        // Determine basis function values \phi_{j,\beta}(x_l) at sample positions on the source patch
        if ( sourceElement->isCluster() ) {
            if ( beta > 0 ) {
                logFatal(-1, "doHigherOrderAreaToAreaFormFactor",
                         "non-constant approximation on source cluster is not possible");
            }
            for ( int l = 0; l < sourceCubatureRule->numberOfNodes; l++ ) {
                sourcePhi[l] = 1.0;
            }
        } else {
            for ( int l = 0; l < sourceCubatureRule->numberOfNodes && sourceBasis != nullptr; l++ ) {
                sourcePhi[l] = sourceBasis->function[beta](sourceCubatureRule->u[l], sourceCubatureRule->v[l]);
            }
        }

        double gBeta[CUBATURE_MAXIMUM_NODES]; // G_beta[k] = G_{j,\beta}(x_k)
        double deltaBeta[CUBATURE_MAXIMUM_NODES]; // delta_beta[k] = \delta_{j,\beta}(x_k)

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
            twoPatchesInteraction->K[alpha * twoPatchesInteraction->numberOfBasisFunctionsOnSource + beta] = (float)(receiverElement->area * gAlphaBeta);

            // Second part of error estimate at receiver node x_k
            for ( int k = 0; k < receiverCubatureRule->numberOfNodes; k++ ) {
                deltaBeta[k] += gAlphaBeta * receiverPhi[alpha][k];
            }
        }

        for ( int k = 0; k < receiverCubatureRule->numberOfNodes; k++ ) {
            deltaRadiance[k].addScaled(deltaRadiance[k], (float) deltaBeta[k], sourceRadiance[beta]);
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
    ColorRgb *deltaRadiance,
    Interaction *link)
{
    // Compute error and write it to interaction deltaK
    if ( link->deltaK != nullptr ) {
        delete[] link->deltaK;
    }
    link->deltaK = new float[1];
    if ( sourceRadiance[0].isBlack() ) {
        // No source radiance: use constant radiance error approximation
        double gav = link->K[0] / receiverElement->area;
        link->deltaK[0] = (float)(gMax - gav);
        if ( gav - gMin > link->deltaK[0] ) {
            link->deltaK[0] = (float)(gav - gMin);
        }
    } else {
        link->deltaK[0] = 0.0;
        for ( int k = 0; k < receiverCubatureRule->numberOfNodes; k++ ) {
            deltaRadiance[k].divide(deltaRadiance[k], sourceRadiance[0]);
            double delta = java::Math::abs(deltaRadiance[k].maximumComponent());
            if ( delta > link->deltaK[0] ) {
                link->deltaK[0] = (float)delta;
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
    const double Gxy[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES],
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
        receiverBasis =
            (receiverElement->patch->numberOfVertices == 3 ? &GLOBAL_galerkin_triBasis : &GLOBAL_galerkin_quadBasis);
    }

    if ( sourceElement->isCluster() ) {
        sourceBasis = nullptr;
    } else {
        sourceBasis =
            (sourceElement->patch->numberOfVertices == 3 ? &GLOBAL_galerkin_triBasis : &GLOBAL_galerkin_quadBasis);
    }

    // 2. Compute form factor (sets K)
    const ColorRgb *sourceRadiance = (galerkinState->galerkinIterationMethod == SOUTH_WELL) ?
         sourceElement->unShotRadiance : sourceElement->radiance;
    ColorRgb deltaRadiance[CUBATURE_MAXIMUM_NODES]; // See Bekaert & Willems, p159 bottom
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
    const VoxelGrid *sceneWorldVoxelGrid,
    const java::ArrayList<Geometry *> *geometryShadowList,
    const bool isSceneGeometry,
    const bool isClusteredGeometry,
    Interaction *link,
    const GalerkinState *galerkinState)
{
    // TODO: Check how to keep the cache, in a re-entrant implementation (use GalerkinState?)
    // Very often, the source or receiver element is the same as the one in
    // the previous call of the function. We cache cubature rules and nodes
    // in order to prevent re-computation
    static CubatureRule *receiveCubatureRule = nullptr; // Cubature rules to be used over the
    static CubatureRule *sourceCubatureRule = nullptr; // Receiving patch and source patch
    static Vector3D x[CUBATURE_MAXIMUM_NODES];
    static Vector3D y[CUBATURE_MAXIMUM_NODES];

    // TODO: To make this re-entrant, should use the class as instanced objects,
    // one per thread, and move static global variables to usual class attributes
    formFactorLastReceived = nullptr;
    formFactorLastSource = nullptr;

    GalerkinElement *receiverElement = link->receiverElement;
    GalerkinElement *sourceElement = link->sourceElement;

    if ( receiverElement->isCluster() || sourceElement->isCluster() ) {
        BoundingBox sourceBoundingBox;
        BoundingBox receiverBoundingBox;
        receiverElement->bounds(&receiverBoundingBox);
        sourceElement->bounds(&sourceBoundingBox);

        // Do not allow interactions between a pair of overlapping source and receiver
        if ( !receiverBoundingBox.disjointToOtherBoundingBox(&sourceBoundingBox) ) {
            // Take 0 as form factor
            if ( link->numberOfBasisFunctionsOnReceiver == 1 && link->numberOfBasisFunctionsOnSource == 1 ) {
                link->K[0] = 0.0f;
            } else {
                for ( int i = 0;
                      i < link->numberOfBasisFunctionsOnReceiver * link->numberOfBasisFunctionsOnSource;
                      i++ ) {
                    link->K[i] = 0.0f;
                }
            }

            // And a large error on the form factor
            link->deltaK = new float[1];
            link->deltaK[0] = 1.0f;
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
                link->K[0] = 0.0f;
            } else {
                for ( int i = 0;
                      i < link->numberOfBasisFunctionsOnReceiver * link->numberOfBasisFunctionsOnSource;
                      i++ ) {
                    link->K[i] = 0.0f;
                }
            }

            // And a 0 error on the form factor
            link->deltaK = new float[1];
            link->deltaK[0] = 0.0f;
            link->numberOfReceiverCubaturePositions = 1;

            // And full occlusion
            link->visibility = 0;
            return;
        }
    }

    // If the receiver is another one than before, determine the cubature
    // rule to be used on it and the nodes (positions on the patch)
    if ( receiverElement != formFactorLastReceived ) {
        determineNodes(receiverElement, GalerkinRole::RECEIVER, galerkinState, &receiveCubatureRule, x);
    }

    // Same for the source element
    if ( sourceElement != formFactorLastSource ) {
        determineNodes(sourceElement, GalerkinRole::SOURCE, galerkinState, &sourceCubatureRule, y);
    }

    // Evaluate the radiosity kernel between each pair of nodes on the source
    // and the receiver element if at least receiver or source changed since
    // last time
    double maximumKernelValue = 0.0;
    double Gxy[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES];
    unsigned visibilityCount = 0; // Number of rays that "pass" occluders

        if ( receiverElement != formFactorLastReceived || sourceElement != formFactorLastSource ) {
        // Use shadow caching for accelerating occlusion detection
        ShadowCache shadowCache;

        // Mark the patches in order to avoid immediate self-intersections
        Patch::dontIntersect(
            4,
            receiverElement->isCluster() ? nullptr : receiverElement->patch,
            receiverElement->isCluster() ? nullptr : receiverElement->patch->twin,
            sourceElement->isCluster() ? nullptr : sourceElement->patch,
            sourceElement->isCluster() ? nullptr : sourceElement->patch->twin);
        geomDontIntersect(
            receiverElement->isCluster() ? receiverElement->geometry : nullptr,
            sourceElement->isCluster() ? sourceElement->geometry : nullptr);

        maximumKernelValue = 0.0; // Compute maximum un-occluded kernel value
        visibilityCount = 0; // Count the number of rays that "pass" occluders
        for ( int r = 0; receiveCubatureRule != nullptr && r < receiveCubatureRule->numberOfNodes; r++ ) {
            for ( int s = 0; sourceCubatureRule != nullptr && s < sourceCubatureRule->numberOfNodes; s++ ) {
                Gxy[r][s] = evaluatePointsPairKernel(
                    &shadowCache,
                    sceneWorldVoxelGrid,
                    &x[r],
                    &y[s],
                    receiverElement,
                    sourceElement,
                    geometryShadowList,
                    isSceneGeometry,
                    isClusteredGeometry,
                    galerkinState);

                if ( Gxy[r][s] > maximumKernelValue ) {
                    maximumKernelValue = Gxy[r][s];
                }
                if ( java::Math::abs(Gxy[r][s]) > Numeric::EPSILON ) {
                    visibilityCount++;
                }
            }
        }

        // Unmark the patches, so they are considered for ray-patch intersections again in future
        Patch::dontIntersect(0);
        geomDontIntersect(nullptr, nullptr);
    }

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
    formFactorLastReceived = receiverElement;
    formFactorLastSource = sourceElement;

    if ( galerkinState->clusteringStrategy == GalerkinClusteringStrategy::ISOTROPIC
        && (receiverElement->isCluster() || sourceElement->isCluster()) ) {
        if ( link-> deltaK != nullptr ) {
            delete[] link->deltaK;
        }
        link->deltaK = new float[1];
        link->deltaK[0] = (float)(maximumKernelValue * sourceElement->area);
    }

    // Returns the visibility: basically the fraction of rays that did not hit an occluder
    if ( sourceCubatureRule != nullptr && receiveCubatureRule != nullptr ) {
        link->visibility = (unsigned char) ((unsigned) (255.0 * (double) visibilityCount /
            (double) (receiveCubatureRule->numberOfNodes * sourceCubatureRule->numberOfNodes)));
    }

    if ( galerkinState->exactVisibility && geometryShadowList != nullptr && link->visibility == 255 ) {
        // Not full visibility, we missed the shadow!
        link->visibility = 254;
    }
}

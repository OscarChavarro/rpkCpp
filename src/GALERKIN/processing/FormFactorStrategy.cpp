#include "common/error.h"
#include "common/mymath.h"
#include "material/statistics.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/processing/FormFactorClusteredStrategy.h"
#include "GALERKIN/processing/FormFactorStrategy.h"

Patch * FormFactorStrategy::patchCache[MAX_CACHE];
int FormFactorStrategy::cachedPatches;
int FormFactorStrategy::numberOfCachedPatches;

/**
Initialize/empty the shadow cache
*/
void
FormFactorStrategy::initShadowCache() {
    FormFactorStrategy::numberOfCachedPatches = 0;
    FormFactorStrategy::cachedPatches = 0;
    for ( int i = 0; i < MAX_CACHE; i++ ) {
        FormFactorStrategy::patchCache[i] = nullptr;
    }
}

/**
Test ray against patches in the shadow cache. Returns nullptr if the ray hits
no patches in the shadow cache, or a pointer to the first hit patch otherwise
*/
RayHit *
FormFactorStrategy::cacheHit(Ray *ray, float *dist, RayHit *hitStore) {
    for ( int i = 0; i < FormFactorStrategy::numberOfCachedPatches; i++ ) {
        RayHit *hit = FormFactorStrategy::patchCache[i]->intersect(
            ray, EPSILON_FLOAT * (*dist), dist, HIT_FRONT | HIT_ANY, hitStore);
        if ( hit != nullptr ) {
            return hit;
        }
    }
    return nullptr;
}

/**
Replace least recently added patch
*/
void
FormFactorStrategy::addToShadowCache(Patch *patch) {
    FormFactorStrategy::patchCache[cachedPatches % MAX_CACHE] = patch;
    FormFactorStrategy::cachedPatches++;
    if ( FormFactorStrategy::numberOfCachedPatches < MAX_CACHE ) {
        FormFactorStrategy::numberOfCachedPatches++;
    }
}

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
    float minimumDistance,
    RayHit *hitStore,
    bool isSceneGeometry,
    bool isClusteredGeometry)
{
    GLOBAL_statistics.numberOfShadowRays++;
    RayHit *hit = cacheHit(ray, &minimumDistance, hitStore);
    if ( hit != nullptr ) {
        GLOBAL_statistics.numberOfShadowCacheHits++;
    } else {
        if ( !isClusteredGeometry && !isSceneGeometry ) {
            hit = geometryListDiscretizationIntersect(
                geometrySceneList,
                ray,
                EPSILON_FLOAT * minimumDistance,
                &minimumDistance,
                HIT_FRONT | HIT_ANY,
                hitStore);
        } else {
            hit = voxelGrid->gridIntersect(
                ray,
                EPSILON_FLOAT * minimumDistance,
                &minimumDistance,
                HIT_FRONT | HIT_ANY,
                hitStore);
        }
        if ( hit ) {
            addToShadowCache(hit->patch);
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
                *cr = role == GalerkinRole::RECEIVER ? galerkinState->rcv3rule : galerkinState->src3rule;
                break;
            case 4:
                *cr = role == GalerkinRole::RECEIVER ? galerkinState->rcv4rule : galerkinState->src4rule;
                break;
            default:
                logFatal(4, "determineNodes", "Can only handle triangular and quadrilateral patches");
        }

        // Compute the transform relating positions on the element to positions on
        // the patch to which it belongs
        if ( element->upTrans != nullptr ) {
            element->topTransform(&topTransform);
        }

        // Compute the positions x[k] corresponding to the nodes of the cubature rule
        // in the unit square or triangle used to parametrise the element
        for ( int k = 0; cr != nullptr && *cr != nullptr && k < (*cr)->numberOfNodes; k++ ) {
            Vector2D node;
            node.u = (float)(*cr)->u[k];
            node.v = (float)(*cr)->v[k];
            if ( element->upTrans != nullptr ) {
                transformPoint2D(topTransform, node, node);
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
    vectorSubtract(*x, *y, ray.dir);
    double distance = vectorNorm(ray.dir);
    vectorScaleInverse((float)distance, ray.dir, ray.dir);

    // Don't allow too nearby nodes to interact
    if ( distance < EPSILON ) {
        logWarning("evaluatePointsPairKernel", "Nodes too close too each other (receiver id %d, source id %d)",
            receiverElement->id, sourceElement->id);
        return 0.0;
    }

    // Emitter factor times scale, see [SILL1995b]
    double cosThetaY;
    if ( sourceElement->isCluster() ) {
        cosThetaY = 0.25;
    } else {
        cosThetaY = vectorDotProduct(ray.dir, sourceElement->patch->normal);
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
        cosThetaX = -vectorDotProduct(ray.dir, receiverElement->patch->normal);
        if ( cosThetaX <= 0.0 ) {
            // Ray hits receiver from the back
            return 0.0;
        }
    }

    // Un-occluded kernel value (without reflectivity term) - see equation (1) from [BEKA1996]
    double formFactorKernelTerm = cosThetaX * cosThetaY / (M_PI * distance * distance);
    float shortenedDistance = (float)(distance * (1.0f - EPSILON));

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
    } else if ( cacheHit(&ray, &shortenedDistance, &hitStore) ) {
        visibilityFactor = 0.0;
    } else {
        // Case never used if clustering disabled
        float minimumFeatureSize = 2.0f
            * (float)std::sqrt(GLOBAL_statistics.totalArea * galerkinState->relMinElemArea / M_PI);
        visibilityFactor = FormFactorClusteredStrategy::geomListMultiResolutionVisibility(
            shadowGeometryList, &ray, shortenedDistance, sourceElement->blockerSize, minimumFeatureSize);
    }

    return formFactorKernelTerm * visibilityFactor;
}

/**
Higher order area to area form factor computation. See

- [BEKA1996] Ph. Bekaert, Y. D. Willems, "Error Control for Radiosity", Euro-graphics Rendering
Workshop, Porto, Portugal, June 1996, p 158.
*/
void
FormFactorStrategy::doHigherOrderAreaToAreaFormFactor(
    Interaction *link,
    const CubatureRule *cubatureRuleRcv,
    const CubatureRule *cubatureRuleSrc,
    const double Gxy[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES],
    const GalerkinState *galerkinState)
{
    double rcvPhi[MAX_BASIS_SIZE][CUBATURE_MAXIMUM_NODES]{};
    double srcPhi[CUBATURE_MAXIMUM_NODES];
    const GalerkinElement *receiverElement = link->receiverElement;
    const GalerkinElement *sourceElement = link->sourceElement;
    const ColorRgb *sourceRadiance = (galerkinState->galerkinIterationMethod == SOUTH_WELL) ?
                               sourceElement->unShotRadiance : sourceElement->radiance;

    // Receiver and source basis description
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

    // Determine basis function values \phi_{i,\alpha}(x_k) at sample positions on the
    // receiver patch for all basis functions \alpha
    ColorRgb deltaRadiance[CUBATURE_MAXIMUM_NODES]; // See Bekaert & Willems, p159 bottom

    for ( int k = 0; k < cubatureRuleRcv->numberOfNodes; k++ ) {
        if ( receiverElement->isCluster() ) {
            // Constant approximation on clusters
            if ( link->numberOfBasisFunctionsOnReceiver != 1 ) {
                logFatal(-1, "doHigherOrderAreaToAreaFormFactor",
                         "non-constant approximation on receiver cluster is not possible");
            }
            rcvPhi[0][k] = 1.0;
        } else {
            for ( int alpha = 0; alpha < link->numberOfBasisFunctionsOnReceiver && receiverBasis != nullptr; alpha++ ) {
                rcvPhi[alpha][k] = receiverBasis->function[alpha](cubatureRuleRcv->u[k], cubatureRuleRcv->v[k]);
            }
        }
        deltaRadiance[k].clear();
    }

    double gMin = HUGE;
    double gMax = -HUGE;
    for ( int beta = 0; beta < link->numberOfBasisFunctionsOnSource; beta++ ) {
        // Determine basis function values \phi_{j,\beta}(x_l) at sample positions on the source patch
        if ( sourceElement->isCluster() ) {
            if ( beta > 0 ) {
                logFatal(-1, "doHigherOrderAreaToAreaFormFactor",
                         "non-constant approximation on source cluster is not possible");
            }
            for ( int l = 0; l < cubatureRuleSrc->numberOfNodes; l++ ) {
                srcPhi[l] = 1.0;
            }
        } else {
            for ( int l = 0; l < cubatureRuleSrc->numberOfNodes && sourceBasis != nullptr; l++ ) {
                srcPhi[l] = sourceBasis->function[beta](cubatureRuleSrc->u[l], cubatureRuleSrc->v[l]);
            }
        }

        double gBeta[CUBATURE_MAXIMUM_NODES]; // G_beta[k] = G_{j,\beta}(x_k)
        double deltaBeta[CUBATURE_MAXIMUM_NODES]; // delta_beta[k] = \delta_{j,\beta}(x_k)

        for ( int k = 0; k < cubatureRuleRcv->numberOfNodes; k++ ) {
            // Compute point-to-patch form factors for positions x_k on receiver and
            // basis function \beta on the source
            gBeta[k] = 0.0;
            for ( int l = 0; l < cubatureRuleSrc->numberOfNodes; l++ ) {
                gBeta[k] += cubatureRuleSrc->w[l] * Gxy[k][l] * srcPhi[l];
            }
            gBeta[k] *= sourceElement->area;

            // First part of error estimate at receiver node x_k
            deltaBeta[k] = -gBeta[k];
        }

        for ( int alpha = 0; alpha < link->numberOfBasisFunctionsOnReceiver; alpha++ ) {
            // Compute patch-to-patch form factor for basis function alpha on the
            // receiver and beta on the source
            double gAlphaBeta = 0.0;
            for ( int k = 0; k < cubatureRuleRcv->numberOfNodes; k++ ) {
                gAlphaBeta += cubatureRuleRcv->w[k] * rcvPhi[alpha][k] * gBeta[k];
            }
            link->K[alpha * link->numberOfBasisFunctionsOnSource + beta] = (float)(receiverElement->area * gAlphaBeta);

            // Second part of error estimate at receiver node x_k
            for ( int k = 0; k < cubatureRuleRcv->numberOfNodes; k++ ) {
                deltaBeta[k] += gAlphaBeta * rcvPhi[alpha][k];
            }
        }

        for ( int k = 0; k < cubatureRuleRcv->numberOfNodes; k++ ) {
            deltaRadiance[k].addScaled(deltaRadiance[k], (float) deltaBeta[k], sourceRadiance[beta]);
        }

        if ( beta == 0 ) {
            // Determine minimum and maximum point-to-patch form factor
            for ( int k = 0; k < cubatureRuleRcv->numberOfNodes; k++ ) {
                if ( gBeta[k] < gMin ) {
                    gMin = gBeta[k];
                }
                if ( gBeta[k] > gMax ) {
                    gMax = gBeta[k];
                }
            }
        }
    }

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
        for ( int k = 0; k < cubatureRuleRcv->numberOfNodes; k++ ) {
            deltaRadiance[k].divide(deltaRadiance[k], sourceRadiance[0]);
            double delta = std::fabs(deltaRadiance[k].maximumComponent());
            if ( delta > link->deltaK[0] ) {
                link->deltaK[0] = (float)delta;
            }
        }
    }
    link->numberOfReceiverCubaturePositions = 1; // One error estimation coefficient
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
 - link->K
 - link->deltaK: generalized form factor(s) and error estimation coefficients (to be used in the refinement oracle
   hierarchicRefinementEvaluateInteraction()
 - link->numberOfReceiverCubaturePositions: number of error estimation coefficients (only 1 for the moment)
 - link->visibility: visibility factor from 0 (totally occluded) to 255 (total visibility)

The caller provides enough storage for storing the coefficients.

Assumptions:
- The first basis function on the elements is constant and equal to 1
- The basis functions are orthonormal on their reference domain (unit square or standard triangle)
- The parameter mapping on the elements is uniform

With these assumptions, the jacobians are all constant and equal to the area
of the elements and the basis overlap matrices are the area of the element times
the unit matrix

Reference:

- [BEKA1996] Ph. Bekaert, Y. D. Willems, "Error Control for Radiosity", Euro-graphics
	Rendering Workshop, Porto, Portugal, June 1996, pp 153--164.

We always use a constant approximation on clusters. For the form factor
computations, a cluster area of one fourth of it's total surface area
is used.

Reference:

- [SILL1995b] F. Sillion, "A Unified Hierarchical Algorithm for Global Illumination
with Scattering Volumes and Object Clusters", IEEE TVCG Vol 1 Nr 3,
sept 1995
*/
void
FormFactorStrategy::computeAreaToAreaFormFactorVisibility(
    const VoxelGrid *sceneWorldVoxelGrid,
    const java::ArrayList<Geometry *> *geometryShadowList,
    const bool isSceneGeometry,
    const bool isClusteredGeometry,
    Interaction *link,
    GalerkinState *galerkinState)
{
    // TODO: Check how to keep the cache, in a re-entrant implementation (use GalerkinState?)
    // Very often, the source or receiver element is the same as the one in
    // the previous call of the function. We cache cubature rules and nodes
    // in order to prevent re-computation
    static CubatureRule *receiveCubatureRule = nullptr; // Cubature rules to be used over the
    static CubatureRule *sourceCubatureRule = nullptr; // Receiving patch and source patch
    static Vector3D x[CUBATURE_MAXIMUM_NODES];
    static Vector3D y[CUBATURE_MAXIMUM_NODES];

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
    if ( receiverElement != galerkinState->formFactorLastReceived ) {
        determineNodes(receiverElement, RECEIVER, galerkinState, &receiveCubatureRule, x);
    }

    // Same for the source element
    if ( sourceElement != galerkinState->formFactorLastSource ) {
        determineNodes(sourceElement, SOURCE, galerkinState, &sourceCubatureRule, y);
    }

    // Evaluate the radiosity kernel between each pair of nodes on the source
    // and the receiver element if at least receiver or source changed since
    // last time
    double maximumKernelValue = 0.0;
    double Gxy[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES];
    unsigned visibilityCount = 0; // Number of rays that "pass" occluders

    if ( receiverElement != galerkinState->formFactorLastReceived
      || sourceElement != galerkinState->formFactorLastSource ) {
        // Use shadow caching for accelerating occlusion detection
        initShadowCache();

        // Mark the patches in order to avoid immediate self-intersections
        Patch::dontIntersect(4, receiverElement->isCluster() ? nullptr : receiverElement->patch,
                             receiverElement->isCluster() ? nullptr : receiverElement->patch->twin,
                             sourceElement->isCluster() ? nullptr : sourceElement->patch,
                             sourceElement->isCluster() ? nullptr : sourceElement->patch->twin);
        geomDontIntersect(receiverElement->isCluster() ? receiverElement->geometry : nullptr,
                          sourceElement->isCluster() ? sourceElement->geometry : nullptr);

        maximumKernelValue = 0.0; // Compute maximum un-occluded kernel value
        visibilityCount = 0; // Count the number of rays that "pass" occluders
        for ( int r = 0; receiveCubatureRule != nullptr && r < receiveCubatureRule->numberOfNodes; r++ ) {
            for ( int s = 0; sourceCubatureRule != nullptr && s < sourceCubatureRule->numberOfNodes; s++ ) {
                Gxy[r][s] = evaluatePointsPairKernel(
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
                if ( std::fabs(Gxy[r][s]) > EPSILON ) {
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
    galerkinState->formFactorLastReceived = receiverElement;
    galerkinState->formFactorLastSource = sourceElement;

    if ( galerkinState->clusteringStrategy == ISOTROPIC
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

#include "common/error.h"
#include "common/mymath.h"
#include "material/statistics.h"
#include "GALERKIN/shadowcaching.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/mrvisibility.h"
#include "GALERKIN/processing/FormFactorStrategy.h"

/**
This routine determines the cubature rule to be used on the given element
and computes the positions on the elements patch that correspond to the nodes
of the cubature rule on the element. The role (RECEIVER or SOURCE) is only
relevant for surface elements
*/
void
FormFactorStrategy::determineNodes(
    GalerkinElement *element,
    CubatureRule **cr,
    Vector3D x[CUBATURE_MAXIMUM_NODES],
    const GalerkinRole role,
    const GalerkinState *galerkinState)
{
    Matrix2x2 topTransform{};

    if ( element->isCluster() ) {
        BoundingBox boundingBox;
        double dx;
        double dy;
        double dz;

        *cr = galerkinState->clusterRule;

        element->bounds(&boundingBox);
        dx = boundingBox.coordinates[MAX_X] - boundingBox.coordinates[MIN_X];
        dy = boundingBox.coordinates[MAX_Y] - boundingBox.coordinates[MIN_Y];
        dz = boundingBox.coordinates[MAX_Z] - boundingBox.coordinates[MIN_Z];
        for ( int k = 0; k < (*cr)->numberOfNodes; k++ ) {
            x[k].set(
            (float) (boundingBox.coordinates[MIN_X] + (*cr)->u[k] * dx),
            (float) (boundingBox.coordinates[MIN_Y] + (*cr)->v[k] * dy),
            (float) (boundingBox.coordinates[MIN_Z] + (*cr)->t[k] * dz));
        }
    } else {
        // What cubature rule should be used over the element
        switch ( element->patch->numberOfVertices ) {
            case 3:
                *cr = role == RECEIVER ? galerkinState->rcv3rule : galerkinState->src3rule;
                break;
            case 4:
                *cr = role == RECEIVER ? galerkinState->rcv4rule : galerkinState->src4rule;
                break;
            default:
                logFatal(4, "determineNodes", "Can only handle triangular and quadrilateral patches");
        }

        // Compute the transform relating positions on the element to positions on
        // the patch to which it belongs
        if ( element->upTrans ) {
            element->topTransform(&topTransform);
        }

        // Compute the positions x[k] corresponding to the nodes of the cubature rule
        // in the unit square or triangle used to parametrise the element
        for ( int k = 0; k < (*cr)->numberOfNodes; k++ ) {
            Vector2D node;
            node.u = (float)(*cr)->u[k];
            node.v = (float)(*cr)->v[k];
            if ( element->upTrans ) transformPoint2D(topTransform, node, node);
            element->patch->uniformPoint(node.u, node.v, &x[k]);
        }
    }
}

/**
Evaluates the radiosity kernel (*not* taking into account the reflectivity of
the receiver) at positions x on the receiver rcv and y on
source src. ShadowList is a list of potential occluders.
Shadow caching is used to speed up the visibility detection
between x and y. The shadow cache is initialized before doing the first
evaluation of the kernel between each pair of patches. The un-occluded
point-to-point form factor is returned. Visibility is filled in vis
To be used when integrating over the surface of the source element
*/
double
FormFactorStrategy::pointKernelEval(
    const VoxelGrid *sceneWorldVoxelGrid,
    const Vector3D *x,
    const Vector3D *y,
    const GalerkinElement *receiverElement,
    const GalerkinElement *sourceElement,
    const java::ArrayList<Geometry *> *shadowGeometryList,
    double *vis,
    const bool isSceneGeometry,
    const bool isClusteredGeometry,
    const GalerkinState *galerkinState)
{
    Ray ray;
    double dist;

    // Trace the ray from source to receiver (y to x) to handle one-sided surfaces correctly
    ray.pos = *y;
    vectorSubtract(*x, *y, ray.dir);
    dist = vectorNorm(ray.dir);
    vectorScaleInverse((float)dist, ray.dir, ray.dir);

    // Don't allow too nearby nodes to interact
    if ( dist < EPSILON ) {
        logWarning("pointKernelEval", "Nodes too close too each other (receiver id %d, source id %d)", receiverElement->id, sourceElement->id);
        return 0.0;
    }

    // Emitter factor times scale, see [SILL1995b] Sillion, "A Unified Hierarchical ...", IEEE TVCG Vol 1 nr 3, sept 1995
    double cosQ;
    if ( sourceElement->isCluster() ) {
        cosQ = 0.25;
    } else {
        cosQ = vectorDotProduct(ray.dir, sourceElement->patch->normal);
        if ( cosQ <= 0.0 ) {
            // Ray leaves behind the source
            return 0.0;
        }
    }

    // Receiver factor times scale
    double cosP;
    if ( receiverElement->isCluster() ) {
        cosP = 0.25;
    } else {
        cosP = -vectorDotProduct(ray.dir, receiverElement->patch->normal);
        if ( cosP <= 0.0 ) {
            // Ray hits receiver from the back
            return 0.0;
        }
    }

    // Un-occluded kernel value
    double formFactor = cosP * cosQ / (M_PI * dist * dist);
    float distance = (float)(dist * (1.0f - EPSILON));

    // Determine transmissivity (visibility)
    RayHit hitStore;
    if ( !shadowGeometryList ) {
        *vis = 1.0;
    } else if ( !galerkinState->multiResolutionVisibility ) {
        if ( shadowTestDiscretization(
                &ray,
                shadowGeometryList,
                sceneWorldVoxelGrid,
                distance,
                &hitStore,
                isSceneGeometry,
                isClusteredGeometry) == nullptr ) {
            *vis = 1.0;
        } else {
            *vis = 0.0;
        }
    } else if ( cacheHit(&ray, &distance, &hitStore) ) {
        *vis = 0.0;
    } else {
        float minimumFeatureSize = 2.0f * (float)std::sqrt(GLOBAL_statistics.totalArea * galerkinState->relMinElemArea / M_PI);
        *vis = geomListMultiResolutionVisibility(shadowGeometryList, &ray, distance, sourceElement->blockerSize, minimumFeatureSize);
    }

    return formFactor;
}

/**
Higher order area to area form factor computation. See

- Ph. Bekaert, Y. D. Willems, "Error Control for Radiosity", Euro-graphics Rendering
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
    static ColorRgb deltaRadiance[CUBATURE_MAXIMUM_NODES]; // See Bekaert & Willems, p159 bottom
    static double rcvPhi[MAX_BASIS_SIZE][CUBATURE_MAXIMUM_NODES];
    static double srcPhi[CUBATURE_MAXIMUM_NODES];
    static double gBeta[CUBATURE_MAXIMUM_NODES]; // G_beta[k] = G_{j,\beta}(x_k)
    static double deltaBeta[CUBATURE_MAXIMUM_NODES]; // delta_beta[k] = \delta_{j,\beta}(x_k)

    GalerkinElement *receiverElement = link->receiverElement;
    GalerkinElement *sourceElement = link->sourceElement;
    GalerkinBasis *receiverBasis;
    GalerkinBasis *sourceBasis;
    double gAlphaBeta;
    double gMin;
    double gMax;
    double gav;
    int alpha;
    int beta;
    ColorRgb *srcRad = (galerkinState->galerkinIterationMethod == SOUTH_WELL) ?
                       sourceElement->unShotRadiance : sourceElement->radiance;

    // Receiver and source basis description
    if ( receiverElement->isCluster() ) {
        // No basis description for clusters: we always use a constant approximation on clusters
        receiverBasis = nullptr;
    } else {
        receiverBasis = (receiverElement->patch->numberOfVertices == 3 ? &GLOBAL_galerkin_triBasis : &GLOBAL_galerkin_quadBasis);
    }

    if ( sourceElement->isCluster() ) {
        sourceBasis = nullptr;
    } else {
        sourceBasis = (sourceElement->patch->numberOfVertices == 3 ? &GLOBAL_galerkin_triBasis : &GLOBAL_galerkin_quadBasis);
    }

    // Determine basis function values \phi_{i,\alpha}(x_k) at sample positions on the
    // receiver patch for all basis functions \alpha
    for ( int k = 0; k < cubatureRuleRcv->numberOfNodes; k++ ) {
        if ( receiverElement->isCluster() ) {
            // Constant approximation on clusters
            if ( link->numberOfBasisFunctionsOnReceiver != 1 ) {
                logFatal(-1, "doHigherOrderAreaToAreaFormFactor",
                         "non-constant approximation on receiver cluster is not possible");
            }
            rcvPhi[0][k] = 1.0;
        } else {
            for ( alpha = 0; alpha < link->numberOfBasisFunctionsOnReceiver && receiverBasis != nullptr; alpha++ ) {
                rcvPhi[alpha][k] = receiverBasis->function[alpha](cubatureRuleRcv->u[k], cubatureRuleRcv->v[k]);
            }
        }
        deltaRadiance[k].clear();
    }

    gMin = HUGE;
    gMax = -HUGE;
    for ( beta = 0; beta < link->numberOfBasisFunctionsOnSource; beta++ ) {
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

        for ( alpha = 0; alpha < link->numberOfBasisFunctionsOnReceiver; alpha++ ) {
            // Compute patch-to-patch form factor for basis function alpha on the
            // receiver and beta on the source
            gAlphaBeta = 0.0;
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
            deltaRadiance[k].addScaled(deltaRadiance[k], (float) deltaBeta[k], srcRad[beta]);
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
    if ( srcRad[0].isBlack() ) {
        // No source radiance: use constant radiance error approximation
        gav = link->K[0] / receiverElement->area;
        link->deltaK[0] = (float)(gMax - gav);
        if ( gav - gMin > link->deltaK[0] ) {
            link->deltaK[0] = (float)(gav - gMin);
        }
    } else {
        link->deltaK[0] = 0.0;
        for ( int k = 0; k < cubatureRuleRcv->numberOfNodes; k++ ) {
            double delta;

            deltaRadiance[k].divide(deltaRadiance[k], srcRad[0]);
            if ( (delta = std::fabs(deltaRadiance[k].maximumComponent())) > link->deltaK[0] ) {
                link->deltaK[0] = (float)delta;
            }
        }
    }
    link->numberOfReceiverCubaturePositions = 1; // One error estimation coefficient
}

/**
Like above, but with a constant approximation on both the receiver and source
element, which makes things slightly simpler
*/
void
FormFactorStrategy::doConstantAreaToAreaFormFactor(
    Interaction *link,
    const CubatureRule *cubatureRuleRcv,
    const CubatureRule *cubatureRuleSrc,
    double Gxy[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES])
{
    GalerkinElement *receiverElement = link->receiverElement;
    GalerkinElement *sourceElement = link->sourceElement;
    double Gx;
    double G = 0.0;
    double gMin = HUGE;
    double gMax = -HUGE;

    for ( int k = 0; k < cubatureRuleRcv->numberOfNodes; k++ ) {
        Gx = 0.0;
        for ( int l = 0; l < cubatureRuleSrc->numberOfNodes; l++ ) {
            Gx += cubatureRuleSrc->w[l] * Gxy[k][l];
        }
        Gx *= sourceElement->area;

        G += cubatureRuleRcv->w[k] * Gx;

        if ( Gx > gMax ) {
            gMax = Gx;
        }
        if ( Gx < gMin ) {
            gMin = Gx;
        }
    }
    link->K[0] = (float)(receiverElement->area * G);

    link->deltaK = new float[1];
    link->deltaK[0] = (float)(G - gMin);
    if ( gMax - G > link->deltaK[0] ) {
        link->deltaK[0] = (float)(gMax - G);
    }

    link->numberOfReceiverCubaturePositions = 1;
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
 - link->deltaK: generalized form factor(s) and error estimation coefficients (to be used in the refinement oracle hierarchicRefinementEvaluateInteraction()
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
    static CubatureRule *cubatureRuleRcv = nullptr; // Cubature rules to be used over the
    static CubatureRule *cubatureRuleSrc = nullptr; // Receiving patch and source patch
    static Vector3D x[CUBATURE_MAXIMUM_NODES];
    static Vector3D y[CUBATURE_MAXIMUM_NODES];

    double Gxy[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES];
    double kVal;
    double vis;
    double maxPtFormFactor;
    unsigned visibilityCount = 0; // Number of rays that "pass" occluders
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
                for ( int i = 0; i < link->numberOfBasisFunctionsOnReceiver * link->numberOfBasisFunctionsOnSource; i++ ) {
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
                for ( int i = 0; i < link->numberOfBasisFunctionsOnReceiver * link->numberOfBasisFunctionsOnSource; i++ ) {
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
    if ( receiverElement != galerkinState->formFactorLastRcv ) {
        determineNodes(receiverElement, &cubatureRuleRcv, x, RECEIVER, galerkinState);
    }

    // Same for the source element
    if ( sourceElement != galerkinState->formFactorLastSrc ) {
        determineNodes(sourceElement, &cubatureRuleSrc, y, SOURCE, galerkinState);
    }

    // Evaluate the radiosity kernel between each pair of nodes on the source
    // and the receiver element if at least receiver or source changed since
    // last time
    double maxKVal = 0.0;

    if ( receiverElement != galerkinState->formFactorLastRcv || sourceElement != galerkinState->formFactorLastSrc ) {
        // Use shadow caching for accelerating occlusion detection
        initShadowCache();

        // Mark the patches in order to avoid immediate self-intersections
        Patch::dontIntersect(4, receiverElement->isCluster() ? nullptr : receiverElement->patch,
                             receiverElement->isCluster() ? nullptr : receiverElement->patch->twin,
                             sourceElement->isCluster() ? nullptr : sourceElement->patch,
                             sourceElement->isCluster() ? nullptr : sourceElement->patch->twin);
        geomDontIntersect(receiverElement->isCluster() ? receiverElement->geometry : nullptr,
                          sourceElement->isCluster() ? sourceElement->geometry : nullptr);

        maxKVal = 0.0; // Compute maximum un-occluded kernel value
        maxPtFormFactor = 0.0; // Maximum un-occluded point-on-receiver to source form factor
        visibilityCount = 0; // Count the number of rays that "pass" occluders
        for ( int k = 0; k < cubatureRuleRcv->numberOfNodes; k++ ) {
            double f = 0.0;
            for ( int l = 0; l < cubatureRuleSrc->numberOfNodes; l++ ) {
                kVal = pointKernelEval(
                    sceneWorldVoxelGrid,
                    &x[k],
                    &y[l],
                    receiverElement,
                    sourceElement,
                    geometryShadowList,
                    &vis,
                    isSceneGeometry,
                    isClusteredGeometry,
                    galerkinState);
                Gxy[k][l] = kVal * vis;
                f += cubatureRuleSrc->w[l] * kVal;

                if ( kVal > maxKVal ) {
                    maxKVal = kVal;
                }
                if ( Gxy[k][l] != 0.0 ) {
                    visibilityCount++;
                }
            }
            if ( f > maxPtFormFactor ) {
                maxPtFormFactor = f;
            }
        }

        // Unmark the patches, so they are considered for ray-patch intersections again in future
        Patch::dontIntersect(0);
        geomDontIntersect(nullptr, nullptr);
    }

    if ( visibilityCount != 0 ) {
        // Actually compute the form factors
        if ( link->numberOfBasisFunctionsOnReceiver == 1 && link->numberOfBasisFunctionsOnSource == 1 ) {
            doConstantAreaToAreaFormFactor(link, cubatureRuleRcv, cubatureRuleSrc, Gxy);
        } else {
            doHigherOrderAreaToAreaFormFactor(link, cubatureRuleRcv, cubatureRuleSrc, Gxy, galerkinState);
        }
    }

    // Remember receiver and source for next time
    galerkinState->formFactorLastRcv = receiverElement;
    galerkinState->formFactorLastSrc = sourceElement;

    if ( galerkinState->clusteringStrategy == ISOTROPIC && (receiverElement->isCluster() || sourceElement->isCluster()) ) {
        if ( link-> deltaK != nullptr ) {
            delete[] link->deltaK;
        }
        link->deltaK = new float[1];
        link->deltaK[0] = (float)(maxKVal * sourceElement->area);
    }

    // Returns the visibility: basically the fraction of rays that did not hit an occluder
    link->visibility = (unsigned)(255.0 * (double) visibilityCount / (double)(cubatureRuleRcv->numberOfNodes * cubatureRuleSrc->numberOfNodes));

    if ( galerkinState->exactVisibility && geometryShadowList != nullptr && link->visibility == 255 ) {
        // Not full visibility, we missed the shadow!
        link->visibility = 254;
    }
}
/**
Cluster-specific operations

Reference:

[SILL1995a] Sillion & Drettakis, "Featured-based Control of Visibility Error: A Multi-resolution
Clustering Algorithm for Global Illumination", SIGGRAPH '95 p145
*/

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "skin/Geometry.h"
#include "GALERKIN/processing/ScratchVisibilityStrategy.h"
#include "GALERKIN/GalerkinState.h"
#include "GALERKIN/processing/ClusterTraversalStrategy.h"

static ColorRgb globalSourceRadiance;
static Vector3D globalSamplePoint;
static double globalProjectedArea;
static ColorRgb *globalPSourceRad;
static Interaction *globalTheLink;

// Area corresponding to one pixel in the scratch frame buffer
static double globalPixelArea;

/**
Executes func for every surface element in the cluster
*/
void
ClusterTraversalStrategy::traverseAllLeafElements(
    GalerkinElement *parentElement,
    void (*leafElementVisitCallBack)(GalerkinElement *elem, GalerkinState *galerkinState, ColorRgb *accumulatedRadiance),
    GalerkinState *galerkinState,
    ColorRgb *accumulatedRadiance)
{
    if ( !parentElement->isCluster() ) {
        leafElementVisitCallBack(parentElement, galerkinState, accumulatedRadiance);
    } else {
        for ( int i = 0; parentElement->irregularSubElements != nullptr && i < parentElement->irregularSubElements->size(); i++ ) {
            GalerkinElement *childCluster = (GalerkinElement *)parentElement->irregularSubElements->get(i);
            ClusterTraversalStrategy::traverseAllLeafElements(childCluster, leafElementVisitCallBack, galerkinState,
                                                              accumulatedRadiance);
        }
    }
}

/**
Uses global variables globalSourceRadiance and globalSamplePoint: accumulates the
power emitted by the element towards the globalSamplePoint in globalSourceRadiance
only taking into account the surface orientation w.r.t. the
sample point, (ignores intra cluster visibility)
*/
void
ClusterTraversalStrategy::accumulatePowerToSamplePoint(GalerkinElement *src, GalerkinState *galerkinState, ColorRgb * /*accumulatedRadiance*/) {
    float srcOs;
    float dist;
    Vector3D dir;
    ColorRgb rad;

    vectorSubtract(globalSamplePoint, src->patch->midPoint, dir);
    dist = vectorNorm(dir);
    if ( dist < EPSILON ) {
        srcOs = 1.0f;
    } else {
        srcOs = vectorDotProduct(dir, src->patch->normal) / dist;
    }
    if ( srcOs <= 0.0f ) {
        // Receiver point is behind the src
        return;
    }

    if ( galerkinState->galerkinIterationMethod == GAUSS_SEIDEL ||
         galerkinState->galerkinIterationMethod == JACOBI ) {
        rad = src->radiance[0];
    } else {
        rad = src->unShotRadiance[0];
    }

    globalSourceRadiance.addScaled(globalSourceRadiance, srcOs * src->area, rad);
}

/**
Returns the radiance or un-shot radiance (depending on the
iteration method) emitted by the source element, a cluster,
towards the sample point
*/
ColorRgb
ClusterTraversalStrategy::clusterRadianceToSamplePoint(GalerkinElement *src, Vector3D sample, GalerkinState *galerkinState) {
    switch ( galerkinState->clusteringStrategy ) {
        case ISOTROPIC:
            return src->radiance[0];

        case ORIENTED: {
            globalSamplePoint = sample;

            // Accumulate the power emitted by the patches in the source cluster
            // towards the sample point
            globalSourceRadiance.clear();
            ClusterTraversalStrategy::traverseAllLeafElements(src,
                                                              ClusterTraversalStrategy::accumulatePowerToSamplePoint,
                                                              galerkinState, &globalSourceRadiance);

            // Divide by the source area used for computing the form factor:
            // src->area / 4.0 (average projected area)
            globalSourceRadiance.scale(4.0f / src->area);
            return globalSourceRadiance;
        }

        case Z_VISIBILITY:
            if ( !src->isCluster() || !src->geometry->boundingBox.outOfBounds(&sample) ) {
                return src->radiance[0];
            } else {
                double areaFactor;

                // Render pointers to the elements in the source cluster into the scratch frame
                // buffer, seen from the sample point
                float *bbx = scratchRenderElements(src, sample, galerkinState);

                // Compute average radiance on the virtual screen
                globalSourceRadiance = scratchRadiance(galerkinState);

                // Area factor = area of virtual screen / source cluster area used for
                // form factor computation
                areaFactor = ((bbx[MAX_X] - bbx[MIN_X]) * (bbx[MAX_Y] - bbx[MIN_Y])) /
                             (0.25 * src->area);
                globalSourceRadiance.scale((float) areaFactor);
                return globalSourceRadiance;
            }

        default:
            logFatal(-1, "clusterRadianceToSamplePoint", "Invalid clustering strategy %d\n",
                     galerkinState->clusteringStrategy);
    }

    return globalSourceRadiance; // This point is never reached
}

/**
Determines the average radiance or un-shot radiance (depending on
the iteration method) emitted by the source cluster towards the
receiver in the link. The source should be a cluster
*/
ColorRgb
ClusterTraversalStrategy::sourceClusterRadiance(Interaction *link, GalerkinState *galerkinState) {
    GalerkinElement *src = link->sourceElement, *rcv = link->receiverElement;

    if ( !src->isCluster() || src == rcv ) {
        logFatal(-1, "sourceClusterRadiance", "Source and receiver are the same or receiver is not a cluster");
    }

    // Take a sample point on the receiver
    return ClusterTraversalStrategy::clusterRadianceToSamplePoint(src, rcv->midPoint(), galerkinState);
}

/**
Computes projected area of receiver surface element towards the sample point
(global variable). Ignores intra cluster visibility
*/
double
ClusterTraversalStrategy::surfaceProjectedAreaToSamplePoint(GalerkinElement *rcv) {
    double rcvCos;
    double dist;
    Vector3D dir;

    vectorSubtract(globalSamplePoint, rcv->patch->midPoint, dir);
    dist = vectorNorm(dir);
    if ( dist < EPSILON ) {
        rcvCos = 1.0;
    } else {
        rcvCos = vectorDotProduct(dir, rcv->patch->normal) / dist;
    }
    if ( rcvCos <= 0.0 ) {
        // Sample point is behind the rcv
        return 0.0;
    }

    return rcvCos * rcv->area;
}

void
ClusterTraversalStrategy::accumulateProjectedAreaToSamplePoint(GalerkinElement *rcv, GalerkinState * /*galerkinState*/, ColorRgb * /*accumulatedRadiance*/) {
    globalProjectedArea += ClusterTraversalStrategy::surfaceProjectedAreaToSamplePoint(rcv);
}

/**
Computes projected area of receiver cluster as seen from the midpoint of the source,
ignoring intra-receiver visibility
*/
double
ClusterTraversalStrategy::receiverArea(Interaction *link, GalerkinState *galerkinState) {
    GalerkinElement *src = link->sourceElement, *rcv = link->receiverElement;

    if ( !rcv->isCluster() || src == rcv ) {
        return rcv->area;
    }

    switch ( galerkinState->clusteringStrategy ) {
        case ISOTROPIC:
            return rcv->area;

        case ORIENTED: {
            globalSamplePoint = src->midPoint();
            globalProjectedArea = 0.0;
            ClusterTraversalStrategy::traverseAllLeafElements(rcv,
                                                              ClusterTraversalStrategy::accumulateProjectedAreaToSamplePoint,
                                                              galerkinState, &globalSourceRadiance);
            return globalProjectedArea;
        }

        case Z_VISIBILITY:
            globalSamplePoint = src->midPoint();
            if ( !rcv->geometry->boundingBox.outOfBounds(&globalSamplePoint) ) {
                return rcv->area;
            } else {
                float *bbx = scratchRenderElements(rcv, globalSamplePoint, galerkinState);

                // Projected area is the number of non-background pixels over
                // the total number of pixels * area of the virtual screen
                globalProjectedArea = (double)scratchNonBackgroundPixels(galerkinState) *
                                      (bbx[MAX_X] - bbx[MIN_X]) * (bbx[MAX_Y] - bbx[MIN_Y]) /
                                      (double)(galerkinState->scratch->vp_width * galerkinState->scratch->vp_height);
                return globalProjectedArea;
            }

        default:
            logFatal(-1, "receiverArea", "Invalid clustering strategy %d",
                     galerkinState->clusteringStrategy);
    }

    return 0.0; // This point is never reached and if it is it's your fault
}

/**
Gathers radiance over the interaction, from the interaction source to
the specified receiver element. area_factor is a correction factor
to account for the fact that the receiver cluster area/4 was used in
form factor computations instead of the true receiver area. The source
radiance is explicit given
*/
void
ClusterTraversalStrategy::isotropicGatherRadiance(GalerkinElement *rcv, double area_factor, Interaction *link, ColorRgb *srcRad) {
    ColorRgb *rcvRad = rcv->receivedRadiance;

    if ( link->numberOfBasisFunctionsOnReceiver == 1 && link->numberOfBasisFunctionsOnSource == 1 ) {
        rcvRad[0].addScaled(rcvRad[0], (float) (area_factor * link->K[0]), srcRad[0]);
    } else {
        int alpha;
        int beta;
        int a;
        int b;
        a = intMin(link->numberOfBasisFunctionsOnReceiver, rcv->basisSize);
        b = intMin(link->numberOfBasisFunctionsOnSource, link->sourceElement->basisSize);
        for ( alpha = 0; alpha < a; alpha++ ) {
            for ( beta = 0; beta < b; beta++ ) {
                rcvRad[alpha].addScaled(
                    rcvRad[alpha],
                    (float)(area_factor * link->K[alpha * link->numberOfBasisFunctionsOnSource + beta]),
                    srcRad[beta]);
            }
        }
    }
}

/**
Requires global variables globalPSourceRad (globalSourceRadiance pointer), globalTheLink (interaction
over which is being gathered) and globalSamplePoint (midpoint of source
element). rcv is a surface element belonging to the receiver cluster
in the interaction. This routines gathers radiance to this receiver
surface, taking into account the projected area of the receiver
towards the midpoint of the source, ignoring visibility in the receiver
cluster
*/
void
ClusterTraversalStrategy::orientedSurfaceGatherRadiance(GalerkinElement *rcv, GalerkinState */*galerkinState*/, ColorRgb * /*accumulatedRadiance*/) {
    double area_factor;

    // globalTheLink->rcv is a cluster, so it's total area divided by 4 (average projected area)
    // was used to compute link->K
    area_factor = ClusterTraversalStrategy::surfaceProjectedAreaToSamplePoint(rcv) / (0.25 * globalTheLink->receiverElement->area);

    ClusterTraversalStrategy::isotropicGatherRadiance(rcv, area_factor, globalTheLink, globalPSourceRad);
}

/**
Same as above, except that the number of pixels in the scratch frame buffer
times the area corresponding one such pixel is used as the visible area
of the element. Uses global variables globalPixelArea, globalTheLink, globalPSourceRad
*/
void
ClusterTraversalStrategy::zVisSurfaceGatherRadiance(GalerkinElement *rcv, GalerkinState */*galerkinState*/, ColorRgb * /*accumulatedRadiance*/) {
    double areaFactor;

    if ( rcv->tmp <= 0 ) {
        // Element occupies no pixels in the scratch frame buffer
        return;
    }

    areaFactor = globalPixelArea * (double) (rcv->tmp) / (0.25 * globalTheLink->receiverElement->area);
    ClusterTraversalStrategy::isotropicGatherRadiance(rcv, areaFactor, globalTheLink, globalPSourceRad);

    rcv->tmp = 0; // Set it to zero for future re-use
}

/**
Distributes the source radiance to the surface elements in the
receiver cluster
*/
void
ClusterTraversalStrategy::gatherRadiance(Interaction *link, ColorRgb *srcRad, GalerkinState *galerkinState) {
    GalerkinElement *src = link->sourceElement, *rcv = link->receiverElement;

    if ( !rcv->isCluster() || src == rcv ) {
        logFatal(-1, "gatherRadiance", "Source and receiver are the same or receiver is not a cluster");
        return;
    }

    globalPSourceRad = srcRad;
    globalTheLink = link;
    globalSamplePoint = src->midPoint();

    switch ( galerkinState->clusteringStrategy ) {
        case ISOTROPIC:
            ClusterTraversalStrategy::isotropicGatherRadiance(rcv, 1.0, link, srcRad);
            break;
        case ORIENTED:
            ClusterTraversalStrategy::traverseAllLeafElements(rcv,
                                                              ClusterTraversalStrategy::orientedSurfaceGatherRadiance,
                                                              galerkinState, &globalSourceRadiance);
            break;
        case Z_VISIBILITY:
            if ( !rcv->geometry->boundingBox.outOfBounds(&globalSamplePoint) ) {
                ClusterTraversalStrategy::traverseAllLeafElements(rcv,
                                                                  ClusterTraversalStrategy::orientedSurfaceGatherRadiance,
                                                                  galerkinState, &globalSourceRadiance);
            } else {
                float *boundingBox = scratchRenderElements(rcv, globalSamplePoint, galerkinState);

                // Count how many pixels each element occupies in the scratch frame buffer
                scratchPixelsPerElement(galerkinState);

                // Area corresponding to one pixel on the virtual screen
                globalPixelArea = (boundingBox[MAX_X] - boundingBox[MIN_X]) * (boundingBox[MAX_Y] - boundingBox[MIN_Y]) /
                                  (double) (galerkinState->scratch->vp_width * galerkinState->scratch->vp_height);

                // Gathers the radiance to each element that occupies at least one
                // pixel in the scratch frame buffer and sets elem->tmp back to zero
                // for those elements
                ClusterTraversalStrategy::traverseAllLeafElements(rcv,
                                                                  ClusterTraversalStrategy::zVisSurfaceGatherRadiance,
                                                                  galerkinState,
                                                                  &globalSourceRadiance);
            }
            break;
        default:
            logFatal(-1, "gatherRadiance", "Invalid clustering strategy %d",
                     galerkinState->clusteringStrategy);
    }
}

void
ClusterTraversalStrategy::leafMaxRadiance(
    GalerkinElement *galerkinElement,
    GalerkinState *galerkinState,
    ColorRgb *accumulatedRadiance)
{
    ColorRgb rad;
    if ( galerkinState->galerkinIterationMethod == GAUSS_SEIDEL ||
         galerkinState->galerkinIterationMethod == JACOBI ) {
        rad = galerkinElement->radiance[0];
    } else {
        rad = galerkinElement->unShotRadiance[0];
    }
    accumulatedRadiance->maximum(*accumulatedRadiance, rad);
}

ColorRgb
ClusterTraversalStrategy::maxRadiance(GalerkinElement *cluster, GalerkinState *galerkinState) {
    ColorRgb radiance;
    radiance.clear();
    ClusterTraversalStrategy::traverseAllLeafElements(cluster, leafMaxRadiance, galerkinState, &radiance);
    return radiance;
}

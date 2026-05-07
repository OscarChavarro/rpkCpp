/**
Generic stochastic Jacobi iteration (local lines)
TODO: combined radiance / importance propagation
TODO: hierarchical refinement for importance propagation
TODO: re-incorporate the rejection sampling technique for
sampling positions on shooters with higher order radiosity approximation
(lower variance)
TODO: lines and line bundles.
*/

package vsdk.toolkit.raycasting.stochasticRaytracing;

import java.util.ArrayList;

import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.skin.RayHitFlag;
import vsdk.toolkit.numericalAnalysis.quasiMonteCarlo.Niederreiter;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.skin.Element;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.RayHit;

public final class StochasticJacobi {
    @FunctionalInterface
    public interface GetRadianceCallback {
        ColorRgb[] apply(StochasticRadiosityElement elem);
    }

    @FunctionalInterface
    public interface GetImportanceCallback {
        float apply(StochasticRadiosityElement elem);
    }

    @FunctionalInterface
    public interface UpdateCallback {
        void apply(StochasticRadiosityElement elem, double w);
    }

    private static GetRadianceCallback getRadianceCallback = null;
    private static GetImportanceCallback getImportanceCallback = null;
    private static UpdateCallback reflectCallback = null;
    private static int useControlVariate = 0; // If uses a constant control variate
    private static int numberOfRaysToShoot = 0; // Number of rays to shoot in the iteration
    private static double sumOfProbabilities = 0.0; // Sum of un-normalised sampling "probabilities"

    private StochasticJacobi() {
    }

    private static void stochasticJacobiInitGlobals(
        int numberOfRays,
        GetRadianceCallback getRadianceCallBack,
        GetImportanceCallback getImportanceCallBack,
        UpdateCallback updateCallBack)
    {
        numberOfRaysToShoot = numberOfRays;
        getRadianceCallback = getRadianceCallBack;
        getImportanceCallback = getImportanceCallBack;
        reflectCallback = updateCallBack;
        // Only use control variates for propagating radiance, not for importance
        useControlVariate = (StochasticRelaxation.activeState().constantControlVariate != 0 && getRadianceCallBack != null) ? 1 : 0;

        if ( getRadianceCallback != null ) {
            StochasticRelaxation.activeState().prevTracedRays = StochasticRelaxation.activeState().tracedRays;
        }
        if ( getImportanceCallback != null ) {
            StochasticRelaxation.activeState().prevImportanceTracedRays = StochasticRelaxation.activeState().importanceTracedRays;
        }
    }

    private static void stochasticJacobiPrintMessage(long nrRays) {
        System.err.printf("%s-directional ",
            StochasticRelaxation.activeState().bidirectionalTransfers != 0 ? "Bi" : "Uni");
        if ( getRadianceCallback != null && getImportanceCallback != null ) {
            System.err.printf("combined ");
        }
        if ( getRadianceCallback != null ) {
            System.err.printf("%s radiance ",
                StochasticRelaxation.activeState().importanceDriven != 0 ? "importance-driven " : "");
        }
        if ( getRadianceCallback != null && getImportanceCallback != null ) {
            System.err.printf("and ");
        }
        if ( getImportanceCallback != null ) {
            System.err.printf("%s importance ",
                StochasticRelaxation.activeState().radianceDriven != 0 ? "radiance-driven " : "");
        }
        System.err.printf("propagation");
        if ( useControlVariate != 0 ) {
            System.err.printf("using a constant control variate ");
        }
        System.err.printf("(%d rays):\n", nrRays);
    }

    /**
Compute (un-normalised) stochasticJacobiProbability of shooting a ray from elem
*/
    private static double stochasticJacobiProbability(StochasticRadiosityElement elem) {
        double prob = 0.0;

        if ( getRadianceCallback != null ) {
            // Probability proportional to power to be propagated
            ColorRgb[] callbackRadiance = getRadianceCallback.apply(elem);
            ColorRgb radiance = new ColorRgb();
            radiance.set(
                callbackRadiance[0].r,
                callbackRadiance[0].g,
                callbackRadiance[0].b);
            if ( StochasticRelaxation.activeState().constantControlVariate != 0 ) {
                radiance.subtract(radiance, StochasticRelaxation.activeState().controlRadiance);
            }
            prob = elem.area * radiance.sumAbsComponents();
            if ( StochasticRelaxation.activeState().importanceDriven != 0 ) {
                // Weight with received importance
                float w = elem.importance - elem.sourceImportance;
                prob *= (w > 0.0f) ? w : 0.0f;
            }
        }

        if ( getImportanceCallback != null && StochasticRelaxation.activeState().importanceDriven != 0 ) {
            double prob2 = elem.area * Math.abs(getImportanceCallback.apply(elem)) *
                StochasticRadiosityElement.stochasticRadiosityElementScalarReflectance(elem);

            if ( StochasticRelaxation.activeState().radianceDriven != 0 ) {
                // Received-radiance weighted importance transport
                ColorRgb receivedRadiance = new ColorRgb();
                receivedRadiance.subtract(elem.radiance[0], elem.sourceRad);
                prob2 *= receivedRadiance.sumAbsComponents();
            }

            // Equal weight to importance and radiance propagation for constant approximation,
            // but higher weight to radiance for higher order approximations. Still OK
            // if only propagating importance
            prob = prob * StochasticRadiosityBasisState.activeState()
                .approxDesc[StochasticRelaxation.activeState().approximationOrderType.ordinal()].basis_size + prob2;
        }

        return prob;
    }

    /**
clear accumulators of all kinds of sample weights and contributions
*/
    private static void stochasticJacobiElementClearAccumulators(StochasticRadiosityElement elem) {
        if ( getRadianceCallback != null ) {
            Coefficientsmcrad.stochasticRadiosityClearCoefficients(elem.receivedRadiance, elem.basis);
        }
        if ( getImportanceCallback != null ) {
            elem.receivedImportance = 0.0f;
        }
    }

    /**
Clears received radiance and importance and accumulates the un-normalized
sampling probabilities at leaf elements
*/
    private static void stochasticJacobiElementSetup(Element element) {
        StochasticRadiosityElement stochasticRadiosityElement = (StochasticRadiosityElement)element;

        if ( stochasticRadiosityElement == null ) {
            return;
        }

        stochasticRadiosityElement.samplingProbability = 0.0f;
        if ( !stochasticRadiosityElement.traverseAllChildren(StochasticJacobi::stochasticJacobiElementSetup) ) {
            // Elem is a leaf element. We need to compute the sum of the un-normalized
            // sampling "probabilities" at the leaf elements
            stochasticRadiosityElement.samplingProbability = (float)stochasticJacobiProbability(stochasticRadiosityElement);
            sumOfProbabilities += stochasticRadiosityElement.samplingProbability;
        }
        if ( stochasticRadiosityElement.parent != null ) {
            // The probability of sampling a non-leaf element is the sum of the
            // probabilities of sampling the sub-elements
            ((StochasticRadiosityElement)stochasticRadiosityElement.parent).samplingProbability +=
                stochasticRadiosityElement.samplingProbability;
        }

        stochasticJacobiElementClearAccumulators(stochasticRadiosityElement);
    }

    /**
Returns true if success, that is: sum of sampling probabilities is nonzero
*/
    private static boolean stochasticJacobiSetup(ArrayList<Patch> scenePatches) {
        // Determine constant control radiosity if required
        StochasticRelaxation.activeState().controlRadiance.clear();
        if ( useControlVariate != 0 ) {
            StochasticRelaxation.activeState().controlRadiance =
                Ccr.determineControlRadiosity(getRadianceCallback::apply, null, scenePatches);
        }

        sumOfProbabilities = 0.0;
        stochasticJacobiElementSetup(ElementHierarchyState.activeState().topCluster);

        if ( sumOfProbabilities < Numeric.EPSILON * Numeric.EPSILON ) {
            Error.warning("Iteration", "No sources");
            return false;
        }
        return true;
    }

    /**
Returns radiance to be propagated from the given location of the element
*/
    private static ColorRgb stochasticJacobiGetSourceRadiance(StochasticRadiosityElement src, double us, double vs) {
        ColorRgb[] srcRad = getRadianceCallback.apply(src);
        return Basismcrad.colorAtUv(src.basis, srcRad, us, vs);
    }

    private static void stochasticJacobiPropagateRadianceToSurface(
        StochasticRadiosityElement rcv,
        double ur,
        double vr,
        ColorRgb rayPower,
        StochasticRadiosityElement src,
        double fraction,
        double weight)
    {
        if ( src == null || weight < 0 ) {
            // Keep parameters used in C++ signature.
        }
        for ( int i = 0; i < rcv.basis.size; i++ ) {
            double dual = rcv.basis.dualFunction[i].eval(ur, vr) / rcv.area;
            double w = dual * fraction / (double)numberOfRaysToShoot;
            rcv.receivedRadiance[i].addScaled(rcv.receivedRadiance[i], (float)w, rayPower);
        }
    }

    private static void stochasticJacobiPropagateRadianceToClusterIsotropic(
        StochasticRadiosityElement cluster,
        ColorRgb rayPower,
        StochasticRadiosityElement src,
        double fraction,
        double weight)
    {
        if ( src == null || weight < 0 ) {
            // Keep parameters used in C++ signature.
        }
        double w = fraction / cluster.area / (double)numberOfRaysToShoot;
        cluster.receivedRadiance[0].addScaled(cluster.receivedRadiance[0], (float)w, rayPower);
    }

    /**
Note: Not considering the MAX_HIERARCHY_DEPTH limit.
*/
    private static void stochasticJacobiPropagateRadianceClusterRecursive(
        StochasticRadiosityElement currentElement,
        ColorRgb rayPower,
        Ray ray,
        float dir,
        double projectedArea,
        double fraction)
    {
        if ( currentElement != null && !currentElement.isCluster() ) {
            // Trivial case
            double c = -dir * currentElement.patch.normal.dotProduct(ray.direction);
            if ( c > 0.0 ) {
                double aFraction = fraction * (c * currentElement.area / projectedArea);
                double w = aFraction / currentElement.area / (double)numberOfRaysToShoot;
                currentElement.receivedRadiance[0].addScaled(currentElement.receivedRadiance[0], (float)w, rayPower);
            }
        } else {
            // Recursive case
            for ( int i = 0;
                  currentElement != null
                      && currentElement.irregularSubElements != null
                      && i < currentElement.irregularSubElements.size();
                  i++ ) {
                stochasticJacobiPropagateRadianceClusterRecursive(
                    (StochasticRadiosityElement)currentElement.irregularSubElements.get(i),
                    rayPower,
                    ray,
                    dir,
                    projectedArea,
                    fraction);
            }
        }
    }

    private static void stochasticJacobiPropagateRadianceToClusterOriented(
        StochasticRadiosityElement cluster,
        ColorRgb rayPower,
        Ray ray,
        float dir,
        StochasticRadiosityElement src,
        double projectedArea,
        double fraction,
        double weight)
    {
        if ( src == null || weight < 0 ) {
            // Keep parameters used in C++ signature.
        }
        stochasticJacobiPropagateRadianceClusterRecursive(cluster, rayPower, ray, dir, projectedArea, fraction);
    }

    /**
Note: Not considering the MAX_HIERARCHY_DEPTH limit.
*/
    private static void stochasticJacobiReceiverProjectedAreaRecursive(
        StochasticRadiosityElement currentElement,
        Ray ray,
        float dir,
        double[] area)
    {
        if ( currentElement != null && !currentElement.isCluster() ) {
            // Trivial case
            double c = -dir * currentElement.patch.normal.dotProduct(ray.direction);
            if ( c > 0.0 ) {
                area[0] += c * currentElement.area;
            }
        } else {
            // Recursive case
            for ( int i = 0;
                  currentElement != null
                      && currentElement.irregularSubElements != null
                      && i < currentElement.irregularSubElements.size();
                  i++ ) {
                stochasticJacobiReceiverProjectedAreaRecursive(
                    (StochasticRadiosityElement)currentElement.irregularSubElements.get(i),
                    ray,
                    dir,
                    area);
            }
        }
    }

    private static double stochasticJacobiReceiverProjectedArea(StochasticRadiosityElement cluster, Ray ray, float dir) {
        double[] area = new double[] {0.0};
        stochasticJacobiReceiverProjectedAreaRecursive(cluster, ray, dir, area);
        return area[0];
    }

    /**
Transfer radiance from src to rcv.
src_prob = un-normalised src birth stochasticJacobiProbability / src area
rcv_prob = un-normalised rcv birth stochasticJacobiProbability / rcv area for bidirectional transfers
      or = 0 for unidirectional transfers
score is weighted with sumOfProbabilities / numberOfRaysToShoot.
ray->dir and dir are used in order to determine projected cluster area
and cosine of incident direction of cluster surface elements when
the receiver is a cluster
*/
    private static void stochasticJacobiPropagateRadiance(
        StochasticRadiosityElement src,
        double us,
        double vs,
        StochasticRadiosityElement rcv,
        double ur,
        double vr,
        double srcProb,
        double rcvProb,
        Ray ray,
        float dir)
    {
        ColorRgb radiance;
        ColorRgb rayPower = new ColorRgb();
        double area;
        double weight = sumOfProbabilities / srcProb; // src area / normalised src prob
        double fraction = srcProb / (srcProb + rcvProb); // 1 for uni-directional transfers

        if ( srcProb < Numeric.EPSILON * Numeric.EPSILON /* this should never happen */
             || fraction < Numeric.EPSILON ) {
            // Reverse transfer from a black surface
            return;
        }

        radiance = stochasticJacobiGetSourceRadiance(src, us, vs);
        if ( StochasticRelaxation.activeState().constantControlVariate != 0 ) {
            radiance.subtract(radiance, StochasticRelaxation.activeState().controlRadiance);
        }
        rayPower.scaledCopy((float)weight, radiance);

        if ( !rcv.isCluster() ) {
            stochasticJacobiPropagateRadianceToSurface(rcv, ur, vr, rayPower, src, fraction, weight);
        } else {
            switch ( ElementHierarchyState.activeState().clustering ) {
                case NO_CLUSTERING:
                    Error.fatal(-1, "Propagate",
                        "Hierarchy::hierarchyRefine() returns cluster although clustering is disabled.\n");
                    break;
                case ISOTROPIC_CLUSTERING:
                    stochasticJacobiPropagateRadianceToClusterIsotropic(rcv, rayPower, src, fraction, weight);
                    break;
                case ORIENTED_CLUSTERING:
                    area = stochasticJacobiReceiverProjectedArea(rcv, ray, dir);
                    if ( area > Numeric.EPSILON ) {
                        stochasticJacobiPropagateRadianceToClusterOriented(
                            rcv, rayPower, ray, dir, src, area, fraction, weight);
                    }
                    break;
                default:
                    Error.fatal(-1, "Propagate", "Invalid clustering mode %d\n",
                        ElementHierarchyState.activeState().clustering.ordinal());
                    break;
            }
        }
    }

    /**
Idem but for importance
*/
    private static void stochasticJacobiPropagateImportance(
        StochasticRadiosityElement src,
        double us,
        double vs,
        StochasticRadiosityElement rcv,
        double ur,
        double vr,
        double srcProb,
        double rcvProb,
        Ray ray,
        float dir)
    {
        if ( ray == null || us < -1 || vs < -1 || ur < -1 || vr < -1 || dir < -2 ) {
            // Keep parameters used in C++ signature.
        }
        double w = sumOfProbabilities / (srcProb + rcvProb) / rcv.area / (double)numberOfRaysToShoot;
        rcv.receivedImportance += (float)(
            w * StochasticRadiosityElement.stochasticRadiosityElementScalarReflectance(src) * getImportanceCallback.apply(src));

        if ( ElementHierarchyState.activeState().do_h_meshing != 0 ||
             ElementHierarchyState.activeState().clustering != HierarchyClusteringMode.NO_CLUSTERING ) {
            Error.fatal(-1, "Propagate",
                "Importance propagation not implemented in combination with hierarchical refinement");
        }
    }

    /**
Src is the leaf element containing the point from which to propagate
radiance on P. P and Q are toplevel surface elements. Transfer
is from P to Q
*/
    private static void stochasticJacobiRefineAndPropagateRadiance(
        StochasticRadiosityElement src,
        double us,
        double vs,
        StochasticRadiosityElement P,
        double up,
        double vp,
        StochasticRadiosityElement Q,
        double uq,
        double vq,
        double srcProb,
        double rcvProb,
        Ray ray,
        float dir,
        RenderOptions renderOptions)
    {
        Link link = Hierarchy.topLink(Q, P);
        double[] rcvU = new double[] {uq};
        double[] rcvV = new double[] {vq};
        double[] srcU = new double[] {up};
        double[] srcV = new double[] {vp};
        link = Hierarchy.hierarchyRefine(
            link, Q, rcvU, rcvV, P, srcU, srcV,
            ElementHierarchyState.activeState().oracle, renderOptions);
        // Propagate from the leaf element src to the admissible receiver element containing/contained by Q
        stochasticJacobiPropagateRadiance(src, us, vs, link.rcv, rcvU[0], rcvV[0], srcProb, rcvProb, ray, dir);
    }

    private static void stochasticJacobiRefineAndPropagateImportance(
        StochasticRadiosityElement P,
        double up,
        double vp,
        StochasticRadiosityElement Q,
        double uq,
        double vq,
        double srcProb,
        double rcvProb,
        Ray ray,
        float dir)
    {
        // No refinement (yet) for importance: propagate between toplevel surfaces
        stochasticJacobiPropagateImportance(P, up, vp, Q, uq, vq, srcProb, rcvProb, ray, dir);
    }

    /**
Ray is a ray connecting the positions with given (u,v) parameters
on the toplevel surface element P to Q. This routine refines the
imaginary interaction between these elements and performs
radiance or importance transfer along the ray, taking into account
bi-directionality if requested
*/
    private static void stochasticJacobiRefineAndPropagate(
        StochasticRadiosityElement P,
        double up,
        double vp,
        StochasticRadiosityElement Q,
        double uq,
        double vq,
        Ray ray,
        RenderOptions renderOptions)
    {
        double srcProb;
        double[] us = new double[] {up};
        double[] vs = new double[] {vp};
        StochasticRadiosityElement src =
            StochasticRadiosityElement.stochasticRadiosityElementRegularLeafElementAtPoint(P, us, vs);
        srcProb = (double)src.samplingProbability / (double)src.area;

        if ( StochasticRelaxation.activeState().bidirectionalTransfers != 0 ) {
            double rcvProb;
            double[] ur = new double[] {uq};
            double[] vr = new double[] {vq};
            StochasticRadiosityElement rcv =
                StochasticRadiosityElement.stochasticRadiosityElementRegularLeafElementAtPoint(Q, ur, vr);
            rcvProb = (double)rcv.samplingProbability / (double)rcv.area;

            if ( getRadianceCallback != null ) {
                stochasticJacobiRefineAndPropagateRadiance(
                    src, us[0], vs[0], P, up, vp, Q, uq, vq, srcProb, rcvProb, ray, +1.0f, renderOptions);
                stochasticJacobiRefineAndPropagateRadiance(
                    rcv, ur[0], vr[0], Q, uq, vq, P, up, vp, rcvProb, srcProb, ray, -1.0f, renderOptions);
            }
            if ( getImportanceCallback != null ) {
                stochasticJacobiRefineAndPropagateImportance(P, up, vp, Q, uq, vq, srcProb, rcvProb, ray, +1.0f);
                stochasticJacobiRefineAndPropagateImportance(Q, uq, vq, P, up, vp, rcvProb, srcProb, ray, -1.0f);
            }
        } else {
            if ( getRadianceCallback != null ) {
                stochasticJacobiRefineAndPropagateRadiance(
                    src, us[0], vs[0], P, up, vp, Q, uq, vq, srcProb, 0.0, ray, +1.0f, renderOptions);
            }
            if ( getImportanceCallback != null ) {
                stochasticJacobiRefineAndPropagateImportance(P, up, vp, Q, uq, vq, srcProb, 0.0, ray, +1.0f);
            }
        }
    }

    private static double[] stochasticJacobiNextSample(
        StochasticRadiosityElement elem,
        int nMostSignificantBit,
        long mostSignificantBit1,
        long rMostSignificantBit2,
        double[] zeta)
    {
        long[] rayIndex = new long[] {getRadianceCallback != null ? elem.rayIndex : elem.importanceRayIndex};
        long[] xi = Niederreiter.NextNiedInRange(
            rayIndex, +1, nMostSignificantBit, mostSignificantBit1, rMostSignificantBit2);

        rayIndex[0]++;
        if ( getRadianceCallback != null ) {
            elem.rayIndex = rayIndex[0];
        } else {
            elem.importanceRayIndex = rayIndex[0];
        }

        long u = (xi[0] & ~3L) | 1L; // Avoid positions on sub-element boundaries
        long v = (xi[1] & ~3L) | 1L;
        if ( elem.numberOfVertices == 3 ) {
            long[] uu = new long[] {u};
            long[] vv = new long[] {v};
            Niederreiter.foldSample(uu, vv);
            u = uu[0];
            v = vv[0];
        }
        zeta[0] = (double)u * Niederreiter.RECIP;
        zeta[1] = (double)v * Niederreiter.RECIP;
        zeta[2] = (double)xi[2] * Niederreiter.RECIP;
        zeta[3] = (double)xi[3] * Niederreiter.RECIP;
        return zeta;
    }

    /**
Determines uniform (u,v) parameters of hit point on hit patch
*/
    private static void stochasticJacobiUniformHitCoordinates(RayHit hit, double[] uHit, double[] vHit) {
        if ( (hit.getFlags() & RayHitFlag.UV) != 0 ) {
            // (u,v) coordinates obtained as side result of intersection test
            uHit[0] = hit.getUv().u;
            vHit[0] = hit.getUv().v;
            if ( hit.getPatch().jacobian != null ) {
                hit.getPatch().biLinearToUniform(uHit, vHit);
            }
        } else {
            Vector3D position = hit.getPoint();
            hit.getPatch().uniformUv(position, uHit, vHit);
        }

        // Clip uv coordinates to lay strictly inside the hit patch
        if ( uHit[0] < Numeric.EPSILON ) {
            uHit[0] = Numeric.EPSILON;
        }
        if ( vHit[0] < Numeric.EPSILON ) {
            vHit[0] = Numeric.EPSILON;
        }
        if ( uHit[0] > 1.0 - Numeric.EPSILON ) {
            uHit[0] = 1.0 - Numeric.EPSILON;
        }
        if ( vHit[0] > 1.0 - Numeric.EPSILON ) {
            vHit[0] = 1.0 - Numeric.EPSILON;
        }
    }

    /**
Traces a local line from 'src' and propagates radiance and/or importance from P to
hit patch (and back for bidirectional transfers)
*/
    private static void stochasticJacobiElementShootRay(
        VoxelGrid sceneWorldVoxelGrid,
        StochasticRadiosityElement src,
        int nMostSignificantBit,
        long mostSignificantBit1,
        long rMostSignificantBit2,
        RenderOptions renderOptions)
    {
        if ( getRadianceCallback != null ) {
            StochasticRelaxation.activeState().tracedRays++;
        }
        if ( getImportanceCallback != null ) {
            StochasticRelaxation.activeState().importanceTracedRays++;
        }

        double[] zeta = new double[4];
        Ray ray = Localline.mcrGenerateLocalLine(
            src.patch,
            stochasticJacobiNextSample(src, nMostSignificantBit, mostSignificantBit1, rMostSignificantBit2, zeta));

        RayHit hitStore = new RayHit();
        RayHit hit = Localline.mcrShootRay(sceneWorldVoxelGrid, src.patch, ray, hitStore);

        if ( hit != null ) {
            double[] uHit = new double[] {0.0};
            double[] vHit = new double[] {0.0};
            stochasticJacobiUniformHitCoordinates(hit, uHit, vHit);
            stochasticJacobiRefineAndPropagate(
                McradP.topLevelStochasticRadiosityElement(src.patch), zeta[0], zeta[1],
                McradP.topLevelStochasticRadiosityElement(hit.getPatch()), uHit[0], vHit[0], ray, renderOptions);
        } else {
            StochasticRelaxation.activeState().numberOfMisses++;
        }
    }

    private static void stochasticJacobiInitPushRayIndex(Element element) {
        StochasticRadiosityElement stochasticRadiosityElement = (StochasticRadiosityElement)element;
        if ( stochasticRadiosityElement == null ) {
            return;
        }
        StochasticRadiosityElement parent = (StochasticRadiosityElement)stochasticRadiosityElement.parent;
        if ( parent != null ) {
            stochasticRadiosityElement.rayIndex = parent.rayIndex;
            stochasticRadiosityElement.importanceRayIndex = parent.importanceRayIndex;
        }
        stochasticRadiosityElement.traverseAllChildren(StochasticJacobi::stochasticJacobiInitPushRayIndex);
    }

    /**
Determines nr of rays to shoot from element and shoots this number of rays
*/
    private static void stochasticJacobiElementShootRays(
        VoxelGrid sceneWorldVoxelGrid,
        StochasticRadiosityElement element,
        int raysThisElem,
        RenderOptions renderOptions)
    {
        int[] sampleRange = new int[1]; // Determines a range in which to generate a sample
        long[] mostSignificantBit1 = new long[1]; // See monteCarloRadiosityElementRange() and NextSample()
        long[] rMostSignificantBit2 = new long[1];

        // Sample number range for 4D Niederreiter sequence
        StochasticRadiosityElement.stochasticRadiosityElementRange(
            element, sampleRange, mostSignificantBit1, rMostSignificantBit2);

        // Shoot the rays
        for ( int i = 0; i < raysThisElem; i++ ) {
            stochasticJacobiElementShootRay(
                sceneWorldVoxelGrid, element, sampleRange[0], mostSignificantBit1[0], rMostSignificantBit2[0], renderOptions);
        }

        if ( element != null && !element.isLeaf() ) {
            // Source got subdivided while shooting the rays
            element.traverseAllChildren(StochasticJacobi::stochasticJacobiInitPushRayIndex);
        }
    }

    private static void stochasticJacobiShootRaysRecursive(
        VoxelGrid sceneWorldVoxelGrid,
        StochasticRadiosityElement element,
        double rnd,
        long[] rayCount,
        double[] cumulative,
        RenderOptions renderOptions)
    {
        if ( element.regularSubElements == null ) {
            // Trivial case
            double p = element.samplingProbability / sumOfProbabilities;
            long raysThisLeaf =
                (long)Math.floor((cumulative[0] + p) * (double)numberOfRaysToShoot + rnd) - rayCount[0];

            if ( raysThisLeaf > 0 ) {
                stochasticJacobiElementShootRays(sceneWorldVoxelGrid, element, (int)raysThisLeaf, renderOptions);
            }

            cumulative[0] += p;
            rayCount[0] += raysThisLeaf;
        } else {
            // Recursive case
            for ( int i = 0; i < 4; i++ ) {
                stochasticJacobiShootRaysRecursive(
                    sceneWorldVoxelGrid,
                    (StochasticRadiosityElement)element.regularSubElements[i],
                    rnd,
                    rayCount,
                    cumulative,
                    renderOptions);
            }
        }
    }

    /**
Fire off rays from the leaf elements, propagate radiance/importance
*/
    private static void stochasticJacobiShootRays(
        VoxelGrid sceneWorldVoxelGrid,
        ArrayList<Patch> scenePatches,
        RenderOptions renderOptions)
    {
        double rnd = Math.random();
        long[] rayCount = new long[] {0};
        double[] cumulative = new double[] {0.0};

        // Loop over all leaf elements in the element hierarchy
        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            stochasticJacobiShootRaysRecursive(
                sceneWorldVoxelGrid,
                McradP.topLevelStochasticRadiosityElement(scenePatches.get(i)),
                rnd,
                rayCount,
                cumulative,
                renderOptions);
        }

        System.err.printf("\n");
    }

    /**
Converts received radiance and importance at a leaf element into a new
approximation of total and un-shot radiance and importance
*/
    private static void stochasticJacobiUpdateElement(StochasticRadiosityElement elem) {
        if ( getRadianceCallback != null ) {
            if ( useControlVariate != 0 ) {
                // Add constant radiosity contribution to received flux
                elem.receivedRadiance[0].add(elem.receivedRadiance[0], StochasticRelaxation.activeState().controlRadiance);
            }
            // Multiply with reflectivity on leaf elements only
            Coefficientsmcrad.stochasticRadiosityMultiplyCoefficients(elem.Rd, elem.receivedRadiance, elem.basis);
        }

        reflectCallback.apply(elem, (double)numberOfRaysToShoot / sumOfProbabilities);

        StochasticRelaxation.activeState().unShotFlux.addScaled(
            StochasticRelaxation.activeState().unShotFlux,
            (float)Math.PI * elem.area,
            elem.unShotRadiance[0]);
        StochasticRelaxation.activeState().totalFlux.addScaled(
            StochasticRelaxation.activeState().totalFlux,
            (float)Math.PI * elem.area,
            elem.radiance[0]);
        StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux.addScaled(
            StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux,
            (float)(Math.PI * elem.area * (elem.importance - elem.sourceImportance)),
            elem.unShotRadiance[0]);
        StochasticRelaxation.activeState().unShotYmp += (float)(elem.area * Math.abs(elem.unShotImportance));
        StochasticRelaxation.activeState().totalYmp += elem.area * elem.importance;
    }

    private static void stochasticJacobiPush(StochasticRadiosityElement parent, StochasticRadiosityElement child) {
        if ( getRadianceCallback != null ) {
            if ( parent.isCluster() && !child.isCluster() ) {
                // Multiply with reflectance (See PropagateRadianceToClusterIsotropic() above)
                ColorRgb rad = new ColorRgb(
                    parent.receivedRadiance[0].r, parent.receivedRadiance[0].g, parent.receivedRadiance[0].b);
                ColorRgb Rd = child.Rd;
                rad.selfScalarProduct(Rd);
                StochasticRadiosityElement.stochasticRadiosityElementPushRadiance(
                    parent, child, new ColorRgb[] {rad}, child.receivedRadiance);
            } else {
                StochasticRadiosityElement.stochasticRadiosityElementPushRadiance(
                    parent, child, parent.receivedRadiance, child.receivedRadiance);
            }
        }

        if ( getImportanceCallback != null ) {
            float[] parentImportance = new float[] {parent.receivedImportance};
            float[] childImportance = new float[] {child.receivedImportance};
            StochasticRadiosityElement.stochasticRadiosityElementPushImportance(parentImportance, childImportance);
            child.receivedImportance = childImportance[0];
        }
    }

    private static void stochasticJacobiPull(StochasticRadiosityElement parent, StochasticRadiosityElement child) {
        if ( getRadianceCallback != null ) {
            StochasticRadiosityElement.stochasticRadiosityElementPullRadiance(parent, child, parent.radiance, child.radiance);
            StochasticRadiosityElement.stochasticRadiosityElementPullRadiance(parent, child, parent.unShotRadiance, child.unShotRadiance);
        }
        if ( getImportanceCallback != null ) {
            float[] parentImportance = new float[] {parent.importance};
            float[] childImportance = new float[] {child.importance};
            StochasticRadiosityElement.stochasticRadiosityElementPullImportance(parent, child, parentImportance, childImportance);
            parent.importance = parentImportance[0];

            float[] parentUnShotImportance = new float[] {parent.unShotImportance};
            float[] childUnShotImportance = new float[] {child.unShotImportance};
            StochasticRadiosityElement.stochasticRadiosityElementPullImportance(
                parent, child, parentUnShotImportance, childUnShotImportance);
            parent.unShotImportance = parentUnShotImportance[0];
        }
    }

    /**
Clears everything to be pulled from children elements to zero
*/
    private static void stochasticJacobiClearElement(StochasticRadiosityElement parent) {
        if ( getRadianceCallback != null ) {
            Coefficientsmcrad.stochasticRadiosityClearCoefficients(parent.radiance, parent.basis);
            Coefficientsmcrad.stochasticRadiosityClearCoefficients(parent.unShotRadiance, parent.basis);
        }
        if ( getImportanceCallback != null ) {
            parent.importance = parent.unShotImportance = 0.0f;
        }
    }

    private static void stochasticJacobiPushUpdatePullChild(Element element) {
        StochasticRadiosityElement child = (StochasticRadiosityElement)element;
        StochasticRadiosityElement parent = (StochasticRadiosityElement)child.parent;
        stochasticJacobiPush(parent, child);
        stochasticJacobiPushUpdatePull(child);
        stochasticJacobiPull(parent, child);
    }

    private static void stochasticJacobiPushUpdatePull(Element element) {
        StochasticRadiosityElement stochasticRadiosityElement = (StochasticRadiosityElement)element;
        if ( stochasticRadiosityElement != null && stochasticRadiosityElement.isLeaf() ) {
            stochasticJacobiUpdateElement(stochasticRadiosityElement);
        } else if ( element != null ) {
            // Not a leaf element
            stochasticJacobiClearElement(stochasticRadiosityElement);
            element.traverseAllChildren(StochasticJacobi::stochasticJacobiPushUpdatePullChild);
        }
    }

    private static void stochasticJacobiPullRdEdFromChild(Element element) {
        StochasticRadiosityElement child = (StochasticRadiosityElement)element;
        StochasticRadiosityElement parent = (StochasticRadiosityElement)child.parent;

        stochasticJacobiPullRdEd(child);

        parent.Ed.addScaled(parent.Ed, child.area / parent.area, child.Ed);
        parent.Rd.addScaled(parent.Rd, child.area / parent.area, child.Rd);
        if ( parent.isCluster() ) {
            parent.Rd.setMonochrome(1.0f);
        }
    }

    private static void stochasticJacobiPullRdEd(StochasticRadiosityElement element) {
        if ( element == null
            || element.isLeaf()
            || (!element.isCluster() && !StochasticRadiosityElement.stochasticRadiosityElementIsTextured(element)) ) {
            return;
        }

        element.Ed.clear();
        element.Rd.clear();
        element.traverseAllChildren(StochasticJacobi::stochasticJacobiPullRdEdFromChild);
    }

    private static void stochasticJacobiPushUpdatePullSweep() {
        // Update radiance, compute new total and un-shot flux
        StochasticRelaxation.activeState().unShotFlux.clear();
        StochasticRelaxation.activeState().unShotYmp = 0.0f;
        StochasticRelaxation.activeState().totalFlux.clear();
        StochasticRelaxation.activeState().totalYmp = 0.0f;
        StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux.clear();

        // Update reflectances and emittances (refinement yields more accurate estimates
        // on textured surfaces)
        stochasticJacobiPullRdEd(ElementHierarchyState.activeState().topCluster);

        stochasticJacobiPushUpdatePull(ElementHierarchyState.activeState().topCluster);
    }

    /**
Generic routine for Stochastic Jacobi iterations:
- nr_rays: nr of rays to use
- getRadianceCallBack: routine returning radiance (total or un-shot) to be
propagated for a given element, or nullptr if no radiance propagation is
required.
- getImportanceCallBack: same, but for importance.
- updateCallBack: routine updating total, un-shot and source radiance and/or
importance based on result received during the iteration.

The operation of this routine is further controlled by stochastic relaxation
state parameters:
- constantControlVariate: perform constant control variate variance reduction
- bidirectionalTransfers: use lines bidirectionally
- importanceDriven: importance-driven radiance propagation
- radianceDriven: radiance-driven importance propagation
- hierarchy.do_h_meshing, hierarchy.clustering: hierarchical refinement/clustering

This routine updates global ray counts and total/un-shot power/importance statistics.

CAVEAT: propagate either radiance or importance alone. Simultaneous
propagation of importance and radiance does not work yet.
*/
    public static void doStochasticJacobiIteration(
        VoxelGrid sceneWorldVoxelGrid,
        long numberOfRays,
        GetRadianceCallback getRadianceCallBack,
        GetImportanceCallback getImportanceCallBack,
        UpdateCallback updateCallBack,
        ArrayList<Patch> scenePatches,
        RenderOptions renderOptions)
    {
        stochasticJacobiInitGlobals((int)numberOfRays, getRadianceCallBack, getImportanceCallBack, updateCallBack);
        stochasticJacobiPrintMessage(numberOfRays);
        if ( !stochasticJacobiSetup(scenePatches) ) {
            return;
        }
        stochasticJacobiShootRays(sceneWorldVoxelGrid, scenePatches, renderOptions);
        stochasticJacobiPushUpdatePullSweep();
    }
}

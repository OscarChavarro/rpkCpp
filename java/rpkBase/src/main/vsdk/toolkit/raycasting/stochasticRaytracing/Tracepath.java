/**
Random walk generation
*/

package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.common.color.ColorRgb;

import vsdk.toolkit.common.Random;

import java.util.ArrayList;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.scene.SceneQuery;
import vsdk.toolkit.scene.SceneQueryWorkspace;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RpkRayHit;

public final class Tracepath {
    @FunctionalInterface
    public interface PatchProbabilityCallback {
        double apply(Patch patch);
    }

    @FunctionalInterface
    public interface ScorePathCallback {
        void apply(Path path, long numberOfPaths, PatchProbabilityCallback birthProb);
    }

    @FunctionalInterface
    public interface UpdatePatchCallback {
        void apply(Patch patch, double w);
    }

    private static PatchProbabilityCallback birthProbability = null;
    private static double sumProbabilities = 0.0;

    private Tracepath() {
    }

    /**
Initialises numberOfNodes, nodes allocated to zero and 'nodes' to the nullptr pointer
*/
    private static void initPath(Path path) {
        path.numberOfNodes = path.nodesAllocated = 0;
        path.nodes = null;
    }

    /**
Sets numberOfNodes to zero (forgets old path, but does not free the memory for the nodes
*/
    private static void clearPath(Path path) {
        path.numberOfNodes = 0;
    }

    /**
Adds a node to the path. Re-allocates more space for the nodes if necessary
*/
    private static void pathAddNode(Path path, Patch patch, double prob, Vector3Dd inPoint, Vector3Dd outpoint) {
        if ( path.numberOfNodes >= path.nodesAllocated ) {
            StochasticRaytracingPathNode[] newNodes = new StochasticRaytracingPathNode[path.nodesAllocated + 20];
            for ( int i = 0; i < newNodes.length; i++ ) {
                newNodes[i] = new StochasticRaytracingPathNode();
            }
            if ( path.nodesAllocated > 0 ) {
                for ( int i = 0; i < path.numberOfNodes; i++ ) {
                    // Copy nodes
                    StochasticRaytracingPathNode dst = newNodes[i];
                    StochasticRaytracingPathNode src = path.nodes[i];
                    dst.patch = src.patch;
                    dst.probability = src.probability;
                    dst.inPoint = src.inPoint;
                    dst.outpoint = src.outpoint;
                }
            }
            path.nodes = newNodes;
            path.nodesAllocated += 20;
        }

        StochasticRaytracingPathNode node = path.nodes[path.numberOfNodes];
        node.patch = patch;
        node.probability = prob;
        node.inPoint = inPoint;
        node.outpoint = outpoint;
        path.numberOfNodes++;
    }

    /**
Disposes of the memory for storing path nodes
*/
    private static void freePathNodes(Path path) {
        path.nodes = null;
        path.nodesAllocated = 0;
    }

    /**
Path nodes are filled in 'path', 'path' itself is returned

Traces a random walk originating at 'origin', with birth stochasticJacobiProbability
'globalBirthProbability' (filled in as stochasticJacobiProbability of the origin node: source term
estimation is being suppressed --- survival stochasticJacobiProbability at the origin is
1). Survival stochasticJacobiProbability at other nodes than the origin is calculated by
'survivalProbabilityCallBack()', results are stored in 'path', which should be an
Path, previously initialised by initPath(). If required, photonMapTracePath()
allocates extra space for storing nodes calls to pathAddNode().
freePathNodes() should be called in order to dispose of this memory
when no longer needed
*/
    private static Path tracePath(
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneWorldSceneQueryWorkspace,
        Patch origin,
        double birthProb,
        PatchProbabilityCallback survivalProbabilityCallBack,
        Path path)
    {
        Vector3Dd inPoint = new Vector3Dd(0.0, 0.0, 0.0);
        Vector3Dd outpoint = new Vector3Dd(0.0, 0.0, 0.0);
        Patch P = origin;
        double survivalProb;
        Ray ray;
        RpkRayHit hit;
        RpkRayHit hitStore = new RpkRayHit();

        StochasticRelaxation.activeState().tracedPaths++;
        clearPath(path);
        pathAddNode(path, origin, birthProb, inPoint, outpoint);
        do {
            StochasticRelaxation.activeState().tracedRays++;
            ray = Localline.mcrGenerateLocalLine(P, Sample4d.sample4D((int)McradP.topLevelStochasticRadiosityElement(P).rayIndex));
            McradP.topLevelStochasticRadiosityElement(P).rayIndex++;
            if ( path.numberOfNodes > 1 && StochasticRelaxation.activeState().continuousRandomWalk != 0 ) {
                // Scattered ray originates at point of incidence of previous ray
                ray = ray.withOrigin(path.nodes[path.numberOfNodes - 1].inPoint);
            }
            path.nodes[path.numberOfNodes - 1].outpoint = ray.getOrigin();

            hit = Localline.mcrShootRay(sceneQuery, sceneWorldSceneQueryWorkspace, P, ray, hitStore);
            if ( hit == null ) {
                // Path disappears into background
                break;
            }

            P = hit.getPatch();
            survivalProb = survivalProbabilityCallBack.apply(P);
            pathAddNode(path, P, survivalProb, hit.getPoint(), outpoint);
        } while ( Random.drand48() < survivalProb ); // Repeat until absorption

        return path;
    }

    private static double patchNormalisedBirthProbability(Patch patch) {
        return birthProbability.apply(patch) / sumProbabilities;
    }

    /**
Traces 'numberOfPaths' paths with given birth probabilities
*/
    public static void tracePaths(
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneWorldSceneQueryWorkspace,
        long numberOfPaths,
        PatchProbabilityCallback birthProbabilityCallBack,
        PatchProbabilityCallback survivalProbabilityCallBack,
        ScorePathCallback scorePathCallBack,
        UpdatePatchCallback updateCallBack,
        ArrayList<Patch> scenePatches)
    {
        double rnd;
        double pCumulative;
        long pathCount;
        Path path = new Path();

        StochasticRelaxation.activeState().prevTracedRays = StochasticRelaxation.activeState().tracedRays;
        birthProbability = birthProbabilityCallBack;

        // Compute sampling probability normalisation factor
        sumProbabilities = 0.0;
        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            Patch patch = scenePatches.get(i);
            sumProbabilities += birthProbabilityCallBack.apply(patch);
            Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchReceivedRad(patch), McradP.getTopLevelPatchBasis(patch));
        }
        if ( sumProbabilities < VSDK.EPSILON ) {
            Logger.reportMessage(null, Logger.WARNING, "tracePaths", "No sources");
            return;
        }

        // Fire off paths from the patches, propagate radiance
        initPath(path);
        rnd = Random.drand48();
        pathCount = 0;
        pCumulative = 0.0;
        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            Patch patch = scenePatches.get(i);
            double p = birthProbabilityCallBack.apply(patch) / sumProbabilities;
            long pathsThisPatch = (long)Math.floor((pCumulative + p) * (double)numberOfPaths + rnd) - pathCount;
            for ( int j = 0; j < pathsThisPatch; j++ ) {
                tracePath(sceneQuery, sceneWorldSceneQueryWorkspace, patch, p, survivalProbabilityCallBack, path);
                scorePathCallBack.apply(path, numberOfPaths, Tracepath::patchNormalisedBirthProbability);
            }
            pCumulative += p;
            pathCount += pathsThisPatch;
        }

        System.err.printf("\n");
        freePathNodes(path);

        // updateCallBack radiance, compute new total and un-shot flux
        StochasticRelaxation.activeState().unShotFlux = new ColorRgb(0.0, 0.0, 0.0);
        StochasticRelaxation.activeState().unShotYmp = 0.0;
        StochasticRelaxation.activeState().totalFlux = new ColorRgb(0.0, 0.0, 0.0);
        StochasticRelaxation.activeState().totalYmp = 0.0;

        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            Patch patch = scenePatches.get(i);
            updateCallBack.apply(patch, (double)numberOfPaths / sumProbabilities);
            StochasticRelaxation.activeState().unShotFlux = (StochasticRelaxation.activeState().unShotFlux).addScaled((double)Math.PI * patch.getArea(), McradP.getTopLevelPatchUnShotRad(patch)[0]);
            StochasticRelaxation.activeState().totalFlux = (StochasticRelaxation.activeState().totalFlux).addScaled((double)Math.PI * patch.getArea(), McradP.getTopLevelPatchRad(patch)[0]);
        }
    }
}

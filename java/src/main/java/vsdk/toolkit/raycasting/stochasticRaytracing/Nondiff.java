/**
Non diffuse first shot
*/

package vsdk.toolkit.raycasting.stochasticRaytracing;

import java.util.ArrayList;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.numericalAnalysis.PatchVisitor;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.RayHit;

public final class Nondiff {
    private static LightSourceTable[] lights = null;
    private static int numberOfLights = 0;
    private static int numberOfSamples = 0;
    private static double totalFlux = 0.0;

    private Nondiff() {
    }

    static void makeLightSourceTable(ArrayList<Patch> scenePatches, ArrayList<Patch> lightPatches) {
        totalFlux = 0.0;
        numberOfLights = Statistics.instance().reader.numberOfLightSources;
        lights = new LightSourceTable[numberOfLights];
        for ( int i = 0; i < numberOfLights; i++ ) {
            lights[i] = new LightSourceTable();
        }

        for ( int i = 0; lightPatches != null && i < lightPatches.size(); i++ ) {
            Patch light = lightPatches.get(i);
            ColorRgb emittedRadiance = PatchVisitor.averageEmittance(light, 0x01 | 0x02 | 0x04);
            double flux = Math.PI * light.area * emittedRadiance.sumAbsComponents();
            totalFlux += flux;
            lights[i] = new LightSourceTable(light, flux);
        }

        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            Patch patch = scenePatches.get(i);
            Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchRad(patch), McradP.getTopLevelPatchBasis(patch));
            Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchUnShotRad(patch), McradP.getTopLevelPatchBasis(patch));
            Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchReceivedRad(patch), McradP.getTopLevelPatchBasis(patch));
            McradP.topLevelStochasticRadiosityElement(patch).sourceRad.clear();
        }
    }

    static void nextLightSample(Patch patch, double[] zeta) {
        double[] xi = Sample4d.sample4D((int)McradP.topLevelStochasticRadiosityElement(patch).rayIndex);
        McradP.topLevelStochasticRadiosityElement(patch).rayIndex++;
        if ( patch.numberOfVertices == 3 ) {
            double[] u = new double[] {xi[0]};
            double[] v = new double[] {xi[1]};
            Sample4d.foldSampleF(u, v);
            zeta[0] = u[0];
            zeta[1] = v[0];
        } else {
            zeta[0] = xi[0];
            zeta[1] = xi[1];
        }
        zeta[2] = xi[2];
        zeta[3] = xi[3];
    }

    static Ray sampleLightRay(Patch patch, ColorRgb emittedRad, double[] pointSelectionPdf, double[] dirSelectionPdf) {
        Ray ray = new Ray();
        do {
            double[] zeta = new double[4];
            RayHit hit = new RayHit();
            nextLightSample(patch, zeta);

            patch.uniformPoint(zeta[0], zeta[1], ray.position);

            hit.init(patch, ray.position, patch.normal, patch.material);
            dirSelectionPdf[0] = 0.0;
            ray.direction.set(0.0f, 0.0f, 0.0f);
            if ( patch.material.getEdf() != null ) {
                ray.direction = patch.material.getEdf().phongEdfSample(
                    hit, 0x01 | 0x02 | 0x04, zeta[2], zeta[3], emittedRad, dirSelectionPdf);
            }
        } while ( dirSelectionPdf[0] == 0.0 );

        // The following is only correct if no rejections would result in the
        // loop above, i.o.w. the surface is not textured, or it is textured, but there
        // are no areas that are non-self emitting
        pointSelectionPdf[0] = 1.0 / patch.area;  // Uniform area sampling
        return ray;
    }

    static void sampleLight(VoxelGrid sceneWorldVoxelGrid, LightSourceTable light, double lightSelectionPdf) {
        ColorRgb rad = new ColorRgb();
        double[] pointSelectionPdf = new double[1];
        double[] dirSelectionPdf = new double[1];
        Ray ray = sampleLightRay(light.patch, rad, pointSelectionPdf, dirSelectionPdf);
        RayHit hitStore = new RayHit();

        StochasticRelaxation.activeState().tracedRays++;
        RayHit hit = Localline.mcrShootRay(sceneWorldVoxelGrid, light.patch, ray, hitStore);
        if ( hit != null ) {
            double pdf = lightSelectionPdf * pointSelectionPdf[0] * dirSelectionPdf[0];
            double outCos = ray.direction.dotProduct(light.patch.normal);
            ColorRgb receivedRadiosity = new ColorRgb();
            ColorRgb rd = McradP.topLevelStochasticRadiosityElement(hit.getPatch()).Rd;
            receivedRadiosity.scaledCopy((float)(outCos / (Math.PI * hit.getPatch().area * pdf * numberOfSamples)), rad);
            receivedRadiosity.selfScalarProduct(rd);
            McradP.getTopLevelPatchRad(hit.getPatch())[0].add(McradP.getTopLevelPatchRad(hit.getPatch())[0], receivedRadiosity);
            McradP.getTopLevelPatchUnShotRad(hit.getPatch())[0].add(McradP.getTopLevelPatchUnShotRad(hit.getPatch())[0], receivedRadiosity);
            McradP.topLevelStochasticRadiosityElement(hit.getPatch()).sourceRad.add(
                McradP.topLevelStochasticRadiosityElement(hit.getPatch()).sourceRad, receivedRadiosity);
        }
    }

    static void sampleLightSources(VoxelGrid sceneWorldVoxelGrid, int samplesCount) {
        double rnd = Math.random();
        int count = 0;
        double pCumulative = 0.0;
        Nondiff.numberOfSamples = samplesCount;
        System.err.printf("Shooting %d light rays ", Nondiff.numberOfSamples);
        System.err.flush();
        for ( int i = 0; i < Nondiff.numberOfLights; i++ ) {
            double p = lights[i].flux / totalFlux;
            int samplesThisLight =
                (int)Math.floor((pCumulative + p) * (double)Nondiff.numberOfSamples + rnd) - count;

            for ( int j = 0; j < samplesThisLight; j++ ) {
                sampleLight(sceneWorldVoxelGrid, lights[i], p);
            }

            pCumulative += p;
            count += samplesThisLight;
        }

        System.err.println();
    }

    static void summarize(ArrayList<Patch> scenePatches) {
        StochasticRelaxation.activeState().unShotFlux.clear();
        StochasticRelaxation.activeState().unShotYmp = 0.0f;
        StochasticRelaxation.activeState().totalFlux.clear();
        StochasticRelaxation.activeState().totalYmp = 0.0f;
        StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux.clear();
        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            Patch patch = scenePatches.get(i);
            StochasticRelaxation.activeState().unShotFlux.addScaled(
                StochasticRelaxation.activeState().unShotFlux,
                (float)Math.PI * patch.area,
                McradP.getTopLevelPatchUnShotRad(patch)[0]);
            StochasticRelaxation.activeState().totalFlux.addScaled(
                StochasticRelaxation.activeState().totalFlux,
                (float)Math.PI * patch.area,
                McradP.getTopLevelPatchRad(patch)[0]);
            StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux.addScaled(
                StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux,
                (float)Math.PI * patch.area * (McradP.topLevelStochasticRadiosityElement(patch).importance -
                                              McradP.topLevelStochasticRadiosityElement(patch).sourceImportance),
                McradP.getTopLevelPatchUnShotRad(patch)[0]);
            StochasticRelaxation.activeState().unShotYmp += patch.area * Math.abs(
                McradP.topLevelStochasticRadiosityElement(patch).unShotImportance);
            StochasticRelaxation.activeState().totalYmp += patch.area * McradP.topLevelStochasticRadiosityElement(patch).importance;
            StochasticRelaxation.activeState().sourceYmp += patch.area * McradP.topLevelStochasticRadiosityElement(patch).sourceImportance;
            Mcrad.monteCarloRadiosityPatchComputeNewColor(patch);
        }
    }

    /**
Initial shooting pass handling non-diffuse light sources
*/
    public static void doNonDiffuseFirstShot(
        Scene scene,
        RadianceMethod radianceMethod,
        RenderOptions renderOptions)
    {
        makeLightSourceTable(scene.patchList, scene.lightSourcePatchList);
        sampleLightSources(
            scene.voxelGrid,
            StochasticRelaxation.activeState().initialLightSourceSamples * numberOfLights);
        summarize(scene.patchList);
        lights = null;
    }
}

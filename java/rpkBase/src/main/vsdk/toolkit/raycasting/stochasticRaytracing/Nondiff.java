/**
Non diffuse first shot
*/

package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.environment.geometry.elements.FaceColorTable;
import vsdk.toolkit.common.Random;

import java.util.ArrayList;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.numericalAnalysis.PatchVisitor;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.scene.SceneQuery;
import vsdk.toolkit.scene.SceneQueryWorkspace;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RpkRayHit;

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
            double flux = Math.PI * light.getArea() * emittedRadiance.sumAbsComponents();
            totalFlux += flux;
            lights[i] = new LightSourceTable(light, flux);
        }

        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            Patch patch = scenePatches.get(i);
            Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchRad(patch), McradP.getTopLevelPatchBasis(patch));
            Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchUnShotRad(patch), McradP.getTopLevelPatchBasis(patch));
            Coefficientsmcrad.stochasticRadiosityClearCoefficients(McradP.getTopLevelPatchReceivedRad(patch), McradP.getTopLevelPatchBasis(patch));
            McradP.topLevelStochasticRadiosityElement(patch).sourceRad = new ColorRgb(0.0, 0.0, 0.0);
        }
    }

    static void nextLightSample(Patch patch, double[] zeta) {
        double[] xi = Sample4d.sample4D((int)McradP.topLevelStochasticRadiosityElement(patch).rayIndex);
        McradP.topLevelStochasticRadiosityElement(patch).rayIndex++;
        if ( patch.getNumberOfVertices() == 3 ) {
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

    static Ray sampleLightRay(Patch patch, ColorRgb[] emittedRad, double[] pointSelectionPdf, double[] dirSelectionPdf) {
        Ray ray = new Ray();
        do {
            double[] zeta = new double[4];
            RpkRayHit hit = new RpkRayHit();
            nextLightSample(patch, zeta);

            ray = ray.withOrigin(patch.uniformPoint(zeta[0], zeta[1]));

            hit.init(patch, ray.getOrigin(), patch.getNormal(), patch.getMaterial());
            dirSelectionPdf[0] = 0.0;
            ray = ray.withDirection(new Vector3Dd(0.0, 0.0, 0.0));
            if ( patch.getMaterial().getEdf() != null ) {
                ray = ray.withDirection(patch.getMaterial().getEdf().phongEdfSample(
                    hit.shadingContext(), 0x01 | 0x02 | 0x04, zeta[2], zeta[3], emittedRad, dirSelectionPdf));
            }
        } while ( dirSelectionPdf[0] == 0.0 );

        // The following is only correct if no rejections would result in the
        // loop above, i.o.w. the surface is not textured, or it is textured, but there
        // are no areas that are non-self emitting
        pointSelectionPdf[0] = 1.0 / patch.getArea();  // Uniform area sampling
        return ray;
    }

    static void sampleLight(SceneQuery sceneQuery, SceneQueryWorkspace sceneWorldSceneQueryWorkspace, LightSourceTable light, double lightSelectionPdf) {
        final ColorRgb[] radHolder = new ColorRgb[] {new ColorRgb(0.0, 0.0, 0.0)};
        double[] pointSelectionPdf = new double[1];
        double[] dirSelectionPdf = new double[1];
        Ray ray = sampleLightRay(light.patch, radHolder, pointSelectionPdf, dirSelectionPdf);
        final ColorRgb rad = radHolder[0];
        RpkRayHit hitStore = new RpkRayHit();

        StochasticRelaxation.activeState().tracedRays++;
        RpkRayHit hit = Localline.mcrShootRay(sceneQuery, sceneWorldSceneQueryWorkspace, light.patch, ray, hitStore);
        if ( hit != null ) {
            double pdf = lightSelectionPdf * pointSelectionPdf[0] * dirSelectionPdf[0];
            double outCos = ray.getDirection().dotProduct(light.patch.getNormal());
            ColorRgb receivedRadiosity = new ColorRgb();
            ColorRgb rd = McradP.topLevelStochasticRadiosityElement(hit.getPatch()).Rd;
            receivedRadiosity = rad.multiply((double)(outCos / (Math.PI * hit.getPatch().getArea() * pdf * numberOfSamples)));
            receivedRadiosity = receivedRadiosity.multiply(rd);
            McradP.getTopLevelPatchRad(hit.getPatch())[0] = (McradP.getTopLevelPatchRad(hit.getPatch())[0]).add(receivedRadiosity);
            McradP.getTopLevelPatchUnShotRad(hit.getPatch())[0] = (McradP.getTopLevelPatchUnShotRad(hit.getPatch())[0]).add(receivedRadiosity);
            McradP.topLevelStochasticRadiosityElement(hit.getPatch()).sourceRad = (McradP.topLevelStochasticRadiosityElement(hit.getPatch()).sourceRad).add(receivedRadiosity);
        }
    }

    static void sampleLightSources(SceneQuery sceneQuery, SceneQueryWorkspace sceneWorldSceneQueryWorkspace, int samplesCount) {
        double rnd = Random.drand48();
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
                sampleLight(sceneQuery, sceneWorldSceneQueryWorkspace, lights[i], p);
            }

            pCumulative += p;
            count += samplesThisLight;
        }

        System.err.println();
    }

    static void summarize(ArrayList<Patch> scenePatches, FaceColorTable faceColors) {
        StochasticRelaxation.activeState().unShotFlux = new ColorRgb(0.0, 0.0, 0.0);
        StochasticRelaxation.activeState().unShotYmp = 0.0;
        StochasticRelaxation.activeState().totalFlux = new ColorRgb(0.0, 0.0, 0.0);
        StochasticRelaxation.activeState().totalYmp = 0.0;
        StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux = new ColorRgb(0.0, 0.0, 0.0);
        for ( int i = 0; scenePatches != null && i < scenePatches.size(); i++ ) {
            Patch patch = scenePatches.get(i);
            StochasticRelaxation.activeState().unShotFlux = (StochasticRelaxation.activeState().unShotFlux).addScaled((double)Math.PI * patch.getArea(), McradP.getTopLevelPatchUnShotRad(patch)[0]);
            StochasticRelaxation.activeState().totalFlux = (StochasticRelaxation.activeState().totalFlux).addScaled((double)Math.PI * patch.getArea(), McradP.getTopLevelPatchRad(patch)[0]);
            StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux = (StochasticRelaxation.activeState().indirectImportanceWeightedUnShotFlux).addScaled((double)Math.PI * patch.getArea() * (McradP.topLevelStochasticRadiosityElement(patch).importance - McradP.topLevelStochasticRadiosityElement(patch).sourceImportance), McradP.getTopLevelPatchUnShotRad(patch)[0]);
            StochasticRelaxation.activeState().unShotYmp += patch.getArea() * Math.abs( McradP.topLevelStochasticRadiosityElement(patch).unShotImportance);
            StochasticRelaxation.activeState().totalYmp += patch.getArea() * McradP.topLevelStochasticRadiosityElement(patch).importance;
            StochasticRelaxation.activeState().sourceYmp += patch.getArea() * McradP.topLevelStochasticRadiosityElement(patch).sourceImportance;
            Mcrad.monteCarloRadiosityPatchComputeNewColor(patch, faceColors);
        }
    }

    /**
Initial shooting pass handling non-diffuse light sources
*/
    public static void doNonDiffuseFirstShot(
        Scene scene,
        RadianceMethod radianceMethod,
        RendererConfiguration renderOptions)
    {
        makeLightSourceTable(scene.patchList, scene.lightSourcePatchList);
        sampleLightSources(
            scene.getSceneQuery(),
            scene.getSceneQueryWorkspace(),
            StochasticRelaxation.activeState().initialLightSourceSamples * numberOfLights);
        summarize(scene.patchList, scene.getFaceColors());
        lights = null;
    }
}

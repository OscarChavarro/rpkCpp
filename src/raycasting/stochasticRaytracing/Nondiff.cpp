/**
Non diffuse first shot
*/
#include "java/lang/System.h"
#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "java/util/ArrayList.txx"
#include "common/Statistics.h"
#include "numericalAnalysis/PatchVisitor.h"
#include "raycasting/common/RayTracer.h"
#include "raycasting/stochasticRaytracing/LightSourceTable.h"
#include "raycasting/stochasticRaytracing/Localline.h"
#include "raycasting/stochasticRaytracing/McradP.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

static LightSourceTable *globalLights;
static int globalNumberOfLights;
static int globalNumberOfSamples;
static double globalTotalFlux;

static void
makeLightSourceTable(const java::ArrayList<Patch *> *scenePatches, const java::ArrayList<Patch *> *lightPatches) {
    globalTotalFlux = 0.0;
    globalNumberOfLights = GLOBAL_statistics.numberOfLightSources;
    globalLights = new LightSourceTable[globalNumberOfLights];

    for ( int i = 0; lightPatches != nullptr && i < lightPatches->size(); i++ ) {
        Patch *light = lightPatches->get(i);
        ColorRgb emittedRadiance = PatchVisitor::averageEmittance(light, ALL_COMPONENTS);
        double flux = M_PI * light->area * emittedRadiance.sumAbsComponents();
        globalTotalFlux += flux;
        globalLights[i] = LightSourceTable(light, flux);
    }

    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        const Patch *patch = scenePatches->get(i);
        stochasticRadiosityClearCoefficients(getTopLevelPatchRad(patch), getTopLevelPatchBasis(patch));
        stochasticRadiosityClearCoefficients(getTopLevelPatchUnShotRad(patch), getTopLevelPatchBasis(patch));
        stochasticRadiosityClearCoefficients(getTopLevelPatchReceivedRad(patch), getTopLevelPatchBasis(patch));
        topLevelStochasticRadiosityElement(patch)->sourceRad.clear();
    }
}

static void
nextLightSample(const Patch *patch, double *zeta) {
    const double *xi = sample4D(static_cast<unsigned int>(topLevelStochasticRadiosityElement(patch)->rayIndex));
    topLevelStochasticRadiosityElement(patch)->rayIndex++;
    if ( patch->numberOfVertices == 3 ) {
        double u = xi[0];
        double v = xi[1];
        foldSampleF(&u, &v);
        zeta[0] = u;
        zeta[1] = v;
    } else {
        zeta[0] = xi[0];
        zeta[1] = xi[1];
    }
    zeta[2] = xi[2];
    zeta[3] = xi[3];
}

static Ray
sampleLightRay(Patch *patch, ColorRgb *emitted_rad, double *point_selection_pdf, double *dirSelectionPdf) {
    Ray ray;
    do {
        double zeta[4];
        RayHit hit;
        nextLightSample(patch, zeta);

        patch->uniformPoint(zeta[0], zeta[1], &ray.position);

        hit.init(patch, &ray.position, &patch->normal, patch->material);
        *dirSelectionPdf = 0.0;
        ray.direction.x = 0.0;
        ray.direction.y = 0.0;
        ray.direction.z = 0.0;
        if ( patch->material->getEdf() != nullptr ) {
            ray.direction = patch->material->getEdf()->phongEdfSample(
                &hit, ALL_COMPONENTS, zeta[2], zeta[3], emitted_rad, dirSelectionPdf);
        }
    } while ( *dirSelectionPdf == 0.0 );

    // The following is only correct if no rejections would result in the
    // loop above, i.o.w. the surface is not textured, or it is textured, but there
    // are no areas that are non-self emitting
    *point_selection_pdf = 1.0 / patch->area;  // Uniform area sampling
    return ray;
}

static void
sampleLight(const VoxelGrid * sceneWorldVoxelGrid, LightSourceTable *light, double light_selection_pdf) {
    ColorRgb rad;
    double pointSelectionPdf;
    double dirSelectionPdf;
    Ray ray = sampleLightRay(light->patch, &rad, &pointSelectionPdf, &dirSelectionPdf);
    RayHit hitStore;
    const RayHit *hit;

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays++;
    hit = mcrShootRay(sceneWorldVoxelGrid, light->patch, &ray, &hitStore);
    if ( hit ) {
        double pdf = light_selection_pdf * pointSelectionPdf * dirSelectionPdf;
        double outCos = ray.direction.dotProduct(light->patch->normal);
        ColorRgb receivedRadiosity;
        ColorRgb Rd = topLevelStochasticRadiosityElement(hit->getPatch())->Rd;
        receivedRadiosity.scaledCopy(static_cast<float>(outCos / (M_PI * hit->getPatch()->area * pdf * globalNumberOfSamples)), rad);
        receivedRadiosity.selfScalarProduct(Rd);
        getTopLevelPatchRad(hit->getPatch())[0].add(getTopLevelPatchRad(hit->getPatch())[0], receivedRadiosity);
        getTopLevelPatchUnShotRad(hit->getPatch())[0].add(getTopLevelPatchUnShotRad(hit->getPatch())[0], receivedRadiosity);
        topLevelStochasticRadiosityElement(hit->getPatch())->sourceRad.add(
            topLevelStochasticRadiosityElement(hit->getPatch())->sourceRad, receivedRadiosity);
    }
}

static void
sampleLightSources(const VoxelGrid *sceneWorldVoxelGrid, int numberOfSamples) {
    double rnd = drand48();
    int count = 0;
    double pCumulative = 0.0;
    globalNumberOfSamples = numberOfSamples;
    java::lang::System::err.printf("Shooting %d light rays ", globalNumberOfSamples);
    java::lang::System::err.flush();
    for ( int i = 0; i < globalNumberOfLights; i++ ) {
        double p = globalLights[i].flux / globalTotalFlux;
        int samples_this_light =
                static_cast<int>(floor((pCumulative + p) * static_cast<double>(globalNumberOfSamples) + rnd)) - count;

        for ( int j = 0; j < samples_this_light; j++ ) {
            sampleLight(sceneWorldVoxelGrid, &globalLights[i], p);
        }

        pCumulative += p;
        count += samples_this_light;
    }

    java::lang::System::err.println();
}

static void
summarize(const java::ArrayList<Patch *> *scenePatches) {
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.clear();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp = 0.0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.clear();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp = 0.0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux.clear();
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.addScaled(
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux,
            static_cast<float>(M_PI) * patch->area,
            getTopLevelPatchUnShotRad(patch)[0]);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.addScaled(
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux,
            static_cast<float>(M_PI) * patch->area,
            getTopLevelPatchRad(patch)[0]);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux.addScaled(
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux,
            static_cast<float>(M_PI) * patch->area * (topLevelStochasticRadiosityElement(patch)->importance -
                                                      topLevelStochasticRadiosityElement(patch)->sourceImportance),
              getTopLevelPatchUnShotRad(patch)[0]);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp += patch->area * java::Math::abs(
                topLevelStochasticRadiosityElement(patch)->unShotImportance);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp += patch->area * topLevelStochasticRadiosityElement(patch)->importance;
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp += patch->area * topLevelStochasticRadiosityElement(patch)->sourceImportance;
        monteCarloRadiosityPatchComputeNewColor(patch);
    }
}

/**
Initial shooting pass handling non-diffuse light sources
*/
void
doNonDiffuseFirstShot(const Scene *scene, const RadianceMethod */*radianceMethod*/, const RenderOptions */*renderOptions*/) {
    makeLightSourceTable(scene->patchList, scene->lightSourcePatchList);
    sampleLightSources(
        scene->voxelGrid,
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialLightSourceSamples * globalNumberOfLights);
    summarize(scene->patchList);

    if ( GLOBAL_rayTracer != nullptr ) {
        // TODO: Verify this is not needed, has been disabled flow on May 29 2024
        //Opengl::openGlRenderScene(scene, GLOBAL_rayTracer, radianceMethod, renderOptions);
    }
    delete[] globalLights;
}

#endif

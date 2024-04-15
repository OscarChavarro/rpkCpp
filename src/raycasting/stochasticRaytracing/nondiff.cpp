/**
Non diffuse first shot
*/

#include "java/util/ArrayList.txx"
#include "material/statistics.h"
#include "render/render.h"
#include "render/opengl.h"
#include "raycasting/common/Raytracer.h"
#include "raycasting/stochasticRaytracing/localline.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "scene/scene.h"

class LightSourceTable {
  public:
    Patch *patch;
    double flux;
};

static LightSourceTable *globalLights;
static int globalNumberOfLights;
static int globalNumberOfSamples;
static double globalTotalFlux;

static void
initLight(LightSourceTable *light, Patch *l, double flux) {
    light->patch = l;
    light->flux = flux;
}

static void
makeLightSourceTable(java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> *lightPatches) {
    globalTotalFlux = 0.0;
    globalNumberOfLights = GLOBAL_statistics.numberOfLightSources;
    globalLights = (LightSourceTable *)malloc(globalNumberOfLights * sizeof(LightSourceTable));

    for ( int i = 0; lightPatches != nullptr && i < lightPatches->size(); i++ ) {
        Patch *light = lightPatches->get(i);
        ColorRgb emittedRadiance = light->averageEmittance(ALL_COMPONENTS);
        double flux = M_PI * light->area * emittedRadiance.sumAbsComponents();
        globalTotalFlux += flux;
        initLight(&globalLights[i], light, flux);
    }

    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        stochasticRadiosityClearCoefficients(getTopLevelPatchRad(patch), getTopLevelPatchBasis(patch));
        stochasticRadiosityClearCoefficients(getTopLevelPatchUnShotRad(patch), getTopLevelPatchBasis(patch));
        stochasticRadiosityClearCoefficients(getTopLevelPatchReceivedRad(patch), getTopLevelPatchBasis(patch));
        topLevelGalerkinElement(patch)->sourceRad.clear();
    }
}

static void
nextLightSample(Patch *patch, double *zeta) {
    double *xi = sample4D(topLevelGalerkinElement(patch)->rayIndex++);
    if ( patch->numberOfVertices == 3 ) {
        double u = xi[0], v = xi[1];
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
sampleLightRay(Patch *patch, ColorRgb *emitted_rad, double *point_selection_pdf, double *dir_selection_pdf) {
    Ray ray;
    do {
        double zeta[4];
        RayHit hit;
        nextLightSample(patch, zeta);

        patch->uniformPoint(zeta[0], zeta[1], &ray.pos);

        hitInit(&hit, patch, nullptr, &ray.pos, &patch->normal, patch->surface->material, 0.0);
        ray.dir = edfSample(patch->surface->material->edf, &hit, ALL_COMPONENTS, zeta[2], zeta[3], emitted_rad,
                            dir_selection_pdf);
    } while ( *dir_selection_pdf == 0.0 );

    /* The following is only correct if no rejections would result in the */
    /* loop above, i.o.w. the surface is not textured, or it is textured, but there */
    /* are no areas that are non-self emitting. */
    *point_selection_pdf = 1. / patch->area;  /* uniform area sampling */
    return ray;
}

static void
sampleLight(VoxelGrid * sceneWorldVoxelGrid, LightSourceTable *light, double light_selection_pdf) {
    ColorRgb rad;
    double point_selection_pdf, dir_selection_pdf;
    Ray ray = sampleLightRay(light->patch, &rad, &point_selection_pdf, &dir_selection_pdf);
    RayHit hitStore;
    RayHit *hit;

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays++;
    hit = mcrShootRay(sceneWorldVoxelGrid, light->patch, &ray, &hitStore);
    if ( hit ) {
        double pdf = light_selection_pdf * point_selection_pdf * dir_selection_pdf;
        double outCos = vectorDotProduct(ray.dir, light->patch->normal);
        ColorRgb receivedRadiosity;
        ColorRgb Rd = topLevelGalerkinElement(hit->patch)->Rd;
        receivedRadiosity.scaledCopy((float) (outCos / (M_PI * hit->patch->area * pdf * globalNumberOfSamples)), rad);
        receivedRadiosity.selfScalarProduct(Rd);
        getTopLevelPatchRad(hit->patch)[0].add(getTopLevelPatchRad(hit->patch)[0], receivedRadiosity);
        getTopLevelPatchUnShotRad(hit->patch)[0].add(getTopLevelPatchUnShotRad(hit->patch)[0], receivedRadiosity);
        topLevelGalerkinElement(hit->patch)->sourceRad.add(
            topLevelGalerkinElement(hit->patch)->sourceRad, receivedRadiosity);
    }
}

static void
sampleLightSources(VoxelGrid *sceneWorldVoxelGrid, int numberOfSamples) {
    double rnd = drand48();
    int count = 0, i;
    double pCumulative = 0.0;
    globalNumberOfSamples = numberOfSamples;
    fprintf(stderr, "Shooting %d light rays ", globalNumberOfSamples);
    fflush(stderr);
    for ( i = 0; i < globalNumberOfLights; i++ ) {
        int j;
        double p = globalLights[i].flux / globalTotalFlux;
        int samples_this_light =
                (int) floor((pCumulative + p) * (double) globalNumberOfSamples + rnd) - count;

        for ( j = 0; j < samples_this_light; j++ ) {
            sampleLight(sceneWorldVoxelGrid, &globalLights[i], p);
        }

        pCumulative += p;
        count += samples_this_light;
    }

    fputc('\n', stderr);
}

static void
summarize(java::ArrayList<Patch *> *scenePatches) {
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.clear();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp = 0.0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.clear();
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp = 0.0;
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux.clear();
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux.addScaled(
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux,
            M_PI * patch->area,
            getTopLevelPatchUnShotRad(patch)[0]);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux.addScaled(
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux,
            M_PI * patch->area,
            getTopLevelPatchRad(patch)[0]);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux.addScaled(
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux,
              M_PI * patch->area * (topLevelGalerkinElement(patch)->importance -
                                        topLevelGalerkinElement(patch)->sourceImportance),
              getTopLevelPatchUnShotRad(patch)[0]);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp += patch->area * std::fabs(
                topLevelGalerkinElement(patch)->unShotImportance);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp += patch->area * topLevelGalerkinElement(patch)->importance;
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp += patch->area * topLevelGalerkinElement(patch)->sourceImportance;
        monteCarloRadiosityPatchComputeNewColor(patch);
    }
}

/**
Initial shooting pass handling non-diffuse light sources
*/
void
doNonDiffuseFirstShot(
    VoxelGrid *sceneWorldVoxelGrid,
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Geometry *> *sceneGeometries,
    java::ArrayList<Geometry *> *sceneClusteredGeometries,
    java::ArrayList<Patch *> *lightPatches,
    Geometry *clusteredWorldGeometry,
    RadianceMethod *context)
{
    makeLightSourceTable(scenePatches, lightPatches);
    sampleLightSources(
        sceneWorldVoxelGrid,
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialLightSourceSamples * globalNumberOfLights);
    summarize(scenePatches);

    int (*f)() = nullptr;
    if ( GLOBAL_raytracer_activeRaytracer != nullptr ) {
        f = GLOBAL_raytracer_activeRaytracer->Redisplay;
    }
    openGlRenderScene(
        scenePatches,
        sceneGeometries,
        sceneClusteredGeometries,
        clusteredWorldGeometry,
        f,
        context);
}

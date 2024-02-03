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

class LIGHTSOURCETABLE {
  public:
    Patch *patch;
    double flux;
};

static LIGHTSOURCETABLE *globalLights;
static int globalNumberOfLights;
static int globalNumberOfSamples;
static double globalTotalFlux;

static void
initLight(LIGHTSOURCETABLE *light, Patch *l, double flux) {
    light->patch = l;
    light->flux = flux;
}

static void
makeLightSourceTable(java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> *lightPatches) {
    globalTotalFlux = 0.0;
    globalNumberOfLights = GLOBAL_statistics_numberOfLightSources;
    globalLights = (LIGHTSOURCETABLE *)malloc(globalNumberOfLights * sizeof(LIGHTSOURCETABLE));

    for ( int i = 0; lightPatches != nullptr && i < lightPatches->size(); i++ ) {
        Patch *light = lightPatches->get(i);
        COLOR emitted_rad = patchAverageEmittance(light, ALL_COMPONENTS);
        double flux = M_PI * light->area * colorSumAbsComponents(emitted_rad);
        globalTotalFlux += flux;
        initLight(&globalLights[i], light, flux);
    }

    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        stochasticRadiosityClearCoefficients(getTopLevelPatchRad(patch), getTopLevelPatchBasis(patch));
        stochasticRadiosityClearCoefficients(getTopLevelPatchUnShotRad(patch), getTopLevelPatchBasis(patch));
        stochasticRadiosityClearCoefficients(getTopLevelPatchReceivedRad(patch), getTopLevelPatchBasis(patch));
        colorClear(topLevelGalerkinElement(patch)->sourceRad);
    }
}

static void
nextLightSample(Patch *patch, double *zeta) {
    double *xi = sample4D(topLevelGalerkinElement(patch)->ray_index++);
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
sampleLightRay(Patch *patch, COLOR *emitted_rad, double *point_selection_pdf, double *dir_selection_pdf) {
    Ray ray;
    do {
        double zeta[4];
        RayHit hit;
        nextLightSample(patch, zeta);

        patchUniformPoint(patch, zeta[0], zeta[1], &ray.pos);

        hitInit(&hit, patch, nullptr, &ray.pos, &patch->normal, patch->surface->material, 0.0);
        ray.dir = edfSample(patch->surface->material->edf, &hit, ALL_COMPONENTS, zeta[2], zeta[3], emitted_rad,
                            dir_selection_pdf);
    } while ( *dir_selection_pdf == 0. );

    /* The following is only correct if no rejections would result in the */
    /* loop above, i.o.w. the surface is not textured, or it is textured, but there */
    /* are no areas that are non-self emitting. */
    *point_selection_pdf = 1. / patch->area;  /* uniform area sampling */
    return ray;
}

static void
sampleLight(LIGHTSOURCETABLE *light, double light_selection_pdf) {
    COLOR rad;
    double point_selection_pdf, dir_selection_pdf;
    Ray ray = sampleLightRay(light->patch, &rad, &point_selection_pdf, &dir_selection_pdf);
    RayHit hitStore;
    RayHit *hit;

    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.tracedRays++;
    hit = mcrShootRay(light->patch, &ray, &hitStore);
    if ( hit ) {
        double pdf = light_selection_pdf * point_selection_pdf * dir_selection_pdf;
        double outcos = VECTORDOTPRODUCT(ray.dir, light->patch->normal);
        COLOR rcvrad;
        COLOR Rd = topLevelGalerkinElement(hit->patch)->Rd;
        colorScale((float)(outcos / (M_PI * hit->patch->area * pdf * globalNumberOfSamples)), rad, rcvrad);
        colorProduct(Rd, rcvrad, rcvrad);
        colorAdd(getTopLevelPatchRad(hit->patch)[0], rcvrad, getTopLevelPatchRad(hit->patch)[0]);
        colorAdd(getTopLevelPatchUnShotRad(hit->patch)[0], rcvrad, getTopLevelPatchUnShotRad(hit->patch)[0]);
        colorAdd(topLevelGalerkinElement(hit->patch)->sourceRad, rcvrad, topLevelGalerkinElement(hit->patch)->sourceRad);
    }
}

static void
sampleLightSources(int nr_samples) {
    double rnd = drand48();
    int count = 0, i;
    double pCumulative = 0.0;
    globalNumberOfSamples = nr_samples;
    fprintf(stderr, "Shooting %d light rays ", globalNumberOfSamples);
    fflush(stderr);
    for ( i = 0; i < globalNumberOfLights; i++ ) {
        int j;
        double p = globalLights[i].flux / globalTotalFlux;
        int samples_this_light =
                (int) floor((pCumulative + p) * (double) globalNumberOfSamples + rnd) - count;

        for ( j = 0; j < samples_this_light; j++ ) {
            sampleLight(&globalLights[i], p);
        }

        pCumulative += p;
        count += samples_this_light;
    }

    fputc('\n', stderr);
}

static void
summarize(java::ArrayList<Patch *> *scenePatches) {
    colorClear(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp = 0.;
    colorClear(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux);
    GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp = 0.;
    colorClear(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux);
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        colorAddScaled(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux, M_PI * patch->area, getTopLevelPatchUnShotRad(patch)[0], GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotFlux);
        colorAddScaled(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux, M_PI * patch->area, getTopLevelPatchRad(patch)[0], GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalFlux);
        colorAddScaled(GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux, M_PI * patch->area * (topLevelGalerkinElement(
                               patch)->imp -
                                                                                                                                        topLevelGalerkinElement(patch)->source_imp), getTopLevelPatchUnShotRad(patch)[0],
                       GLOBAL_stochasticRaytracing_monteCarloRadiosityState.indirectImportanceWeightedUnShotFlux);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.unShotYmp += patch->area * std::fabs(
                topLevelGalerkinElement(patch)->unshot_imp);
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.totalYmp += patch->area * topLevelGalerkinElement(patch)->imp;
        GLOBAL_stochasticRaytracing_monteCarloRadiosityState.sourceYmp += patch->area * topLevelGalerkinElement(patch)->source_imp;
        monteCarloRadiosityPatchComputeNewColor(patch);
    }
}

/**
Initial shooting pass handling non-diffuse light sources
*/
void
doNonDiffuseFirstShot(java::ArrayList<Patch *> *scenePatches, java::ArrayList<Patch *> *lightPatches) {
    makeLightSourceTable(scenePatches, lightPatches);
    sampleLightSources(
            GLOBAL_stochasticRaytracing_monteCarloRadiosityState.initialLightSourceSamples * globalNumberOfLights);
    summarize(scenePatches);

    int (*f)() = nullptr;
    if ( GLOBAL_raytracer_activeRaytracer != nullptr ) {
        f = GLOBAL_raytracer_activeRaytracer->Redisplay;
    }
    openGlRenderScene(scenePatches, f);
}

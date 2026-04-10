/**
Non diffuse first shot
*/

#include <stdlib.h>

#include "java/lang/System.h"
#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "java/util/ArrayList.txx"
#include "common/statistics/Statistics.h"
#include "numericalAnalysis/PatchVisitor.h"
#include "raycasting/stochasticRaytracing/LightSourceTable.h"
#include "raycasting/stochasticRaytracing/Localline.h"
#include "raycasting/stochasticRaytracing/McradP.h"
#include "raycasting/stochasticRaytracing/Nondiff.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

LightSourceTable *Nondiff::lights = NULL;
int Nondiff::numberOfLights = 0;
int Nondiff::numberOfSamples = 0;
double Nondiff::totalFlux = 0.0;

void
Nondiff::makeLightSourceTable(const ArrayList<Patch *> *scenePatches, const ArrayList<Patch *> *lightPatches) {
    totalFlux = 0.0;
    numberOfLights = Statistics::instance().reader.numberOfLightSources;
    lights = new LightSourceTable[numberOfLights];

    for ( int i = 0; lightPatches != NULL && i < lightPatches->size(); i++ ) {
        Patch *light = lightPatches->get(i);
        ColorRgb emittedRadiance = PatchVisitor::averageEmittance(light, XxdfComponentFlagInfo::ALL_COMPONENTS);
        double flux = M_PI * light->area * emittedRadiance.sumAbsComponents();
        totalFlux += flux;
        lights[i] = LightSourceTable(light, flux);
    }

    for ( int i = 0; scenePatches != NULL && i < scenePatches->size(); i++ ) {
        const Patch *patch = scenePatches->get(i);
        Coefficientsmcrad::stchsRadClearCoeff(McradP::getTopLevelPatchRad(patch), McradP::getTopLevelPatchBasis(patch));
        Coefficientsmcrad::stchsRadClearCoeff(McradP::getTopLevelPatchUnShotRad(patch), McradP::getTopLevelPatchBasis(patch));
        Coefficientsmcrad::stchsRadClearCoeff(McradP::getTopLevelPatchReceivedRad(patch), McradP::getTopLevelPatchBasis(patch));
        McradP::topLvlStochRadElem(patch)->sourceRad.clear();
    }
}

void
Nondiff::nextLightSample(const Patch *patch, double *zeta) {
    const double *xi = Sample4d::sample4D(((unsigned int)(McradP::topLvlStochRadElem(patch)->rayIndex)));
    McradP::topLvlStochRadElem(patch)->rayIndex++;
    if ( patch->numberOfVertices == 3 ) {
        double u = xi[0];
        double v = xi[1];
        Sample4d::foldSampleF(&u, &v);
        zeta[0] = u;
        zeta[1] = v;
    } else {
        zeta[0] = xi[0];
        zeta[1] = xi[1];
    }
    zeta[2] = xi[2];
    zeta[3] = xi[3];
}

Ray
Nondiff::sampleLightRay(Patch *patch, ColorRgb *emitted_rad, double *point_selection_pdf, double *dirSelectionPdf) {
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
        if ( patch->material->getEdf() != NULL ) {
            ray.direction = patch->material->getEdf()->phongEdfSample(
                &hit, XxdfComponentFlagInfo::ALL_COMPONENTS, zeta[2], zeta[3], emitted_rad, dirSelectionPdf);
        }
    } while ( *dirSelectionPdf == 0.0 );

    // The following is only correct if no rejections would result in the
    // loop above, i.o.w. the surface is not textured, or it is textured, but there
    // are no areas that are non-self emitting
    *point_selection_pdf = 1.0 / patch->area;  // Uniform area sampling
    return ray;
}

void
Nondiff::sampleLight(const VoxelGrid *sceneWorldVoxelGrid, LightSourceTable *light, double light_selection_pdf) {
    ColorRgb rad;
    double pointSelectionPdf;
    double dirSelectionPdf;
    Ray ray = sampleLightRay(light->patch, &rad, &pointSelectionPdf, &dirSelectionPdf);
    RayHit hitStore;
    const RayHit *hit;

    StochasticRelaxation::activeState().tracedRays++;
    hit = Localline::mcrShootRay(sceneWorldVoxelGrid, light->patch, &ray, &hitStore);
    if ( hit ) {
        double pdf = light_selection_pdf * pointSelectionPdf * dirSelectionPdf;
        double outCos = ray.direction.dotProduct(light->patch->normal);
        ColorRgb receivedRadiosity;
        ColorRgb Rd = McradP::topLvlStochRadElem(hit->getPatch())->Rd;
        receivedRadiosity.scaledCopy(((float)(outCos / (M_PI * hit->getPatch()->area * pdf * numberOfSamples))), rad);
        receivedRadiosity.selfScalarProduct(Rd);
        McradP::getTopLevelPatchRad(hit->getPatch())[0].add(McradP::getTopLevelPatchRad(hit->getPatch())[0], receivedRadiosity);
        McradP::getTopLevelPatchUnShotRad(hit->getPatch())[0].add(McradP::getTopLevelPatchUnShotRad(hit->getPatch())[0], receivedRadiosity);
        McradP::topLvlStochRadElem(hit->getPatch())->sourceRad.add(
            McradP::topLvlStochRadElem(hit->getPatch())->sourceRad, receivedRadiosity);
    }
}

void
Nondiff::sampleLightSources(const VoxelGrid *sceneWorldVoxelGrid, int samplesCount) {
    double rnd = drand48();
    int count = 0;
    double pCumulative = 0.0;
    Nondiff::numberOfSamples = samplesCount;
    System::err.printf("Shooting %d light rays ", Nondiff::numberOfSamples);
    System::err.flush();
    for ( int i = 0; i < Nondiff::numberOfLights; i++ ) {
        double p = lights[i].flux / totalFlux;
        int samples_this_light =
                ((int)(floor((pCumulative + p) * ((double)(Nondiff::numberOfSamples)) + rnd))) - count;

        for ( int j = 0; j < samples_this_light; j++ ) {
            sampleLight(sceneWorldVoxelGrid, &lights[i], p);
        }

        pCumulative += p;
        count += samples_this_light;
    }

    System::err.println();
}

void
Nondiff::summarize(const ArrayList<Patch *> *scenePatches) {
    StochasticRelaxation::activeState().unShotFlux.clear();
    StochasticRelaxation::activeState().unShotYmp = 0.0;
    StochasticRelaxation::activeState().totalFlux.clear();
    StochasticRelaxation::activeState().totalYmp = 0.0;
    StochasticRelaxation::activeState().indrcImpWghtdUnShotFlux.clear();
    for ( int i = 0; scenePatches != NULL && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        StochasticRelaxation::activeState().unShotFlux.addScaled(
            StochasticRelaxation::activeState().unShotFlux,
            ((float)(M_PI)) * patch->area,
            McradP::getTopLevelPatchUnShotRad(patch)[0]);
        StochasticRelaxation::activeState().totalFlux.addScaled(
            StochasticRelaxation::activeState().totalFlux,
            ((float)(M_PI)) * patch->area,
            McradP::getTopLevelPatchRad(patch)[0]);
        StochasticRelaxation::activeState().indrcImpWghtdUnShotFlux.addScaled(
            StochasticRelaxation::activeState().indrcImpWghtdUnShotFlux,
            ((float)(M_PI)) * patch->area * (McradP::topLvlStochRadElem(patch)->importance -
                                                      McradP::topLvlStochRadElem(patch)->sourceImportance),
              McradP::getTopLevelPatchUnShotRad(patch)[0]);
        StochasticRelaxation::activeState().unShotYmp += patch->area * Math::abs(
                McradP::topLvlStochRadElem(patch)->unShotImportance);
        StochasticRelaxation::activeState().totalYmp += patch->area * McradP::topLvlStochRadElem(patch)->importance;
        StochasticRelaxation::activeState().sourceYmp += patch->area * McradP::topLvlStochRadElem(patch)->sourceImportance;
        Mcrad::mntCarloRadPtchCompNewClr(patch);
    }
}

/**
Initial shooting pass handling non-diffuse light sources
*/
void
Nondiff::doNonDiffuseFirstShot(const Scene *scene, const RadianceMethod */*radianceMethod*/, const RenderOptions */*renderOptions*/) {
    makeLightSourceTable(scene->patchList, scene->lightSourcePatchList);
    sampleLightSources(
        scene->voxelGrid,
        StochasticRelaxation::activeState().initialLightSourceSamples * numberOfLights);
    summarize(scene->patchList);
    delete[] lights;
    lights = NULL;
}

#endif

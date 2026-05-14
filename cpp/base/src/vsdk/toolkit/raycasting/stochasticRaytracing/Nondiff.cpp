/**
Non diffuse first shot
*/

#include <cstdlib>

#include "vsdk/toolkit/java/lang/System.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "vsdk/toolkit/java/util/ArrayList.txx"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/numericalAnalysis/PatchVisitor.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/LightSourceTable.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Localline.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/McradP.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Nondiff.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation.h"

LightSourceTable *Nondiff::lights = nullptr;
int Nondiff::numberOfLights = 0;
int Nondiff::numberOfSamples = 0;
double Nondiff::totalFlux = 0.0;

void
Nondiff::makeLightSourceTable(const java::ArrayList<Patch *> *scenePatches, const java::ArrayList<Patch *> *lightPatches) {
    totalFlux = 0.0;
    numberOfLights = Statistics::instance().reader.numberOfLightSources;
    lights = new LightSourceTable[numberOfLights];

    for ( int i = 0; lightPatches != nullptr && i < lightPatches->size(); i++ ) {
        Patch *light = lightPatches->get(i);
        ColorRgbMutable emittedRadiance = PatchVisitor::averageEmittance(light, XxdfComponentFlagInfo::ALL_COMPONENTS);
        double flux = M_PI * light->getArea() * emittedRadiance.sumAbsComponents();
        totalFlux += flux;
        lights[i] = LightSourceTable(light, flux);
    }

    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        const Patch *patch = scenePatches->get(i);
        Coefficientsmcrad::stochasticRadiosityClearCoefficients(McradP::getTopLevelPatchRad(patch), McradP::getTopLevelPatchBasis(patch));
        Coefficientsmcrad::stochasticRadiosityClearCoefficients(McradP::getTopLevelPatchUnShotRad(patch), McradP::getTopLevelPatchBasis(patch));
        Coefficientsmcrad::stochasticRadiosityClearCoefficients(McradP::getTopLevelPatchReceivedRad(patch), McradP::getTopLevelPatchBasis(patch));
        McradP::topLevelStochasticRadiosityElement(patch)->sourceRad.clear();
    }
}

void
Nondiff::nextLightSample(const Patch *patch, double *zeta) {
    const double *xi = Sample4d::sample4D(static_cast<unsigned int>(McradP::topLevelStochasticRadiosityElement(patch)->rayIndex));
    McradP::topLevelStochasticRadiosityElement(patch)->rayIndex++;
    if ( patch->getNumberOfVertices() == 3 ) {
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
Nondiff::sampleLightRay(Patch *patch, ColorRgbMutable *emitted_rad, double *point_selection_pdf, double *dirSelectionPdf) {
    Ray ray;
    do {
        double zeta[4];
        RayHit hit;
        nextLightSample(patch, zeta);

        patch->uniformPoint(zeta[0], zeta[1], &ray.position);

        hit.init(patch, &ray.position, &patch->getNormal(), patch->getMaterial());
        *dirSelectionPdf = 0.0;
        ray.direction.x = 0.0;
        ray.direction.y = 0.0;
        ray.direction.z = 0.0;
        if ( patch->getMaterial()->getEdf() != nullptr ) {
            bool shctxOk = false;
            ShadingContext shctx = hit.shadingContext(&shctxOk);
            if ( !shctxOk ) {
                continue;
            }
            ray.direction = patch->getMaterial()->getEdf()->phongEdfSample(
                &shctx, XxdfComponentFlagInfo::ALL_COMPONENTS, zeta[2], zeta[3], emitted_rad, dirSelectionPdf);
        }
    } while ( *dirSelectionPdf == 0.0 );

    // The following is only correct if no rejections would result in the
    // loop above, i.o.w. the surface is not textured, or it is textured, but there
    // are no areas that are non-self emitting
    *point_selection_pdf = 1.0 / patch->getArea();  // Uniform area sampling
    return ray;
}

void
Nondiff::sampleLight(const VoxelGrid *sceneWorldVoxelGrid, LightSourceTable *light, double light_selection_pdf) {
    ColorRgbMutable rad(0.0, 0.0, 0.0);
    double pointSelectionPdf;
    double dirSelectionPdf;
    Ray ray = sampleLightRay(light->patch, &rad, &pointSelectionPdf, &dirSelectionPdf);
    RayHit hitStore;
    const RayHit *hit;

    StochasticRelaxation::activeState().tracedRays++;
    hit = Localline::mcrShootRay(sceneWorldVoxelGrid, light->patch, &ray, &hitStore);
    if ( hit ) {
        double pdf = light_selection_pdf * pointSelectionPdf * dirSelectionPdf;
        double outCos = ray.direction.dotProduct(light->patch->getNormal());
        ColorRgbMutable receivedRadiosity(0.0, 0.0, 0.0);
        ColorRgbMutable Rd = McradP::topLevelStochasticRadiosityElement(hit->getPatch())->Rd;
        receivedRadiosity.scaledCopy(static_cast<float>(outCos / (M_PI * hit->getPatch()->getArea() * pdf * numberOfSamples)), rad);
        receivedRadiosity.selfScalarProduct(Rd);
        McradP::getTopLevelPatchRad(hit->getPatch())[0].add(McradP::getTopLevelPatchRad(hit->getPatch())[0], receivedRadiosity);
        McradP::getTopLevelPatchUnShotRad(hit->getPatch())[0].add(McradP::getTopLevelPatchUnShotRad(hit->getPatch())[0], receivedRadiosity);
        McradP::topLevelStochasticRadiosityElement(hit->getPatch())->sourceRad.add(
            McradP::topLevelStochasticRadiosityElement(hit->getPatch())->sourceRad, receivedRadiosity);
    }
}

void
Nondiff::sampleLightSources(const VoxelGrid *sceneWorldVoxelGrid, int samplesCount) {
    double rnd = drand48();
    int count = 0;
    double pCumulative = 0.0;
    Nondiff::numberOfSamples = samplesCount;
    java::System::err.printf("Shooting %d light rays ", Nondiff::numberOfSamples);
    java::System::err.flush();
    for ( int i = 0; i < Nondiff::numberOfLights; i++ ) {
        double p = lights[i].flux / totalFlux;
        int samples_this_light =
                static_cast<int>(floor((pCumulative + p) * static_cast<double>(Nondiff::numberOfSamples) + rnd)) - count;

        for ( int j = 0; j < samples_this_light; j++ ) {
            sampleLight(sceneWorldVoxelGrid, &lights[i], p);
        }

        pCumulative += p;
        count += samples_this_light;
    }

    java::System::err.println();
}

void
Nondiff::summarize(const java::ArrayList<Patch *> *scenePatches) {
    StochasticRelaxation::activeState().unShotFlux.clear();
    StochasticRelaxation::activeState().unShotYmp = 0.0;
    StochasticRelaxation::activeState().totalFlux.clear();
    StochasticRelaxation::activeState().totalYmp = 0.0;
    StochasticRelaxation::activeState().indirectImportanceWeightedUnShotFlux.clear();
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        StochasticRelaxation::activeState().unShotFlux.addScaled(
            StochasticRelaxation::activeState().unShotFlux,
            static_cast<float>(M_PI) * patch->getArea(),
            McradP::getTopLevelPatchUnShotRad(patch)[0]);
        StochasticRelaxation::activeState().totalFlux.addScaled(
            StochasticRelaxation::activeState().totalFlux,
            static_cast<float>(M_PI) * patch->getArea(),
            McradP::getTopLevelPatchRad(patch)[0]);
        StochasticRelaxation::activeState().indirectImportanceWeightedUnShotFlux.addScaled(
            StochasticRelaxation::activeState().indirectImportanceWeightedUnShotFlux,
            static_cast<float>(M_PI) * patch->getArea() * (McradP::topLevelStochasticRadiosityElement(patch)->importance -
                                                      McradP::topLevelStochasticRadiosityElement(patch)->sourceImportance),
              McradP::getTopLevelPatchUnShotRad(patch)[0]);
        StochasticRelaxation::activeState().unShotYmp += patch->getArea() * java::Math::abs(
                McradP::topLevelStochasticRadiosityElement(patch)->unShotImportance);
        StochasticRelaxation::activeState().totalYmp += patch->getArea() * McradP::topLevelStochasticRadiosityElement(patch)->importance;
        StochasticRelaxation::activeState().sourceYmp += patch->getArea() * McradP::topLevelStochasticRadiosityElement(patch)->sourceImportance;
        Mcrad::monteCarloRadiosityPatchComputeNewColor(patch);
    }
}

/**
Initial shooting pass handling non-diffuse light sources
*/
void
Nondiff::doNonDiffuseFirstShot(const Scene *scene, const RadianceMethod */*radianceMethod*/, const RendererConfiguration */*renderOptions*/) {
    makeLightSourceTable(scene->patchList, scene->lightSourcePatchList);
    sampleLightSources(
        scene->voxelGrid,
        StochasticRelaxation::activeState().initialLightSourceSamples * numberOfLights);
    summarize(scene->patchList);
    delete[] lights;
    lights = nullptr;
}

#endif

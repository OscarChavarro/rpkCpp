/**
Non diffuse first shot
*/

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include <cstdlib>

#include "java/lang/Math.h"
#include "java/lang/System.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

#include "java/util/ArrayList.txx"
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
        const ColorRgb emittedRadiance = PatchVisitor::averageEmittance(light, XxdfComponentFlagInfo::ALL_COMPONENTS);
        double flux = M_PI * light->getArea() * (java::Math::abs(emittedRadiance.getR()) + java::Math::abs(emittedRadiance.getG()) + java::Math::abs(emittedRadiance.getB()));
        totalFlux += flux;
        lights[i] = LightSourceTable(light, flux);
    }

    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        const Patch *patch = scenePatches->get(i);
        Coefficientsmcrad::stochasticRadiosityClearCoefficients(McradP::getTopLevelPatchRad(patch), McradP::getTopLevelPatchBasis(patch));
        Coefficientsmcrad::stochasticRadiosityClearCoefficients(McradP::getTopLevelPatchUnShotRad(patch), McradP::getTopLevelPatchBasis(patch));
        Coefficientsmcrad::stochasticRadiosityClearCoefficients(McradP::getTopLevelPatchReceivedRad(patch), McradP::getTopLevelPatchBasis(patch));
        McradP::topLevelStochasticRadiosityElement(patch)->sourceRad = ColorRgb(0.0, 0.0, 0.0);
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

RpkRay
Nondiff::sampleLightRay(Patch *patch, ColorRgb *emitted_rad, double *point_selection_pdf, double *dirSelectionPdf) {
    RpkRay ray;
    do {
        double zeta[4];
        RpkRayHit hit;
        nextLightSample(patch, zeta);

        patch->uniformPoint(zeta[0], zeta[1], &ray.position);

        const Vector3Df patchNormal = patch->getNormal();

        hit.init(patch, &ray.position, &patchNormal, patch->getMaterial());
        *dirSelectionPdf = 0.0;
        ray.direction = Vector3Df(0.0, 0.0, 0.0);
        if ( patch->getMaterial()->getEdf() != nullptr ) {
            bool shctxOk = false;
            ShadingContext shctx = hit.shadingContext(&shctxOk);
            if ( !shctxOk ) {
                continue;
            }
            ColorRgb emittedRadiance(0.0, 0.0, 0.0);
            ray.direction = patch->getMaterial()->getEdf()->phongEdfSample(
                &shctx, XxdfComponentFlagInfo::ALL_COMPONENTS, zeta[2], zeta[3], &emittedRadiance, dirSelectionPdf);
            *emitted_rad = emittedRadiance;
        }
    } while ( *dirSelectionPdf == 0.0 );

    // The following is only correct if no rejections would result in the
    // loop above, i.o.w. the surface is not textured, or it is textured, but there
    // are no areas that are non-self emitting
    *point_selection_pdf = 1.0 / patch->getArea();  // Uniform area sampling
    return ray;
}

void
Nondiff::sampleLight(const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, LightSourceTable *light, double light_selection_pdf) {
    ColorRgb rad(0.0, 0.0, 0.0);
    double pointSelectionPdf;
    double dirSelectionPdf;
    RpkRay ray = sampleLightRay(light->patch, &rad, &pointSelectionPdf, &dirSelectionPdf);
    RpkRayHit hitStore;
    const RpkRayHit *hit;

    StochasticRelaxation::activeState().tracedRays++;
    hit = Localline::mcrShootRay(sceneQuery, sceneWorldSceneQueryWorkspace, light->patch, &ray, &hitStore);
    if ( hit ) {
        double pdf = light_selection_pdf * pointSelectionPdf * dirSelectionPdf;
        double outCos = ray.direction.dotProduct(light->patch->getNormal());
        const double scale = static_cast<float>(outCos / (M_PI * hit->getPatch()->getArea() * pdf * numberOfSamples));
        double receivedRadiosity[3] = {
            scale * rad.getR(), scale * rad.getG(), scale * rad.getB()};
        const auto &reflectance = McradP::topLevelStochasticRadiosityElement(hit->getPatch())->Rd;
        receivedRadiosity[0] *= reflectance.getR();
        receivedRadiosity[1] *= reflectance.getG();
        receivedRadiosity[2] *= reflectance.getB();
        const ColorRgb contribution(receivedRadiosity[0], receivedRadiosity[1], receivedRadiosity[2]);
        StochasticRadiosityElement *element = McradP::topLevelStochasticRadiosityElement(hit->getPatch());
        element->radiance[0] = element->radiance[0].add(contribution);
        element->unShotRadiance[0] = element->unShotRadiance[0].add(contribution);
        element->sourceRad = element->sourceRad.add(contribution);
    }
}

void
Nondiff::sampleLightSources(const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, int samplesCount) {
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
            sampleLight(sceneQuery, sceneWorldSceneQueryWorkspace, &lights[i], p);
        }

        pCumulative += p;
        count += samples_this_light;
    }

    java::System::err.println();
}

void
Nondiff::summarize(const java::ArrayList<Patch *> *scenePatches, FaceColorTable *faceColors) {
    StochasticRelaxation::activeState().unShotFlux = ColorRgb(0.0, 0.0, 0.0);
    StochasticRelaxation::activeState().unShotYmp = 0.0;
    StochasticRelaxation::activeState().totalFlux = ColorRgb(0.0, 0.0, 0.0);
    StochasticRelaxation::activeState().totalYmp = 0.0;
    StochasticRelaxation::activeState().indirectImportanceWeightedUnShotFlux = ColorRgb(0.0, 0.0, 0.0);
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        StochasticRelaxation::activeState().unShotFlux = StochasticRelaxation::activeState().unShotFlux.addScaled(static_cast<float>(M_PI) * patch->getArea(), McradP::getTopLevelPatchUnShotRad(patch)[0]);
        StochasticRelaxation::activeState().totalFlux = StochasticRelaxation::activeState().totalFlux.addScaled(static_cast<float>(M_PI) * patch->getArea(), McradP::getTopLevelPatchRad(patch)[0]);
        StochasticRelaxation::activeState().indirectImportanceWeightedUnShotFlux = StochasticRelaxation::activeState().indirectImportanceWeightedUnShotFlux.addScaled(static_cast<float>(M_PI) * patch->getArea() * (McradP::topLevelStochasticRadiosityElement(patch)->importance -
                                                      McradP::topLevelStochasticRadiosityElement(patch)->sourceImportance), McradP::getTopLevelPatchUnShotRad(patch)[0]);
        StochasticRelaxation::activeState().unShotYmp += patch->getArea() * java::Math::abs(
                McradP::topLevelStochasticRadiosityElement(patch)->unShotImportance);
        StochasticRelaxation::activeState().totalYmp += patch->getArea() * McradP::topLevelStochasticRadiosityElement(patch)->importance;
        StochasticRelaxation::activeState().sourceYmp += patch->getArea() * McradP::topLevelStochasticRadiosityElement(patch)->sourceImportance;
        Mcrad::monteCarloRadiosityPatchComputeNewColor(patch, faceColors);
    }
}

/**
Initial shooting pass handling non-diffuse light sources
*/
void
Nondiff::doNonDiffuseFirstShot(const Scene *scene, const RadianceMethod */*radianceMethod*/, const RendererConfiguration */*renderOptions*/) {
    makeLightSourceTable(scene->patchList, scene->lightSourcePatchList);
    sampleLightSources(
        scene->getSceneQuery(),
        scene->getSceneQueryWorkspace(),
        StochasticRelaxation::activeState().initialLightSourceSamples * numberOfLights);
    summarize(scene->patchList, scene->getFaceColors());
    delete[] lights;
    lights = nullptr;
}

#endif

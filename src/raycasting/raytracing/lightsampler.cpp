#include "common/error.h"
#include "skin/Patch.h"
#include "raycasting/raytracing/lightsampler.h"

CUniformLightSampler::CUniformLightSampler() {
    // if(gLightList)
    // iterator = new CLightList_Iter(*gLightList);
    iterator = nullptr;
    currentPatch = nullptr;
    unitsActive = false;
}

bool CUniformLightSampler::ActivateFirstUnit() {
    if ( !iterator ) {
        if ( GLOBAL_lightList ) {
            iterator = new LightListIterator(*GLOBAL_lightList);
        } else {
            return false;
        }
    }

    currentPatch = iterator->First(*GLOBAL_lightList);

    if ( currentPatch != nullptr ) {
        unitsActive = true;
        return true;
    } else {
        return false;
    }
}

bool CUniformLightSampler::ActivateNextUnit() {
    currentPatch = iterator->Next();
    return (currentPatch != nullptr);
}

bool
CUniformLightSampler::sample(
        SimpleRaytracingPathNode *prevNode/*prevNode*/,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        double x1,
        double x2,
        bool /* doRR */doRR,
        BSDF_FLAGS flags)
{
    double pdfLight;
    double pdfPoint;
    Patch *light;
    Vector3D point;

    // thisNode is NOT used or altered. If you want the nodes connected,
    // use the Connect method of a surface sampler and a light direction
    // sampler. Otherwise, pdf's cannot be calculated
    // Visibility is NOT determined here!

    newNode->m_depth = 0;
    newNode->m_rayType = STOPS;

    newNode->m_useBsdf = nullptr;
    newNode->m_inBsdf = nullptr;
    newNode->m_outBsdf = nullptr;
    newNode->m_G = 1.0;

    // Choose light
    if ( unitsActive ) {
        if ( currentPatch ) {
            light = currentPatch;
            pdfLight = 1.0;
        } else {
            logWarning("sample Unit Light Node", "No valid light selected");
            return false;
        }
    } else {
        light = GLOBAL_lightList->sample(&x1, &pdfLight);

        if ( light == nullptr ) {
            logWarning("FillLightNode", "No light found");
            return false;
        }
    }

    // Choose point (uniform for real, sampled for background)
    if ( light->hasZeroVertices()) {
        double pdf;
        Vector3D dir = edfSample(light->surface->material->edf, &(thisNode->m_hit), flags, x1, x2, nullptr, &pdf);
        vectorSubtract(thisNode->m_hit.point, dir, point);   // fake hit at distance 1!

        hitInit(&newNode->m_hit, light, nullptr, &point, nullptr,
                light->surface->material, 0.0);

        // Fill in directions
        vectorScale(-1, dir, newNode->m_inDirT);
        vectorCopy(dir, newNode->m_inDirF);
        vectorCopy(dir, newNode->m_normal);

        pdfPoint = pdf;   // every direction corresponds to 1 point
    } else {
        light->uniformPoint(x1, x2, &point);
        pdfPoint = 1.0 / light->area;

        // Fake a hit record
        hitInit(&newNode->m_hit, light, nullptr, &point, &light->normal,
                light->surface->material, 0.0);
        hitShadingNormal(&newNode->m_hit, &newNode->m_hit.normal);
        vectorCopy(newNode->m_hit.normal, newNode->m_normal);
    }

    // inDir's not filled in
    newNode->m_pdfFromPrev = pdfLight * pdfPoint;

    // Component propagation
    newNode->m_accUsedComponents = NO_COMPONENTS; // Light has no accumulated comps.

    newNode->accumulatedRussianRouletteFactors = 1.0;

    return true;
}

double
CUniformLightSampler::EvalPDF(
        SimpleRaytracingPathNode */*thisNode*/,
        SimpleRaytracingPathNode *newNode,
        BSDF_FLAGS /*flags*/, double * /*pdf*/,
        double * /*pdfRR*/)
{
    double pdf;
    double pdfdir;

    // The light point is in NEW NODE!
    if ( unitsActive ) {
        pdf = 1.0;
    } else {
        pdf = GLOBAL_lightList->evalPdf(newNode->m_hit.patch, &newNode->m_hit.point);
    }

    // Prob for choosing this point(/direction)
    if ( newNode->m_hit.patch->hasZeroVertices() ) {
        // virtual patch has no area!
        // choosing a point == choosing a dir --> use pdf from evalEdf
        edfEval(newNode->m_hit.patch->surface->material->edf,
                nullptr,
                &newNode->m_inDirF,
                DIFFUSE_COMPONENT | GLOSSY_COMPONENT | SPECULAR_COMPONENT,
                &pdfdir);

        pdf *= pdfdir;
    } else {
        // Normal patch, choosing point uniformly
        if ( pdf >= EPSILON && newNode->m_hit.patch->area > EPSILON ) {
            pdf = pdf / newNode->m_hit.patch->area;
        } else {
            pdf = 0.0;
        }

    }

    return pdf;
}

/**
Important light sampler : attach weights to each lamp
*/
bool
CImportantLightSampler::sample(
        SimpleRaytracingPathNode *prevNode/*prevNode*/,
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        double x1,
        double x2,
        bool /* doRR */doRR,
        BSDF_FLAGS flags)
{
    double pdfLight, pdfPoint;
    Patch *light;
    Vector3D point;

    // thisNode is NOT used or altered. If you want the nodes connected,
    // use the Connect method of a surface sampler and a light direction
    // sampler. Otherwise, pdf's cannot be calculated
    // Visibility is NOT determined here!
    newNode->m_depth = 0;
    newNode->m_rayType = STOPS;
    newNode->m_useBsdf = nullptr;
    newNode->m_inBsdf = nullptr;
    newNode->m_outBsdf = nullptr;
    newNode->m_G = 1.0;

    // Choose light

    if ( thisNode->m_hit.flags & HIT_BACK ) {
        if ( thisNode->m_outBsdf == nullptr ) {
            Vector3D invNormal;

            vectorScale(-1, thisNode->m_normal, invNormal);

            light = GLOBAL_lightList->sampleImportant(&thisNode->m_hit.point,
                                                      &invNormal, &x1, &pdfLight);
        } else {
            // No (important) light sampling inside a material
            light = nullptr;
        }
    } else {
        if ( thisNode->m_inBsdf == nullptr ) {
            light = GLOBAL_lightList->sampleImportant(&thisNode->m_hit.point,
                                                      &thisNode->m_normal,
                                                      &x1, &pdfLight);
        } else {
            light = nullptr;
        }
    }

    if ( light == nullptr ) {
        // Warning("FillLightNode", "No light found");
        return false;
    }

    // Choose point (uniform for real, sampled for background)
    if ( light->hasZeroVertices() ) {
        double pdf;
        Vector3D dir = edfSample(light->surface->material->edf, nullptr, flags, x1, x2, nullptr, &pdf);
        vectorAdd(thisNode->m_hit.point, dir, point);   // fake hit at distance 1!

        hitInit(&newNode->m_hit, light, nullptr, &point, nullptr, light->surface->material, 0.0);

        // fill in directions
        vectorScale(-1, dir, newNode->m_inDirT);
        vectorCopy(dir, newNode->m_inDirF);
        vectorCopy(dir, newNode->m_normal);

        pdfPoint = pdf; // Every direction corresponds to 1 point
    } else {
        light->uniformPoint(x1, x2, &point);

        pdfPoint = 1.0 / light->area;

        // Light position and value are known now

        // Fake a hit record
        hitInit(&newNode->m_hit, light, nullptr, &point, &light->normal, light->surface->material, 0.0);
        hitShadingNormal(&newNode->m_hit, &newNode->m_hit.normal);
        vectorCopy(newNode->m_hit.normal, newNode->m_normal);
    }

    // outDir's, m_G not filled in yet (light direction sampler does this)

    newNode->m_pdfFromPrev = pdfLight * pdfPoint;

    return true;
}

double
CImportantLightSampler::EvalPDF(
        SimpleRaytracingPathNode *thisNode,
        SimpleRaytracingPathNode *newNode,
        BSDF_FLAGS /*flags*/, double * /*pdf*/,
        double * /*pdfRR*/)
{
    double pdf;
    double pdfdir;

    // The light point is in NEW NODE !!
    pdf = GLOBAL_lightList->evalPdfImportant(
    newNode->m_hit.patch,
    &newNode->m_hit.point,
    &thisNode->m_hit.point,
    &thisNode->m_normal);

    // Prob for choosing this point(/direction)
    if ( newNode->m_hit.patch->hasZeroVertices() ) {
        // virtual patch has no area!
        // choosing a point == choosing a dir --> use pdf from evalEdf
        edfEval(newNode->m_hit.patch->surface->material->edf,
                nullptr,
                &newNode->m_inDirF,
                DIFFUSE_COMPONENT | GLOSSY_COMPONENT | SPECULAR_COMPONENT,
                &pdfdir);

        pdf *= pdfdir;
    } else {
        // Normal patch, choosing point uniformly
        if ( pdf >= EPSILON && newNode->m_hit.patch->area > EPSILON ) {
            pdf = pdf / newNode->m_hit.patch->area;
        } else {
            pdf = 0.0;
        }
    }

    return pdf;
}

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "common/error.h"
#include "skin/Patch.h"
#include "raycasting/bidirectionalRaytracing/LightSampler.h"

UniformLightSampler::UniformLightSampler() {
    // if(gLightList)
    // iterator = new CLightList_Iter(*gLightList);
    iterator = nullptr;
    currentPatch = nullptr;
    unitsActive = false;
}

bool
UniformLightSampler::ActivateFirstUnit() {
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

bool UniformLightSampler::ActivateNextUnit() {
    currentPatch = iterator->Next();
    return (currentPatch != nullptr);
}

bool
UniformLightSampler::sample(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SimpleRaytracingPathNode *prevNode,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    double x1,
    double x2,
    bool doRR,
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
    if ( light->hasZeroVertices() ) {
        double pdf = 0.0;
        Vector3D dir(0.0f, 0.0f, 0.0f);

        if ( light->material->getEdf() != nullptr ) {
            dir = light->material->getEdf()->phongEdfSample(&(thisNode->m_hit), flags, x1, x2, nullptr, &pdf);
        }

        point.subtraction(thisNode->m_hit.getPoint(), dir); // Fake hit at distance 1!

        newNode->m_hit.init(light, &point, nullptr, light->material);

        // Fill in directions
        vectorScale(-1, dir, newNode->m_inDirT);
        newNode->m_inDirF.copy(dir);
        newNode->m_normal.copy(dir);

        pdfPoint = pdf;   // every direction corresponds to 1 point
    } else {
        light->uniformPoint(x1, x2, &point);
        pdfPoint = 1.0 / light->area;

        // Fake a hit record
        newNode->m_hit.init(light, &point, &light->normal, light->material);
        Vector3D normal = newNode->m_hit.getNormal();
        newNode->m_hit.shadingNormal(&normal);
        newNode->m_hit.setNormal(&normal);
        newNode->m_normal.copy(newNode->m_hit.getNormal());
    }

    // inDir's not filled in
    newNode->m_pdfFromPrev = pdfLight * pdfPoint;

    // Component propagation
    newNode->m_accUsedComponents = NO_COMPONENTS; // Light has no accumulated comps.

    newNode->accumulatedRussianRouletteFactors = 1.0;

    return true;
}

double
UniformLightSampler::evalPDF(
    Camera *camera,
    SimpleRaytracingPathNode */*thisNode*/,
    SimpleRaytracingPathNode *newNode,
    BSDF_FLAGS /*flags*/, double * /*pdf*/,
    double * /*pdfRR*/)
{
    double pdf;
    double pdfDir;

    // The light point is in NEW NODE!
    if ( unitsActive ) {
        pdf = 1.0;
    } else {
        Vector3D position = newNode->m_hit.getPoint();
        pdf = GLOBAL_lightList->evalPdf(newNode->m_hit.getPatch(), &position);
    }

    // Prob for choosing this point(/direction)
    if ( newNode->m_hit.getPatch()->hasZeroVertices() ) {
        // virtual patch has no area!
        // choosing a point == choosing a dir --> use pdf from evalEdf
        if ( newNode->m_hit.getPatch()->material->getEdf() == nullptr ) {
            pdfDir = 0.0;
        } else {
            newNode->m_hit.getPatch()->material->getEdf()->phongEdfEval(
                nullptr,
                &newNode->m_inDirF,
                DIFFUSE_COMPONENT | GLOSSY_COMPONENT | SPECULAR_COMPONENT,
                &pdfDir);
        }

        pdf *= pdfDir;
    } else {
        // Normal patch, choosing point uniformly
        if ( pdf >= EPSILON && newNode->m_hit.getPatch()->area > EPSILON ) {
            pdf = pdf / newNode->m_hit.getPatch()->area;
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
ImportantLightSampler::sample(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    SimpleRaytracingPathNode *prevNode,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    double x1,
    double x2,
    bool doRR,
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

    if ( thisNode->m_hit.getFlags() & HIT_BACK ) {
        if ( thisNode->m_outBsdf == nullptr ) {
            Vector3D invNormal;

            vectorScale(-1, thisNode->m_normal, invNormal);

            Vector3D position = thisNode->m_hit.getPoint();
            light = GLOBAL_lightList->sampleImportant(&position,
                                                      &invNormal, &x1, &pdfLight);
        } else {
            // No (important) light sampling inside a material
            light = nullptr;
        }
    } else {
        if ( thisNode->m_inBsdf == nullptr ) {
            Vector3D position = thisNode->m_hit.getPoint();
            light = GLOBAL_lightList->sampleImportant(&position,
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
        double pdf = 0.0;
        Vector3D dir(0.0f, 0.0f, 0.0f);

        if ( light->material->getEdf() != nullptr ) {
            dir = light->material->getEdf()->phongEdfSample(nullptr, flags, x1, x2, nullptr, &pdf);
        }

        vectorAdd(thisNode->m_hit.getPoint(), dir, point);   // fake hit at distance 1!

        newNode->m_hit.init(light, &point, nullptr, light->material);

        // fill in directions
        vectorScale(-1, dir, newNode->m_inDirT);
        newNode->m_inDirF.copy(dir);
        newNode->m_normal.copy(dir);

        pdfPoint = pdf; // Every direction corresponds to 1 point
    } else {
        light->uniformPoint(x1, x2, &point);

        pdfPoint = 1.0 / light->area;

        // Light position and value are known now

        // Fake a hit record
        newNode->m_hit.init(light, &point, &light->normal, light->material);
        Vector3D normal = newNode->m_hit.getNormal();
        newNode->m_hit.shadingNormal(&normal);
        newNode->m_hit.setNormal(&normal);
        newNode->m_normal.copy(newNode->m_hit.getNormal());
    }

    // outDir's, m_G not filled in yet (light direction sampler does this)

    newNode->m_pdfFromPrev = pdfLight * pdfPoint;

    return true;
}

double
ImportantLightSampler::evalPDF(
    Camera *camera,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    BSDF_FLAGS /*flags*/, double * /*pdf*/,
    double * /*pdfRR*/)
{
    double pdf;
    double pdfdir;

    // The light point is in NEW NODE !!
    Vector3D newPosition = newNode->m_hit.getPoint();
    Vector3D thisPosition = thisNode->m_hit.getPoint();
    pdf = GLOBAL_lightList->evalPdfImportant(
    newNode->m_hit.getPatch(),
    &newPosition,
    &thisPosition,
    &thisNode->m_normal);

    // Prob for choosing this point(/direction)
    if ( newNode->m_hit.getPatch()->hasZeroVertices() ) {
        // virtual patch has no area!
        // choosing newPosition point == choosing newPosition dir --> use pdf from evalEdf
        if ( newNode->m_hit.getPatch()->material->getEdf() == nullptr ) {
            pdfdir = 0.0;
        } else {
            newNode->m_hit.getPatch()->material->getEdf()->phongEdfEval(
                nullptr,
                &newNode->m_inDirF,
                DIFFUSE_COMPONENT | GLOSSY_COMPONENT | SPECULAR_COMPONENT,
                &pdfdir);
        }

        pdf *= pdfdir;
    } else {
        // Normal patch, choosing point uniformly
        if ( pdf >= EPSILON && newNode->m_hit.getPatch()->area > EPSILON ) {
            pdf = pdf / newNode->m_hit.getPatch()->area;
        } else {
            pdf = 0.0;
        }
    }

    return pdf;
}

#endif

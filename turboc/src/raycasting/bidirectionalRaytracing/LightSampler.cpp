#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED
#include "common/RenderOptions.h"
#include "common/Error.h"
#include "raycasting/bidirectionalRaytracing/LightSampler.h"

UniformLightSampler::UniformLightSampler(LightList *inLightList):
    lightList(inLightList)
{
    // if(gLightList)
    // iterator = new CLightList_Iter(*gLightList);
    iterator = NULL;
    currentPatch = NULL;
    unitsActive = false;
}

UniformLightSampler::~UniformLightSampler() {
    if ( iterator != NULL ) {
        delete iterator;
        iterator = NULL;
    }
}

bool
UniformLightSampler::ActivateFirstUnit() {
    if ( lightList == NULL ) {
        return false;
    }

    if ( !iterator ) {
        iterator = new LightListIterator(*lightList);
    }

    currentPatch = iterator->First(*lightList);

    if ( currentPatch != NULL ) {
        unitsActive = true;
        return true;
    } else {
        return false;
    }
}

bool UniformLightSampler::ActivateNextUnit() {
    currentPatch = iterator->Next();
    return (currentPatch != NULL);
}

bool
UniformLightSampler::sample(
    Camera */*camera*/,
    VoxelGrid */*sceneVoxelGrid*/,
    Background */*sceneBackground*/,
    SimpleRaytracingPathNode */*prevNode*/,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    double x1,
    double x2,
    bool /*doRR*/,
    char flags)
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

    newNode->m_useBsdf = NULL;
    newNode->m_inBsdf = NULL;
    newNode->m_outBsdf = NULL;
    newNode->m_G = 1.0;

    // Choose light
    if ( unitsActive ) {
        if ( currentPatch ) {
            light = currentPatch;
            pdfLight = 1.0;
        } else {
            Error::warning("sample Unit Light Node", "No valid light selected");
            return false;
        }
    } else {
        if ( lightList == NULL ) {
            Error::warning("FillLightNode", "No light list available");
            return false;
        }
        light = lightList->sample(&x1, &pdfLight);

        if ( light == NULL ) {
            Error::warning("FillLightNode", "No light found");
            return false;
        }
    }

    // Choose point (uniform for real, sampled for background)
    if ( light->hasZeroVertices() ) {
        double pdf = 0.0;
        Vector3D dir(0.0f, 0.0f, 0.0f);

        if ( light->material->getEdf() != NULL ) {
            dir = light->material->getEdf()->phongEdfSample(&(thisNode->m_hit), flags, x1, x2, NULL, &pdf);
        }

        point.subtraction(thisNode->m_hit.getPoint(), dir); // Fake hit at distance 1!

        newNode->m_hit.init(light, &point, NULL, light->material);

        // Fill in directions
        newNode->m_inDirT.scaledCopy(-1, dir);
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
    newNode->m_accUsedComponents = XxdfComponentFlagInfo::NO_COMPONENTS; // Light has no accumulated comps.

    newNode->accmlRssnRlttFctrs = 1.0;

    return true;
}

double
UniformLightSampler::evalPDF(
    Camera */*camera*/,
    SimpleRaytracingPathNode */*thisNode*/,
    SimpleRaytracingPathNode *newNode,
    char /*flags*/,
    double * /*pdf*/,
    double * /*pdfRR*/)
{
    double pdf;
    double pdfDir;

    // The light point is in NEW NODE!
    if ( unitsActive ) {
        pdf = 1.0;
    } else {
        if ( lightList == NULL ) {
            return 0.0;
        }
        Vector3D position = newNode->m_hit.getPoint();
        pdf = lightList->evalPdf(newNode->m_hit.getPatch(), &position);
    }

    // Prob for choosing this point(/direction)
    if ( newNode->m_hit.getPatch()->hasZeroVertices() ) {
        // virtual patch has no area!
        // choosing a point == choosing a dir --> use pdf from evalEdf
        if ( newNode->m_hit.getPatch()->material->getEdf() == NULL ) {
            pdfDir = 0.0;
        } else {
            newNode->m_hit.getPatch()->material->getEdf()->phongEdfEval(
                NULL,
                &newNode->m_inDirF,
                DIFFUSE_COMPONENT | GLOSSY_COMPONENT | SPECULAR_COMPONENT,
                &pdfDir);
        }

        pdf *= pdfDir;
    } else {
        // Normal patch, choosing point uniformly
        if ( pdf >= Numeric::EPSILON && newNode->m_hit.getPatch()->area > Numeric::EPSILON ) {
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
ImportantLightSampler::ImportantLightSampler(LightList *inLightList):
    lightList(inLightList)
{
}

bool
ImportantLightSampler::sample(
    Camera */*camera*/,
    VoxelGrid */*sceneVoxelGrid*/,
    Background */*sceneBackground*/,
    SimpleRaytracingPathNode */*prevNode*/,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    double x1,
    double x2,
    bool /*doRR*/,
    char flags)
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
    newNode->m_useBsdf = NULL;
    newNode->m_inBsdf = NULL;
    newNode->m_outBsdf = NULL;
    newNode->m_G = 1.0;

    // Choose light
    if ( lightList == NULL ) {
        return false;
    }

    if ( thisNode->m_hit.getFlags() & BACK ) {
        if ( thisNode->m_outBsdf == NULL ) {
            Vector3D invNormal;

            invNormal.scaledCopy(-1, thisNode->m_normal);

            Vector3D position = thisNode->m_hit.getPoint();
            light = lightList->sampleImportant(&position, &invNormal, &x1, &pdfLight);
        } else {
            // No (important) light sampling inside a material
            light = NULL;
        }
    } else {
        if ( thisNode->m_inBsdf == NULL ) {
            Vector3D position = thisNode->m_hit.getPoint();
            light = lightList->sampleImportant(&position, &thisNode->m_normal, &x1, &pdfLight);
        } else {
            light = NULL;
        }
    }

    if ( light == NULL ) {
        // No light found
        return false;
    }

    // Choose point (uniform for real, sampled for background)
    if ( light->hasZeroVertices() ) {
        double pdf = 0.0;
        Vector3D dir(0.0f, 0.0f, 0.0f);

        if ( light->material->getEdf() != NULL ) {
            dir = light->material->getEdf()->phongEdfSample(NULL, flags, x1, x2, NULL, &pdf);
        }

        point.addition(thisNode->m_hit.getPoint(), dir);   // fake hit at distance 1!

        newNode->m_hit.init(light, &point, NULL, light->material);

        // fill in directions
        newNode->m_inDirT.scaledCopy(-1, dir);
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
    Camera */*camera*/,
    SimpleRaytracingPathNode *thisNode,
    SimpleRaytracingPathNode *newNode,
    char /*flags*/,
    double * /*pdf*/,
    double * /*pdfRR*/)
{
    double pdf;
    double pdfDir;

    // The light point is in NEW NODE !!
    if ( lightList == NULL ) {
        return 0.0;
    }
    Vector3D newPosition = newNode->m_hit.getPoint();
    Vector3D thisPosition = thisNode->m_hit.getPoint();
    pdf = lightList->evalPdfImportant(
        newNode->m_hit.getPatch(),
        &newPosition,
        &thisPosition,
        &thisNode->m_normal);

    // Prob for choosing this point(/direction)
    if ( newNode->m_hit.getPatch()->hasZeroVertices() ) {
        // virtual patch has no area!
        // choosing newPosition point == choosing newPosition dir --> use pdf from evalEdf
        if ( newNode->m_hit.getPatch()->material->getEdf() == NULL ) {
            pdfDir = 0.0;
        } else {
            newNode->m_hit.getPatch()->material->getEdf()->phongEdfEval(
                NULL,
                &newNode->m_inDirF,
                DIFFUSE_COMPONENT | GLOSSY_COMPONENT | SPECULAR_COMPONENT,
                &pdfDir);
        }

        pdf *= pdfDir;
    } else {
        // Normal patch, choosing point uniformly
        if ( pdf >= Numeric::EPSILON && newNode->m_hit.getPatch()->area > Numeric::EPSILON ) {
            pdf = pdf / newNode->m_hit.getPatch()->area;
        } else {
            pdf = 0.0;
        }
    }

    return pdf;
}

#endif

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
        if ( gLightList ) {
            iterator = new CLightList_Iter(*gLightList);
        } else {
            return false;
        }
    }

    currentPatch = iterator->First(*gLightList);

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

void CUniformLightSampler::DeactivateUnits() {
    unitsActive = false;
    currentPatch = nullptr;
}

bool CUniformLightSampler::Sample(CPathNode */*prevNode*/,
                                  CPathNode *thisNode,
                                  CPathNode *newNode, double x_1, double x_2,
                                  bool /* doRR */, BSDFFLAGS flags) {
    double pdfLight, pdfPoint;
    PATCH *light;
    Vector3D point;

    /* thisNode is NOT used or altered. If you want the nodes connected,
       use the Connect method of a surface sampler and a light direction
       sampler. Otherwise pdf's cannot be calculated
       Visibility is NOT determined here ! */

    newNode->m_depth = 0;
    newNode->m_rayType = Stops;

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
            logWarning("Sample Unit Light Node", "No valid light selected");
            return false;
        }
    } else {
        light = gLightList->Sample(&x_1, &pdfLight);

        if ( light == nullptr ) {
            logWarning("FillLightNode", "No light found");
            return false;
        }
    }

    // Choose point (uniform for real, sampled for background)
    if ( light->isVirtual()) {
        double pdf;
        Vector3D dir = EdfSample(light->surface->material->edf, &(thisNode->m_hit), flags, x_1, x_2, nullptr, &pdf);
        VECTORSUBTRACT(thisNode->m_hit.point, dir, point);   // fake hit at distance 1!

        InitHit(&newNode->m_hit, light, nullptr, &point, nullptr,
                light->surface->material, 0.);

        // fill in directions
        VECTORSCALE(-1, dir, newNode->m_inDirT);
        VECTORCOPY(dir, newNode->m_inDirF);
        VECTORCOPY(dir, newNode->m_normal);

        pdfPoint = pdf;   // every direction corresponds to 1 point
    } else {
        patchUniformPoint(light, x_1, x_2, &point);
        pdfPoint = 1.0 / light->area;

        // Fake a hit record

        InitHit(&newNode->m_hit, light, nullptr, &point, &light->normal,
                light->surface->material, 0.);
        HitShadingNormal(&newNode->m_hit, &newNode->m_hit.normal);
        VECTORCOPY(newNode->m_hit.normal, newNode->m_normal);
    }

    // inDir's not filled in
    newNode->m_pdfFromPrev = pdfLight * pdfPoint;

    // Component propagation
    newNode->m_accUsedComponents = NO_COMPONENTS; // Light has no accumulated comps.

    newNode->m_rracc = 1.0;

    return true;
}

double CUniformLightSampler::EvalPDF(CPathNode */*thisNode*/,
                                     CPathNode *newNode,
                                     BSDFFLAGS /*flags*/, double * /*pdf*/,
                                     double * /*pdfRR*/) {
    double pdf, pdfdir;

    // The light point is in NEW NODE !!

    if ( unitsActive ) {
        pdf = 1.0;
    } else {
        pdf = gLightList->EvalPDF(newNode->m_hit.patch, &newNode->m_hit.point);
    }

    // Prob for choosing this point(/direction)
    if ( newNode->m_hit.patch->isVirtual())          // virtual patch
    {
        // virtual patch has no area!
        // choosing a point == choosing a dir --> use pdf from evalEdf
        EdfEval(newNode->m_hit.patch->surface->material->edf,
                (HITREC *) nullptr,
                &newNode->m_inDirF,
                DIFFUSE_COMPONENT || GLOSSY_COMPONENT || SPECULAR_COMPONENT,
                &pdfdir);

        pdf *= pdfdir;
    } else {                                            // normal patch
        // choosing point uniformly
        if ( pdf >= EPSILON && newNode->m_hit.patch->area > EPSILON ) {
            pdf = pdf / newNode->m_hit.patch->area;
        } else {
            pdf = 0.0;
        }

    }

    return pdf;
}


/****** Important light sampler : attach weights to each lamp ******/

bool CImportantLightSampler::Sample(CPathNode */*prevNode*/,
                                    CPathNode *thisNode,
                                    CPathNode *newNode, double x_1, double x_2,
                                    bool /* doRR */, BSDFFLAGS flags) {
    double pdfLight, pdfPoint;
    PATCH *light;
    Vector3D point;

    /* thisNode is NOT used or altered. If you want the nodes connected,
       use the Connect method of a surface sampler and a light direction
       sampler. Otherwise pdf's cannot be calculated
       Visibility is NOT determined here ! */

    newNode->m_depth = 0;
    newNode->m_rayType = Stops;

    newNode->m_useBsdf = nullptr;
    newNode->m_inBsdf = nullptr;
    newNode->m_outBsdf = nullptr;
    newNode->m_G = 1.0;

    // Choose light

    if ( thisNode->m_hit.flags & HIT_BACK ) {
        if ( thisNode->m_outBsdf == nullptr ) {
            Vector3D invNormal;

            VECTORSCALE(-1, thisNode->m_normal, invNormal);

            light = gLightList->SampleImportant(&thisNode->m_hit.point,
                                                &invNormal, &x_1, &pdfLight);
        } else {
            // No (important) light sampling inside a material
            light = nullptr;
        }
    } else {
        if ( thisNode->m_inBsdf == nullptr ) {
            light = gLightList->SampleImportant(&thisNode->m_hit.point,
                                                &thisNode->m_normal,
                                                &x_1, &pdfLight);
        } else {
            light = nullptr;
        }
    }

    if ( light == nullptr ) {
        // Warning("FillLightNode", "No light found");
        return false;
    }

    // Choose point (uniform for real, sampled for background)
    if ( light->isVirtual()) {
        double pdf;
        Vector3D dir = EdfSample(light->surface->material->edf, nullptr, flags, x_1, x_2, nullptr, &pdf);
        VECTORADD(thisNode->m_hit.point, dir, point);   // fake hit at distance 1!

        InitHit(&newNode->m_hit, light, nullptr, &point, nullptr,
                light->surface->material, 0.);

        // fill in directions
        VECTORSCALE(-1, dir, newNode->m_inDirT);
        VECTORCOPY(dir, newNode->m_inDirF);
        VECTORCOPY(dir, newNode->m_normal);

        pdfPoint = pdf;   // every direction corresponds to 1 point
    } else {
        patchUniformPoint(light, x_1, x_2, &point);

        pdfPoint = 1.0 / light->area;

        // Light position and value are known now

        // Fake a hit record

        InitHit(&newNode->m_hit, light, nullptr, &point, &light->normal,
                light->surface->material, 0.);
        HitShadingNormal(&newNode->m_hit, &newNode->m_hit.normal);
        VECTORCOPY(newNode->m_hit.normal, newNode->m_normal);
    }

    // outDir's, m_G not filled in yet (lightdirection sampler does this)

    newNode->m_pdfFromPrev = pdfLight * pdfPoint;

    return true;
}

double CImportantLightSampler::EvalPDF(CPathNode *thisNode,
                                       CPathNode *newNode,
                                       BSDFFLAGS /*flags*/, double * /*pdf*/,
                                       double * /*pdfRR*/) {
    double pdf, pdfdir;

    // The light point is in NEW NODE !!
    pdf = gLightList->EvalPDFImportant(newNode->m_hit.patch,
                                       &newNode->m_hit.point,
                                       &thisNode->m_hit.point,
                                       &thisNode->m_normal);

    // Prob for choosing this point(/direction)
    if ( newNode->m_hit.patch->isVirtual())           // virtual patch
    {
        // virtual patch has no area!
        // choosing a point == choosing a dir --> use pdf from evalEdf
        EdfEval(newNode->m_hit.patch->surface->material->edf,
                (HITREC *) nullptr,
                &newNode->m_inDirF,
                DIFFUSE_COMPONENT || GLOSSY_COMPONENT || SPECULAR_COMPONENT,
                &pdfdir);

        pdf *= pdfdir;
    } else {                                            // normal patch
        // choosing point uniformly
        if ( pdf >= EPSILON && newNode->m_hit.patch->area > EPSILON ) {
            pdf = pdf / newNode->m_hit.patch->area;
        } else {
            pdf = 0.0;
        }

    }

    return pdf;
}

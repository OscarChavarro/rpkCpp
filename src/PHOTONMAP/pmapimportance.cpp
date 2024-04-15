/**
Importon tracing
*/

#include "skin/Patch.h"
#include "PHOTONMAP/pmapimportance.h"
#include "PHOTONMAP/pmapoptions.h"
#include "PHOTONMAP/pmapconfig.h"
#include "PHOTONMAP/screensampler.h"
#include "scene/scene.h"

/**
Store a importon/poton. Some acceptance tests are performed first
**/
static bool
HasDiffuseOrGlossy(SimpleRaytracingPathNode *node) {
    if ( node->m_hit.patch->surface->material ) {
        BSDF *bsdf = node->m_hit.patch->surface->material->bsdf;
        return !zeroAlbedo(bsdf, &node->m_hit,
                           BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT);
    } else {
        return false;
    }
}

static bool
BounceDiffuseOrGlossy(SimpleRaytracingPathNode *node) {
    return node->m_usedComponents & (BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT);
}

static bool
DoImportanceStore(CImportanceMap *map, SimpleRaytracingPathNode *node, ColorRgb importance) {
    if ( HasDiffuseOrGlossy(node) ) {
        float importanceF = importance.average();
        float potentialF = 1.0; // COLOR_AVERAGE(potential) * Ax;

        // Compute footprint
        float footprintF = 1.0;

        CImporton importon(node->m_hit.point, importanceF, potentialF, footprintF,
                           node->m_inDirF);

        return map->AddPhoton(importon, node->m_hit.normal, 0);
    } else {
        return false;
    }
}

// Returns whether a valid potential path was returned.
static bool
TracePotentialPath(
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    PhotonMapConfig *config)
{
    SimpleRaytracingPathNode *path = config->biPath.m_eyePath;
    CSamplerConfig &scfg = config->eyeConfig;

    // Eye node
    path = scfg.traceNode(sceneVoxelGrid, sceneBackground, path, drand48(), drand48(), BSDF_ALL_COMPONENTS);
    if ( path == nullptr ) {
        return false;
    }
    config->biPath.m_eyePath = path;  // In case no nodes were present

    ColorRgb accImportance;  // Track importance along the ray
    accImportance.setMonochrome(1.0);

    // Adjust importance for eye ray
    float factor = (float)(path->m_G / path->m_pdfFromPrev);
    accImportance.scale(factor);

    bool indirectImportance = false; // Can we store in the indirect importance map

    // New node
    path->ensureNext();
    SimpleRaytracingPathNode *node = path->next();

    // Keep tracing nodes until sampling fails, store importons along the way

    double x1;
    double x2;

    x1 = drand48();
    x2 = drand48();

    while ( scfg.traceNode(
            sceneVoxelGrid,
            sceneBackground,
            node,
            x1,
            x2,
            (BSDF_FLAGS)((indirectImportance ? BSDF_SPECULAR_COMPONENT : BSDF_ALL_COMPONENTS))
            ) ) {
        // Successful trace
        SimpleRaytracingPathNode *prev = node->previous();

        // Determine scatter type
        bool didDG = BounceDiffuseOrGlossy(prev);
        bool tooClose = (node->m_G > GLOBAL_photonMap_state.gThreshold);

        if ( didDG ) {
            if ( !tooClose ) {
                indirectImportance = true;
            }
        }

        // Adjust importance
        accImportance.selfScalarProduct(prev->m_bsdfEval);
        factor = (float)(node->m_G / node->m_pdfFromPrev);
        accImportance.scale(factor);

        // Store in map
        CImportanceMap *imap = (indirectImportance ? config->importanceMap :
                                config->importanceCMap);
        if ( imap ) {
            DoImportanceStore(imap, node, accImportance);
        }

        // New node
        node->ensureNext();
        node = node->next();
        x1 = drand48();
        x2 = drand48();
    }

    return true;
}

void
tracePotentialPaths(Background *sceneBackground, int nrPaths) {
    int i;

    // Fill in config structures
    GLOBAL_photonMap_config.eyeConfig.maxDepth = 7; // Maximum of 4 specular bounces
    GLOBAL_photonMap_config.eyeConfig.minDepth = 3;

    for ( i = 0; i < nrPaths; i++ ) {
        TracePotentialPath(GLOBAL_scene_worldVoxelGrid, sceneBackground, &GLOBAL_photonMap_config);
    }

    GLOBAL_photonMap_config.eyeConfig.maxDepth = 1; // Back to NEE state
    GLOBAL_photonMap_config.eyeConfig.minDepth = 1;
}

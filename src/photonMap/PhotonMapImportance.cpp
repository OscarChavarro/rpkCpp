#include <cstdlib>

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

/**
Importon tracing
*/
#include "common/RenderOptions.h"
#include "photonMap/PhotonMapImportance.h"
#include "photonMap/PhotonMapConfig.h"
#include "photonMap/Importon.h"
#include "photonMap/ScreenSampler.h"
/**
Store a importon/poton. Some acceptance tests are performed first
**/
bool
PhotonMapImportance::hasDiffuseOrGlossy(SimpleRaytracingPathNode *node) {
    if ( node->m_hit.getPatch()->material ) {
        const PhongBidirectionalScatteringDistributionFunction *bsdf = node->m_hit.getPatch()->material->getBsdf();
        return !PhotonMap::zeroAlbedo(bsdf, &node->m_hit,
                           BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT);
    } else {
        return false;
    }
}

bool
PhotonMapImportance::bounceDiffuseOrGlossy(const SimpleRaytracingPathNode *node) {
    return node->m_usedComponents & (BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT);
}

bool
PhotonMapImportance::doImportanceStore(ImportanceMap *map, SimpleRaytracingPathNode *node, ColorRgb importance) {
    if ( PhotonMapImportance::hasDiffuseOrGlossy(node) ) {
        float importanceF = importance.average();
        float potentialF = 1.0;

        // Compute footprint
        float footprintF = 1.0;

        Importon importon(node->m_hit.getPoint(), importanceF, potentialF, footprintF, node->m_inDirF);

        return map->addPhoton(importon, node->m_hit.getNormal(), 0);
    } else {
        return false;
    }
}

// Returns whether a valid potential path was returned.
bool
PhotonMapImportance::tracePotentialPath(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    PhotonMapConfig *config)
{
    SimpleRaytracingPathNode *path = config->biPath.m_eyePath;
    const SamplerConfig &scfg = config->eyeConfig;

    // Eye node
    path = scfg.traceNode(camera, sceneVoxelGrid, sceneBackground, path, drand48(), drand48(), BSDF_ALL_COMPONENTS);
    if ( path == nullptr ) {
        return false;
    }
    config->biPath.m_eyePath = path;  // In case no nodes were present

    ColorRgb accImportance;  // Track importance along the ray
    accImportance.setMonochrome(1.0);

    // Adjust importance for eye ray
    float factor = static_cast<float>(path->m_G / path->m_pdfFromPrev);
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
            camera,
            sceneVoxelGrid,
            sceneBackground,
            node,
            x1,
            x2,
            static_cast<char>(indirectImportance ? BSDF_SPECULAR_COMPONENT : BSDF_ALL_COMPONENTS)
            ) ) {
        // Successful trace
        const SimpleRaytracingPathNode *prev = node->previous();

        // Determine scatter type
        bool didDG = PhotonMapImportance::bounceDiffuseOrGlossy(prev);
        bool tooClose = (node->m_G > GLOBAL_photonMap_state.gThreshold);

        if ( didDG && !tooClose ) {
            indirectImportance = true;
        }

        // Adjust importance
        accImportance.selfScalarProduct(prev->m_bsdfEval);
        factor = static_cast<float>(node->m_G / node->m_pdfFromPrev);
        accImportance.scale(factor);

        // Store in map
        ImportanceMap *imap = (indirectImportance ? config->importanceMap :
                                config->importanceCMap);
        if ( imap ) {
            PhotonMapImportance::doImportanceStore(imap, node, accImportance);
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
PhotonMapImportance::tracePotentialPaths(
    Camera *camera,
    VoxelGrid *sceneVoxelGrid,
    Background *sceneBackground,
    int numberOfPaths)
{
    // Fill in config structures
    GLOBAL_photonMap_config.eyeConfig.maxDepth = 7; // Maximum of 4 specular bounces
    GLOBAL_photonMap_config.eyeConfig.minDepth = 3;

    for ( int i = 0; i < numberOfPaths; i++ ) {
        PhotonMapImportance::tracePotentialPath(camera, sceneVoxelGrid, sceneBackground, &GLOBAL_photonMap_config);
    }

    GLOBAL_photonMap_config.eyeConfig.maxDepth = 1; // Back to NEE state
    GLOBAL_photonMap_config.eyeConfig.minDepth = 1;
}

#endif

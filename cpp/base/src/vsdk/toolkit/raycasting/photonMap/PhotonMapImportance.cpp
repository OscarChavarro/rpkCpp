#include <cstdlib>

#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED

/**
Importon tracing
*/
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMapImportance.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMapConfig.h"
#include "vsdk/toolkit/raycasting/photonMap/Importon.h"
#include "vsdk/toolkit/raycasting/photonMap/ScreenSampler.h"
/**
Store a importon/poton. Some acceptance tests are performed first
**/
bool
PhotonMapImportance::hasDiffuseOrGlossy(SimpleRaytracingPathNode *node) {
    if ( node->m_hit.getPatch()->getMaterial() ) {
        const PhongBidirectionalScatteringDistributionFunction *bsdf = node->m_hit.getPatch()->getMaterial()->getBsdf();
        return !PhotonMap::zeroAlbedo(bsdf, &node->m_hit,
                           BsdfComponentInfo::BSDF_DIFFUSE_COMPONENT | BsdfComponentInfo::BSDF_GLOSSY_COMPONENT);
    } else {
        return false;
    }
}

bool
PhotonMapImportance::bounceDiffuseOrGlossy(const SimpleRaytracingPathNode *node) {
    return node->m_usedComponents & (BsdfComponentInfo::BSDF_DIFFUSE_COMPONENT | BsdfComponentInfo::BSDF_GLOSSY_COMPONENT);
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
    PhotonMapState &photonMapState,
    PhotonMapConfig &photonMapConfig)
{
    SimpleRaytracingPathNode *path = photonMapConfig.biPath.m_eyePath;
    const SamplerConfig &scfg = photonMapConfig.eyeConfig;

    // Eye node
    path = scfg.traceNode(camera, sceneVoxelGrid, sceneBackground, path, drand48(), drand48(), BsdfComponentInfo::BSDF_ALL_COMPONENTS);
    if ( path == nullptr ) {
        return false;
    }
    photonMapConfig.biPath.m_eyePath = path;  // In case no nodes were present

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
            static_cast<char>(indirectImportance ? BsdfComponentInfo::BSDF_SPECULAR_COMPONENT : BsdfComponentInfo::BSDF_ALL_COMPONENTS)
            ) ) {
        // Successful trace
        const SimpleRaytracingPathNode *prev = node->previous();

        // Determine scatter type
        bool didDG = PhotonMapImportance::bounceDiffuseOrGlossy(prev);
        bool tooClose = (node->m_G > photonMapState.gThreshold);

        if ( didDG && !tooClose ) {
            indirectImportance = true;
        }

        // Adjust importance
        accImportance.selfScalarProduct(prev->m_bsdfEval);
        factor = static_cast<float>(node->m_G / node->m_pdfFromPrev);
        accImportance.scale(factor);

        // Store in map
        ImportanceMap *imap = (indirectImportance ? photonMapConfig.importanceMap :
                                photonMapConfig.importanceCMap);
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
    int numberOfPaths,
    PhotonMapState &photonMapState,
    PhotonMapConfig &photonMapConfig)
{
    // Fill in config structures
    photonMapConfig.eyeConfig.maxDepth = 7; // Maximum of 4 specular bounces
    photonMapConfig.eyeConfig.minDepth = 3;

    for ( int i = 0; i < numberOfPaths; i++ ) {
        PhotonMapImportance::tracePotentialPath(
            camera,
            sceneVoxelGrid,
            sceneBackground,
            photonMapState,
            photonMapConfig);
    }

    photonMapConfig.eyeConfig.maxDepth = 1; // Back to NEE state
    photonMapConfig.eyeConfig.minDepth = 1;
}

#endif

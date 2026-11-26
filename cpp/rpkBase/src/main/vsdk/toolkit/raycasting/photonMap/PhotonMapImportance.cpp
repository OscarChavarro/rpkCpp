#include <cstdlib>

#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"

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
PhotonMapImportance::doImportanceStore(ImportanceMap *map, SimpleRaytracingPathNode *node, const ColorRgb &importance) {
    if ( PhotonMapImportance::hasDiffuseOrGlossy(node) ) {
        double importanceF = static_cast<double>(importance.average());
        double potentialF = 1.0;

        // Compute footprint
        double footprintF = 1.0;

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
    const SceneQuery *sceneQuery,
    SceneQueryWorkspace *sceneSceneQueryWorkspace,
    Background *sceneBackground,
    PhotonMapState &photonMapState,
    PhotonMapConfig &photonMapConfig)
{
    SimpleRaytracingPathNode *path = photonMapConfig.biPath.m_eyePath;
    const SamplerConfig &scfg = photonMapConfig.eyeConfig;

    // Eye node
    path = scfg.traceNode(camera, sceneQuery, sceneSceneQueryWorkspace, sceneBackground, path, drand48(), drand48(), BsdfComponentInfo::BSDF_ALL_COMPONENTS);
    if ( path == nullptr ) {
        return false;
    }
    photonMapConfig.biPath.m_eyePath = path;  // In case no nodes were present

    double importanceR = 1.0;
    double importanceG = 1.0;
    double importanceB = 1.0;

    // Adjust importance for eye ray
    double factor = static_cast<double>(path->m_G / path->m_pdfFromPrev);
    importanceR *= factor;
    importanceG *= factor;
    importanceB *= factor;

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
            sceneQuery,
            sceneSceneQueryWorkspace,
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
        importanceR *= prev->m_bsdfEval.getR();
        importanceG *= prev->m_bsdfEval.getG();
        importanceB *= prev->m_bsdfEval.getB();
        factor = static_cast<double>(node->m_G / node->m_pdfFromPrev);
        importanceR *= factor;
        importanceG *= factor;
        importanceB *= factor;

        // Store in map
        ImportanceMap *imap = (indirectImportance ? photonMapConfig.importanceMap :
                                photonMapConfig.importanceCMap);
        if ( imap ) {
            PhotonMapImportance::doImportanceStore(imap, node, ColorRgb(importanceR, importanceG, importanceB));
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
    const SceneQuery *sceneQuery,
    SceneQueryWorkspace *sceneSceneQueryWorkspace,
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
            sceneQuery,
            sceneSceneQueryWorkspace,
            sceneBackground,
            photonMapState,
            photonMapConfig);
    }

    photonMapConfig.eyeConfig.maxDepth = 1; // Back to NEE state
    photonMapConfig.eyeConfig.minDepth = 1;
}

#endif

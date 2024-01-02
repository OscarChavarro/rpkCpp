/*
 * pmapimportance.C: Trace importons
 */

#include "PHOTONMAP/pmapimportance.h"
#include "PHOTONMAP/pmapoptions.h"
#include "PHOTONMAP/pmapconfig.h"
#include "PHOTONMAP/screensampler.h"

/** Store a importon/poton. Some acceptance tests are performed first **/

static bool HasDiffuseOrGlossy(CPathNode *node) {
    if ( node->m_hit.patch->surface->material ) {
        BSDF *bsdf = node->m_hit.patch->surface->material->bsdf;
        return !ZeroAlbedo(bsdf, &node->m_hit,
                           BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT);
    } else {
        return false;
    }
}

static bool BounceDiffuseOrGlossy(CPathNode *node) {
    return node->m_usedComponents & (BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT);
}


static bool DoImportanceStore(CImportanceMap *map, CPathNode *node, COLOR importance) {
    if ( HasDiffuseOrGlossy(node)) {
        float importanceF = COLORAVERAGE(importance);
        float potentialF = 1.0; // COLORAVERAGE(potential)*Ax;

        // Compute footprint
        float footprintF = 1.0;

        CImporton importon(node->m_hit.point, importanceF, potentialF, footprintF,
                           node->m_inDirF);

        return map->AddPhoton(importon, node->m_hit.normal, 0);
    } else {
        return false;
    }
}


/** -- Handle one potential path : store at all end positions in direct
    readout importance map (TODO...) and at the end point in the
    indirect readout importance map.
**/

void HandlePotentialPath(PMAPCONFIG *config) {
    CBiPath *bp = &config->bipath;
    COLOR accImportance;  // Importance : integration


    // Store first diffuse/glossy -> direct importance map
    // Store last node -> indirect importance map
    //   tracing made sure importance is low enough for the indirect importance map

    // bp->m_eyeSize = 1;
    CPathNode *currentNode = bp->m_eyePath;
    CPathNode *prevNode = nullptr;

    bp->m_geomConnect = 1.0; // No connection yet (not needed)

    COLORSETMONOCHROME(accImportance, 1.0);

    bool isLastNode = false;
    int DGBounces = 0;

    // Iterate
    while ( !isLastNode ) {
        // Adjust importance
        float factor = currentNode->m_G / currentNode->m_pdfFromPrev;
        COLORSCALE(factor, accImportance, accImportance);

        if ( prevNode )  // Skip the eye node
        {
            // Check if direct importance
            if ( DGBounces == 0 ) {
                // No diffuse or glossy bounce yet: direct importance map

                // DoImportanceStore(GLOBAL_photonMap_config->importanceCMap, currentNode, importance);
            }
        }

        // Next node
        if ( currentNode->Ends()) {
            isLastNode = true;
        } else {
            COLORPROD(currentNode->m_bsdfEval, accImportance, accImportance);

            if ( currentNode->m_usedComponents & (BSDF_DIFFUSE_COMPONENT | BSDF_GLOSSY_COMPONENT)) {
                DGBounces++;
            }

            currentNode = currentNode->Next();
        }
    }

    // currentnode is the last node in the path

    if ( DGBounces > 0 ) {
        // It's an indirect photon
        DoImportanceStore(config->importanceMap, currentNode, accImportance);
    }
}


// Returns whether a valid potential path was returned.
static bool TracePotentialPath(PMAPCONFIG *config) {
    CPathNode *path = config->bipath.m_eyePath;
    CSamplerConfig &scfg = config->eyeConfig;

    // Eye node
    path = scfg.TraceNode(path, drand48(), drand48(), BSDF_ALL_COMPONENTS);
    if ( path == nullptr ) {
        return false;
    }
    config->bipath.m_eyePath = path;  // In case no nodes were present

    COLOR accImportance;  // Track importance along the ray
    COLORSETMONOCHROME(accImportance, 1.0);

    // Adjust importance for eye ray
    float factor = path->m_G / path->m_pdfFromPrev;
    COLORSCALE(factor, accImportance, accImportance);


    int DGBounces = 0;  // Number of diffuse/glossy bounces
    bool indirectImportance = false; // Can we store in the indirect importance map

    // New node
    path->EnsureNext();
    CPathNode *node = path->Next();

    // Keep tracing nodes until sampling fails, store importons along the way

    double x_1, x_2;

    x_1 = drand48();
    x_2 = drand48();

    while ( scfg.TraceNode(node, x_1, x_2,
                           (indirectImportance ? BSDF_SPECULAR_COMPONENT : BSDF_ALL_COMPONENTS))) {
        // Succesful trace
        CPathNode *prev = node->Previous();

        // Determine scatter type
        bool didDG = BounceDiffuseOrGlossy(prev);
        bool tooClose = (node->m_G > pmapstate.gThreshold);

        if ( didDG ) {
            DGBounces++;
            if ( !tooClose ) {
                indirectImportance = true;
            }
        }

        // Adjust importance
        COLORPROD(prev->m_bsdfEval, accImportance, accImportance);
        factor = node->m_G / node->m_pdfFromPrev;
        COLORSCALE(factor, accImportance, accImportance);


        // Store in map

        CImportanceMap *imap = (indirectImportance ? config->importanceMap :
                                config->importanceCMap);
        if ( imap ) {
            DoImportanceStore(imap, node, accImportance);
        }

        // New node
        node->EnsureNext();
        node = node->Next();
        x_1 = drand48();
        x_2 = drand48();
    }

    // fprintf(stderr, "DGBounces %i\n", DGBounces);
    return true;
}

void TracePotentialPaths(int nrPaths) {
    int i;

    // Fill in config structures

    GLOBAL_photonMap_config.eyeConfig.maxDepth = 7; // Maximum of 4 specular bounces
    GLOBAL_photonMap_config.eyeConfig.minDepth = 3;

    CPathNode::m_dmaxsize = 0; // No need for derivative structures


    for ( i = 0; i < nrPaths; i++ ) {
        TracePotentialPath(&GLOBAL_photonMap_config);
    }


    GLOBAL_photonMap_config.eyeConfig.maxDepth = 1; // Back to NEE state
    GLOBAL_photonMap_config.eyeConfig.minDepth = 1;
}



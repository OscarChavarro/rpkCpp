/**
Class implementation of path nodes. These node are building blocks of paths.
and contain necessary information for raytracing-like algorithms
*/

#include <cstdio>

#include "common/error.h"
#include "skin/Patch.h"
#include "raycasting/common/pathnode.h"

SimpleRaytracingPathNode::SimpleRaytracingPathNode():
        m_G(), m_pdfFromPrev(), m_pdfFromNext(), m_rrPdfFromNext(), accumulatedRussianRouletteFactors(),
        m_usedComponents(), m_accUsedComponents(), m_useBsdf(), m_inBsdf(), m_outBsdf(),
        m_rayType(), m_depth()
{
    m_next = nullptr;
    m_previous = nullptr;

}

SimpleRaytracingPathNode::~SimpleRaytracingPathNode() {
}

void
SimpleRaytracingPathNode::print(FILE *out) {
    fprintf(out, "Path node at depth %i\n", m_depth);
    fprintf(out, "Pos : ");
    vector3DPrint(out, m_hit.point);
    fprintf(out, "\n");
    fprintf(out, "Norm: ");
    vector3DPrint(out, m_normal);
    fprintf(out, "\n");
    if ( m_previous ) {
        fprintf(out, "InF: ");
        vector3DPrint(out, m_inDirF);
        fprintf(out, "\n");
        fprintf(out, "Cos in  %f\n", vectorDotProduct(m_normal, m_inDirF));
        fprintf(out, "GCos in %f\n", vectorDotProduct(m_hit.patch->normal,
                                                      m_inDirF));
    }
    if ( m_next ) {
        fprintf(out, "OutF: ");
        vector3DPrint(out, m_next->m_inDirT);
        fprintf(out, "\n");
        fprintf(out, "Cos out %f\n", vectorDotProduct(m_normal, m_next->m_inDirT));
        fprintf(out, "GCos out %f\n", vectorDotProduct(m_hit.patch->normal,
                                                       m_next->m_inDirT));
    }
}

/* GetPreviousBsdf
 *
 * Searches back in the path to find the bsdf on the outside
 * of the current path/material. The last node should be
 * a back hit -> Leaving ray-type. If not, the current bsdf
 * is returned and an error is reported
 */

/* Helper routine, searches the corresponding 'Enters'
 * node for this node
 */

SimpleRaytracingPathNode *
SimpleRaytracingPathNode::GetMatchingNode() {
    BSDF *thisBsdf;
    int backHits;
    SimpleRaytracingPathNode *tmpNode = previous();
    SimpleRaytracingPathNode *matchedNode = nullptr;

    thisBsdf = m_useBsdf;
    backHits = 1;

    while ( tmpNode && backHits > 0 ) {
        switch ( tmpNode->m_rayType ) {
            case ENTERS:
                if ( tmpNode->m_hit.patch->surface->material->bsdf == thisBsdf ) {
                    backHits--; // Entering point in this material
                }
                break;
            case LEAVES:
                if ( tmpNode->m_inBsdf == thisBsdf ) {
                    backHits++; // Leaves the same material more than one time
                }
                break;
            case REFLECTS:
                break;
            default:
                logError("CPathNode::GetMatchingNode", "Wrong ray type in path");
        }

        matchedNode = tmpNode;
        tmpNode = tmpNode->previous();
    }

    if ( backHits == 0 ) {
        return (matchedNode);
    } else {
        return nullptr;  // No matching node
    }
}

BSDF *
SimpleRaytracingPathNode::getPreviousBsdf() {
    SimpleRaytracingPathNode *matchedNode;

    if ( !(m_hit.flags & HIT_BACK) ) {
        logError("CPathNode::getPreviousBsdf", "Last node not a back hit");
        return (m_inBsdf);  // Should not happen
    }

    if ( m_hit.patch->surface->material->bsdf !=
         m_inBsdf ) {
        logWarning("CPathNode::GetPreviousBtdf", "Last back hit has wrong bsdf");
    }

    // Find the corresponding ray that enters the material
    matchedNode = GetMatchingNode();

    if ( matchedNode == nullptr ) {
        logWarning("CPathNode::GetPreviousBtdf", "No corresponding entering ray");
        return (m_inBsdf);  // Should not happen
    }

    return matchedNode->m_inBsdf;
}

void
SimpleRaytracingPathNode::assignBsdfAndNormal() {
    Material *thisMaterial;

    if ( m_hit.patch == nullptr ) {
        // Invalid node
        return;
    }

    thisMaterial = m_hit.patch->surface->material;

    vectorCopy(m_hit.normal, m_normal); // Possible double format

    // Assign bsdf's
    m_useBsdf = thisMaterial->bsdf;

    if ( m_hit.flags & HIT_FRONT ) {
        m_outBsdf = m_useBsdf; // In filled in when creating this node
    } else {
        // BACK HIT
        m_outBsdf = getPreviousBsdf();
    }
}

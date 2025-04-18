/**
Class implementation of path nodes. These node are building blocks of paths.
and contain necessary information for raytracing-like algorithms
*/

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

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
SimpleRaytracingPathNode::print(FILE *out) const {
    fprintf(out, "Path node at depth %i\n", m_depth);
    fprintf(out, "Pos : ");
    m_hit.getPoint().print(out);
    fprintf(out, "\n");
    fprintf(out, "Norm: ");
    m_normal.print(out);
    fprintf(out, "\n");
    if ( m_previous ) {
        fprintf(out, "InF: ");
        m_inDirF.print(out);
        fprintf(out, "\n");
        fprintf(out, "Cos in  %f\n", m_normal.dotProduct(m_inDirF));
        fprintf(out, "GCos in %f\n", m_hit.getPatch()->normal.dotProduct(m_inDirF));
    }
    if ( m_next ) {
        fprintf(out, "OutF: ");
        m_next->m_inDirT.print(out);
        fprintf(out, "\n");
        fprintf(out, "Cos out %f\n", m_normal.dotProduct(m_next->m_inDirT));
        fprintf(out, "GCos out %f\n", m_hit.getPatch()->normal.dotProduct(m_next->m_inDirT));
    }
}

/**
GetPreviousBsdf

Searches back in the path to find the bsdf on the outside
of the current path/material. The last node should be
a back hit -> Leaving ray-type. If not, the current bsdf
is returned and an error is reported
*/

/**
Helper routine, searches the corresponding 'Enters' node for this node
*/

SimpleRaytracingPathNode *
SimpleRaytracingPathNode::GetMatchingNode() {
    const PhongBidirectionalScatteringDistributionFunction *thisBsdf;
    int backHits;
    SimpleRaytracingPathNode *tmpNode = previous();
    SimpleRaytracingPathNode *matchedNode = nullptr;

    thisBsdf = m_useBsdf;
    backHits = 1;

    while ( tmpNode && backHits > 0 ) {
        switch ( tmpNode->m_rayType ) {
            case PathRayType::ENTERS:
                if ( tmpNode->m_hit.getPatch()->material->getBsdf() == thisBsdf ) {
                    backHits--; // Entering point in this material
                }
                break;
            case PathRayType::LEAVES:
                if ( tmpNode->m_inBsdf == thisBsdf ) {
                    backHits++; // Leaves the same material more than one time
                }
                break;
            case PathRayType::REFLECTS:
                break;
            default:
                logError("CPathNode::GetMatchingNode", "Wrong ray type in path");
        }

        matchedNode = tmpNode;
        tmpNode = tmpNode->previous();
    }

    if ( backHits == 0 ) {
        return matchedNode;
    } else {
        return nullptr;  // No matching node
    }
}

PhongBidirectionalScatteringDistributionFunction *
SimpleRaytracingPathNode::getPreviousBsdf() {
    SimpleRaytracingPathNode *matchedNode;

    if ( !(m_hit.getFlags() & RayHitFlag::BACK) ) {
        logError("CPathNode::getPreviousBsdf", "Last node not a back hit");
        return m_inBsdf;  // Should not happen
    }

    if ( m_hit.getPatch()->material->getBsdf() != m_inBsdf ) {
        logWarning("CPathNode::GetPreviousBtdf", "Last back hit has wrong bsdf");
    }

    // Find the corresponding ray that enters the material
    matchedNode = GetMatchingNode();

    if ( matchedNode == nullptr ) {
        logWarning("CPathNode::GetPreviousBtdf", "No corresponding entering ray");
        return m_inBsdf;  // Should not happen
    }

    return matchedNode->m_inBsdf;
}

void
SimpleRaytracingPathNode::assignBsdfAndNormal() {
    const Material *thisMaterial;

    if ( m_hit.getPatch() == nullptr ) {
        // Invalid node
        return;
    }

    thisMaterial = m_hit.getPatch()->material;

    m_normal.copy(m_hit.getNormal()); // Possible double format

    // Assign bsdf's
    m_useBsdf = thisMaterial->getBsdf();

    if ( m_hit.getFlags() & RayHitFlag::FRONT ) {
        m_outBsdf = m_useBsdf; // In filled in when creating this node
    } else {
        // BACK HIT
        m_outBsdf = getPreviousBsdf();
    }
}

#endif

#include "common/RenderOptions.h"

/**
Class implementation of path nodes. These node are building blocks of paths.
and contain necessary information for raytracing-like algorithms
*/


#ifdef RAYTRACING_ENABLED
#include "common/RenderOptions.h"
#include "common/Error.h"
#include "skin/Patch.h"
#include "raycasting/common/SimpleRaytracingPathNode.h"

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
SimpleRaytracingPathNode::print(java::PrintStream *out) const {
    if ( out == nullptr ) {
        return;
    }

    out->printf("Path node at depth %i\n", m_depth);
    out->printf("Pos : ");
    m_hit.getPoint().print(out);
    out->printf("\n");
    out->printf("Norm: ");
    m_normal.print(out);
    out->printf("\n");
    if ( m_previous ) {
        out->printf("InF: ");
        m_inDirF.print(out);
        out->printf("\n");
        out->printf("Cos in  %f\n", m_normal.dotProduct(m_inDirF));
        out->printf("GCos in %f\n", m_hit.getPatch()->normal.dotProduct(m_inDirF));
    }
    if ( m_next ) {
        out->printf("OutF: ");
        m_next->m_inDirT.print(out);
        out->printf("\n");
        out->printf("Cos out %f\n", m_normal.dotProduct(m_next->m_inDirT));
        out->printf("GCos out %f\n", m_hit.getPatch()->normal.dotProduct(m_next->m_inDirT));
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
                Error::error("CPathNode::GetMatchingNode", "Wrong ray type in path");
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
        Error::error("CPathNode::getPreviousBsdf", "Last node not a back hit");
        return m_inBsdf;  // Should not happen
    }

    if ( m_hit.getPatch()->material->getBsdf() != m_inBsdf ) {
        Error::warning("CPathNode::GetPreviousBtdf", "Last back hit has wrong bsdf");
    }

    // Find the corresponding ray that enters the material
    matchedNode = GetMatchingNode();

    if ( matchedNode == nullptr ) {
        Error::warning("CPathNode::GetPreviousBtdf", "No corresponding entering ray");
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

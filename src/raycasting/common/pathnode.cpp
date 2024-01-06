/* pathnode.C
 *
 * Class implementation of pathnodes. These node are building blocks of paths.
 * and contain necessary information for raytracing-like algorithms
 */

#include <cstdio>

#include "common/error.h"
#include "scene/scene.h"
#include "raycasting/common/pathnode.h"

/* PathNode functions */

int CPathNode::m_dmaxsize = 0;

CPathNode::CPathNode() {
    m_next = nullptr;
    m_previous = nullptr;

}


CPathNode::~CPathNode() {
}

// Delete all nodes, includind the supplied 'node'
void CPathNode::ReleaseAll(CPathNode *node) {
    CPathNode *tmp;

    while ( node ) {
        tmp = node;
        node = node->Next();
        delete tmp;
    }
}

void CPathNode::Print(FILE *out) {
    fprintf(out, "Pathnode at depth %i\n", m_depth);
    fprintf(out, "Pos : ");
    VectorPrint(out, m_hit.point);
    fprintf(out, "\n");
    fprintf(out, "Norm: ");
    VectorPrint(out, m_normal);
    fprintf(out, "\n");
    if ( m_previous ) {
        fprintf(out, "InF: ");
        VectorPrint(out, m_inDirF);
        fprintf(out, "\n");
        fprintf(out, "Cos in  %f\n", VECTORDOTPRODUCT(m_normal, m_inDirF));
        fprintf(out, "GCos in %f\n", VECTORDOTPRODUCT(m_hit.patch->normal,
                                                      m_inDirF));
    }
    if ( m_next ) {
        fprintf(out, "OutF: ");
        VectorPrint(out, m_next->m_inDirT);
        fprintf(out, "\n");
        fprintf(out, "Cos out %f\n", VECTORDOTPRODUCT(m_normal, m_next->m_inDirT));
        fprintf(out, "GCos out %f\n", VECTORDOTPRODUCT(m_hit.patch->normal,
                                                       m_next->m_inDirT));
    }
}

/******************************************************************/
/* GetPreviousBsdf
 *
 * Searches back in the path to find the bsdf on the outside
 * of the current path/material. The last node should be
 * a back hit -> Leaving ray-type. If not, the current bsdf
 * is returned and an error is reported
 */
/******************************************************************/

/* Helper routine, searches the corresponding 'Enters'
 * node for this node
 */

CPathNode *CPathNode::GetMatchingNode() {
    BSDF *thisBsdf;
    int backhits;
    CPathNode *tmpNode = Previous();
    CPathNode *matchedNode = nullptr;

    thisBsdf = m_useBsdf;
    backhits = 1;

    while ( tmpNode && backhits > 0 ) {
        switch ( tmpNode->m_rayType ) {
            case Enters:
                if ( tmpNode->m_hit.patch->surface->material->bsdf == thisBsdf ) {
                    backhits--; // Aha an entering point in this material
                }
                break;
            case Leaves:
                if ( tmpNode->m_inBsdf == thisBsdf ) {
                    backhits++; // Leaves the same material more than one time
                }
                break;
            case Reflects:
                break;
            default:
                logError("CPathNode::GetMatchingNode", "Wrong ray type in path");
        }

        matchedNode = tmpNode;
        tmpNode = tmpNode->Previous();
    }

    if ( backhits == 0 ) {
        return (matchedNode);
    } else {
        return nullptr;  // No matching node
    }
}


BSDF *CPathNode::GetPreviousBsdf() {
    CPathNode *matchedNode;

    if ( !(m_hit.flags & HIT_BACK)) {
        logError("CPathNode::GetPreviousBsdf", "Last node not a back hit");
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


void CPathNode::AssignBsdfAndNormal() {
    MATERIAL *thisMaterial;

    if ( m_hit.patch == nullptr ) {
        // Invalid node
        return;
    }

    thisMaterial = m_hit.patch->surface->material;

    VECTORCOPY(m_hit.normal, m_normal); // Possible double format

    // Assign bsdf's

    m_useBsdf = thisMaterial->bsdf;

    if ( m_hit.flags & HIT_FRONT ) {
        m_outBsdf = m_useBsdf; // in filled in when creating this node
    } else // BACK HIT
    {
        m_outBsdf = GetPreviousBsdf();
    }
}

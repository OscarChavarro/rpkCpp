/**
Class definition of pathnodes. These node are building blocks of paths.
and contain necessary information for raytracing-like algorithms
*/

#ifndef __PATH_NODE__
#define __PATH_NODE__

#include "common/Ray.h"
#include "common/color.h"
#include "material/bsdf.h"

// -- For evaluation of bi paths, should change!
#include "raycasting/common/bsdfcomp.h"


// Macro determining the heuristic used for weighting
#define MULTIPLE_IMPORTANCE_SAMPLING_FUNC(a) ((a)*(a))


// Type definitions used in CPathNode

// PATHRAYTYPE indicates what the ray does further in the path
// F.i. it can be reflected, it can enter a material, leave it
// or the path can end with this ray.

enum PATHRAYTYPE {
    Starts, Enters, Leaves, Reflects, Stops, Environment
};

// -- TODO clean up, additional functions that are now duplicated
// -- in the samplers, accessor methods, splitting in a
// -- pathnode and spearate connections


class CPathNode {
public:
    static int m_dmaxsize;


public:
    RayHit m_hit;
    Vector3D m_inDirT; // towards the patch
    Vector3D m_inDirF; // from the patch
    Vector3D m_normal;

    double m_G; // Geometric factor x(i-1) -> x(i)

    double m_pdfFromPrev;
    double m_pdfFromNext;
    double m_rrPdfFromNext; // Path length in other direction not
    //  known beforehand => separate components needed.
    double m_rracc; // Accumulated russian roulette factors

    COLOR m_bsdfEval;
    CBsdfComp m_bsdfComp;

    BSDFFLAGS m_usedComponents; // Components used for scattering in this
    // node. For full bsdf sampling, these are all components, independent
    // of which component is actually used (M.I.S.), but other types
    // of sampling are possible.
    BSDFFLAGS m_accUsedComponents; // Accumulated used components. This allows
    // one to check immediately if any previous node in the path had a certain
    // scattering component used. accUsed = prev->used | prev->accUsed

    BSDF *m_useBsdf; // bsdf used for scattering
    BSDF *m_inBsdf; // Medium of incoming ray
    BSDF *m_outBsdf;//Medium of a possible transmitted ray (other side of normal)
    PATHRAYTYPE m_rayType;
    int m_depth; // First node in a path has depth 0

protected:
    CPathNode *m_next;
    CPathNode *m_previous;

    // friend CPath;
    // methods
public:
    CPathNode();

    ~CPathNode();

    // Navigation in a path
    CPathNode *next() { return m_next; }

    CPathNode *previous() { return m_previous; }

    void setNext(CPathNode *node) { m_next = node; }

    void setPrevious(CPathNode *node) { m_previous = node; }

    void attach(CPathNode *node) {
        m_next = node;
        node->setPrevious(this);
    }

    void ensureNext() {
        if ( m_next == nullptr ) {
            attach(new CPathNode);
        }
    }

    BSDF *getPreviousBsdf(); // Searches backwards for matching node
    void assignBsdfAndNormal(); // Assigns outgoing bsdf for a filled node

    void print(FILE *out);

    bool ends() const {
        return (m_rayType == Stops) || (m_rayType == Environment);
    }

    // Delete all nodes, including the supplied 'node'
    static void ReleaseAll(CPathNode *node);

protected:
    CPathNode *GetMatchingNode();
};

#endif

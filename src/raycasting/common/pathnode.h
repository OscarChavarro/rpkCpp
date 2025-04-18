/**
Class definition of path nodes. These node are building blocks of paths.
and contain necessary information for raytracing-like algorithms
*/

#ifndef __PATH_NODE__
#define __PATH_NODE__

#include "common/Ray.h"
#include "common/ColorRgb.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"

// -- For evaluation of bi paths, should change!
#include "raycasting/common/bsdfcomp.h"
#include "raycasting/common/PathRayType.h"

/**
Heuristic for multiple Importance sampling / weighting
*/
inline double
multipleImportanceSampling(double a) {
    return a * a;
}

// Type definitions used in CPathNode

// -- TODO clean up, additional functions that are now duplicated
// -- in the samplers, accessor methods, splitting in a
// -- path node and spear rate connections

class SimpleRaytracingPathNode {
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
    double accumulatedRussianRouletteFactors;

    ColorRgb m_bsdfEval;
    BsdfComp m_bsdfComp;

    char m_usedComponents; // Components used for scattering in this
    // node. For full bsdf sampling, these are all components, independent
    // of which component is actually used (M.I.S.), but other types
    // of sampling are possible.
    char m_accUsedComponents; // Accumulated used components. This allows
    // one to check immediately if any previous node in the path had a certain
    // scattering component used. accUsed = prev->used | prev->accUsed

    PhongBidirectionalScatteringDistributionFunction *m_useBsdf; // bsdf used for scattering
    PhongBidirectionalScatteringDistributionFunction *m_inBsdf; // Medium of incoming ray
    PhongBidirectionalScatteringDistributionFunction *m_outBsdf;//Medium of a possible transmitted ray (other side of normal)
    PathRayType m_rayType;
    int m_depth; // First node in a path has depth 0

  private:
    SimpleRaytracingPathNode *m_next;
    SimpleRaytracingPathNode *m_previous;

  public:
    SimpleRaytracingPathNode();
    ~SimpleRaytracingPathNode();

    // Navigation in a path
    SimpleRaytracingPathNode *next() { return m_next; }

    SimpleRaytracingPathNode *previous() { return m_previous; }

    void setNext(SimpleRaytracingPathNode *node) { m_next = node; }

    void setPrevious(SimpleRaytracingPathNode *node) { m_previous = node; }

    void attach(SimpleRaytracingPathNode *node) {
        m_next = node;
        node->setPrevious(this);
    }

    void ensureNext() {
        if ( m_next == nullptr ) {
            attach(new SimpleRaytracingPathNode);
        }
    }

    PhongBidirectionalScatteringDistributionFunction *getPreviousBsdf(); // Searches backwards for matching node
    void assignBsdfAndNormal(); // Assigns outgoing bsdf for a filled node

    void print(FILE *out) const;

    bool ends() const {
        return (m_rayType == PathRayType::STOPS) || (m_rayType == PathRayType::ENVIRONMENT);
    }

  protected:
    SimpleRaytracingPathNode *GetMatchingNode();
};

#endif

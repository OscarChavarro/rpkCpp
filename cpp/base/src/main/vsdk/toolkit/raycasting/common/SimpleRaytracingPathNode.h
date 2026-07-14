/**
Class definition of path nodes. These node are building blocks of paths.
and contain necessary information for raytracing-like algorithms
*/

#ifndef PATH_NODE__
#define PATH_NODE__

#include "java/io/PrintStream.h"
#include "vsdk/toolkit/common/linealAlgebra/Ray.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/material/PhongBidirectionalScatteringDistributionFunction.h"
#include "vsdk/toolkit/raycasting/common/BsdfComp.h"
#include "vsdk/toolkit/raycasting/common/PathRayType.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHit.h"

// Type definitions used in CPathNode

// -- TODO clean up, additional functions that are now duplicated
// -- in the samplers, accessor methods, splitting in a
// -- path node and spear rate connections

class SimpleRaytracingPathNode {
  public:
    /**
    Heuristic for multiple Importance sampling / weighting
    */
    inline static double
    multipleImportanceSampling(double a) {
        return a * a;
    }

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

    ColorRgbMutable m_bsdfEval;
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
    PhongBidirectionalScatteringDistributionFunction *m_outBsdf; //Medium of a possible transmitted ray (other side of normal)
    PathRayType m_rayType;
    int m_depth; // First node in a path has depth 0

  private:
    SimpleRaytracingPathNode *m_next;
    SimpleRaytracingPathNode *m_previous;

  public:
    SimpleRaytracingPathNode();
    ~SimpleRaytracingPathNode();

    // Navigation in a path
    SimpleRaytracingPathNode *next() const;
    SimpleRaytracingPathNode *previous() const;
    void setNext(SimpleRaytracingPathNode *node);
    void setPrevious(SimpleRaytracingPathNode *node);
    void attach(SimpleRaytracingPathNode *node);
    void ensureNext();

    PhongBidirectionalScatteringDistributionFunction *getPreviousBsdf() const; // Searches backwards for matching node
    void assignBsdfAndNormal(); // Assigns outgoing bsdf for a filled node

    void print(java::PrintStream *out) const;

    bool ends() const;

  protected:
    SimpleRaytracingPathNode *GetMatchingNode() const;
};

inline SimpleRaytracingPathNode *
SimpleRaytracingPathNode::next() const {
    return m_next;
}

inline SimpleRaytracingPathNode *
SimpleRaytracingPathNode::previous() const {
    return m_previous;
}

inline void
SimpleRaytracingPathNode::setNext(SimpleRaytracingPathNode *node) {
    m_next = node;
}

inline void
SimpleRaytracingPathNode::setPrevious(SimpleRaytracingPathNode *node) {
    m_previous = node;
}

inline void
SimpleRaytracingPathNode::attach(SimpleRaytracingPathNode *node) {
    m_next = node;
    node->setPrevious(this);
}

inline void
SimpleRaytracingPathNode::ensureNext() {
    if ( m_next == nullptr ) {
        attach(new SimpleRaytracingPathNode);
    }
}

inline bool
SimpleRaytracingPathNode::ends() const {
    return (m_rayType == PathRayType::STOPS) || (m_rayType == PathRayType::ENVIRONMENT);
}

#endif

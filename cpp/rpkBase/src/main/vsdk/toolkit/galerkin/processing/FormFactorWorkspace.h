#ifndef __FORM_FACTOR_WORKSPACE__
#define __FORM_FACTOR_WORKSPACE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/common/statistics/ShadowStatistics.h"
#include "vsdk/toolkit/numericalAnalysis/CubatureRule.h"

class GalerkinElement;

/**
Scratch of the area to area form factor computation. Very often the source or
receiver element is the same as in the previous computation, so the cubature
rules, nodes and kernel values of the last element pair are kept here and
reused.

Workspace: mutable, owned by the solver (the serial one) or by a parallel
Jacobi worker, and passed explicitly to FormFactorStrategy.
It also tells where the shadow rays are counted (the global statistics, or the
counters of a worker, added to them afterwards) and whether the "nodes too
close" warnings are reported at once or kept, as element pairs, for a later
report in the serial order.
*/
class FormFactorWorkspace {
  private:
    const GalerkinElement *lastReceiver;
    const GalerkinElement *lastSource;
    CubatureRule *receiverCubatureRule;
    CubatureRule *sourceCubatureRule;
    Vector3Df receiverNodes[CubatureRule::MAXIMUM_NODES];
    Vector3Df sourceNodes[CubatureRule::MAXIMUM_NODES];
    double kernel[CubatureRule::MAXIMUM_NODES][CubatureRule::MAXIMUM_NODES];
    double maximumKernelValue;
    unsigned visibilityCount; // Number of rays that "pass" occluders
    ShadowStatistics *shadowStatistics; // Not owned
    java::ArrayList<const GalerkinElement *> *deferredNodesTooClose; // Not owned; nullptr to report at once

  public:
    explicit FormFactorWorkspace(ShadowStatistics *inShadowStatistics);
    FormFactorWorkspace(const FormFactorWorkspace &) = delete;
    FormFactorWorkspace &operator=(const FormFactorWorkspace &) = delete;

    inline const GalerkinElement *getLastReceiver() const;
    inline const GalerkinElement *getLastSource() const;
    inline void setLastPair(const GalerkinElement *receiver, const GalerkinElement *source);

    inline CubatureRule *getReceiverCubatureRule() const;
    inline CubatureRule *getSourceCubatureRule() const;
    inline void setReceiverCubatureRule(CubatureRule *rule);
    inline void setSourceCubatureRule(CubatureRule *rule);
    inline Vector3Df *getReceiverNodes();
    inline Vector3Df *getSourceNodes();

    inline double (*getKernel())[CubatureRule::MAXIMUM_NODES];
    inline double getMaximumKernelValue() const;
    inline void setMaximumKernelValue(double value);
    inline unsigned getVisibilityCount() const;
    inline void setVisibilityCount(unsigned value);
    inline ShadowStatistics *getShadowStatistics() const;
    inline void setDeferredNodesTooClose(java::ArrayList<const GalerkinElement *> *pairs);
    inline bool deferNodesTooClose(const GalerkinElement *receiver, const GalerkinElement *source);
};

inline const GalerkinElement *
FormFactorWorkspace::getLastReceiver() const {
    return lastReceiver;
}

inline const GalerkinElement *
FormFactorWorkspace::getLastSource() const {
    return lastSource;
}

inline void
FormFactorWorkspace::setLastPair(const GalerkinElement *receiver, const GalerkinElement *source) {
    lastReceiver = receiver;
    lastSource = source;
}

inline CubatureRule *
FormFactorWorkspace::getReceiverCubatureRule() const {
    return receiverCubatureRule;
}

inline CubatureRule *
FormFactorWorkspace::getSourceCubatureRule() const {
    return sourceCubatureRule;
}

inline void
FormFactorWorkspace::setReceiverCubatureRule(CubatureRule *rule) {
    receiverCubatureRule = rule;
}

inline void
FormFactorWorkspace::setSourceCubatureRule(CubatureRule *rule) {
    sourceCubatureRule = rule;
}

inline Vector3Df *
FormFactorWorkspace::getReceiverNodes() {
    return receiverNodes;
}

inline Vector3Df *
FormFactorWorkspace::getSourceNodes() {
    return sourceNodes;
}

inline double (*FormFactorWorkspace::getKernel())[CubatureRule::MAXIMUM_NODES] {
    return kernel;
}

inline double
FormFactorWorkspace::getMaximumKernelValue() const {
    return maximumKernelValue;
}

inline void
FormFactorWorkspace::setMaximumKernelValue(const double value) {
    maximumKernelValue = value;
}

inline unsigned
FormFactorWorkspace::getVisibilityCount() const {
    return visibilityCount;
}

inline void
FormFactorWorkspace::setVisibilityCount(const unsigned value) {
    visibilityCount = value;
}

inline ShadowStatistics *
FormFactorWorkspace::getShadowStatistics() const {
    return shadowStatistics;
}

/**
@param pairs list receiving (receiver, source) pairs, or nullptr to report the warning at once
*/
inline void
FormFactorWorkspace::setDeferredNodesTooClose(java::ArrayList<const GalerkinElement *> *pairs) {
    deferredNodesTooClose = pairs;
}

/**
@return true when the warning was kept for a later report, false when it is to be reported now
*/
inline bool
FormFactorWorkspace::deferNodesTooClose(const GalerkinElement *receiver, const GalerkinElement *source) {
    if ( deferredNodesTooClose == nullptr ) {
        return false;
    }
    deferredNodesTooClose->add(receiver);
    deferredNodesTooClose->add(source);
    return true;
}

#endif

#ifndef __FORM_FACTOR_WORKSPACE__
#define __FORM_FACTOR_WORKSPACE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/common/statistics/ShadowStatistics.h"
#include "vsdk/toolkit/numericalAnalysis/CubatureRule.h"

class GalerkinElement;

/**
Scratch of the area to area form factor computation. Very often the source or
receiver element is the same as in the previous computation, so the cubature
rules, nodes and kernel values of the last element pair are kept here and
reused.

Workspace: mutable, owned by the solver and passed explicitly to
FormFactorStrategy.
It also tells where the shadow rays are counted (the global statistics).
Serial Turbo C++ port: the cpp port's deferred "nodes too close" warnings of
parallel Jacobi workers are left out, the warning is always reported at once.
*/
class FormFactorWorkspace {
  private:
    const GalerkinElement *lastReceiver;
    const GalerkinElement *lastSource;
    CubatureRule *receiverCubatureRule;
    CubatureRule *sourceCubatureRule;
    Vector3Df receiverNodes[CUBATURE_MAXIMUM_NODES];
    Vector3Df sourceNodes[CUBATURE_MAXIMUM_NODES];
    double kernel[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES];
    double maximumKernelValue;
    unsigned visibilityCount; // Number of rays that "pass" occluders
    ShadowStatistics *shadowStatistics; // Not owned

    // Not copyable
    FormFactorWorkspace(const FormFactorWorkspace &);
    FormFactorWorkspace &operator=(const FormFactorWorkspace &);

  public:
    FormFactorWorkspace(ShadowStatistics *inShadowStatistics);

    inline const GalerkinElement *getLastReceiver() const;
    inline const GalerkinElement *getLastSource() const;
    inline void setLastPair(const GalerkinElement *receiver, const GalerkinElement *source);

    inline CubatureRule *getReceiverCubatureRule() const;
    inline CubatureRule *getSourceCubatureRule() const;
    inline void setReceiverCubatureRule(CubatureRule *rule);
    inline void setSourceCubatureRule(CubatureRule *rule);
    inline Vector3Df *getReceiverNodes();
    inline Vector3Df *getSourceNodes();

    inline double (*getKernel())[CUBATURE_MAXIMUM_NODES];
    inline double getMaximumKernelValue() const;
    inline void setMaximumKernelValue(double value);
    inline unsigned getVisibilityCount() const;
    inline void setVisibilityCount(unsigned value);
    inline ShadowStatistics *getShadowStatistics() const;
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

inline double (*FormFactorWorkspace::getKernel())[CUBATURE_MAXIMUM_NODES] {
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

#endif

#ifndef __FORM_FACTOR_WORKSPACE__
#define __FORM_FACTOR_WORKSPACE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/numericalAnalysis/CubatureRule.h"

class GalerkinElement;

/**
Scratch of the area to area form factor computation. Very often the source or
receiver element is the same as in the previous computation, so the cubature
rules, nodes and kernel values of the last element pair are kept here and
reused.

Workspace: mutable, owned by the solver (one per worker later) and passed
explicitly to FormFactorStrategy.
*/
class FormFactorWorkspace {
  private:
    const GalerkinElement *lastReceiver;
    const GalerkinElement *lastSource;
    CubatureRule *receiverCubatureRule;
    CubatureRule *sourceCubatureRule;
    Vector3D receiverNodes[CUBATURE_MAXIMUM_NODES];
    Vector3D sourceNodes[CUBATURE_MAXIMUM_NODES];
    double kernel[CUBATURE_MAXIMUM_NODES][CUBATURE_MAXIMUM_NODES];
    double maximumKernelValue;
    unsigned visibilityCount; // Number of rays that "pass" occluders

    // Not copyable
    FormFactorWorkspace(const FormFactorWorkspace &);
    FormFactorWorkspace &operator=(const FormFactorWorkspace &);

  public:
    FormFactorWorkspace();

    inline const GalerkinElement *getLastReceiver() const;
    inline const GalerkinElement *getLastSource() const;
    inline void setLastPair(const GalerkinElement *receiver, const GalerkinElement *source);

    inline CubatureRule *getReceiverCubatureRule() const;
    inline CubatureRule *getSourceCubatureRule() const;
    inline void setReceiverCubatureRule(CubatureRule *rule);
    inline void setSourceCubatureRule(CubatureRule *rule);
    inline Vector3D *getReceiverNodes();
    inline Vector3D *getSourceNodes();

    inline double (*getKernel())[CUBATURE_MAXIMUM_NODES];
    inline double getMaximumKernelValue() const;
    inline void setMaximumKernelValue(double value);
    inline unsigned getVisibilityCount() const;
    inline void setVisibilityCount(unsigned value);
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

inline Vector3D *
FormFactorWorkspace::getReceiverNodes() {
    return receiverNodes;
}

inline Vector3D *
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

#endif

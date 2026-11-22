#ifndef __POWER_ACCUMULATOR_VISITOR__
#define __POWER_ACCUMULATOR_VISITOR__

#include "vsdk/toolkit/galerkin/processing/visitors/ClusterLeafVisitor.h"

class PowerAccumulatorVisitor: public ClusterLeafVisitor{ private:
    ColorRgbMutable sourceRadiance;
    Vector3Df samplePoint;
    ColorRgbMutable accumulatedRadiance;

  public:
    explicit
    PowerAccumulatorVisitor(ColorRgbMutable inSourceRadiance, Vector3Df inSamplePoint);
    ~PowerAccumulatorVisitor();
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState);
    ColorRgbMutable getAccumulatedRadiance() const;
};

#endif

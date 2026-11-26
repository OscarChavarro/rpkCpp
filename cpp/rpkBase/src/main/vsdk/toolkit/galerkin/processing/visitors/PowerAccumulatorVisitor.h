#ifndef __POWER_ACCUMULATOR_VISITOR__
#define __POWER_ACCUMULATOR_VISITOR__

#include "vsdk/toolkit/galerkin/processing/visitors/ClusterLeafVisitor.h"

class PowerAccumulatorVisitor final : public ClusterLeafVisitor {
  private:
    double sourceRadiance[3];
    Vector3Dd samplePoint;
    double accumulatedRadiance[3];

  public:
    explicit
    PowerAccumulatorVisitor(const ColorRgb &inSourceRadiance, Vector3Dd inSamplePoint);
    ~PowerAccumulatorVisitor() final;
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState) final;
    ColorRgb getAccumulatedRadiance() const;
};

#endif

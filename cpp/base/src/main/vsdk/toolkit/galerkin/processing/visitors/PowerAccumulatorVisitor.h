#ifndef POWER_ACCUMULATOR_VISITOR__
#define POWER_ACCUMULATOR_VISITOR__

#include "vsdk/toolkit/galerkin/processing/visitors/ClusterLeafVisitor.h"

class PowerAccumulatorVisitor final : public ClusterLeafVisitor {
  private:
    ColorRgbMutable sourceRadiance;
    Vector3D samplePoint;
    ColorRgbMutable accumulatedRadiance;

  public:
    explicit
    PowerAccumulatorVisitor(ColorRgbMutable inSourceRadiance, Vector3D inSamplePoint);
    ~PowerAccumulatorVisitor() final;
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState) final;
    ColorRgbMutable getAccumulatedRadiance() const;
};

#endif

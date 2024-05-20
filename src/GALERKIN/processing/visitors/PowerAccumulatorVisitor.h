#ifndef __POWER_ACCUMULATOR_VISITOR__
#define __POWER_ACCUMULATOR_VISITOR__

#include "GALERKIN/processing/visitors/ClusterLeafVisitor.h"

class PowerAccumulatorVisitor final : public ClusterLeafVisitor {
  private:
    ColorRgb sourceRadiance;
    Vector3D samplePoint;
    ColorRgb accumulatedRadiance;

  public:
    explicit
    PowerAccumulatorVisitor(
        ColorRgb inSourceRadiance,
        Vector3D inSamplePoint);

    ~PowerAccumulatorVisitor() final;
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState) final;
    ColorRgb getAccumulatedRadiance() const;
};

#endif

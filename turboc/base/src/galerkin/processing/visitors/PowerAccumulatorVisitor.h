#ifndef __POWER_ACCUMULATOR_VISITOR__
#define __POWER_ACCUMULATOR_VISITOR__

#include "galerkin/processing/visitors/ClusterLeafVisitor.h"

class PowerAccumulatorVisitor: public ClusterLeafVisitor{ private:
    ColorRgb sourceRadiance;
    Vector3D samplePoint;
    ColorRgb accumulatedRadiance;

  public:
    explicit
    PowerAccumulatorVisitor(ColorRgb inSourceRadiance, Vector3D inSamplePoint);
    ~PowerAccumulatorVisitor();
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState);
    ColorRgb getAccumulatedRadiance() const;
};

#endif

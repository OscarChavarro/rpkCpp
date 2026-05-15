#ifndef __POWER_ACCUMULATOR_VISITOR__
#define __POWER_ACCUMULATOR_VISITOR__

#include "galerkin/processing/visitors/ClusterLeafVisitor.h"

class PowerAccumulatorVisitor: public ClusterLeafVisitor{ private:
    ColorRgbMutable sourceRadiance;
    Vector3D samplePoint;
    ColorRgbMutable accumulatedRadiance;

  public:
    explicit
    PowerAccumulatorVisitor(ColorRgbMutable inSourceRadiance, Vector3D inSamplePoint);
    ~PowerAccumulatorVisitor();
    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState);
    ColorRgbMutable getAccumulatedRadiance() const;
};

#endif

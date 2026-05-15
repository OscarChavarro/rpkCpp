#ifndef PRJCT_AREA_ACCML_VSTR
#define PRJCT_AREA_ACCML_VSTR

#include "galerkin/processing/visitors/ClusterLeafVisitor.h"

class ProjectedAreaAccumulatorVisitor: public ClusterLeafVisitor{ private:
    double totalProjectedArea;

  public:
    ProjectedAreaAccumulatorVisitor();
    ~ProjectedAreaAccumulatorVisitor();

    void visit(GalerkinElement *galerkinElement, const GalerkinState *galerkinState);
    double getTotalProjectedArea() const;
};

#endif

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/galerkin/processing/visitors/ScratchRendererVisitor.h"

ScratchRendererVisitor::ScratchRendererVisitor(Vector3Dd inEyePoint, ScratchVisibilityBuffer &inTarget):
    eyePoint(inEyePoint),
    target(inTarget)
{
}

ScratchRendererVisitor::~ScratchRendererVisitor() {
}

void
ScratchRendererVisitor::visit(
    GalerkinElement *galerkinElement,
    const GalerkinState * /*galerkinState*/)
{
    const Patch *patch = galerkinElement->patch;
    Vector3Dd v[4];

    // Backface culling test: only render the element if it is turned towards
    // the current eye point
    if ( patch->getNormal().dotProduct(eyePoint) + patch->getPlaneConstant() < VSDK::EPSILON ) {
        return;
    }

    for ( int i = 0; i < patch->getNumberOfVertices(); i++ ) {
        v[i] = *patch->getVertices()[i]->point;
    }

    target.drawPolygon(patch->getNumberOfVertices(), v, galerkinElement);
}

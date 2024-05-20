#include "GALERKIN/processing/visitors/ScratchRendererVisitor.h"

ScratchRendererVisitor::ScratchRendererVisitor(Vector3D inEyePoint) {
    eyePoint = inEyePoint;
}

ScratchRendererVisitor::~ScratchRendererVisitor() {
}

void
ScratchRendererVisitor::visit(
    GalerkinElement *galerkinElement,
    const GalerkinState * /*galerkinState*/,
    ColorRgb * /*accumulatedRadiance*/)
{
    const Patch *patch = galerkinElement->patch;
    Vector3D v[4];

    // Backface culling test: only render the element if it is turned towards
    // the current eye point
    if ( patch->normal.dotProduct(eyePoint) + patch->planeConstant < Numeric::EPSILON ) {
        return;
    }

    for ( int i = 0; i < patch->numberOfVertices; i++ ) {
        v[i] = *patch->vertex[i]->point;
    }

    // TODO: Extend SGL_CONTEXT to support Element*
    GLOBAL_sgl_currentContext->sglSetColor((SGL_PIXEL)galerkinElement);
    GLOBAL_sgl_currentContext->sglPolygon(patch->numberOfVertices, v);
}

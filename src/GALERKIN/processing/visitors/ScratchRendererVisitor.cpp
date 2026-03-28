#include "GALERKIN/processing/visitors/ScratchRendererVisitor.h"

ScratchRendererVisitor::ScratchRendererVisitor(Vector3D inEyePoint, SGL_CONTEXT *inSglContext) {
    eyePoint = inEyePoint;
    sglContext = inSglContext;
}

ScratchRendererVisitor::~ScratchRendererVisitor() {
}

void
ScratchRendererVisitor::visit(
    GalerkinElement *galerkinElement,
    const GalerkinState * /*galerkinState*/)
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

    if ( sglContext == nullptr ) {
        return;
    }

    // TODO: Extend SGL_CONTEXT to support Element*
    sglContext->sglSetColor(reinterpret_cast<SGL_PIXEL>(galerkinElement));
    sglContext->sglPolygon(patch->numberOfVertices, v);
}

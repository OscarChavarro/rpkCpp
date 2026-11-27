/**
Small Graphics Library
*/

#ifndef __SGL_CONTEXT__
#define __SGL_CONTEXT__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/elements/Element.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/render/sgl/SglConstants.h"
#include "vsdk/toolkit/render/sgl/SglPixelContent.h"

typedef unsigned long SGL_PIXEL;
typedef unsigned long SGL_Z_VALUE;

class SglContext {
  private:
    static void clearFrameBuffer(SglContext *sglContext, SGL_PIXEL backgroundColor);
    static const Matrix4x4d &identityMatrix();

  public:
    Matrix4x4d transformStack[SglConstants::SGL_TRANSFORM_STACK_SIZE]; // Transform stack
    Matrix4x4d *currentTransform;
    bool clipping; // Whether to do clipping or not
    int vp_x; // Viewport
    int vp_y;
    double near; // Depth range
    double far;

//public:
    SglPixelContent pixelData;
    SGL_PIXEL *frameBuffer;
    Patch **patchBuffer;
    Element **galerkinElementBuffer;

    SGL_PIXEL currentPixel;
    const Patch *currentPatch;
    const Element *currentGalerkinElement;

    SGL_Z_VALUE *depthBuffer; // Z buffer

    int width; // canvas size
    int height;
    int vp_width;
    int vp_height;

    explicit SglContext(int width, int height);
    ~SglContext();

    void sglClearZBuffer(SGL_Z_VALUE defZVal) const;
    void sglClear(SGL_PIXEL backgroundColor, SGL_Z_VALUE defZVal);
    void sglDepthTesting(bool on);
    void sglClipping(bool on);
    void sglLoadMatrix(const Matrix4x4d *xf) const;
    void sglMultiplyMatrix(const Matrix4x4d *xf) const;
    void sglSetPatch(const Patch *col);
    void sglSetGalerkinElement(const Element *galerkinElement);
    void sglViewport(int x, int y, int viewPortWidth, int viewPortHeight);
    void sglPolygon(int numberOfVertices, const Vector3Dd *vertices);
};

#endif

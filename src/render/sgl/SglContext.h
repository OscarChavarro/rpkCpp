/**
Small Graphics Library
*/

#ifndef __SGL__
#define __SGL__

#include "common/linealAlgebra/Matrix4x4.h"
#include "skin/Patch.h"
#include "render/sgl/SglConstants.h"
#include "render/sgl/SglPixelContent.h"

using SGL_PIXEL = unsigned long;
using SGL_Z_VALUE = unsigned long;

class Element;

class SglContext {
  private:
    static void clearFrameBuffer(SglContext *sglContext, SGL_PIXEL backgroundColor);
    static const Matrix4x4 &identityMatrix();

  public:
    Matrix4x4 transformStack[SglConstants::SGL_TRANSFORM_STACK_SIZE]; // Transform stack
    Matrix4x4 *currentTransform;
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
    void sglLoadMatrix(const Matrix4x4 *xf) const;
    void sglMultiplyMatrix(const Matrix4x4 *xf) const;
    void sglSetColor(SGL_PIXEL col);
    void sglSetPatch(const Patch *col);
    void sglSetGalerkinElement(const Element *galerkinElement);
    void sglViewport(int x, int y, int viewPortWidth, int viewPortHeight);
    void sglPolygon(int numberOfVertices, const Vector3D *vertices);
};

#endif

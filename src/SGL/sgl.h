/**
Small Graphics Library
*/

#ifndef __SGL__
#define __SGL__

#include "common/linealAlgebra/Matrix4x4.h"
#include "skin/Patch.h"
#include "SGL/SglPixelContent.h"

typedef unsigned long SGL_PIXEL;
typedef unsigned long SGL_Z_VALUE;

#define SGL_MAXIMUM_Z 4294967295U
#define SGL_TRANSFORM_STACK_SIZE 4

class SGL_CONTEXT {
public:
    Matrix4x4 transformStack[SGL_TRANSFORM_STACK_SIZE]; // Transform stack
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
    Element **elementBuffer;

    SGL_PIXEL currentPixel;
    const Patch *currentPatch;
    Element *currentElement;

    SGL_Z_VALUE *depthBuffer; // Z buffer

    int width; // canvas size
    int height;
    int vp_width;
    int vp_height;

    explicit SGL_CONTEXT(int width, int height);
    ~SGL_CONTEXT();

    void sglClearZBuffer(SGL_Z_VALUE defZVal) const;
    void sglClear(SGL_PIXEL backgroundColor, SGL_Z_VALUE defZVal);
    void sglDepthTesting(bool on);
    void sglClipping(bool on);
    void sglLoadMatrix(const Matrix4x4 *xf) const;
    void sglMultiplyMatrix(const Matrix4x4 *xf) const;
    void sglSetColor(SGL_PIXEL col);
    void sglSetPatch(const Patch *col);
    void sglViewport(int x, int y, int viewPortWidth, int viewPortHeight);
    void sglPolygon(int numberOfVertices, const Vector3D *vertices);
};

extern SGL_CONTEXT *GLOBAL_sgl_currentContext;

extern SGL_CONTEXT *sglMakeCurrent(SGL_CONTEXT *context);

#endif

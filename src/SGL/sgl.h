/**
Small Graphics Library
*/

#ifndef __SGL__
#define __SGL__

#include "common/linealAlgebra/Matrix4x4.h"
#include "skin/Patch.h"

typedef unsigned long SGL_PIXEL;
typedef unsigned long SGL_Z_VALUE;
typedef int SGL_BOOLEAN;

#define SGL_MAXIMUM_Z 4294967295U

#define SGL_TRANSFORM_STACK_SIZE 4

// TODO: Extend SGL_CONTEXT to support Element*
enum PixelContent {
    PIXEL,
    PATCH_POINTER,
    GALERKIN_ELEMENT_POINTER
};

class SGL_CONTEXT {
public:
    Matrix4x4 transformStack[SGL_TRANSFORM_STACK_SIZE]; // Transform stack
    Matrix4x4 *currentTransform;
    SGL_BOOLEAN clipping; // Whether to do clipping or not
    int vp_x; // Viewport
    int vp_y;
    double near; // Depth range
    double far;

public:
    PixelContent pixelData;
    SGL_PIXEL *frameBuffer;
    Patch **patchBuffer;
    Element **elementBuffer;

    SGL_PIXEL currentPixel;
    Patch *currentPatch;
    Element *currentElement;

    SGL_Z_VALUE *depthBuffer; // Z buffer

    int width; // canvas size
    int height;
    int vp_width;
    int vp_height;

    explicit SGL_CONTEXT(int width, int height);
    ~SGL_CONTEXT();

    void sglClearZBuffer(SGL_Z_VALUE defZVal);
    void sglClear(SGL_PIXEL backgroundColor, SGL_Z_VALUE defZVal);
    void sglDepthTesting(SGL_BOOLEAN on);
    void sglClipping(SGL_BOOLEAN on);
    void sglLoadMatrix(Matrix4x4 xf);
    void sglMultiplyMatrix(Matrix4x4 xf);
    void sglSetColor(SGL_PIXEL col);
    void sglSetPatch(Patch *col);
    void sglViewport(int x, int y, int width, int height);
    void sglPolygon(int numberOfVertices, Vector3D *vertices);
};

extern SGL_CONTEXT *GLOBAL_sgl_currentContext;

extern SGL_CONTEXT *sglMakeCurrent(SGL_CONTEXT *context);

#endif

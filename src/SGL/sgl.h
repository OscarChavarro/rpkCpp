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
    Matrix4x4 transformStack[SGL_TRANSFORM_STACK_SIZE]; // Transform stack
    Matrix4x4 *currentTransform;
    SGL_BOOLEAN clipping; // Whether to do clipping or not
    int vp_x; // Viewport
    int vp_y;
    int vp_width;
    int vp_height;
    double near; // Depth range
    double far;
};

extern SGL_CONTEXT *GLOBAL_sgl_currentContext;

extern SGL_CONTEXT *sglOpen(int width, int height);
extern void sglClose(SGL_CONTEXT *context);

extern SGL_CONTEXT *sglMakeCurrent(SGL_CONTEXT *context);

extern void sglClearZBuffer(SGL_CONTEXT *sglContext, SGL_Z_VALUE defZVal);
extern void sglClear(SGL_CONTEXT *sglContext, SGL_PIXEL backgroundColor, SGL_Z_VALUE defZVal);
extern void sglDepthTesting(SGL_CONTEXT *sglContext, SGL_BOOLEAN on);
extern void sglClipping(SGL_CONTEXT *sglContext, SGL_BOOLEAN on);
extern void sglLoadMatrix(SGL_CONTEXT *sglContext, Matrix4x4 xf);
extern void sglMultiplyMatrix(SGL_CONTEXT *sglContext, Matrix4x4 xf);
extern void sglSetColor(SGL_CONTEXT *sglContext, SGL_PIXEL col);
extern void sglSetPatch(SGL_CONTEXT *sglContext, Patch *col);
extern void sglViewport(SGL_CONTEXT *sglContext, int x, int y, int width, int height);
extern void sglPolygon(SGL_CONTEXT *sglContext, int numberOfVertices, Vector3D *vertices);

#endif

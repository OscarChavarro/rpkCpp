/* sgl.h: Small Graphics Library */

#ifndef _SGL_H_
#define _SGL_H_

#include "common/linealAlgebra/Matrix4x4.h"

typedef unsigned long SGL_PIXEL;
typedef unsigned long SGL_Z_VALUE;
typedef int SGL_BOOLEAN;

#define SGL_MAXIMUM_Z 4294967295U

#define SGL_TRANSFORM_STACK_SIZE 4

class SGL_CONTEXT {
public:
    SGL_PIXEL *frameBuffer;
    SGL_Z_VALUE *depthBuffer; // Z buffer
    int width; // canvas size
    int height;
    Matrix4x4 transformStack[SGL_TRANSFORM_STACK_SIZE]; // Transform stack
    Matrix4x4 *currentTransform;
    SGL_PIXEL currentPixel;
    SGL_BOOLEAN clipping; // Whether to do clipping or not
    int vp_x; // Viewport
    int vp_y;
    int vp_width;
    int vp_height;
    double near; // Depth range
    double far;
};

extern SGL_CONTEXT *GLOBAL_sgl_currentContext;

/* creates, destroys an SGL rendering context. sglOpen() also makes the new context
 * the current context. */
extern SGL_CONTEXT *sglOpen(int width, int height);

extern void sglClose(SGL_CONTEXT *context);

/* makes the specified context current, returns the previous current context */
extern SGL_CONTEXT *sglMakeCurrent(SGL_CONTEXT *context);

/* returns current sgl renderer */
extern void sglClearZBuffer(SGL_Z_VALUE defZVal);
extern void sglClear(SGL_PIXEL backgroundColor, SGL_Z_VALUE defZVal);
extern void sglDepthTesting(SGL_BOOLEAN on);
extern void sglClipping(SGL_BOOLEAN on);
extern void sglLoadMatrix(Matrix4x4 xf);
extern void sglMultiplyMatrix(Matrix4x4 xf);
extern void sglSetColor(SGL_PIXEL col);
extern void sglViewport(int x, int y, int width, int height);
extern void sglPolygon(int numberOfVertices, Vector3D *vertices);

#endif

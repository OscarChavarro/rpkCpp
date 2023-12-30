/* sgl.h: Small Graphics Library */

#ifndef _SGL_H_
#define _SGL_H_

#include "SGL/sgl_context.h"

/* creates, destroys an SGL rendering context. sglOpen() also makes the new context
 * the current context. */
extern SGL_CONTEXT *sglOpen(int width, int height);

extern void sglClose(SGL_CONTEXT *context);

/* makes the specified context current, returns the previous current context */
extern SGL_CONTEXT *sglMakeCurrent(SGL_CONTEXT *context);

/* returns current sgl renderer */
#define sglGetCurrent()    (current_sgl_context)

/* all the following operate on the current SGL context and behave very similar as
 * the corresponding functions in OpenGL. */
extern void sglClearFrameBuffer(SGL_PIXEL backgroundcol);

extern void sglClearZBuffer(SGL_ZVAL defzval);

extern void sglClear(SGL_PIXEL backgroundcol, SGL_ZVAL defzval);

extern void sglDepthTesting(SGL_BOOLEAN on);

extern void sglClipping(SGL_BOOLEAN on);

extern void sglLoadMatrix(Matrix4x4 xf);

extern void sglMultMatrix(Matrix4x4 xf);

extern void sglSetColor(SGL_PIXEL col);

extern void sglViewport(int x, int y, int width, int height);

extern void sglPolygon(int nrverts, Vector3D *verts);

#endif

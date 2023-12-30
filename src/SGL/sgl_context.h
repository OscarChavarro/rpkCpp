/* sgl_context.h: SGL context type declaration */

#ifndef _SGL_CONTEXT_H_
#define _SGL_CONTEXT_H_

#include "common/linealAlgebra/Matrix4x4.h"

typedef unsigned long SGL_PIXEL;
typedef unsigned long SGL_ZVAL;
typedef int SGL_BOOLEAN;

/* minimum and maximum Z values possible */
#define SGL_ZMAX 4294967295U

#define SGL_TRANSFORM_STACK_SIZE 4

class SGL_CONTEXT {
  public:
    SGL_PIXEL *fbuf;        /* framebuffer pointer */
    SGL_ZVAL *zbuf;        /* Z buffer */
    int width, height;        /* canvas size. */
    Matrix4x4 transform_stack[SGL_TRANSFORM_STACK_SIZE];    /* transform stack */
    Matrix4x4 *curtrans;        /* current transform */
    SGL_PIXEL curpixel;        /* current pixel value */
    SGL_BOOLEAN clipping;        /* whether to do clipping or not */
    int vp_x, vp_y, vp_width, vp_height; /* viewport */
    double near, far;        /* depth range */
};

extern SGL_CONTEXT *current_sgl_context;    /* current context */

#endif

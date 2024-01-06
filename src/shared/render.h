#ifndef _RENDER_H_
#define _RENDER_H_

#include <cstdio>

#include "skin/bounds.h"
#include "common/linealAlgebra/Vector4D.h"
#include "material/color.h"
#include "shared/camera.h"
#include "skin/Patch.h"

/* creates an offscreen window for rendering */
extern void RenderCreateOffscreenWindow(int hres, int vres);

/* returns FALSE until rendering is fully initialized. Do not attempt to
 * render something until this function returns TRUE. */
extern int RenderInitialized();

/* renders the whole scene */
extern void RenderScene();

/* Patch ID rendering. Returns an array of size (*x)*(*y) containing the IDs of
 * the patches visible through each pixel or 0 if the background is visible through 
 * the pixel. x is normally the width and y the height of the canvas window. */
extern unsigned long *RenderIds(long *x, long *y);

/* Renders an image of m lines of n pixels at column x on row y (= lower
 * left corner of image, relative to the lower left corner of the window) */
extern void RenderPixels(int x, int y, int n, int m, RGB *rgb);

/* sets the current color for line or outline drawing */
extern void RenderSetColor(RGB *rgb);

/* renders a convex polygon flat shaded in the current color */
extern void RenderPolygonFlat(int nrverts, Vector3D *verts);

/* renders a convex polygon with Gouraud shading */
extern void RenderPolygonGouraud(int nrverts, Vector3D *verts, RGB *vertcols);

/* renders the outline of the given patch in the current color */
extern void RenderPatchOutline(PATCH *patch);

/* renders the all the patches using default colors */
extern void RenderPatch(PATCH *patch);

/* renders a line from point p to point q, for eg debugging */
extern void RenderLine(Vector3D *p, Vector3D *q);

/* renders an anti aliased line from p to q. If
   anti-aliasing is not supported, a normal line is drawn */
extern void RenderAALine(Vector3D *p, Vector3D *q);

/* Start a strip */
extern void RenderBeginTriangleStrip();

/* Supply the next point (one at a time) */
extern void RenderNextTrianglePoint(Vector3D *point, RGB *col);

/* End a strip */
extern void RenderEndTriangleStrip();

/* renders a bounding box. */
extern void RenderBounds(BOUNDINGBOX bounds);

/* renders the bounding boxes of all objects in the scene */
extern void RenderBoundingBoxHierarchy();

/* renders the cluster hierarchy bounding boxes */
extern void RenderClusterHierarchy();

/* saves a RGB image in the front buffer */
extern void SaveScreen(char *filename, FILE *fp, int ispipe);

/* renders alternate camera, virtual screen etc ... for didactical pictures etc.. */
extern void RenderCameras();

/* rerenders last raytraced image if any, Returns TRUE if there is one,
 * and FALSE if not. */
extern int RenderRayTraced();

/* sets line width for outlines etc... */
extern void RenderSetLineWidth(float width);

/* traverses the patches in the scene in such a way to obtain
 * hierarchical view frustum culling + sorted (large patches first +
 * near to far) rendering. For every patch that is not culled,
 * render_patch is called. */
extern void RenderWorldOctree(void (*render_patch)(PATCH *));

/* display background, no Z-buffer, fill whole screen */
extern void RenderBackground(CAMERA *cam);

/* rendering options */
class RENDEROPTIONS {
  public:
    RGB outline_color,      /* color in which to draw outlines */
    bounding_box_color, /* color in which to draw bounding boxes */
    cluster_color,      /* color in which to show cluster bounding boxes */
    camera_color;      /* color for drawing alternate cameras */
    float camsize,      /* determines how large alternate cameras are drawn */
    linewidth;      /* linewidth */
    char draw_outlines,    /* True for drawing facet outlines */
    no_shading,       /* False for using any kind of shading */
    smooth_shading,      /* True for rendering with Gouraud interpolation */
    backface_culling,    /* true for backface culling */
    draw_bounding_boxes,    /* true for showing bounding boxes */
    draw_clusters,        /* true for showing cluster
                                         * hierarchy */
    use_display_lists,    /* true for using display
                                         * lists for faster display */
    frustum_culling,        /* frustum culling accelerates
					 * rendering of large scenes. */
    draw_cameras,        /* true for drawing alternate
                                         * viewpoints */
    render_raytraced_image, /* for freezing raytraced image on
		                         * the screen when approriate */
    trace,            /* jp: high-dynamic range
                                         * raytraced tiff */
    use_background;         /* use background image when rendering */
};

extern RENDEROPTIONS renderopts;

/* switches backface culling ... on when the argument is nonzero and off
 * if the argument is zero */
extern void RenderSetBackfaceCulling(char truefalse);

extern void RenderSetSmoothShading(char truefalse);

extern void RenderSetOutlineDrawing(char truefalse);

extern void RenderSetBoundingBoxDrawing(char truefalse);

extern void RenderSetClusterDrawing(char truefalse);

extern void RenderUseDisplayLists(char truefalse);

extern void RenderUseFrustumCulling(char truefalse);

extern void RenderSetNoShading(char truefalse);

/* color for drawing outlines ... */
extern void RenderSetOutlineColor(RGB *outline_color);

extern void RenderSetBoundingBoxColor(RGB *outline_color);

extern void RenderSetClusterColor(RGB *outline_color);

/* computes front- and backclipping plane distance for the current GLOBAL_scene_world and
 * GLOBAL_camera_mainCamera */
extern void RenderGetNearFar(float *near, float *far);

/* indicates that the scene has modified, so a new display list should be
 * compiled and rendered from now on. Only relevant when using display lists. */
extern void RenderNewDisplayList();

extern void ParseRenderingOptions(int *argc, char **argv);

#endif

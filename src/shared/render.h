#ifndef __RENDER__
#define __RENDER__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "shared/Camera.h"

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

extern RENDEROPTIONS GLOBAL_render_renderOptions;

extern void openGlRenderLine(Vector3D *x, Vector3D *y);
extern void openGlRenderSetColor(RGB *rgb);
extern void openGlRenderWorldOctree(void (*render_patch)(Patch *));
extern void openGlRenderScene(java::ArrayList<Patch *> *scenePatches);
extern void openGlMesaRenderCreateOffscreenWindow(int width, int height);
extern void openGlRenderBeginTriangleStrip();
extern void openGlRenderNextTrianglePoint(Vector3D *point, RGB *col);
extern void openGlRenderEndTriangleStrip();
extern void openGlRenderPatchOutline(Patch *patch);
extern void openGlRenderPolygonFlat(int nrverts, Vector3D *verts);
extern void openGlRenderPolygonGouraud(int nrverts, Vector3D *verts, RGB *vertcols);
extern void openGlRenderPixels(int x, int y, int width, int height, RGB *rgb);
extern void openGlRenderPatch(Patch *patch);
extern void openGlRenderNewDisplayList();

extern unsigned long *sglRenderIds(long *x, long *y, java::ArrayList<Patch *> *scenePatches);

extern void renderBounds(BOUNDINGBOX bounds);
extern void renderBoundingBoxHierarchy();
extern void renderClusterHierarchy();
extern void renderSetBackfaceCulling(char truefalse);
extern void renderSetSmoothShading(char truefalse);
extern void renderSetOutlineDrawing(char truefalse);
extern void renderSetBoundingBoxDrawing(char truefalse);
extern void renderSetClusterDrawing(char truefalse);
extern void renderUseDisplayLists(char truefalse);
extern void renderUseFrustumCulling(char truefalse);
extern void renderSetNoShading(char truefalse);
extern void renderSetOutlineColor(RGB *outline_color);
extern void renderSetBoundingBoxColor(RGB *outline_color);
extern void renderSetClusterColor(RGB *cluster_color);
extern void renderGetNearFar(float *near, float *far);
extern void parseRenderingOptions(int *argc, char **argv);

#endif

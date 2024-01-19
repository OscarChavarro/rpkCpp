#ifndef __RENDER__
#define __RENDER__

#include "skin/Patch.h"
#include "shared/camera.h"

extern unsigned long *renderIds(long *x, long *y);
extern void renderWorldOctree(void (*render_patch)(Patch *));
extern void renderScene();
extern void renderCreateOffscreenWindow(int hres, int vres);
extern void renderPatch(Patch *patch);
extern void renderSetColor(RGB *rgb);
extern void renderPixels(int x, int y, int width, int height, RGB *rgb);
extern void renderPatchOutline(Patch *patch);
extern void renderBeginTriangleStrip();
extern int renderInitialized();
extern void renderPolygonFlat(int nrverts, Vector3D *verts);
extern void renderPolygonGouraud(int nrverts, Vector3D *verts, RGB *vertcols);
extern void renderLine(Vector3D *x, Vector3D *y);
extern void renderNextTrianglePoint(Vector3D *point, RGB *col);
extern void renderEndTriangleStrip();
extern void saveScreen(char *fileName, FILE *fp, int isPipe);
extern void renderCameras();
extern void renderSetLineWidth(float width);
extern void renderBackground(CAMERA *camera);

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

extern void renderBounds(BOUNDINGBOX bounds);
extern void renderBoundingBoxHierarchy();
extern void renderClusterHierarchy();
extern int renderRayTraced();
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
extern void renderNewDisplayList();
extern void parseRenderingOptions(int *argc, char **argv);

#endif

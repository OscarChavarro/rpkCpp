/* rendercommon.c: rendering stuff independent of the graphics library being used. */

#include "common/mymath.h"
#include "shared/render.h"
#include "common/linealAlgebra/vectorMacros.h"
#include "scene/scene.h"
#include "shared/camera.h"
#include "skin/Geometry.h"
#include "shared/options.h"
#include "raycasting/common/Raytracer.h"

RENDEROPTIONS renderopts;

void RenderSetSmoothShading(char yesno) {
    renderopts.smooth_shading = yesno;
}

void RenderSetNoShading(char yesno) {
    renderopts.no_shading = yesno;
}

void RenderSetBackfaceCulling(char backface_culling) {
    renderopts.backface_culling = backface_culling;
}

void RenderSetOutlineDrawing(char draw_outlines) {
    renderopts.draw_outlines = draw_outlines;
}

void RenderSetBoundingBoxDrawing(char yesno) {
    renderopts.draw_bounding_boxes = yesno;
}

void RenderSetClusterDrawing(char yesno) {
    renderopts.draw_clusters = yesno;
}

void RenderUseDisplayLists(char truefalse) {
    renderopts.use_display_lists = truefalse;
}

void RenderUseFrustumCulling(char truefalse) {
    renderopts.frustum_culling = truefalse;
}

void RenderSetOutlineColor(RGB *outline_color) {
    renderopts.outline_color = *outline_color;
}

void RenderSetBoundingBoxColor(RGB *outline_color) {
    renderopts.bounding_box_color = *outline_color;
}

void RenderSetClusterColor(RGB *cluster_color) {
    renderopts.cluster_color = *cluster_color;
}

static void DisplayListsOption(void *value) {
    renderopts.use_display_lists = true;
}

static void FlatOption(void *value) {
    renderopts.smooth_shading = false;
}

static void NoCullingOption(void *value) {
    renderopts.backface_culling = false;
}

static void OutlinesOption(void *value) {
    renderopts.draw_outlines = true;
}

static void TraceOption(void *value) {
    renderopts.trace = true;
}

static CMDLINEOPTDESC renderingOptions[] = {
        {"-display-lists", 10, TYPELESS,
                nullptr, DisplayListsOption,
                "-display-lists\t\t"
                ": use display lists for faster hardware-assisted rendering"},
        {"-flat-shading",  5,  TYPELESS,
                nullptr, FlatOption,
                "-flat-shading\t\t: render without Gouraud (color) interpolation"},
        {"-raycast",       5,  TYPELESS,
                nullptr, TraceOption,
                "-raycast\t\t: save raycasted scene view as a high dynamic range image"},
        {"-no-culling",    5,  TYPELESS,
                nullptr, NoCullingOption,
                "-no-culling\t\t: don't use backface culling"},
        {"-outlines",      5,  TYPELESS,
                nullptr, OutlinesOption,
                "-outlines\t\t: draw polygon outlines"},
        {"-outline-color", 10, TRGB,
                &renderopts.outline_color, DEFAULT_ACTION,
                "-outline-color <rgb> \t: color for polygon outlines"},
        {nullptr,             0,  TYPELESS,
                nullptr,                      DEFAULT_ACTION,
                nullptr}
};

void ParseRenderingOptions(int *argc, char **argv) {
    parseOptions(renderingOptions, argc, argv);
}

extern CAMERA GLOBAL_camera_alternateCamera;

/* computes distance to front- and backclipping plane */
void RenderGetNearFar(float *near, float *far) {
    BOUNDINGBOX bounds;
    Vector3D b[2], d;
    int i, j, k;
    float z;

    if ( !GLOBAL_scene_world ) {
        *far = 10.;
        *near = 0.1;    /* zomaar iets */
        return;
    }
    geometryListBounds(GLOBAL_scene_world, bounds);

    VECTORSET(b[0], bounds[MIN_X], bounds[MIN_Y], bounds[MIN_Z]);
    VECTORSET(b[1], bounds[MAX_X], bounds[MAX_Y], bounds[MAX_Z]);

    *far = -HUGE;
    *near = HUGE;
    for ( i = 0; i <= 1; i++ ) {
        for ( j = 0; j <= 1; j++ ) {
            for ( k = 0; k <= 1; k++ ) {
                VECTORSET(d, b[i].x, b[j].y, b[k].z);
                VECTORSUBTRACT(d, GLOBAL_camera_mainCamera.eyep, d);
                z = VECTORDOTPRODUCT(d, GLOBAL_camera_mainCamera.Z);

                if ( z > *far ) {
                    *far = z;
                }
                if ( z < *near ) {
                    *near = z;
                }
            }
        }
    }

    if ( renderopts.draw_cameras ) {
        CAMERA *cam = &GLOBAL_camera_alternateCamera;
        float camlen = renderopts.camsize,
                hsiz = camlen * cam->viewdist * cam->tanhfov,
                vsiz = camlen * cam->viewdist * cam->tanvfov;
        Vector3D c, P[5];

        VECTORCOMB2(1., cam->eyep, camlen * cam->viewdist, cam->Z, c);
        VECTORCOMB3(c, hsiz, cam->X, vsiz, cam->Y, P[0]);
        VECTORCOMB3(c, hsiz, cam->X, -vsiz, cam->Y, P[1]);
        VECTORCOMB3(c, -hsiz, cam->X, -vsiz, cam->Y, P[2]);
        VECTORCOMB3(c, -hsiz, cam->X, vsiz, cam->Y, P[3]);
        P[4] = cam->eyep;

        for ( i = 0; i < 5; i++ ) {
            VECTORSUBTRACT(P[i], GLOBAL_camera_mainCamera.eyep, d);
            z = VECTORDOTPRODUCT(d, GLOBAL_camera_mainCamera.Z);
            if ( z > *far ) {
                *far = z;
            }
            if ( z < *near ) {
                *near = z;
            }
        }
    }

    /* take 2% extra distance for near as well as far clipping plane */
    *far += 0.02 * (*far);
    *near -= 0.02 * (*near);
    if ( *far < EPSILON ) {
        *far = GLOBAL_camera_mainCamera.viewdist;
    }
    if ( *near < EPSILON ) {
        *near = GLOBAL_camera_mainCamera.viewdist / 100.;
    }
}

void RenderBounds(BOUNDINGBOX bounds) {
    Vector3D p[8];

    VECTORSET(p[0], bounds[MIN_X], bounds[MIN_Y], bounds[MIN_Z]);
    VECTORSET(p[1], bounds[MAX_X], bounds[MIN_Y], bounds[MIN_Z]);
    VECTORSET(p[2], bounds[MIN_X], bounds[MAX_Y], bounds[MIN_Z]);
    VECTORSET(p[3], bounds[MAX_X], bounds[MAX_Y], bounds[MIN_Z]);
    VECTORSET(p[4], bounds[MIN_X], bounds[MIN_Y], bounds[MAX_Z]);
    VECTORSET(p[5], bounds[MAX_X], bounds[MIN_Y], bounds[MAX_Z]);
    VECTORSET(p[6], bounds[MIN_X], bounds[MAX_Y], bounds[MAX_Z]);
    VECTORSET(p[7], bounds[MAX_X], bounds[MAX_Y], bounds[MAX_Z]);

    renderLine(&p[0], &p[1]);
    renderLine(&p[1], &p[3]);
    renderLine(&p[3], &p[2]);
    renderLine(&p[2], &p[0]);
    renderLine(&p[4], &p[5]);
    renderLine(&p[5], &p[7]);
    renderLine(&p[7], &p[6]);
    renderLine(&p[6], &p[4]);
    renderLine(&p[0], &p[4]);
    renderLine(&p[1], &p[5]);
    renderLine(&p[2], &p[6]);
    renderLine(&p[3], &p[7]);
}

void RenderGeomBounds(Geometry *geom) {
    float *geombounds = geomBounds(geom);

    if ( geom->bounded && geombounds ) {
        RenderBounds(geombounds);
    }

    if ( geomIsAggregate(geom)) {
        GeomListIterate(geomPrimList(geom), RenderGeomBounds);
    }
}

void RenderBoundingBoxHierarchy() {
    renderSetColor(&renderopts.bounding_box_color);
    GeomListIterate(GLOBAL_scene_world, RenderGeomBounds);
}

void RenderClusterHierarchy() {
    renderSetColor(&renderopts.cluster_color);
    GeomListIterate(GLOBAL_scene_clusteredWorld, RenderGeomBounds);
}

int RenderRayTraced() {
    if ( !Global_Raytracer_activeRaytracer || !Global_Raytracer_activeRaytracer->Redisplay ) {
        return false;
    } else {
        return Global_Raytracer_activeRaytracer->Redisplay();
    }
}

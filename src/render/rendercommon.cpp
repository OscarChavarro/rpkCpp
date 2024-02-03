/**
Rendering stuff independent of the graphics library being used
*/

#include "common/mymath.h"
#include "common/linealAlgebra/vectorMacros.h"
#include "scene/scene.h"
#include "scene/Camera.h"
#include "skin/Geometry.h"
#include "common/options.h"
#include "render/opengl.h"
#include "render/render.h"

void
renderSetSmoothShading(char yesno) {
    GLOBAL_render_renderOptions.smoothShading = yesno;
}

void
renderSetNoShading(char yesno) {
    GLOBAL_render_renderOptions.noShading = yesno;
}

void
renderSetBackfaceCulling(char backface_culling) {
    GLOBAL_render_renderOptions.backfaceCulling = backface_culling;
}

void
renderSetOutlineDrawing(char draw_outlines) {
    GLOBAL_render_renderOptions.drawOutlines = draw_outlines;
}

void
renderSetBoundingBoxDrawing(char yesno) {
    GLOBAL_render_renderOptions.drawBoundingBoxes = yesno;
}

void
renderSetClusterDrawing(char yesno) {
    GLOBAL_render_renderOptions.drawClusters = yesno;
}

void
renderUseDisplayLists(char truefalse) {
    GLOBAL_render_renderOptions.useDisplayLists = truefalse;
}

void
renderUseFrustumCulling(char truefalse) {
    GLOBAL_render_renderOptions.frustumCulling = truefalse;
}

void
renderSetOutlineColor(RGB *outline_color) {
    GLOBAL_render_renderOptions.outline_color = *outline_color;
}

void
renderSetBoundingBoxColor(RGB *outline_color) {
    GLOBAL_render_renderOptions.bounding_box_color = *outline_color;
}

void
renderSetClusterColor(RGB *cluster_color) {
    GLOBAL_render_renderOptions.cluster_color = *cluster_color;
}

static void
displayListsOption(void *value) {
    GLOBAL_render_renderOptions.useDisplayLists = true;
}

static void
flatOption(void *value) {
    GLOBAL_render_renderOptions.smoothShading = false;
}

static void
noCullingOption(void *value) {
    GLOBAL_render_renderOptions.backfaceCulling = false;
}

static void
outlinesOption(void *value) {
    GLOBAL_render_renderOptions.drawOutlines = true;
}

static void
traceOption(void *value) {
    GLOBAL_render_renderOptions.trace = true;
}

static CommandLineOptionDescription renderingOptions[] = {
        {"-display-lists", 10, TYPELESS,
                nullptr, displayListsOption,
                "-display-lists\t\t"
                ": use display lists for faster hardware-assisted rendering"},
        {"-flat-shading",  5,  TYPELESS,
                nullptr, flatOption,
                "-flat-shading\t\t: render without Gouraud (color) interpolation"},
        {"-raycast",       5,  TYPELESS,
                nullptr, traceOption,
                "-raycast\t\t: save raycasted scene view as a high dynamic range image"},
        {"-no-culling",    5,  TYPELESS,
                nullptr, noCullingOption,
                "-no-culling\t\t: don't use backface culling"},
        {"-outlines",      5,  TYPELESS,
                nullptr, outlinesOption,
                "-outlines\t\t: draw polygon outlines"},
        {"-outline-color", 10, TRGB,
                &GLOBAL_render_renderOptions.outline_color, DEFAULT_ACTION,
                "-outline-color <rgb> \t: color for polygon outlines"},
        {nullptr,             0,  TYPELESS,
                nullptr,                      DEFAULT_ACTION,
                nullptr}
};

void
renderParseOptions(int *argc, char **argv) {
    parseOptions(renderingOptions, argc, argv);
}

/**
Computes front- and back-clipping plane distance for the current GLOBAL_scene_world and
GLOBAL_camera_mainCamera
*/
void
renderGetNearFar(float *near, float *far) {
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
                VECTORSUBTRACT(d, GLOBAL_camera_mainCamera.eyePosition, d);
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

    if ( GLOBAL_render_renderOptions.drawCameras ) {
        Camera *cam = &GLOBAL_camera_alternateCamera;
        float camlen = GLOBAL_render_renderOptions.cameraSize,
                hsiz = camlen * cam->viewDistance * cam->pixelWidthTangent,
                vsiz = camlen * cam->viewDistance * cam->pixelHeightTangent;
        Vector3D c, P[5];

        VECTORCOMB2(1., cam->eyePosition, camlen * cam->viewDistance, cam->Z, c);
        VECTORCOMB3(c, hsiz, cam->X, vsiz, cam->Y, P[0]);
        VECTORCOMB3(c, hsiz, cam->X, -vsiz, cam->Y, P[1]);
        VECTORCOMB3(c, -hsiz, cam->X, -vsiz, cam->Y, P[2]);
        VECTORCOMB3(c, -hsiz, cam->X, vsiz, cam->Y, P[3]);
        P[4] = cam->eyePosition;

        for ( i = 0; i < 5; i++ ) {
            VECTORSUBTRACT(P[i], GLOBAL_camera_mainCamera.eyePosition, d);
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
        *far = GLOBAL_camera_mainCamera.viewDistance;
    }
    if ( *near < EPSILON ) {
        *near = GLOBAL_camera_mainCamera.viewDistance / 100.;
    }
}

/**
Renders a bounding box
*/
void
renderBounds(BOUNDINGBOX bounds) {
    Vector3D p[8];

    VECTORSET(p[0], bounds[MIN_X], bounds[MIN_Y], bounds[MIN_Z]);
    VECTORSET(p[1], bounds[MAX_X], bounds[MIN_Y], bounds[MIN_Z]);
    VECTORSET(p[2], bounds[MIN_X], bounds[MAX_Y], bounds[MIN_Z]);
    VECTORSET(p[3], bounds[MAX_X], bounds[MAX_Y], bounds[MIN_Z]);
    VECTORSET(p[4], bounds[MIN_X], bounds[MIN_Y], bounds[MAX_Z]);
    VECTORSET(p[5], bounds[MAX_X], bounds[MIN_Y], bounds[MAX_Z]);
    VECTORSET(p[6], bounds[MIN_X], bounds[MAX_Y], bounds[MAX_Z]);
    VECTORSET(p[7], bounds[MAX_X], bounds[MAX_Y], bounds[MAX_Z]);

    openGlRenderLine(&p[0], &p[1]);
    openGlRenderLine(&p[1], &p[3]);
    openGlRenderLine(&p[3], &p[2]);
    openGlRenderLine(&p[2], &p[0]);
    openGlRenderLine(&p[4], &p[5]);
    openGlRenderLine(&p[5], &p[7]);
    openGlRenderLine(&p[7], &p[6]);
    openGlRenderLine(&p[6], &p[4]);
    openGlRenderLine(&p[0], &p[4]);
    openGlRenderLine(&p[1], &p[5]);
    openGlRenderLine(&p[2], &p[6]);
    openGlRenderLine(&p[3], &p[7]);
}

void
renderGeomBounds(Geometry *geom) {
    float *geombounds = geomBounds(geom);

    if ( geom->bounded && geombounds ) {
        renderBounds(geombounds);
    }

    if ( geomIsAggregate(geom) ) {
        for ( GeometryListNode *window = geomPrimList(geom); window != nullptr; window = window->next ) {
            renderGeomBounds(window->geometry);
        }
    }
}

/**
Renders the bounding boxes of all objects in the scene
*/
void
renderBoundingBoxHierarchy() {
    openGlRenderSetColor(&GLOBAL_render_renderOptions.bounding_box_color);
    for ( GeometryListNode *window = GLOBAL_scene_world; window != nullptr; window = window->next ) {
        renderGeomBounds(window->geometry);
    }
}

/**
Renders the cluster hierarchy bounding boxes
*/
void
renderClusterHierarchy() {
    openGlRenderSetColor(&GLOBAL_render_renderOptions.cluster_color);
    for ( GeometryListNode *window = GLOBAL_scene_clusteredWorld; window != nullptr; window = window->next ) {
        renderGeomBounds(window->geometry);
    }
}

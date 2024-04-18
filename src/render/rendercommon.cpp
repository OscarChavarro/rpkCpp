/**
Rendering stuff independent of the graphics library being used
*/

#include "common/mymath.h"
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
renderSetOutlineColor(ColorRgb *outline_color) {
    GLOBAL_render_renderOptions.outline_color = *outline_color;
}

void
renderSetBoundingBoxColor(ColorRgb *outline_color) {
    GLOBAL_render_renderOptions.bounding_box_color = *outline_color;
}

void
renderSetClusterColor(ColorRgb *cluster_color) {
    GLOBAL_render_renderOptions.cluster_color = *cluster_color;
}

static void
displayListsOption(void * /*value*/) {
    GLOBAL_render_renderOptions.useDisplayLists = true;
}

static void
flatOption(void * /*value*/) {
    GLOBAL_render_renderOptions.smoothShading = false;
}

static void
noCullingOption(void * /*value*/) {
    GLOBAL_render_renderOptions.backfaceCulling = false;
}

static void
outlinesOption(void * /*value*/) {
    GLOBAL_render_renderOptions.drawOutlines = true;
}

static void
traceOption(void * /*value*/) {
    GLOBAL_render_renderOptions.trace = true;
}

static CommandLineOptionDescription renderingOptions[] = {
    {"-display-lists", 10, TYPELESS, nullptr, displayListsOption,
     "-display-lists\t\t"
     ": use display lists for faster hardware-assisted rendering"},
    {"-flat-shading", 5, TYPELESS, nullptr, flatOption,
     "-flat-shading\t\t: render without Gouraud (color) interpolation"},
    {"-raycast", 5, TYPELESS, nullptr, traceOption,
     "-raycast\t\t: save raycasted scene view as a high dynamic range image"},
    {"-no-culling", 5, TYPELESS, nullptr, noCullingOption,
     "-no-culling\t\t: don't use backface culling"},
    {"-outlines", 5, TYPELESS, nullptr, outlinesOption,
     "-outlines\t\t: draw polygon outlines"},
    {"-outline-color", 10, TRGB, &GLOBAL_render_renderOptions.outline_color, DEFAULT_ACTION,
     "-outline-color <rgb> \t: color for polygon outlines"},
    {nullptr, 0, nullptr, nullptr, nullptr, nullptr}
};

void
renderParseOptions(int *argc, char **argv) {
    parseGeneralOptions(renderingOptions, argc, argv);
}

/**
Computes front- and back-clipping plane distance for the current GLOBAL_scene_world and
GLOBAL_camera_mainCamera
*/
void
renderGetNearFar(float *near, float *far, java::ArrayList<Geometry *> *sceneGeometries) {
    BoundingBox bounds;
    Vector3D b[2], d;
    int i, j, k;
    float z;

    if ( sceneGeometries == nullptr || sceneGeometries->size() == 0 ) {
        *far = 10.0;
        *near = 0.1;
        return;
    }

    geometryListBounds(sceneGeometries, &bounds);

    b[0].set(bounds.coordinates[MIN_X], bounds.coordinates[MIN_Y], bounds.coordinates[MIN_Z]);
    b[1].set(bounds.coordinates[MAX_X], bounds.coordinates[MAX_Y], bounds.coordinates[MAX_Z]);

    *far = -HUGE;
    *near = HUGE;
    for ( i = 0; i <= 1; i++ ) {
        for ( j = 0; j <= 1; j++ ) {
            for ( k = 0; k <= 1; k++ ) {
                d.set(b[i].x, b[j].y, b[k].z);
                vectorSubtract(d, GLOBAL_camera_mainCamera.eyePosition, d);
                z = vectorDotProduct(d, GLOBAL_camera_mainCamera.Z);

                if ( z > *far ) {
                    *far = z;
                }
                if ( z < *near ) {
                    *near = z;
                }
            }
        }
    }

    // Take 2% extra distance for near as well as far clipping plane
    *far += 0.02f * (*far);
    *near -= 0.02f * (*near);
    if ( *far < EPSILON ) {
        *far = GLOBAL_camera_mainCamera.viewDistance;
    }
    if ( *near < EPSILON ) {
        *near = GLOBAL_camera_mainCamera.viewDistance / 100.0f;
    }
}

/**
Renders a bounding box
*/
void
renderBounds(BoundingBox bounds) {
    Vector3D p[8];

    p[0].set(bounds.coordinates[MIN_X], bounds.coordinates[MIN_Y], bounds.coordinates[MIN_Z]);
    p[1].set(bounds.coordinates[MAX_X], bounds.coordinates[MIN_Y], bounds.coordinates[MIN_Z]);
    p[2].set(bounds.coordinates[MIN_X], bounds.coordinates[MAX_Y], bounds.coordinates[MIN_Z]);
    p[3].set(bounds.coordinates[MAX_X], bounds.coordinates[MAX_Y], bounds.coordinates[MIN_Z]);
    p[4].set(bounds.coordinates[MIN_X], bounds.coordinates[MIN_Y], bounds.coordinates[MAX_Z]);
    p[5].set(bounds.coordinates[MAX_X], bounds.coordinates[MIN_Y], bounds.coordinates[MAX_Z]);
    p[6].set(bounds.coordinates[MIN_X], bounds.coordinates[MAX_Y], bounds.coordinates[MAX_Z]);
    p[7].set(bounds.coordinates[MAX_X], bounds.coordinates[MAX_Y], bounds.coordinates[MAX_Z]);

    openGlRenderLine(&GLOBAL_camera_mainCamera, &p[0], &p[1]);
    openGlRenderLine(&GLOBAL_camera_mainCamera, &p[1], &p[3]);
    openGlRenderLine(&GLOBAL_camera_mainCamera, &p[3], &p[2]);
    openGlRenderLine(&GLOBAL_camera_mainCamera, &p[2], &p[0]);
    openGlRenderLine(&GLOBAL_camera_mainCamera, &p[4], &p[5]);
    openGlRenderLine(&GLOBAL_camera_mainCamera, &p[5], &p[7]);
    openGlRenderLine(&GLOBAL_camera_mainCamera, &p[7], &p[6]);
    openGlRenderLine(&GLOBAL_camera_mainCamera, &p[6], &p[4]);
    openGlRenderLine(&GLOBAL_camera_mainCamera, &p[0], &p[4]);
    openGlRenderLine(&GLOBAL_camera_mainCamera, &p[1], &p[5]);
    openGlRenderLine(&GLOBAL_camera_mainCamera, &p[2], &p[6]);
    openGlRenderLine(&GLOBAL_camera_mainCamera, &p[3], &p[7]);
}

void
renderGeomBounds(Geometry *geometry) {
    BoundingBox geometryBoundingBox = getBoundingBox(geometry);

    if ( geometry->bounded ) {
        renderBounds(geometryBoundingBox);
    }

    if ( geometry->isCompound() ) {
        java::ArrayList<Geometry *> *geometryList = geomPrimListCopy(geometry);
        for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
            renderGeomBounds(geometryList->get(i));
        }
        delete geometryList;
    }
}

/**
Renders the bounding boxes of all objects in the scene
*/
void
renderBoundingBoxHierarchy(java::ArrayList<Geometry *> *sceneGeometries) {
    openGlRenderSetColor(&GLOBAL_render_renderOptions.bounding_box_color);
    for ( int i = 0; sceneGeometries != nullptr && i < sceneGeometries->size(); i++ ) {
        renderGeomBounds(sceneGeometries->get(i));
    }
}

/**
Renders the cluster hierarchy bounding boxes
*/
void
renderClusterHierarchy(java::ArrayList<Geometry *> *clusteredGeometryList) {
    openGlRenderSetColor(&GLOBAL_render_renderOptions.cluster_color);
    for ( int i = 0; clusteredGeometryList != nullptr && i < clusteredGeometryList->size(); i++ ) {
        renderGeomBounds(clusteredGeometryList->get(i));
    }
}

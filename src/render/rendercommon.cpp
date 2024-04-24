/**
Rendering stuff independent of the graphics library being used
*/

#include "common/mymath.h"
#include "common/options.h"
#include "skin/Geometry.h"
#include "scene/Camera.h"
#include "render/opengl.h"
#include "render/render.h"

static RenderOptions globalRenderOptions;

void
renderSetSmoothShading(char yesno) {
    globalRenderOptions.smoothShading = yesno;
}

void
renderSetNoShading(char yesno) {
    globalRenderOptions.noShading = yesno;
}

void
renderSetBackfaceCulling(char backface_culling) {
    globalRenderOptions.backfaceCulling = backface_culling;
}

void
renderSetOutlineDrawing(char draw_outlines) {
    globalRenderOptions.drawOutlines = draw_outlines;
}

void
renderSetBoundingBoxDrawing(char yesno) {
    globalRenderOptions.drawBoundingBoxes = yesno;
}

void
renderSetClusterDrawing(char yesno) {
    globalRenderOptions.drawClusters = yesno;
}

void
renderUseDisplayLists(char truefalse) {
    globalRenderOptions.useDisplayLists = truefalse;
}

void
renderUseFrustumCulling(char truefalse) {
    globalRenderOptions.frustumCulling = truefalse;
}

void
renderSetOutlineColor(ColorRgb *outline_color) {
    globalRenderOptions.outlineColor = *outline_color;
}

void
renderSetBoundingBoxColor(ColorRgb *outline_color) {
    globalRenderOptions.boundingBoxColor = *outline_color;
}

void
renderSetClusterColor(ColorRgb *cluster_color) {
    globalRenderOptions.clusterColor = *cluster_color;
}

static void
displayListsOption(void * /*value*/) {
    globalRenderOptions.useDisplayLists = true;
}

static void
flatOption(void * /*value*/) {
    globalRenderOptions.smoothShading = false;
}

static void
noCullingOption(void * /*value*/) {
    globalRenderOptions.backfaceCulling = false;
}

static void
outlinesOption(void * /*value*/) {
    globalRenderOptions.drawOutlines = true;
}

static void
traceOption(void * /*value*/) {
    globalRenderOptions.trace = true;
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
    {"-outline-color", 10, TRGB, &globalRenderOptions.outlineColor, DEFAULT_ACTION,
     "-outline-color <rgb> \t: color for polygon outlines"},
    {nullptr, 0, nullptr, nullptr, nullptr, nullptr}
};

void
renderParseOptions(int *argc, char **argv, RenderOptions *renderOptions) {
    globalRenderOptions = *renderOptions;
    parseGeneralOptions(renderingOptions, argc, argv);
    *renderOptions = globalRenderOptions;
}

/**
Computes front- and back-clipping plane distance for the current GLOBAL_scene_world and
camera
*/
void
renderGetNearFar(
    Camera *camera,
    java::ArrayList<Geometry *> *sceneGeometries)
{
    BoundingBox bounds;
    Vector3D b[2];
    Vector3D d;
    int i;
    int j;
    int k;
    float z;

    if ( sceneGeometries == nullptr || sceneGeometries->size() == 0 ) {
        camera->far = 10.0;
        camera->near = 0.1;
        return;
    }

    geometryListBounds(sceneGeometries, &bounds);

    b[0].set(bounds.coordinates[MIN_X], bounds.coordinates[MIN_Y], bounds.coordinates[MIN_Z]);
    b[1].set(bounds.coordinates[MAX_X], bounds.coordinates[MAX_Y], bounds.coordinates[MAX_Z]);

    camera->far = -HUGE;
    camera->near = HUGE;
    for ( i = 0; i <= 1; i++ ) {
        for ( j = 0; j <= 1; j++ ) {
            for ( k = 0; k <= 1; k++ ) {
                d.set(b[i].x, b[j].y, b[k].z);
                vectorSubtract(d, camera->eyePosition, d);
                z = vectorDotProduct(d, camera->Z);

                if ( z > camera->far ) {
                    camera->far = z;
                }
                if ( z < camera->near ) {
                    camera->near = z;
                }
            }
        }
    }

    // Take 2% extra distance for near as well as far clipping plane
    camera->far += 0.02f * (camera->far);
    camera->near -= 0.02f * (camera->near);
    if ( camera->far < EPSILON ) {
        camera->far = camera->viewDistance;
    }
    if ( camera->near < EPSILON ) {
        camera->near = camera->viewDistance / 100.0f;
    }
}

/**
Renders a bounding box
*/
void
renderBounds(Camera *camera, BoundingBox bounds) {
    Vector3D p[8];

    p[0].set(bounds.coordinates[MIN_X], bounds.coordinates[MIN_Y], bounds.coordinates[MIN_Z]);
    p[1].set(bounds.coordinates[MAX_X], bounds.coordinates[MIN_Y], bounds.coordinates[MIN_Z]);
    p[2].set(bounds.coordinates[MIN_X], bounds.coordinates[MAX_Y], bounds.coordinates[MIN_Z]);
    p[3].set(bounds.coordinates[MAX_X], bounds.coordinates[MAX_Y], bounds.coordinates[MIN_Z]);
    p[4].set(bounds.coordinates[MIN_X], bounds.coordinates[MIN_Y], bounds.coordinates[MAX_Z]);
    p[5].set(bounds.coordinates[MAX_X], bounds.coordinates[MIN_Y], bounds.coordinates[MAX_Z]);
    p[6].set(bounds.coordinates[MIN_X], bounds.coordinates[MAX_Y], bounds.coordinates[MAX_Z]);
    p[7].set(bounds.coordinates[MAX_X], bounds.coordinates[MAX_Y], bounds.coordinates[MAX_Z]);

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
renderGeomBounds(Camera *camera, Geometry *geometry) {
    BoundingBox geometryBoundingBox = getBoundingBox(geometry);

    if ( geometry->bounded ) {
        renderBounds(camera, geometryBoundingBox);
    }

    if ( geometry->isCompound() ) {
        java::ArrayList<Geometry *> *geometryList = geomPrimListCopy(geometry);
        for ( int i = 0; geometryList != nullptr && i < geometryList->size(); i++ ) {
            renderGeomBounds(camera, geometryList->get(i));
        }
        delete geometryList;
    }
}

/**
Renders the bounding boxes of all objects in the scene
*/
void
renderBoundingBoxHierarchy(Camera *camera, java::ArrayList<Geometry *> *sceneGeometries, RenderOptions *renderOptions) {
    openGlRenderSetColor(&renderOptions->boundingBoxColor);
    for ( int i = 0; sceneGeometries != nullptr && i < sceneGeometries->size(); i++ ) {
        renderGeomBounds(camera, sceneGeometries->get(i));
    }
}

/**
Renders the cluster hierarchy bounding boxes
*/
void
renderClusterHierarchy(Camera *camera, java::ArrayList<Geometry *> *clusteredGeometryList, RenderOptions *renderOptions) {
    openGlRenderSetColor(&renderOptions->clusterColor);
    for ( int i = 0; clusteredGeometryList != nullptr && i < clusteredGeometryList->size(); i++ ) {
        renderGeomBounds(camera, clusteredGeometryList->get(i));
    }
}

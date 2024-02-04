#include <cstdlib>

#include <GL/glu.h>
#include <GL/osmesa.h>

#include "common/error.h"
#include "scene/scene.h"
#include "skin/radianceinterfaces.h"
#include "render/canvas.h"
#include "render/renderhook.h"
#include "render/softids.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "render/opengl.h"
#include "render/render.h"

class OctreeChild {
public:
    Geometry *geom;
    float dist;
};

static int globalDisplayListId = -1;
static int globalOpenGlInitialized = false;
static GLubyte *globalBackground = nullptr;
static GLuint globalBackgroundTex = 0;

/**
Re-renders last ray-traced image if any, Returns TRUE if there is one,
and FALSE if not
*/
static int
openGlRenderRayTraced(int (*reDisplayCallback)()) {
    if ( reDisplayCallback == nullptr ) {
        return false;
    } else {
        return reDisplayCallback();
    }
}

void
openGlRenderClearWindow() {
    glClearColor(GLOBAL_camera_mainCamera.background.r, GLOBAL_camera_mainCamera.background.g, GLOBAL_camera_mainCamera.background.b, 0.0);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
openGlRenderSetCamera() {
    openGlRenderClearWindow();

    // Use the full viewport
    glViewport(0, 0, GLOBAL_camera_mainCamera.xSize, GLOBAL_camera_mainCamera.ySize);

    // Draw background when needed
    if ( GLOBAL_render_renderOptions.useBackground && globalBackground ) {
        openGlRenderBackground(&GLOBAL_camera_mainCamera);
    }

    // Determine distance to front- and back-clipping plane
    renderGetNearFar(&GLOBAL_camera_mainCamera.near, &GLOBAL_camera_mainCamera.far);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(GLOBAL_camera_mainCamera.verticalFov * 2.0,
                   (float)GLOBAL_camera_mainCamera.xSize / (float)GLOBAL_camera_mainCamera.ySize,
                   GLOBAL_camera_mainCamera.near,
                   GLOBAL_camera_mainCamera.far);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(GLOBAL_camera_mainCamera.eyePosition.x, GLOBAL_camera_mainCamera.eyePosition.y, GLOBAL_camera_mainCamera.eyePosition.z,
              GLOBAL_camera_mainCamera.lookPosition.x, GLOBAL_camera_mainCamera.lookPosition.y, GLOBAL_camera_mainCamera.lookPosition.z,
              GLOBAL_camera_mainCamera.upDirection.x, GLOBAL_camera_mainCamera.upDirection.y, GLOBAL_camera_mainCamera.upDirection.z);
}

static void
openGlInitState() {
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    glDrawBuffer(GL_FRONT_AND_BACK);

    openGlRenderClearWindow();
    glFinish();

    globalOpenGlInitialized = true;
    globalDisplayListId = -1;
}

/**
Creates an offscreen window for rendering
*/
void
openGlMesaRenderCreateOffscreenWindow(int width, int height) {
    GLubyte *imageBuffer = (GLubyte *)malloc(width * height * sizeof(GLubyte) * 4);

    OSMesaContext osMesaContext = OSMesaCreateContext(OSMESA_RGBA, nullptr);
    if ( !osMesaContext ) {
        logFatal(1, nullptr, "Couldn't create Mesa offscreen rendering context");
    }

    if ( !OSMesaMakeCurrent(osMesaContext, imageBuffer, GL_UNSIGNED_BYTE, width, height) ) {
        logFatal(1, nullptr, "Couldn't bind Mesa offscreen rendering context to image buffer of size %d x %d", width, height);
    }

    openGlInitState();
    OSMesaDestroyContext(osMesaContext);
    free(imageBuffer);
}

/**
Returns FALSE until rendering is fully initialized. Do not attempt to
render something until this function returns TRUE
*/
int
openGlRenderInitialized() {
    return globalOpenGlInitialized;
}

/**
Renders a line from point p to point q, for eg debugging
*/
void
openGlRenderLine(Vector3D *x, Vector3D *y) {
    Vector3D X;
    Vector3D Y;
    Vector3D dir;

    glBegin(GL_LINES);

    // Move the line a bit closer to the eye-point to avoid Z buffer artefacts
    VECTORSUBTRACT(GLOBAL_camera_mainCamera.eyePosition, *x, dir);
    VECTORSUMSCALED(*x, 0.01, dir, X);
    glVertex3fv((GLfloat *) &X);

    VECTORSUBTRACT(GLOBAL_camera_mainCamera.eyePosition, *y, dir);
    VECTORSUMSCALED(*y, 0.01, dir, Y);
    glVertex3fv((GLfloat *) &Y);

    glEnd();
}

/**
Sets the current color for line or outline drawing
*/
void
openGlRenderSetColor(RGB *rgb) {
    RGB correctedRgb{};

    correctedRgb = *rgb;
    toneMappingGammaCorrection(correctedRgb);
    glColor3fv((GLfloat *) &correctedRgb);
}

/**
Sets line width for outlines, etc
*/
void
openGlRenderSetLineWidth(float width) {
    glLineWidth(width);
}

/**
Renders a convex polygon flat shaded in the current color
*/
void
openGlRenderPolygonFlat(int numberOfVertices, Vector3D *vertices) {
    int i;

    glBegin(GL_POLYGON);
    for ( i = 0; i < numberOfVertices; i++ ) {
        glVertex3fv((GLfloat *) &vertices[i]);
    }
    glEnd();
}

/**
Renders a convex polygon with Gouraud shading
*/
void
openGlRenderPolygonGouraud(int numberOfVertices, Vector3D *vertices, RGB *verticesColors) {
    int i;

    glBegin(GL_POLYGON);
    for ( i = 0; i < numberOfVertices; i++ ) {
        openGlRenderSetColor(&verticesColors[i]);
        glVertex3fv((GLfloat *) &vertices[i]);
    }
    glEnd();
}

/**
Start a strip
*/
void
openGlRenderBeginTriangleStrip() {
    glBegin(GL_TRIANGLE_STRIP);
}

/**
Supply the next point (one at a time)
*/
void
openGlRenderNextTrianglePoint(Vector3D *point, RGB *col) {
    if ( col ) {
        openGlRenderSetColor(col);
    }
    glVertex3fv((float *) point);
}

/**
End a strip
*/
void
openGlRenderEndTriangleStrip() {
    glEnd();
}

void
openGlRenderPatchFlat(Patch *patch) {
    openGlRenderSetColor(&patch->color);
    switch ( patch->numberOfVertices ) {
        case 3:
            glBegin(GL_TRIANGLES);
            glVertex3fv((GLfloat *) patch->vertex[0]->point);
            glVertex3fv((GLfloat *) patch->vertex[1]->point);
            glVertex3fv((GLfloat *) patch->vertex[2]->point);
            glEnd();
            break;
        case 4:
            glBegin(GL_QUADS);
            glVertex3fv((GLfloat *) patch->vertex[0]->point);
            glVertex3fv((GLfloat *) patch->vertex[1]->point);
            glVertex3fv((GLfloat *) patch->vertex[2]->point);
            glVertex3fv((GLfloat *) patch->vertex[3]->point);
            glEnd();
            break;
        default:
            glBegin(GL_POLYGON);
            for ( int i = 0; i < patch->numberOfVertices; i++ ) {
                glVertex3fv((GLfloat *) patch->vertex[i]->point);
            }
            glEnd();
    }
}

void
openGlRenderPatchSmooth(Patch *patch) {
    int i;

    switch ( patch->numberOfVertices ) {
        case 3:
            glBegin(GL_TRIANGLES);
            openGlRenderSetColor(&patch->vertex[0]->color);
            glVertex3fv((GLfloat *) patch->vertex[0]->point);
            openGlRenderSetColor(&patch->vertex[1]->color);
            glVertex3fv((GLfloat *) patch->vertex[1]->point);
            openGlRenderSetColor(&patch->vertex[2]->color);
            glVertex3fv((GLfloat *) patch->vertex[2]->point);
            glEnd();
            break;
        case 4:
            glBegin(GL_QUADS);
            openGlRenderSetColor(&patch->vertex[0]->color);
            glVertex3fv((GLfloat *) patch->vertex[0]->point);
            openGlRenderSetColor(&patch->vertex[1]->color);
            glVertex3fv((GLfloat *) patch->vertex[1]->point);
            openGlRenderSetColor(&patch->vertex[2]->color);
            glVertex3fv((GLfloat *) patch->vertex[2]->point);
            openGlRenderSetColor(&patch->vertex[3]->color);
            glVertex3fv((GLfloat *) patch->vertex[3]->point);
            glEnd();
            break;
        default:
            glBegin(GL_POLYGON);
            for ( i = 0; i < patch->numberOfVertices; i++ ) {
                openGlRenderSetColor(&patch->vertex[i]->color);
                glVertex3fv((GLfloat *) patch->vertex[i]->point);
            }
            glEnd();
    }
}

/**
Renders the patch outline in the current color
*/
void
openGlRenderPatchOutline(Patch *patch) {
    int i;
    Vector3D tmp;
    Vector3D dir;

    glBegin(GL_LINE_LOOP);
    for ( i = 0; i < patch->numberOfVertices; i++ ) {
        // Move the outlines a bit closer to the eye-point to avoid Z buffer artefacts
        VECTORSUBTRACT(GLOBAL_camera_mainCamera.eyePosition, *patch->vertex[i]->point, dir);
        VECTORSUMSCALED(*patch->vertex[i]->point, 0.01, dir, tmp);
        glVertex3fv((GLfloat *) &tmp);
    }
    glEnd();
}

/**
Renders the all the patches using default colors
*/
void
openGlRenderPatch(Patch *patch) {
    if ( !GLOBAL_render_renderOptions.noShading ) {
        if ( GLOBAL_render_renderOptions.smoothShading ) {
            openGlRenderPatchSmooth(patch);
        } else {
            openGlRenderPatchFlat(patch);
        }
    }

    if ( GLOBAL_render_renderOptions.drawOutlines &&
         (VECTORDOTPRODUCT(patch->normal, GLOBAL_camera_mainCamera.eyePosition) + patch->planeConstant > EPSILON
          || GLOBAL_render_renderOptions.useDisplayLists)) {
        openGlRenderSetColor(&GLOBAL_render_renderOptions.outline_color);
        openGlRenderPatchOutline(patch);
    }
}

static void
openGlReallyRenderOctreeLeaf(Geometry *geometry, void (*renderPatch)(Patch *)) {
    java::ArrayList<Patch *> *patchList = geomPatchArrayList(geometry);
    for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
        renderPatch(patchList->get(i));
    }
}

static void
openGlRenderOctreeLeaf(Geometry *geom, void (*render_patch)(Patch *)) {
    if ( GLOBAL_render_renderOptions.useDisplayLists ) {
        if ( geom->displayListId <= 0 ) {
            geom->displayListId = geom->id;
            glNewList(geom->displayListId, GL_COMPILE_AND_EXECUTE);
            openGlReallyRenderOctreeLeaf(geom, render_patch);
            glEndList();
        } else {
            glCallList(geom->displayListId);
        }
    } else {
        openGlReallyRenderOctreeLeaf(geom, render_patch);
    }
}

static int
openGlViewCullBounds(float *bounds) {
    for ( int i = 0; i < NUMBER_OF_VIEW_PLANES; i++ ) {
        if ( boundsBehindPlane(bounds, &GLOBAL_camera_mainCamera.viewPlane[i].normal,
                               GLOBAL_camera_mainCamera.viewPlane[i].d)) {
            return true;
        }
    }
    return false;
}

/**
Squared distance to midpoint (avoid taking square root)
*/
static float
openGlBoundsDistance2(Vector3D p, float *bounds) {
    Vector3D mid;
    Vector3D d;

    VECTORSET(mid,
              0.5f * (bounds[MIN_X] + bounds[MAX_X]),
              0.5f * (bounds[MIN_Y] + bounds[MAX_Y]),
              0.5f * (bounds[MIN_Z] + bounds[MAX_Z]));
    VECTORSUBTRACT(mid, p, d);

    return VECTORNORM2(d);
}

/**
Geometry is a surface or a compound with 1 surface and up to 8 compound children geometries,
clusteredWorldGeom is such a geometry e.g.
*/
static void
openGlRenderOctreeNonLeaf(Geometry *geometry, void (*render_patch)(Patch *)) {
    int i;
    int n;
    int remaining;
    OctreeChild octree_children[8];
    GeometryListNode *children = geomPrimList(geometry);

    i = 0;
    for ( GeometryListNode *window = children; window != nullptr; window = window->next ) {
        Geometry *child = window->geometry;
        if ( geomIsAggregate(child) ) {
            if ( i >= 8 ) {
                logError("openGlRenderOctreeNonLeaf", "Invalid octree geometry node (more than 8 compound children)");
                return;
            }
            octree_children[i++].geom = child;
        } else {
            // render the patches associated with the octree node right away
            openGlRenderOctreeLeaf(child, render_patch);
        }
    }
    n = i; // Number of compound children

    // cull the non-leaf octree children geoms
    for ( i = 0; i < n; i++ ) {
        if ( openGlViewCullBounds(octree_children[i].geom->bounds)) {
            octree_children[i].geom = nullptr; // culled
            octree_children[i].dist = HUGE;
        } else {
            // not culled, compute distance from eye to midpoint of child
            octree_children[i].dist = openGlBoundsDistance2(GLOBAL_camera_mainCamera.eyePosition,
                                                            octree_children[i].geom->bounds);
        }
    }

    // Render children geometries, front to back order
    remaining = n;
    while ( remaining > 0 ) {
        // Find the closest remaining child
        int closest = 0;
        for ( i = 1; i < n; i++ ) {
            if ( octree_children[i].dist < octree_children[closest].dist ) {
                closest = i;
            }
        }

        if ( !octree_children[closest].geom ) {
            break;
        }

        // render it
        openGlRenderOctreeNonLeaf(octree_children[closest].geom, render_patch);

        // remove it from the list
        octree_children[closest].geom = nullptr;
        octree_children[closest].dist = HUGE;
        remaining--;
    }
}

/**
Traverses the patches in the scene in such a way to obtain
hierarchical view frustum culling + sorted (large patches first +
near to far) rendering. For every patch that is not culled,
render_patch is called. */
void
openGlRenderWorldOctree(void (*render_patch)(Patch *)) {
    if ( !GLOBAL_scene_clusteredWorldGeom ) {
        return;
    }
    if ( !render_patch ) {
        render_patch = openGlRenderPatch;
    }
    if ( geomIsAggregate(GLOBAL_scene_clusteredWorldGeom)) {
        openGlRenderOctreeNonLeaf(GLOBAL_scene_clusteredWorldGeom, render_patch);
    } else {
        openGlRenderOctreeLeaf(GLOBAL_scene_clusteredWorldGeom, render_patch);
    }
}

static void
openGlGeometryDeleteDLists(Geometry *geom) {
    if ( geom->displayListId >= 0 ) {
        glDeleteLists(geom->displayListId, 1);
    }
    geom->displayListId = -1;

    if ( geomIsAggregate(GLOBAL_scene_clusteredWorldGeom)) {
        GeometryListNode *children = geomPrimList(geom);
        for ( GeometryListNode *window = children; window != nullptr; window = window->next ) { \
            Geometry *child = window->geometry;
            openGlGeometryDeleteDLists(child);
        }
    }
}

static void
openGlRenderNewOctreeDisplayLists() {
    if ( GLOBAL_scene_clusteredWorldGeom ) {
        openGlGeometryDeleteDLists(GLOBAL_scene_clusteredWorldGeom);
    }
}

/**
Indicates that the scene has modified, so a new display list should be
compiled and rendered from now on. Only relevant when using display lists
*/
void
openGlRenderNewDisplayList() {
    if ( globalDisplayListId >= 0 ) {
        glDeleteLists(globalDisplayListId, 1);
    }
    globalDisplayListId = -1;

    if ( GLOBAL_render_renderOptions.frustumCulling ) {
        openGlRenderNewOctreeDisplayLists();
    }
}

static void
openGlReallyRender(java::ArrayList<Patch *> *scenePatches) {
    if ( GLOBAL_radiance_currentRadianceMethodHandle && GLOBAL_radiance_currentRadianceMethodHandle->renderScene ) {
        GLOBAL_radiance_currentRadianceMethodHandle->renderScene(scenePatches);
    } else if ( GLOBAL_render_renderOptions.frustumCulling ) {
            openGlRenderWorldOctree(openGlRenderPatch);
    } else {
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            openGlRenderPatch(scenePatches->get(i));
        }
    }
}

static void
openGlRenderRadiance(java::ArrayList<Patch *> *scenePatches) {
    if ( GLOBAL_render_renderOptions.smoothShading ) {
        glShadeModel(GL_SMOOTH);
    } else {
        glShadeModel(GL_FLAT);
    }

    openGlRenderSetCamera();

    if ( GLOBAL_render_renderOptions.backfaceCulling ) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }

    if ( GLOBAL_render_renderOptions.useDisplayLists && !GLOBAL_render_renderOptions.frustumCulling ) {
        if ( globalDisplayListId <= 0 ) {
            globalDisplayListId = 1;
            glNewList(globalDisplayListId, GL_COMPILE_AND_EXECUTE);
            // Render the scene
            openGlReallyRender(scenePatches);
            glEndList();
        } else {
            glCallList(1);
        }
    } else {
        // Just render the scene
        openGlReallyRender(scenePatches);
    }

    if ( GLOBAL_render_renderOptions.drawBoundingBoxes ) {
        renderBoundingBoxHierarchy();
    }

    if ( GLOBAL_render_renderOptions.drawClusters ) {
        renderClusterHierarchy();
    }

    if ( GLOBAL_render_renderOptions.drawCameras ) {
        openGlRenderCameras();
    }
}

/**
Renders the whole scene
*/
void
openGlRenderScene(java::ArrayList<Patch *> *scenePatches, int (*reDisplayCallback)()) {
    if ( !globalOpenGlInitialized ) {
        return;
    }

    openGlRenderSetLineWidth(GLOBAL_render_renderOptions.lineWidth);

    canvasPushMode();

    if ( GLOBAL_camera_mainCamera.changed ) {
        GLOBAL_render_renderOptions.renderRayTracedImage = false;
    }

    if ( !GLOBAL_render_renderOptions.renderRayTracedImage || !openGlRenderRayTraced(reDisplayCallback)) {
        openGlRenderRadiance(scenePatches);
    }

    // Call installed render hooks, that want to render something in the scene
    renderHooks();

    glFinish();

    glDrawBuffer(GL_FRONT_AND_BACK);

    canvasPullMode();
}

/**
Renders an image of m lines of n pixels at column x on row y (= lower
left corner of image, relative to the lower left corner of the window)
*/
void
openGlRenderPixels(int x, int y, int width, int height, RGB *rgb) {
    int rowLength;

    // Length of one row of RGBA image data rounded up to a multiple of 8
    rowLength = (4 * width * sizeof(GLubyte) + 7) & ~7;
    GLubyte *c = new GLubyte[height * rowLength + 8];

    for ( int j = 0; j < height; j++ ) {
        RGB *rgbP = &rgb[j * width];

        GLubyte *p = c + j * rowLength; // Let each line start on an 8-byte boundary
        for ( int i = 0; i < width; i++, rgbP++ ) {
            RGB corrected_rgb = *rgbP;
            toneMappingGammaCorrection(corrected_rgb);
            *p++ = (GLubyte) (corrected_rgb.r * 255.0);
            *p++ = (GLubyte) (corrected_rgb.g * 255.0);
            *p++ = (GLubyte) (corrected_rgb.b * 255.0);
            *p++ = 255; // alpha = 1.0
        }
    }

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, GLOBAL_camera_mainCamera.xSize, 0, GLOBAL_camera_mainCamera.ySize, -1.0, 1.0);

    glDisable(GL_DEPTH_TEST);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
    glRasterPos2i(x, y);
    glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, c);

    glFlush();
    glFinish();

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    delete[] c;
}

static void
openGlRenderFrustum(Camera *cam) {
    Vector3D c;
    Vector3D P;
    Vector3D Q;
    float cameraSize = GLOBAL_render_renderOptions.cameraSize;
    float width = cameraSize * cam->viewDistance * cam->pixelWidthTangent;
    float height = cameraSize * cam->viewDistance * cam->pixelHeightTangent;
    int i;
    int j;
    int maxI = 12;
    int maxJ = (int)((float) maxI * height / width);

    VECTORCOMB2(1.0, cam->eyePosition, cameraSize * cam->viewDistance, cam->Z, c);

    glDisable(GL_DEPTH_TEST);

    openGlRenderSetColor(&GLOBAL_render_renderOptions.camera_color);

    for ( i = 0; i <= maxI; i++ ) {
        VECTORCOMB3(c, (-1.0f + 2.0f * ((float) i / (float) maxI)) * width, cam->X, -height, cam->Y, P);
        VECTORCOMB3(c, (-1.0f + 2.0f * ((float) i / (float) maxI)) * width, cam->X, +height, cam->Y, Q);
        openGlRenderLine(&P, &Q);
    }

    for ( j = 0; j <= maxJ; j++ ) {
        VECTORCOMB3(c, -width, cam->X, (-1.0f + 2.0f * ((float) j / (float) maxJ)) * height, cam->Y, P);
        VECTORCOMB3(c, +width, cam->X, (-1.0f + 2.0f * ((float) j / (float) maxJ)) * height, cam->Y, Q);
        openGlRenderLine(&P, &Q);
    }

    VECTORCOMB3(c, width, cam->X, -height, cam->Y, Q);
    openGlRenderLine(&cam->eyePosition, &Q);
    VECTORCOMB3(c, -width, cam->X, -height, cam->Y, Q);
    openGlRenderLine(&cam->eyePosition, &Q);
    VECTORCOMB3(c, -width, cam->X, height, cam->Y, Q);
    openGlRenderLine(&cam->eyePosition, &Q);
    VECTORCOMB3(c, width, cam->X, height, cam->Y, Q);
    openGlRenderLine(&cam->eyePosition, &Q);

    glLineWidth(1);
    glEnable(GL_DEPTH_TEST);
}

/**
Renders alternate camera, virtual screen etc ... for didactic pictures etc
*/
void
openGlRenderCameras() {
    openGlRenderFrustum(&GLOBAL_camera_alternateCamera);
}

/**
Display background, no Z-buffer, fill whole screen
*/
void
openGlRenderBackground(Camera *camera) {
    // Turn off Z-buffer
    glDisable(GL_DEPTH_TEST);

    // Push matrix and create bogus camera
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, camera->xSize, 0, camera->ySize);

    // Draw background (as texture)
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glBindTexture(GL_TEXTURE_2D, globalBackgroundTex);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f((float)camera->xSize, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f((float)camera->xSize, (float)camera->ySize, 0.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(0.0f, (float)camera->ySize, 0.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    // Pop matrix
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    // Enable Z-buffer back
    glEnable(GL_DEPTH_TEST);
}

/**
Patch ID rendering. Returns an array of size (*x)*(*y) containing the IDs of
the patches visible through each pixel or 0 if the background is visible through
the pixel. x is normally the width and y the height of the canvas window
*/
unsigned long *
sglRenderIds(long *x, long *y, java::ArrayList<Patch *> *scenePatches) {
    return softRenderIds(x, y, scenePatches);
}

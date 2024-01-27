#include <cstdlib>

#include <GL/glu.h>
#include <GL/osmesa.h>

#include "common/error.h"
#include "scene/scene.h"
#include "skin/radianceinterfaces.h"
#include "shared/render.h"
#include "shared/canvas.h"
#include "shared/renderhook.h"
#include "shared/softids.h"
#include "shared/options.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "raycasting/simple/RayCaster.h"
#include "app/opengl.h"

static int displayListId = -1;
static int openglInitialized = false;
static GLubyte *background_ptr = nullptr;
static GLuint backgroundTex = 0;

Raytracer *GLOBAL_raytracer_activeRaytracer = (Raytracer *) nullptr;

void
renderClearWindow() {
    glClearColor(GLOBAL_camera_mainCamera.background.r, GLOBAL_camera_mainCamera.background.g, GLOBAL_camera_mainCamera.background.b, 0.0);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
renderSetCamera() {
    renderClearWindow();

    // use full viewport
    glViewport(0, 0, GLOBAL_camera_mainCamera.xSize, GLOBAL_camera_mainCamera.ySize);

    // draw backgroudn when needed
    if ( GLOBAL_render_renderOptions.use_background && background_ptr ) {
        renderBackground(&GLOBAL_camera_mainCamera);
    }

    // determine distance to front- and backclipping plane
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

    renderClearWindow();
    glFinish();

    openglInitialized = true;
    displayListId = -1;
}

/**
Creates an offscreen window for rendering
*/
void
renderCreateOffscreenWindow(int hres, int vres) {
    GLubyte *image_buffer = (GLubyte *)malloc(hres * vres * sizeof(GLubyte) * 4);

    OSMesaContext osctx = OSMesaCreateContext(OSMESA_RGBA, nullptr);
    if ( !osctx ) {
        logFatal(1, nullptr, "Couldn't create Mesa offscreen rendering context");
    }

    if ( !OSMesaMakeCurrent(osctx, image_buffer, GL_UNSIGNED_BYTE, hres, vres)) {
        logFatal(1, nullptr, "Couldn't bind Mesa offscreen rendering context to image buffer of size %d x %d", hres,
                 vres);
    }

    openGlInitState();
}

/**
Returns FALSE until rendering is fully initialized. Do not attempt to
render something until this function returns TRUE
*/
int
renderInitialized() {
    return openglInitialized;
}

/**
Renders a line from point p to point q, for eg debugging
*/
void
renderLine(Vector3D *x, Vector3D *y) {
    Vector3D X;
    Vector3D Y;
    Vector3D dir;

    glBegin(GL_LINES);

    // move the line a bit closer to the eyepoint to avoid Z buffer artefacts
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
renderSetColor(RGB *rgb) {
    RGB correctedRgb{};

    correctedRgb = *rgb;
    toneMappingGammaCorrection(correctedRgb);
    glColor3fv((GLfloat *) &correctedRgb);
}

/**
Sets line width for outlines, etc
*/
void
renderSetLineWidth(float width) {
    glLineWidth(width);
}

/**
Renders a convex polygon flat shaded in the current color
*/
void
renderPolygonFlat(int nrverts, Vector3D *verts) {
    int i;

    glBegin(GL_POLYGON);
    for ( i = 0; i < nrverts; i++ ) {
        glVertex3fv((GLfloat *) &verts[i]);
    }
    glEnd();
}

/**
Renders a convex polygon with Gouraud shading
*/
void
renderPolygonGouraud(int nrverts, Vector3D *verts, RGB *vertcols) {
    int i;

    glBegin(GL_POLYGON);
    for ( i = 0; i < nrverts; i++ ) {
        renderSetColor(&vertcols[i]);
        glVertex3fv((GLfloat *) &verts[i]);
    }
    glEnd();
}

/**
Start a strip
*/
void
renderBeginTriangleStrip() {
    glBegin(GL_TRIANGLE_STRIP);
}

/**
Supply the next point (one at a time)
*/
void
renderNextTrianglePoint(Vector3D *point, RGB *col) {
    if ( col ) {
        renderSetColor(col);
    }
    glVertex3fv((float *) point);
}

/**
End a strip
*/
void
renderEndTriangleStrip() {
    glEnd();
}

void
renderPatchFlat(Patch *patch) {
    int i;

    renderSetColor(&patch->color);
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
            for ( i = 0; i < patch->numberOfVertices; i++ ) {
                glVertex3fv((GLfloat *) patch->vertex[i]->point);
            }
            glEnd();
    }
}

void
renderPatchSmooth(Patch *patch) {
    int i;

    switch ( patch->numberOfVertices ) {
        case 3:
            glBegin(GL_TRIANGLES);
            renderSetColor(&patch->vertex[0]->color);
            glVertex3fv((GLfloat *) patch->vertex[0]->point);
            renderSetColor(&patch->vertex[1]->color);
            glVertex3fv((GLfloat *) patch->vertex[1]->point);
            renderSetColor(&patch->vertex[2]->color);
            glVertex3fv((GLfloat *) patch->vertex[2]->point);
            glEnd();
            break;
        case 4:
            glBegin(GL_QUADS);
            renderSetColor(&patch->vertex[0]->color);
            glVertex3fv((GLfloat *) patch->vertex[0]->point);
            renderSetColor(&patch->vertex[1]->color);
            glVertex3fv((GLfloat *) patch->vertex[1]->point);
            renderSetColor(&patch->vertex[2]->color);
            glVertex3fv((GLfloat *) patch->vertex[2]->point);
            renderSetColor(&patch->vertex[3]->color);
            glVertex3fv((GLfloat *) patch->vertex[3]->point);
            glEnd();
            break;
        default:
            glBegin(GL_POLYGON);
            for ( i = 0; i < patch->numberOfVertices; i++ ) {
                renderSetColor(&patch->vertex[i]->color);
                glVertex3fv((GLfloat *) patch->vertex[i]->point);
            }
            glEnd();
    }
}

/**
Renders the patch outline in the current color
*/
void
renderPatchOutline(Patch *patch) {
    int i;
    Vector3D tmp;
    Vector3D dir;

    glBegin(GL_LINE_LOOP);
    for ( i = 0; i < patch->numberOfVertices; i++ ) {
        // move the outlines a bit closer to the eyepoint to avoid Z buffer artefacts
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
renderPatch(Patch *patch) {
    if ( !GLOBAL_render_renderOptions.no_shading ) {
        if ( GLOBAL_render_renderOptions.smooth_shading ) {
            renderPatchSmooth(patch);
        } else {
            renderPatchFlat(patch);
        }
    }

    if ( GLOBAL_render_renderOptions.draw_outlines &&
         (VECTORDOTPRODUCT(patch->normal, GLOBAL_camera_mainCamera.eyePosition) + patch->planeConstant > EPSILON
          || GLOBAL_render_renderOptions.use_display_lists)) {
        renderSetColor(&GLOBAL_render_renderOptions.outline_color);
        renderPatchOutline(patch);
    }
}

static void
reallyRenderOctreeLeaf(Geometry *geom, void (*renderPatch)(Patch *)) {
    PatchSet *patchList = geomPatchList(geom);
    PatchSet *pathWindow = (PatchSet *)patchList;
    if ( pathWindow != nullptr) {
        for ( PatchSet *window = pathWindow; window; window = window->next ) {
            renderPatch(window->patch);
        }
    }
}

static void
renderOctreeLeaf(Geometry *geom, void (*render_patch)(Patch *)) {
    if ( GLOBAL_render_renderOptions.use_display_lists ) {
        if ( geom->dlistid <= 0 ) {
            geom->dlistid = geom->id;
            glNewList(geom->dlistid, GL_COMPILE_AND_EXECUTE);
            reallyRenderOctreeLeaf(geom, render_patch);
            glEndList();
        } else {
            glCallList(geom->dlistid);
        }
    } else {
        reallyRenderOctreeLeaf(geom, render_patch);
    }
}

static int
viewCullBounds(float *bounds) {
    int i;
    for ( i = 0; i < NUMBER_OF_VIEW_PLANES; i++ ) {
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
boundsDistance2(Vector3D p, float *bounds) {
    Vector3D mid, d;
    VECTORSET(mid,
              0.5f * (bounds[MIN_X] + bounds[MAX_X]),
              0.5f * (bounds[MIN_Y] + bounds[MAX_Y]),
              0.5f * (bounds[MIN_Z] + bounds[MAX_Z]));
    VECTORSUBTRACT(mid, p, d);
    return VECTORNORM2(d);
}

class OctreeChild {
  public:
    Geometry *geom;
    float dist;
};

/**
geometry is a surface or a compoint with 1 surface and up to 8 compound children geoms,
CLusetredWorldGeom is such a geometry e.g.
*/
static void
renderOctreeNonLeaf(Geometry *geometry, void (*render_patch)(Patch *)) {
    int i, n, remaining;
    OctreeChild octree_children[8];
    GeometryListNode *children = geomPrimList(geometry);

    i = 0;
    ForAllGeoms(child, children)
                {
                    if ( geomIsAggregate(child)) {
                        if ( i >= 8 ) {
                            logError("renderOctreeNonLeaf", "Invalid octree geometry node (more than 8 compound children)");
                            return;
                        }
                        octree_children[i++].geom = child;
                    } else {
                        // render the patches associated with the octree node right away
                        renderOctreeLeaf(child, render_patch);
                    }
                }
    EndForAll;
    n = i; // Number of compound children

    // cull the non-leaf octree children geoms
    for ( i = 0; i < n; i++ ) {
        if ( viewCullBounds(octree_children[i].geom->bounds)) {
            octree_children[i].geom = nullptr; // culled
            octree_children[i].dist = HUGE;
        } else {
            // not culled, compute distance from eye to midpoint of child
            octree_children[i].dist = boundsDistance2(GLOBAL_camera_mainCamera.eyePosition, octree_children[i].geom->bounds);
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
        renderOctreeNonLeaf(octree_children[closest].geom, render_patch);

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
renderWorldOctree(void (*render_patch)(Patch *)) {
    if ( !GLOBAL_scene_clusteredWorldGeom ) {
        return;
    }
    if ( !render_patch ) {
        render_patch = renderPatch;
    }
    if ( geomIsAggregate(GLOBAL_scene_clusteredWorldGeom)) {
        renderOctreeNonLeaf(GLOBAL_scene_clusteredWorldGeom, render_patch);
    } else {
        renderOctreeLeaf(GLOBAL_scene_clusteredWorldGeom, render_patch);
    }
}

static void
geometryDeleteDLists(Geometry *geom) {
    if ( geom->dlistid >= 0 ) {
        glDeleteLists(geom->dlistid, 1);
    }
    geom->dlistid = -1;

    if ( geomIsAggregate(GLOBAL_scene_clusteredWorldGeom)) {
        GeometryListNode *children = geomPrimList(geom);
        ForAllGeoms(child, children)
                    {
                        geometryDeleteDLists(child);
                    }
        EndForAll;
    }
}

static void
renderNewOctreeDisplayLists() {
    if ( GLOBAL_scene_clusteredWorldGeom ) {
        geometryDeleteDLists(GLOBAL_scene_clusteredWorldGeom);
    }
}

/**
Indicates that the scene has modified, so a new display list should be
compiled and rendered from now on. Only relevant when using display lists
*/
void
renderNewDisplayList() {
    if ( displayListId >= 0 ) {
        glDeleteLists(displayListId, 1);
    }
    displayListId = -1;

    if ( GLOBAL_render_renderOptions.frustum_culling ) {
        renderNewOctreeDisplayLists();
    }
}

void
reallyRender(java::ArrayList<Patch *> *scenePatches) {
    if ( GLOBAL_radiance_currentRadianceMethodHandle && GLOBAL_radiance_currentRadianceMethodHandle->renderScene ) {
        GLOBAL_radiance_currentRadianceMethodHandle->renderScene(scenePatches);
    } else if ( GLOBAL_render_renderOptions.frustum_culling ) {
        renderWorldOctree(renderPatch);
    } else {
        for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
            renderPatch(scenePatches->get(i));
        }
    }
}

void
renderRadiance(java::ArrayList<Patch *> *scenePatches) {
    if ( GLOBAL_render_renderOptions.smooth_shading ) {
        glShadeModel(GL_SMOOTH);
    } else {
        glShadeModel(GL_FLAT);
    }

    renderSetCamera();

    if ( GLOBAL_render_renderOptions.backface_culling ) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }

    if ( GLOBAL_render_renderOptions.use_display_lists && !GLOBAL_render_renderOptions.frustum_culling ) {
        if ( displayListId <= 0 ) {
            displayListId = 1;
            glNewList(displayListId, GL_COMPILE_AND_EXECUTE);
            // Render the scene
            reallyRender(scenePatches);
            glEndList();
        } else {
            glCallList(1);
        }
    } else {
        // Just render the scene
        reallyRender(scenePatches);
    }

    if ( GLOBAL_render_renderOptions.draw_bounding_boxes ) {
        renderBoundingBoxHierarchy();
    }

    if ( GLOBAL_render_renderOptions.draw_clusters ) {
        renderClusterHierarchy();
    }

    if ( GLOBAL_render_renderOptions.draw_cameras ) {
        renderCameras();
    }
}

/**
Renders the whole scene
*/
void
renderScene(java::ArrayList<Patch *> *scenePatches) {
    if ( !openglInitialized ) {
        return;
    }

    renderSetLineWidth(GLOBAL_render_renderOptions.linewidth);

    canvasPushMode();

    if ( GLOBAL_camera_mainCamera.changed ) {
        GLOBAL_render_renderOptions.render_raytraced_image = false;
    }

    if ( !GLOBAL_render_renderOptions.render_raytraced_image || !renderRayTraced(GLOBAL_raytracer_activeRaytracer)) {
        renderRadiance(scenePatches);
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
renderPixels(int x, int y, int width, int height, RGB *rgb) {
    int j;
    int rowlen;
    GLubyte *c;

    // length of one row of RGBA image data rounded up to a multiple of 8
    rowlen = (4 * width * sizeof(GLubyte) + 7) & ~7;
    c = (GLubyte *)malloc(height * rowlen + 8);

    for ( j = 0; j < height; j++ ) {
        RGB *rgbp = &rgb[j * width];
        GLubyte *p = c + j * rowlen; // let each line start on a 8-byte boundary
        int i;
        for ( i = 0; i < width; i++, rgbp++ ) {
            RGB corrected_rgb = *rgbp;
            toneMappingGammaCorrection(corrected_rgb);
            *p++ = (GLubyte) (corrected_rgb.r * 255.);
            *p++ = (GLubyte) (corrected_rgb.g * 255.);
            *p++ = (GLubyte) (corrected_rgb.b * 255.);
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

    free((char *)c);
}

/**
Saves a RGB image in the front buffer
*/
void
saveScreen(char *fileName, FILE *fp, int isPipe, java::ArrayList<Patch *> *scenePatches) {
    ImageOutputHandle *img;
    long j, x = GLOBAL_camera_mainCamera.xSize, y = GLOBAL_camera_mainCamera.ySize;
    GLubyte *screen;
    unsigned char *buf;

    // RayCast() saves the current picture in display-mapped (!) real values
    if ( GLOBAL_render_renderOptions.trace ) {
        rayCast(fileName, fp, isPipe, scenePatches);
        return;
    }

    if ( !fp || !(img = createImageOutputHandle(fileName, fp, isPipe, x, y))) {
        return;
    }

    screen = (GLubyte *)malloc((int) (x * y) * sizeof(GLubyte) * 4);
    buf = (unsigned char *)malloc(3 * x);

    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, x, y, GL_RGBA, GL_UNSIGNED_BYTE, screen);

    for ( j = y - 1; j >= 0; j-- ) {
        long i;
        unsigned char *pbuf = buf;
        GLubyte *pixel = &screen[j * x * 4];
        for ( i = 0; i < x; i++, pixel += 4 ) {
            *pbuf++ = pixel[0];
            *pbuf++ = pixel[1];
            *pbuf++ = pixel[2];
        }
        writeDisplayRGB(img, buf);
    }

    free((char *) buf);
    free((char *) screen);

    deleteImageOutputHandle(img);
}

/**
Patch ID rendering. Returns an array of size (*x)*(*y) containing the IDs of
the patches visible through each pixel or 0 if the background is visible through
the pixel. x is normally the width and y the height of the canvas window
*/
unsigned long *
renderIds(long *x, long *y, java::ArrayList<Patch *> *scenePatches) {
    return softRenderIds(x, y, scenePatches);
}

static void
renderFrustum(Camera *cam) {
    Vector3D c;
    Vector3D P;
    Vector3D Q;
    float camlen = GLOBAL_render_renderOptions.camsize;
    float hsiz = camlen * cam->viewDistance * cam->pixelWidthTangent;
    float vsiz = camlen * cam->viewDistance * cam->pixelHeightTangent;
    int i;
    int j;
    int maxi = 12;
    int maxj = (int)((float) maxi * vsiz / hsiz);

    VECTORCOMB2(1., cam->eyePosition, camlen * cam->viewDistance, cam->Z, c);

    glDisable(GL_DEPTH_TEST);

    renderSetColor(&GLOBAL_render_renderOptions.camera_color);

    for ( i = 0; i <= maxi; i++ ) {
        VECTORCOMB3(c, (-1.0 + 2.0 * ((float) i / (float) maxi)) * hsiz, cam->X, -vsiz, cam->Y, P);
        VECTORCOMB3(c, (-1.0 + 2.0 * ((float) i / (float) maxi)) * hsiz, cam->X, +vsiz, cam->Y, Q);
        renderLine(&P, &Q);
    }

    for ( j = 0; j <= maxj; j++ ) {
        VECTORCOMB3(c, -hsiz, cam->X, (-1. + 2. * ((float) j / (float) maxj)) * vsiz, cam->Y, P);
        VECTORCOMB3(c, +hsiz, cam->X, (-1. + 2. * ((float) j / (float) maxj)) * vsiz, cam->Y, Q);
        renderLine(&P, &Q);
    }

    VECTORCOMB3(c, hsiz, cam->X, -vsiz, cam->Y, Q);
    renderLine(&cam->eyePosition, &Q);
    VECTORCOMB3(c, -hsiz, cam->X, -vsiz, cam->Y, Q);
    renderLine(&cam->eyePosition, &Q);
    VECTORCOMB3(c, -hsiz, cam->X, vsiz, cam->Y, Q);
    renderLine(&cam->eyePosition, &Q);
    VECTORCOMB3(c, hsiz, cam->X, vsiz, cam->Y, Q);
    renderLine(&cam->eyePosition, &Q);

    glLineWidth(1);
    glEnable(GL_DEPTH_TEST);
}

/**
Renders alternate camera, virtual screen etc ... for didactical pictures etc
*/
void
renderCameras() {
    renderFrustum(&GLOBAL_camera_alternateCamera);
}

/**
Display background, no Z-buffer, fill whole screen
*/
void
renderBackground(Camera *camera) {
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
    glBindTexture(GL_TEXTURE_2D, backgroundTex);
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

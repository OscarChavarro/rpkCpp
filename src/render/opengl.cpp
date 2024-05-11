#include <cstdlib>

#include <GL/glu.h>
#include <GL/osmesa.h>

#include "common/error.h"
#include "common/mymath.h"
#include "scene/RadianceMethod.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "render/canvas.h"
#include "render/renderhook.h"
#include "render/softids.h"
#include "render/opengl.h"
#include "render/render.h"
#include "render/glutDebugTools.h"

class OctreeChild {
  public:
    Geometry *geometry;
    float distance;
};

static int globalDisplayListId = -1;

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

static void
openGlRenderClearWindow(const Camera *camera) {
    glClearColor(camera->background.r, camera->background.g, camera->background.b, 0.0);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void
openGlRenderSetCamera(Camera *camera, const java::ArrayList<Geometry *> *sceneGeometries) {
    openGlRenderClearWindow(camera);

    // Use the full viewport
    glViewport(0, 0, camera->xSize, camera->ySize);

    // Determine distance to front- and back-clipping plane
    renderGetNearFar(camera, sceneGeometries);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(camera->verticalFov * 2.0,
                   (float)camera->xSize / (float)camera->ySize,
                   camera->near / 10,
                   camera->far * 10);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(camera->eyePosition.x, camera->eyePosition.y, camera->eyePosition.z,
              camera->lookPosition.x, camera->lookPosition.y, camera->lookPosition.z,
              camera->upDirection.x, camera->upDirection.y, camera->upDirection.z);
}

static void
openGlInitState(const Camera *camera) {
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    glDrawBuffer(GL_FRONT_AND_BACK);

    openGlRenderClearWindow(camera);
    glFinish();

    globalDisplayListId = -1;

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
}

/**
Creates an offscreen window for rendering
*/
void
openGlMesaRenderCreateOffscreenWindow(const Camera *camera, const int width, const int height) {
    GLubyte *imageBuffer = (GLubyte *)malloc(width * height * sizeof(GLubyte) * 4);

    OSMesaContext osMesaContext = OSMesaCreateContext(OSMESA_RGBA, nullptr);
    if ( !osMesaContext ) {
        logFatal(1, nullptr, "Couldn't create Mesa offscreen rendering context");
    }

    if ( !OSMesaMakeCurrent(osMesaContext, imageBuffer, GL_UNSIGNED_BYTE, width, height) ) {
        logFatal(1, nullptr, "Couldn't bind Mesa offscreen rendering context to image buffer of size %d x %d", width, height);
    }

    openGlInitState(camera);
    OSMesaDestroyContext(osMesaContext);
    free(imageBuffer);
}

/**
Renders a line from point p to point q, for eg debugging
*/
void
openGlRenderLine(Vector3D *x, Vector3D *y) {
    glDisable(GL_POLYGON_OFFSET_FILL);

    glBegin(GL_LINES);
        glVertex3fv((GLfloat *) x);
        glVertex3fv((GLfloat *) y);
    glEnd();

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
}

/**
Sets the current color for line or outline drawing
*/
void
openGlRenderSetColor(const ColorRgb *rgb) {
    ColorRgb correctedRgb{};

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
    glBegin(GL_POLYGON);
    for ( int i = 0; i < numberOfVertices; i++ ) {
        glVertex3fv((GLfloat *)&vertices[i]);
    }
    glEnd();
}

/**
Renders a convex polygon with Gouraud shading
*/
void
openGlRenderPolygonGouraud(int numberOfVertices, Vector3D *vertices, const ColorRgb *verticesColors) {
    glBegin(GL_POLYGON);
    for ( int i = 0; i < numberOfVertices; i++ ) {
        openGlRenderSetColor(&verticesColors[i]);
        glVertex3fv((GLfloat *)&vertices[i]);
    }
    glEnd();
}

static void
openGlRenderPatchFlat(const Patch *patch) {
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

static void
openGlRenderPatchSmooth(const Patch *patch) {
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
            for ( int i = 0; i < patch->numberOfVertices; i++ ) {
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
openGlRenderPatchOutline(const Patch *patch) {
    glBegin(GL_LINE_LOOP);
    for ( int i = 0; i < patch->numberOfVertices; i++ ) {
        glVertex3fv((GLfloat *) &patch->vertex[i]->point);
    }
    glEnd();
}

/**
Renders the all the patches using default colors
*/
void
openGlRenderPatch(Patch *patch, const Camera *camera, const RenderOptions *renderOptions) {
    if ( !renderOptions->noShading ) {
        if ( renderOptions->smoothShading ) {
            openGlRenderPatchSmooth(patch);
        } else {
            openGlRenderPatchFlat(patch);
        }
    }

    if ( renderOptions->drawOutlines &&
         (vectorDotProduct(patch->normal, camera->eyePosition) + patch->planeConstant > EPSILON) ) {
        openGlRenderSetColor(&renderOptions->outlineColor);
        openGlRenderPatchOutline(patch);
    }
}

static void
openGlReallyRenderOctreeLeaf(
    const Camera *camera,
    Geometry *geometry,
    void (*renderPatch)(Patch *, const Camera *, const RenderOptions *),
    const RenderOptions *renderOptions)
{
    const java::ArrayList<Patch *> *patchList = geomPatchArrayListReference(geometry);
    for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
        renderPatch(patchList->get(i), camera, renderOptions);
    }
}

static void
openGlRenderOctreeLeaf(
    const Camera *camera,
    Geometry *geometry,
    void (*renderPatchCallback)(Patch *, const Camera *, const RenderOptions *),
    const RenderOptions *renderOptions)
{
    openGlReallyRenderOctreeLeaf(camera, geometry, renderPatchCallback, renderOptions);
}

static int
openGlViewCullBounds(const Camera *camera, const BoundingBox *bounds) {
    for ( int i = 0; i < NUMBER_OF_VIEW_PLANES; i++ ) {
        if ( bounds->behindPlane(&camera->viewPlanes[i].normal, camera->viewPlanes[i].d) ) {
            return true;
        }
    }
    return false;
}

/**
Squared distance to midpoint (avoid taking square root)
*/
static float
openGlBoundsDistance2(Vector3D p, const float *bounds) {
    Vector3D mid;
    Vector3D d;

    mid.set(
        0.5f * (bounds[MIN_X] + bounds[MAX_X]),
        0.5f * (bounds[MIN_Y] + bounds[MAX_Y]),
        0.5f * (bounds[MIN_Z] + bounds[MAX_Z]));
    d.subtraction(mid, p);

    return vectorNorm2(d);
}

/**
Geometry is a surface or a compound with 1 surface and up to 8 compound children geometries,
clusteredWorldGeom is such a geometry e.g.
*/
static void
openGlRenderOctreeNonLeaf(
    Camera *camera,
    const Geometry *geometry,
    void (*renderPatchCallback)(Patch *, const Camera *, const RenderOptions *renderOptions),
    const RenderOptions *renderOptions)
{
    int i;
    int n;
    int remaining;
    OctreeChild octree_children[8];
    java::ArrayList<Geometry *> *children = geomPrimListCopy(geometry);

    i = 0;
    for ( int j = 0; children != nullptr && j < children->size(); j++ ) {
        Geometry *child = children->get(j);
        if ( child->isCompound() ) {
            if ( i >= 8 ) {
                logError("openGlRenderOctreeNonLeaf", "Invalid octree geometry node (more than 8 compound children)");
                delete children;
                return;
            }
            octree_children[i++].geometry = child;
        } else {
            // Render the patches associated with the octree node right away
            openGlRenderOctreeLeaf(camera, child, renderPatchCallback, renderOptions);
        }
    }
    n = i; // Number of compound children

    // cull the non-leaf octree children geoms
    for ( i = 0; i < n; i++ ) {
        if ( openGlViewCullBounds(camera, &octree_children[i].geometry->boundingBox) ) {
            octree_children[i].geometry = nullptr; // culled
            octree_children[i].distance = HUGE_FLOAT;
        } else {
            // Not culled, compute distance from eye to midpoint of child
            octree_children[i].distance = openGlBoundsDistance2(
                camera->eyePosition, octree_children[i].geometry->boundingBox.coordinates);
        }
    }

    // Render children geometries, front to back order
    remaining = n;
    while ( remaining > 0 ) {
        // Find the closest remaining child
        int closest = 0;
        for ( i = 1; i < n; i++ ) {
            if ( octree_children[i].distance < octree_children[closest].distance ) {
                closest = i;
            }
        }

        if ( !octree_children[closest].geometry ) {
            break;
        }

        // render it
        openGlRenderOctreeNonLeaf(camera, octree_children[closest].geometry, renderPatchCallback, renderOptions);

        // remove it from the list
        octree_children[closest].geometry = nullptr;
        octree_children[closest].distance = HUGE_FLOAT;
        remaining--;
    }
    delete children;
}

/**
Traverses the patches in the scene in such a way to obtain
hierarchical view frustum culling + sorted (large patches first +
near to far) rendering. For every patch that is not culled,
renderPatchCallback is called
*/
void
openGlRenderWorldOctree(
    const Scene *scene,
    void (*renderPatchCallback)(Patch *, const Camera *, const RenderOptions *),
    const RenderOptions *renderOptions)
{
    if ( scene->clusteredRootGeometry == nullptr ) {
        return;
    }
    if ( renderPatchCallback == nullptr ) {
        renderPatchCallback = openGlRenderPatch;
    }
    if ( scene->clusteredRootGeometry->isCompound() ) {
        openGlRenderOctreeNonLeaf(scene->camera, scene->clusteredRootGeometry, renderPatchCallback, renderOptions);
    } else {
        openGlRenderOctreeLeaf(scene->camera, scene->clusteredRootGeometry, renderPatchCallback, renderOptions);
    }
}

static void
openGlGeometryDeleteDLists(Geometry *geometry, Geometry *clusteredWorldGeometry) {
    if ( geometry->displayListId >= 0 ) {
        glDeleteLists(geometry->displayListId, 1);
    }
    geometry->displayListId = -1;

    if ( clusteredWorldGeometry->isCompound() ) {
        java::ArrayList<Geometry *> *children = geomPrimListCopy(geometry);
        for ( int i = 0; children != nullptr && i < children->size(); i++ ) {
            openGlGeometryDeleteDLists(children->get(i), clusteredWorldGeometry);
        }
        delete children;
    }
}


static void
openGlRenderNewOctreeDisplayLists(Geometry *clusteredWorldGeometry) {
    if ( clusteredWorldGeometry ) {
        openGlGeometryDeleteDLists(clusteredWorldGeometry, clusteredWorldGeometry);
    }
}

/**
Indicates that the scene has modified, so a new display list should be
compiled and rendered from now on. Only relevant when using display lists
*/
void
openGlRenderNewDisplayList(Geometry *clusteredWorldGeometry, const RenderOptions *renderOptions) {
    if ( globalDisplayListId >= 0 ) {
        glDeleteLists(globalDisplayListId, 1);
    }
    globalDisplayListId = -1;

    if ( renderOptions->frustumCulling ) {
        openGlRenderNewOctreeDisplayLists(clusteredWorldGeometry);
    }
}

static void
openGlReallyRender(const Scene *scene, const RadianceMethod *radianceMethod, const RenderOptions *renderOptions) {
    glPushMatrix();
    glRotated(GLOBAL_render_glutDebugState.angle, 0, 0, 1);
    if ( radianceMethod != nullptr ) {
        radianceMethod->renderScene(scene, renderOptions);
    } else if ( renderOptions->frustumCulling ) {
        openGlRenderWorldOctree(scene, openGlRenderPatch, renderOptions);
    } else {
        for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
            openGlRenderPatch(scene->patchList->get(i), scene->camera, renderOptions);
        }
    }
    glPopMatrix();
}

static void
openGlRenderRadiance(const Scene *scene, const RadianceMethod *radianceMethod, const RenderOptions *renderOptions) {
    if ( renderOptions->smoothShading ) {
        glShadeModel(GL_SMOOTH);
    } else {
        glShadeModel(GL_FLAT);
    }

    openGlRenderSetCamera(scene->camera, scene->geometryList);

    if ( renderOptions->backfaceCulling ) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }

    openGlReallyRender(scene, radianceMethod, renderOptions);

    if ( renderOptions->drawBoundingBoxes ) {
        renderBoundingBoxHierarchy(scene->camera, scene->geometryList, renderOptions);
    }

    if ( renderOptions->drawClusters ) {
        renderClusterHierarchy(scene->camera, scene->clusteredGeometryList, renderOptions);
    }
}

/**
Renders the whole scene
*/
void
openGlRenderScene(
    const Scene *scene,
    int (*reDisplayCallback)(),
    const RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions)
{
    openGlRenderSetLineWidth(renderOptions->lineWidth);

    canvasPushMode();

    if ( !renderOptions->renderRayTracedImage || !openGlRenderRayTraced(reDisplayCallback) ) {
        openGlRenderRadiance(scene, radianceMethod, renderOptions);
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
openGlRenderPixels(const Camera *camera, int x, int y, int width, int height, const ColorRgb *rgb) {
    int rowLength;

    // Length of one row of RGBA image data rounded up to a multiple of 8
    rowLength = (int)((4 * width * sizeof(GLubyte) + 7) & ~7);
    GLubyte *c = new GLubyte[height * rowLength + 8];

    for ( int j = 0; j < height; j++ ) {
        const ColorRgb *rgbP = &rgb[j * width];

        GLubyte *p = c + j * rowLength; // Let each line start on an 8-byte boundary
        for ( int i = 0; i < width; i++, rgbP++ ) {
            ColorRgb corrected_rgb = *rgbP;
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
    glOrtho(0, camera->xSize, 0, camera->ySize, -1.0, 1.0);

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

/**
Patch ID rendering. Returns an array of size (*x)*(*y) containing the IDs of
the patches visible through each pixel or 0 if the background is visible through
the pixel. x is normally the width and y the height of the canvas window
*/
unsigned long *
sglRenderIds(long *x, long *y, const Scene *scene, const RenderOptions *renderOptions) {
    return softRenderIds(x, y, scene, renderOptions);
}

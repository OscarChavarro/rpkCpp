#include <GL/glu.h>
#include <GL/osmesa.h>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "scene/RadianceMethod.h"
#include "tonemap/ToneMap.h"
#include "render/canvas.h"
#include "render/softids.h"
#include "render/opengl.h"
#include "render/render.h"

class OctreeChild {
  public:
    Geometry *geometry;
    float distance;
};

static int globalDisplayListId = -1;

void
openGlRenderClearWindow(const Camera *camera) {
    glClearColor(camera->background.r, camera->background.g, camera->background.b, 0.0);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
    GLubyte *imageBuffer = new GLubyte[width * height * 4];

    OSMesaContext osMesaContext = OSMesaCreateContext(OSMESA_RGBA, nullptr);
    if ( !osMesaContext ) {
        logFatal(1, nullptr, "Couldn't create Mesa offscreen rendering context");
    }

    if ( !OSMesaMakeCurrent(osMesaContext, imageBuffer, GL_UNSIGNED_BYTE, width, height) ) {
        logFatal(1, nullptr, "Couldn't bind Mesa offscreen rendering context to image buffer of size %d x %d", width, height);
    }

    openGlInitState(camera);
    OSMesaDestroyContext(osMesaContext);
    delete[] imageBuffer;
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

static void
openGlReallyRenderOctreeLeaf(
    const Camera *camera,
    Geometry *geometry,
    void (*renderPatch)(const Patch *, const Camera *, const RenderOptions *),
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
    void (*renderPatchCallback)(const Patch *, const Camera *, const RenderOptions *),
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

    return d.norm2();
}

/**
Geometry is a surface or a compound with 1 surface and up to 8 compound children geometries,
clusteredWorldGeom is such a geometry e.g.
*/
static void
openGlRenderOctreeNonLeaf(
    Camera *camera,
    const Geometry *geometry,
    void (*renderPatchCallback)(const Patch *, const Camera *, const RenderOptions *renderOptions),
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
            octree_children[i].distance = Numeric::HUGE_FLOAT_VALUE;
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
        octree_children[closest].distance = Numeric::HUGE_FLOAT_VALUE;
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
    void (*renderPatchCallback)(const Patch *, const Camera *, const RenderOptions *),
    const RenderOptions *renderOptions)
{
    if ( scene->clusteredRootGeometry == nullptr ) {
        return;
    }
    if ( renderPatchCallback == nullptr ) {
        renderPatchCallback = openGlRenderPatchCallBack;
    }
    if ( scene->clusteredRootGeometry->isCompound() ) {
        openGlRenderOctreeNonLeaf(scene->camera, scene->clusteredRootGeometry, renderPatchCallback, renderOptions);
    } else {
        openGlRenderOctreeLeaf(scene->camera, scene->clusteredRootGeometry, renderPatchCallback, renderOptions);
    }
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

/**
Renders the all the patches using default colors
*/
void
openGlRenderPatchCallBack(const Patch *patch, const Camera *camera, const RenderOptions *renderOptions) {
    if ( !renderOptions->noShading ) {
        if ( renderOptions->smoothShading ) {
            openGlRenderPatchSmooth(patch);
        } else {
            openGlRenderPatchFlat(patch);
        }
    }

    if ( renderOptions->drawOutlines &&
         (patch->normal.dotProduct(camera->eyePosition) + patch->planeConstant > Numeric::EPSILON) ) {
        openGlRenderSetColor(&renderOptions->outlineColor);
        openGlRenderPatchOutline(patch);
    }
}

#ifdef RAYTRACING_ENABLED
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
#endif

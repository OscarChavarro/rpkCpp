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
#include "IMAGE/tonemapping.h"
#include "raycasting/simple/RayCaster.h"

static int displayListId = -1;
static int openglInitialized = false;
static int do_render_normals;
static GLubyte *background_ptr = nullptr;
static GLuint backgroundTex = 0;

extern CAMERA GLOBAL_camera_alternateCamera;

void
RenderClearWindow() {
    glClearColor(GLOBAL_camera_mainCamera.background.r, GLOBAL_camera_mainCamera.background.g, GLOBAL_camera_mainCamera.background.b, 0.0);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
RenderSetCamera() {
    RenderClearWindow();

    // use full viewport
    glViewport(0, 0, GLOBAL_camera_mainCamera.hres, GLOBAL_camera_mainCamera.vres);

    // draw backgroudn when needed
    if ( renderopts.use_background && background_ptr ) {
        RenderBackground(&GLOBAL_camera_mainCamera);
    }

    // determine distance to front- and backclipping plane
    RenderGetNearFar(&GLOBAL_camera_mainCamera.near, &GLOBAL_camera_mainCamera.far);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(GLOBAL_camera_mainCamera.vfov * 2.0,
                   (float)GLOBAL_camera_mainCamera.hres / (float)GLOBAL_camera_mainCamera.vres,
                   GLOBAL_camera_mainCamera.near,
                   GLOBAL_camera_mainCamera.far);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(GLOBAL_camera_mainCamera.eyep.x, GLOBAL_camera_mainCamera.eyep.y, GLOBAL_camera_mainCamera.eyep.z,
              GLOBAL_camera_mainCamera.lookp.x, GLOBAL_camera_mainCamera.lookp.y, GLOBAL_camera_mainCamera.lookp.z,
              GLOBAL_camera_mainCamera.updir.x, GLOBAL_camera_mainCamera.updir.y, GLOBAL_camera_mainCamera.updir.z);
}

static void
OpenGLInitState() {
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);

    glDrawBuffer(GL_FRONT_AND_BACK);

    RenderClearWindow();
    glFinish();

    openglInitialized = true;
    displayListId = -1;

    do_render_normals = false;
}

/*
creates an offscreen window for rendering
*/
void
RenderCreateOffscreenWindow(int hres, int vres) {
    GLubyte *image_buffer = (GLubyte *)malloc(hres * vres * sizeof(GLubyte) * 4);

    OSMesaContext osctx = OSMesaCreateContext(OSMESA_RGBA, nullptr);
    if ( !osctx ) {
        Fatal(1, nullptr, "Couldn't create Mesa offscreen rendering context");
    }

    if ( !OSMesaMakeCurrent(osctx, image_buffer, GL_UNSIGNED_BYTE, hres, vres)) {
        Fatal(1, nullptr, "Couldn't bind Mesa offscreen rendering context to image buffer of size %d x %d", hres, vres);
    }

    OpenGLInitState();
}

int
RenderInitialized() {
    return openglInitialized;
}

void RenderLine(Vector3D *x, Vector3D *y) {
    Vector3D X;
    Vector3D Y;
    Vector3D dir;

    glBegin(GL_LINES);

    // move the line a bit closer to the eyepoint to avoid Z buffer artefacts
    VECTORSUBTRACT(GLOBAL_camera_mainCamera.eyep, *x, dir);
    VECTORSUMSCALED(*x, 0.01, dir, X);
    glVertex3fv((GLfloat *) &X);

    VECTORSUBTRACT(GLOBAL_camera_mainCamera.eyep, *y, dir);
    VECTORSUMSCALED(*y, 0.01, dir, Y);
    glVertex3fv((GLfloat *) &Y);

    glEnd();
}


void
RenderAALine(Vector3D *x, Vector3D *y) {
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

    RenderLine(x, y);

    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
}

/*
sets the current color
*/
void
RenderSetColor(RGB *rgb) {
    RGB corrected_rgb;

    corrected_rgb = *rgb;
    RGBGAMMACORRECT(corrected_rgb);
    glColor3fv((GLfloat *) &corrected_rgb);
}

/*
sets line width
*/
void
RenderSetLineWidth(float width) {
    glLineWidth(width);
}

/*
renders a convex polygon flat shaded in the current color
*/
void
RenderPolygonFlat(int nrverts, Vector3D *verts) {
    int i;

    glBegin(GL_POLYGON);
    for ( i = 0; i < nrverts; i++ ) {
        glVertex3fv((GLfloat *) &verts[i]);
    }
    glEnd();
}

/*
renders a convex polygon with Gouraud shading
*/
void
RenderPolygonGouraud(int nrverts, Vector3D *verts, RGB *vertcols) {
    int i;

    glBegin(GL_POLYGON);
    for ( i = 0; i < nrverts; i++ ) {
        RenderSetColor(&vertcols[i]);
        glVertex3fv((GLfloat *) &verts[i]);
    }
    glEnd();
}

void
RenderBeginTriangleStrip() {
    glBegin(GL_TRIANGLE_STRIP);
}

void
RenderNextTrianglePoint(Vector3D *point, RGB *col) {
    if ( col ) {
        RenderSetColor(col);
    }
    glVertex3fv((float *) point);
}

void
RenderEndTriangleStrip() {
    glEnd();
}

void
RenderPatchFlat(PATCH *patch) {
    int i;

    RenderSetColor(&patch->color);
    switch ( patch->nrvertices ) {
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
            for ( i = 0; i < patch->nrvertices; i++ ) {
                glVertex3fv((GLfloat *) patch->vertex[i]->point);
            }
            glEnd();
    }
}

void
RenderPatchSmooth(PATCH *patch) {
    int i;

    switch ( patch->nrvertices ) {
        case 3:
            glBegin(GL_TRIANGLES);
            RenderSetColor(&patch->vertex[0]->color);
            glVertex3fv((GLfloat *) patch->vertex[0]->point);
            RenderSetColor(&patch->vertex[1]->color);
            glVertex3fv((GLfloat *) patch->vertex[1]->point);
            RenderSetColor(&patch->vertex[2]->color);
            glVertex3fv((GLfloat *) patch->vertex[2]->point);
            glEnd();
            break;
        case 4:
            glBegin(GL_QUADS);
            RenderSetColor(&patch->vertex[0]->color);
            glVertex3fv((GLfloat *) patch->vertex[0]->point);
            RenderSetColor(&patch->vertex[1]->color);
            glVertex3fv((GLfloat *) patch->vertex[1]->point);
            RenderSetColor(&patch->vertex[2]->color);
            glVertex3fv((GLfloat *) patch->vertex[2]->point);
            RenderSetColor(&patch->vertex[3]->color);
            glVertex3fv((GLfloat *) patch->vertex[3]->point);
            glEnd();
            break;
        default:
            glBegin(GL_POLYGON);
            for ( i = 0; i < patch->nrvertices; i++ ) {
                RenderSetColor(&patch->vertex[i]->color);
                glVertex3fv((GLfloat *) patch->vertex[i]->point);
            }
            glEnd();
    }
}

/*
renders the patch outline in the current color
*/
void
RenderPatchOutline(PATCH *patch) {
    int i;
    Vector3D tmp;
    Vector3D dir;

    glBegin(GL_LINE_LOOP);
    for ( i = 0; i < patch->nrvertices; i++ ) {
        // move the outlines a bit closer to the eyepoint to avoid Z buffer artefacts
        VECTORSUBTRACT(GLOBAL_camera_mainCamera.eyep, *patch->vertex[i]->point, dir);
        VECTORSUMSCALED(*patch->vertex[i]->point, 0.01, dir, tmp);
        glVertex3fv((GLfloat *) &tmp);
    }
    glEnd();
}

void
RenderPatch(PATCH *patch) {
    if ( !renderopts.no_shading ) {
        if ( renderopts.smooth_shading ) {
            RenderPatchSmooth(patch);
        } else {
            RenderPatchFlat(patch);
        }
    }

    if ( renderopts.draw_outlines &&
         (VECTORDOTPRODUCT(patch->normal, GLOBAL_camera_mainCamera.eyep) + patch->plane_constant > EPSILON
          || renderopts.use_display_lists)) {
        RenderSetColor(&renderopts.outline_color);
        RenderPatchOutline(patch);
    }
}

static void
ReallyRenderOctreeLeaf(Geometry *geom, void (*render_patch)(PATCH *)) {
    PATCHLIST *patchlist = geomPatchList(geom);
    ForAllPatches(P, patchlist)
                {
                    render_patch(P);
                }
    EndForAll;
}

static void
RenderOctreeLeaf(Geometry *geom, void (*render_patch)(PATCH *)) {
    if ( renderopts.use_display_lists ) {
        if ( geom->dlistid <= 0 ) {
            geom->dlistid = geom->id;
            glNewList(geom->dlistid, GL_COMPILE_AND_EXECUTE);
            ReallyRenderOctreeLeaf(geom, render_patch);
            glEndList();
        } else {
            glCallList(geom->dlistid);
        }
    } else {
        ReallyRenderOctreeLeaf(geom, render_patch);
    }
}

static int
ViewCullBounds(float *bounds) {
    int i;
    for ( i = 0; i < NR_VIEW_PLANES; i++ ) {
        if ( BoundsBehindPlane(bounds, &GLOBAL_camera_mainCamera.viewplane[i].norm, GLOBAL_camera_mainCamera.viewplane[i].d)) {
            return true;
        }
    }
    return false;
}

/*
squared distance to midpoint (avoid taking square root)
*/
static float
BoundsDistance2(Vector3D p, float *bounds) {
    Vector3D mid, d;
    VECTORSET(mid,
              0.5 * (bounds[MIN_X] + bounds[MAX_X]),
              0.5 * (bounds[MIN_Y] + bounds[MAX_Y]),
              0.5 * (bounds[MIN_Z] + bounds[MAX_Z]));
    VECTORSUBTRACT(mid, p, d);
    return VECTORNORM2(d);
}

class OctreeChild {
  public:
    Geometry *geom;
    float dist;
};

/*
geom is a surface or a compoint with 1 surface and up to 8 compound children geoms,
CLusetredWorldGeom is such a geom e.g.
*/
static void
RenderOctreeNonLeaf(Geometry *geom, void (*render_patch)(PATCH *)) {
    int i, n, remaining;
    OctreeChild octree_children[8];
    GEOMLIST *children = geomPrimList(geom);

    i = 0;
    ForAllGeoms(child, children)
                {
                    if ( geomIsAggregate(child)) {
                        if ( i >= 8 ) {
                            Error("RenderOctreeNonLeaf", "Invalid octree geom node (more than 8 compound children)");
                            return;
                        }
                        octree_children[i++].geom = child;
                    } else {
                        // render the patches associated with the octree node right away
                        RenderOctreeLeaf(child, render_patch);
                    }
                }
    EndForAll;
    n = i; // nr of compound children

    // cull the non-leaf octree children geoms
    for ( i = 0; i < n; i++ ) {
        if ( ViewCullBounds(octree_children[i].geom->bounds)) {
            octree_children[i].geom = nullptr; // culled
            octree_children[i].dist = HUGE;
        } else {
            // not culled, compute distance from eye to midpoint of child
            octree_children[i].dist = BoundsDistance2(GLOBAL_camera_mainCamera.eyep, octree_children[i].geom->bounds);
        }
    }

    // render children geoms in front to back order
    remaining = n;
    while ( remaining > 0 ) {
        // find closest remaining child
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
        RenderOctreeNonLeaf(octree_children[closest].geom, render_patch);

        // remove it from the list
        octree_children[closest].geom = nullptr;
        octree_children[closest].dist = HUGE;
        remaining--;
    }
}

void
RenderWorldOctree(void (*render_patch)(PATCH *)) {
    if ( !GLOBAL_scene_clusteredWorldGeom ) {
        return;
    }
    if ( !render_patch ) {
        render_patch = RenderPatch;
    }
    if ( geomIsAggregate(GLOBAL_scene_clusteredWorldGeom)) {
        RenderOctreeNonLeaf(GLOBAL_scene_clusteredWorldGeom, render_patch);
    } else {
        RenderOctreeLeaf(GLOBAL_scene_clusteredWorldGeom, render_patch);
    }
}

static void
GeomDeleteDLists(Geometry *geom) {
    if ( geom->dlistid >= 0 ) {
        glDeleteLists(geom->dlistid, 1);
    }
    geom->dlistid = -1;

    if ( geomIsAggregate(GLOBAL_scene_clusteredWorldGeom)) {
        GEOMLIST *children = geomPrimList(geom);
        ForAllGeoms(child, children)
                    {
                        GeomDeleteDLists(child);
                    }
        EndForAll;
    }
}

static void
RenderNewOctreeDisplayLists() {
    if ( GLOBAL_scene_clusteredWorldGeom ) {
        GeomDeleteDLists(GLOBAL_scene_clusteredWorldGeom);
    }
}

void
RenderNewDisplayList() {
    if ( displayListId >= 0 ) {
        glDeleteLists(displayListId, 1);
    }
    displayListId = -1;

    if ( renderopts.frustum_culling ) {
        RenderNewOctreeDisplayLists();
    }
}

void
ReallyRender() {
    if ( Radiance && Radiance->RenderScene ) {
        Radiance->RenderScene();
    } else if ( renderopts.frustum_culling ) {
        RenderWorldOctree(RenderPatch);
    } else {
        PatchListIterate(GLOBAL_scene_patches, RenderPatch);
    }
}

void
RenderRadiance() {
    if ( renderopts.smooth_shading ) {
        glShadeModel(GL_SMOOTH);
    } else {
        glShadeModel(GL_FLAT);
    }

    RenderSetCamera();

    if ( renderopts.backface_culling ) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }

    if ( renderopts.use_display_lists && !renderopts.frustum_culling ) {
        if ( displayListId <= 0 ) {
            displayListId = 1;
            glNewList(displayListId, GL_COMPILE_AND_EXECUTE);
            // render the scene
            ReallyRender();
            glEndList();
        } else {
            glCallList(1);
        }
    } else {
        // just render the scene
        ReallyRender();
    }

    if ( renderopts.draw_bounding_boxes ) {
        RenderBoundingBoxHierarchy();
    }

    if ( renderopts.draw_clusters ) {
        RenderClusterHierarchy();
    }

    if ( renderopts.draw_cameras ) {
        RenderCameras();
    }
}

void
RenderScene() {
    if ( !openglInitialized ) {
        return;
    }

    RenderSetLineWidth(renderopts.linewidth);

    CanvasPushMode();

    if ( GLOBAL_camera_mainCamera.changed ) {
        renderopts.render_raytraced_image = false;
    }

    if ( !renderopts.render_raytraced_image || !RenderRayTraced()) {
        RenderRadiance();
    }

    // Call installed render hooks, that want to render something in the scene
    RenderHooks();

    glFinish();

    glDrawBuffer(GL_FRONT_AND_BACK);

    CanvasPullMode();

    if ( do_render_normals ) {
        RenderNormals();
    }
}

/*
returns only after all issued graphics commands have been executed
*/
void
RenderFinish() {
    glFinish();
}

/*
Renders an image of m lines of n pixels at column x on row y (= lower
left corner of image, relative to the lower left corner of the window)
*/
void
RenderPixels(int x, int y, int width, int height, RGB *rgb) {
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
            RGBGAMMACORRECT(corrected_rgb);
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
    glOrtho(0, GLOBAL_camera_mainCamera.hres, 0, GLOBAL_camera_mainCamera.vres, -1.0, 1.0);

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

void
SaveScreen(char *fname, FILE *fp, int ispipe) {
    ImageOutputHandle *img;
    long j, x = GLOBAL_camera_mainCamera.hres, y = GLOBAL_camera_mainCamera.vres;
    GLubyte *screen;
    unsigned char *buf;

    // RayCast() saves the current picture in display-mapped (!) real values
    if ( renderopts.trace ) {
        RayCast(fname, fp, ispipe);
        return;
    }

    if ( !fp || !(img = CreateImageOutputHandle(fname, fp, ispipe, x, y))) {
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
        WriteDisplayRGB(img, buf);
    }

    free((char *) buf);
    free((char *) screen);

    DeleteImageOutputHandle(img);
}

unsigned long *
RenderIds(long *x, long *y) {
    return SoftRenderIds(x, y);
}

static void
RenderFrustum(CAMERA *cam) {
    Vector3D c;
    Vector3D P;
    Vector3D Q;
    float camlen = renderopts.camsize;
    float hsiz = camlen * cam->viewdist * cam->tanhfov;
    float vsiz = camlen * cam->viewdist * cam->tanvfov;
    int i;
    int j;
    int maxi = 12;
    int maxj = (int)((float) maxi * vsiz / hsiz);

    VECTORCOMB2(1., cam->eyep, camlen * cam->viewdist, cam->Z, c);

    glDisable(GL_DEPTH_TEST);

    RenderSetColor(&renderopts.camera_color);

    for ( i = 0; i <= maxi; i++ ) {
        VECTORCOMB3(c, (-1.0 + 2.0 * ((float) i / (float) maxi)) * hsiz, cam->X, -vsiz, cam->Y, P);
        VECTORCOMB3(c, (-1.0 + 2.0 * ((float) i / (float) maxi)) * hsiz, cam->X, +vsiz, cam->Y, Q);
        RenderLine(&P, &Q);
    }

    for ( j = 0; j <= maxj; j++ ) {
        VECTORCOMB3(c, -hsiz, cam->X, (-1. + 2. * ((float) j / (float) maxj)) * vsiz, cam->Y, P);
        VECTORCOMB3(c, +hsiz, cam->X, (-1. + 2. * ((float) j / (float) maxj)) * vsiz, cam->Y, Q);
        RenderLine(&P, &Q);
    }

    VECTORCOMB3(c, hsiz, cam->X, -vsiz, cam->Y, Q);
    RenderLine(&cam->eyep, &Q);
    VECTORCOMB3(c, -hsiz, cam->X, -vsiz, cam->Y, Q);
    RenderLine(&cam->eyep, &Q);
    VECTORCOMB3(c, -hsiz, cam->X, vsiz, cam->Y, Q);
    RenderLine(&cam->eyep, &Q);
    VECTORCOMB3(c, hsiz, cam->X, vsiz, cam->Y, Q);
    RenderLine(&cam->eyep, &Q);

    glLineWidth(1);
    glEnable(GL_DEPTH_TEST);
}

void
RenderCameras() {
    RenderFrustum(&GLOBAL_camera_alternateCamera);
}

void
RenderBackground(CAMERA *cam) {
    // turn of Z-buffer
    glDisable(GL_DEPTH_TEST);

    // push matrix and create bogus cam
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, cam->hres, 0, cam->vres);

    // draw background (as texture)
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glBindTexture(GL_TEXTURE_2D, backgroundTex);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 1.0);
    glVertex3f(0, 0, 0.0);
    glTexCoord2f(1.0, 1.0);
    glVertex3f(cam->hres, 0, 0.0);
    glTexCoord2f(1.0, 0.0);
    glVertex3f(cam->hres, cam->vres, 0.0);
    glTexCoord2f(0.0, 0.0);
    glVertex3f(0, cam->vres, 0.0);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    // pop matrix
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    // enable Z-buffer back
    glEnable(GL_DEPTH_TEST);
}

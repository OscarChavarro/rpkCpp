#include "common/RenderOptions.h"
#include "common/Error.h"

#ifdef OPEN_GL_ENABLED
    #ifdef __APPLE__
        #include <OpenGL/glu.h>
    #else
        #include <GL/glu.h>
    #endif

    #include "render/RenderHookList.h"
    #include "render/opengl/visualDebugTools/GlutDebugTools.h"
#endif

#include "java/util/ArrayList.txx"
#include "scene/RadianceMethod.h"
#include "tonemap/ToneMap.h"
#include "render/Canvas.h"
#include "render/opengl/GalerkinOpenGLRenderer.h"
#include "render/OctreeChild.h"
#include "render/OpenGlRenderTraversalCallback.h"
#include "render/opengl/Opengl.h"
#include "render/Render.h"
#include "java/lang/System.h"

#ifdef OPEN_GL_ENABLED
void
Opengl::openGlRenderClearWindow(const Camera *camera) {
    glClearColor(camera->background.r, camera->background.g, camera->background.b, 0.0);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
#endif

/**
Renders a line from point p to point q, for example debugging
*/
void
Opengl::openGlRenderLine(Vector3D *x, Vector3D *y) {
#ifdef OPEN_GL_ENABLED
    glDisable(GL_POLYGON_OFFSET_FILL);

    glBegin(GL_LINES);
        glVertex3fv(reinterpret_cast<GLfloat *>(x));
        glVertex3fv(reinterpret_cast<GLfloat *>(y));
    glEnd();

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
#endif
}

/**
Sets the current color for line or outline drawing
*/
void
Opengl::openGlRenderSetColor(const ColorRgb *rgb, const RenderOptions *renderOptions) {
    if ( renderOptions == nullptr || renderOptions->toneMapOptions == nullptr ) {
        Error::fatal(-1, "Opengl::openGlRenderSetColor", "Tone mapping context not set in render options");
    }

    ColorRgb correctedRgb{};

    correctedRgb = *rgb;
    ToneMap::toneMappingGammaCorrection(correctedRgb, *renderOptions->toneMapOptions);
#ifdef OPEN_GL_ENABLED
    glColor3fv(reinterpret_cast<GLfloat *>(&correctedRgb));
#endif
}

/**
Renders a convex polygon flat shaded in the current color
*/
void
Opengl::openGlRenderPolygonFlat(int numberOfVertices, Vector3D *vertices) {
#ifdef OPEN_GL_ENABLED
    glBegin(GL_POLYGON);
    for ( int i = 0; i < numberOfVertices; i++ ) {
        glVertex3fv(reinterpret_cast<GLfloat *>(&vertices[i]));
    }
    glEnd();
#endif
}

/**
Renders a convex polygon with Gouraud shading
*/
void
Opengl::openGlRenderPolygonGouraud(
    int numberOfVertices,
    Vector3D *vertices,
    const ColorRgb *verticesColors,
    const RenderOptions *renderOptions)
{
#ifdef OPEN_GL_ENABLED
    glBegin(GL_POLYGON);
    for ( int i = 0; i < numberOfVertices; i++ ) {
        Opengl::openGlRenderSetColor(&verticesColors[i], renderOptions);
        glVertex3fv(reinterpret_cast<GLfloat *>(&vertices[i]));
    }
    glEnd();
#endif
}

#ifdef OPEN_GL_ENABLED

Vector3D
Opengl::sceneRotationPivot(const Scene *scene) {
    if ( scene == nullptr ) {
        return Vector3D(0.0f, 0.0f, 0.0f);
    }

    if ( scene->clusteredRootGeometry != nullptr && scene->clusteredRootGeometry->bounded ) {
        return scene->clusteredRootGeometry->boundingBox.center();
    }

    if ( scene->geometryList != nullptr && scene->geometryList->size() > 0 ) {
        BoundingBox sceneBounds;
        Geometry::listBounds(scene->geometryList, &sceneBounds);
        return sceneBounds.center();
    }

    return Vector3D(0.0f, 0.0f, 0.0f);
}

void
Opengl::viewportAxesInWorld(const Scene *scene, Vector3D *axisU, Vector3D *axisV) {
    if ( axisU == nullptr || axisV == nullptr ) {
        return;
    }

    axisU->set(1.0f, 0.0f, 0.0f);
    axisV->set(0.0f, 1.0f, 0.0f);

    if ( scene == nullptr || scene->camera == nullptr ) {
        return;
    }

    const Camera *camera = scene->camera;

    Vector3D cameraU;
    cameraU.copy(camera->X);
    Vector3D cameraV;
    cameraV.copy(camera->Y);

    if ( cameraU.norm2() < Numeric::EPSILON_FLOAT || cameraV.norm2() < Numeric::EPSILON_FLOAT ) {
        Vector3D viewDirection;
        viewDirection.subtraction(camera->lookPosition, camera->eyePosition);
        if ( viewDirection.norm2() < Numeric::EPSILON_FLOAT ) {
            return;
        }
        viewDirection.normalize(Numeric::EPSILON_FLOAT);

        Vector3D upDirection;
        upDirection.copy(camera->upDirection);
        if ( upDirection.norm2() < Numeric::EPSILON_FLOAT ) {
            upDirection.set(0.0f, 0.0f, 1.0f);
        } else {
            upDirection.normalize(Numeric::EPSILON_FLOAT);
        }

        cameraU.crossProduct(viewDirection, upDirection);
        if ( cameraU.norm2() < Numeric::EPSILON_FLOAT ) {
            upDirection.set(0.0f, 1.0f, 0.0f);
            cameraU.crossProduct(viewDirection, upDirection);
        }
        if ( cameraU.norm2() < Numeric::EPSILON_FLOAT ) {
            return;
        }
        cameraU.normalize(Numeric::EPSILON_FLOAT);
        cameraV.crossProduct(viewDirection, cameraU);
    }

    if ( cameraU.norm2() < Numeric::EPSILON_FLOAT || cameraV.norm2() < Numeric::EPSILON_FLOAT ) {
        return;
    }

    cameraU.normalize(Numeric::EPSILON_FLOAT);
    cameraV.normalize(Numeric::EPSILON_FLOAT);

    axisU->copy(cameraU);
    axisV->copy(cameraV);
}

void
Opengl::openGlRenderPatchFlat(const Patch *patch, const RenderOptions *renderOptions) {
    Opengl::openGlRenderSetColor(&patch->color, renderOptions);
    switch ( patch->numberOfVertices ) {
        case 3:
            glBegin(GL_TRIANGLES);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->vertex[0]->point));
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->vertex[1]->point));
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->vertex[2]->point));
            glEnd();
            break;
        case 4:
            glBegin(GL_QUADS);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->vertex[0]->point));
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->vertex[1]->point));
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->vertex[2]->point));
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->vertex[3]->point));
            glEnd();
            break;
        default:
            glBegin(GL_POLYGON);
            for ( int i = 0; i < patch->numberOfVertices; i++ ) {
                glVertex3fv(reinterpret_cast<GLfloat *>(patch->vertex[i]->point));
            }
            glEnd();
    }
}

void
Opengl::openGlRenderPatchSmooth(const Patch *patch, const RenderOptions *renderOptions) {
    switch ( patch->numberOfVertices ) {
        case 3:
            glBegin(GL_TRIANGLES);
            Opengl::openGlRenderSetColor(&patch->vertex[0]->color, renderOptions);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->vertex[0]->point));
            Opengl::openGlRenderSetColor(&patch->vertex[1]->color, renderOptions);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->vertex[1]->point));
            Opengl::openGlRenderSetColor(&patch->vertex[2]->color, renderOptions);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->vertex[2]->point));
            glEnd();
            break;
        case 4:
            glBegin(GL_QUADS);
            Opengl::openGlRenderSetColor(&patch->vertex[0]->color, renderOptions);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->vertex[0]->point));
            Opengl::openGlRenderSetColor(&patch->vertex[1]->color, renderOptions);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->vertex[1]->point));
            Opengl::openGlRenderSetColor(&patch->vertex[2]->color, renderOptions);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->vertex[2]->point));
            Opengl::openGlRenderSetColor(&patch->vertex[3]->color, renderOptions);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->vertex[3]->point));
            glEnd();
            break;
        default:
            glBegin(GL_POLYGON);
            for ( int i = 0; i < patch->numberOfVertices; i++ ) {
                Opengl::openGlRenderSetColor(&patch->vertex[i]->color, renderOptions);
                glVertex3fv(reinterpret_cast<GLfloat *>(patch->vertex[i]->point));
            }
            glEnd();
    }
}
#endif

/**
Renders the patch outline in the current color
*/
void
Opengl::openGlRenderPatchOutline(const Patch *patch) {
#ifdef OPEN_GL_ENABLED
    glBegin(GL_LINE_LOOP);
    for ( int i = 0; i < patch->numberOfVertices; i++ ) {
        glVertex3fv(reinterpret_cast<GLfloat *>(&patch->vertex[i]->point));
    }
    glEnd();
#endif
}

#ifdef OPEN_GL_ENABLED
void
Opengl::openGlInvokeRenderPatch(
    const OpenGlRenderTraversalCallback &renderPatch,
    const Patch *patch,
    const Camera *camera,
    const RenderOptions *renderOptions)
{
    if ( renderPatch.callbackWithData != nullptr ) {
        renderPatch.callbackWithData(patch, camera, renderOptions, renderPatch.callbackData);
    } else if ( renderPatch.callbackWithoutData != nullptr ) {
        renderPatch.callbackWithoutData(patch, camera, renderOptions);
    }
}

void
Opengl::openGlReallyRenderOctreeLeaf(
    const Camera *camera,
    const Geometry *geometry,
    const OpenGlRenderTraversalCallback &renderPatch,
    const RenderOptions *renderOptions)
{
    const java::ArrayList<Patch *> *patchList = Geometry::patchListReference(geometry);
    for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
        Opengl::openGlInvokeRenderPatch(renderPatch, patchList->get(i), camera, renderOptions);
    }
}

void
Opengl::openGlRenderOctreeLeaf(
    const Camera *camera,
    const Geometry *geometry,
    const OpenGlRenderTraversalCallback &renderPatchCallback,
    const RenderOptions *renderOptions)
{
    Opengl::openGlReallyRenderOctreeLeaf(camera, geometry, renderPatchCallback, renderOptions);
}

bool
Opengl::openGlViewCullBounds(const Camera *camera, const BoundingBox *bounds) {
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
float
Opengl::openGlBoundsDistance2(Vector3D p, const BoundingBox *boundingBox) {
    Vector3D mid = boundingBox->center();
    Vector3D d;
    d.subtraction(mid, p);

    return d.norm2();
}

/**
Geometry is a surface or a compound with 1 surface and up to 8 compound children geometries,
clusteredWorldGeom is such a geometry e.g.
*/
void
Opengl::openGlRenderOctreeNonLeaf(
    Camera *camera,
    const Geometry *geometry,
    const OpenGlRenderTraversalCallback &renderPatchCallback,
    const RenderOptions *renderOptions)
{
    OctreeChild octree_children[8];
    java::ArrayList<Geometry *> *children = Geometry::primitiveListCopy(geometry);

    int i = 0;
    for ( int j = 0; children != nullptr && j < children->size(); j++ ) {
        Geometry *child = children->get(j);
        if ( child->isCompound() ) {
            if ( i >= 8 ) {
                Error::error("openGlRenderOctreeNonLeaf", "Invalid octree geometry node (more than 8 compound children)");
                delete children;
                return;
            }
            octree_children[i++].geometry = child;
        } else {
            // Render the patches associated with the octree node right away
            Opengl::openGlRenderOctreeLeaf(camera, child, renderPatchCallback, renderOptions);
        }
    }
    int n = i; // Number of compound children

    // cull the non-leaf octree children geoms
    for ( i = 0; i < n; i++ ) {
        if ( Opengl::openGlViewCullBounds(camera, &octree_children[i].geometry->boundingBox) ) {
            octree_children[i].geometry = nullptr; // culled
            octree_children[i].distance = Numeric::HUGE_FLOAT_VALUE;
        } else {
            // Not culled, compute distance from eye to midpoint of child
            octree_children[i].distance = Opengl::openGlBoundsDistance2(
                camera->eyePosition, &octree_children[i].geometry->boundingBox);
        }
    }

    // Render children geometries, front to back order
    int remaining = n;
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
        Opengl::openGlRenderOctreeNonLeaf(camera, octree_children[closest].geometry, renderPatchCallback, renderOptions);

        // remove it from the list
        octree_children[closest].geometry = nullptr;
        octree_children[closest].distance = Numeric::HUGE_FLOAT_VALUE;
        remaining--;
    }
    delete children;
}
#endif

/**
Traverses the patches in the scene in such a way to obtain
hierarchical view frustum culling + sorted (large patches first +
near to far) rendering. For every patch that is not culled,
renderPatchCallback is called
*/
void
Opengl::openGlRenderWorldOctree(
    const Scene *scene,
    OpenGlRenderPatchCallback renderPatchCallback,
    const RenderOptions *renderOptions)
{
    if ( scene->clusteredRootGeometry == nullptr ) {
        return;
    }
#ifdef OPEN_GL_ENABLED
    OpenGlRenderTraversalCallback callbackContext{};
    if ( renderPatchCallback == nullptr ) {
        renderPatchCallback = Opengl::openGlRenderPatchCallBack;
    }
    callbackContext.callbackWithoutData = renderPatchCallback;
    callbackContext.callbackWithData = nullptr;
    callbackContext.callbackData = nullptr;
    if ( scene->clusteredRootGeometry->isCompound() ) {
        Opengl::openGlRenderOctreeNonLeaf(scene->camera, scene->clusteredRootGeometry, callbackContext, renderOptions);
    } else {
        Opengl::openGlRenderOctreeLeaf(scene->camera, scene->clusteredRootGeometry, callbackContext, renderOptions);
    }
#endif
}

void
Opengl::openGlRenderWorldOctreeWithData(
    const Scene *scene,
    OpenGlRenderPatchCallbackWithData renderPatchCallback,
    void *callbackData,
    const RenderOptions *renderOptions)
{
    if ( scene->clusteredRootGeometry == nullptr || renderPatchCallback == nullptr ) {
        return;
    }
#ifdef OPEN_GL_ENABLED
    OpenGlRenderTraversalCallback callbackContext{};
    callbackContext.callbackWithoutData = nullptr;
    callbackContext.callbackWithData = renderPatchCallback;
    callbackContext.callbackData = callbackData;
    if ( scene->clusteredRootGeometry->isCompound() ) {
        Opengl::openGlRenderOctreeNonLeaf(scene->camera, scene->clusteredRootGeometry, callbackContext, renderOptions);
    } else {
        Opengl::openGlRenderOctreeLeaf(scene->camera, scene->clusteredRootGeometry, callbackContext, renderOptions);
    }
#endif
}

/**
Renders the all the patches using default colors
*/
void
Opengl::openGlRenderPatchCallBack(const Patch *patch, const Camera *camera, const RenderOptions *renderOptions) {
#ifdef OPEN_GL_ENABLED
    if ( !renderOptions->noShading ) {
        if ( renderOptions->smoothShading ) {
            Opengl::openGlRenderPatchSmooth(patch, renderOptions);
        } else {
            Opengl::openGlRenderPatchFlat(patch, renderOptions);
        }
    }

    if ( renderOptions->drawOutlines &&
         (patch->normal.dotProduct(camera->eyePosition) + patch->planeConstant > Numeric::EPSILON) ) {
        Opengl::openGlRenderSetColor(&renderOptions->outlineColor, renderOptions);
        Opengl::openGlRenderPatchOutline(patch);
    }
#endif
}

#ifdef OPEN_GL_ENABLED
/**
Sets line width for outlines, etc
*/
void
Opengl::openGlRenderSetLineWidth(float width) {
    glLineWidth(width);
}

void
Opengl::openGlRenderSetCamera(Camera *camera, const java::ArrayList<Geometry *> *sceneGeometries) {
    Opengl::openGlRenderClearWindow(camera);

    // Use the full viewport
    glViewport(0, 0, camera->xSize, camera->ySize);

    // Determine distance to front- and back-clipping plane
    Render::renderGetNearFar(camera, sceneGeometries);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(camera->verticalFov * 2.0,
                   static_cast<float>(camera->xSize) / static_cast<float>(camera->ySize),
                   camera->near / 10,
                   camera->far * 10);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(camera->eyePosition.x, camera->eyePosition.y, camera->eyePosition.z,
              camera->lookPosition.x, camera->lookPosition.y, camera->lookPosition.z,
              camera->upDirection.x, camera->upDirection.y, camera->upDirection.z);
}

void
Opengl::openGlApplyDebugSceneRotation(const Scene *scene) {
    const bool hasRotation =
        GLOBAL_render_glutDebugState.angleAroundViewportU != 0.0f ||
        GLOBAL_render_glutDebugState.angleAroundViewportV != 0.0f;
    if ( !hasRotation ) {
        return;
    }

    const Vector3D pivot = Opengl::sceneRotationPivot(scene);
    Vector3D axisU;
    Vector3D axisV;
    Opengl::viewportAxesInWorld(scene, &axisU, &axisV);

    glTranslated(pivot.x, pivot.y, pivot.z);
    glRotated(GLOBAL_render_glutDebugState.angleAroundViewportU, axisU.x, axisU.y, axisU.z);
    glRotated(GLOBAL_render_glutDebugState.angleAroundViewportV, axisV.x, axisV.y, axisV.z);
    glTranslated(-pivot.x, -pivot.y, -pivot.z);
}

void
Opengl::openGlReallyRender(const Scene *scene, const RadianceMethod *radianceMethod, const RenderOptions *renderOptions) {
    glPushMatrix();
    Opengl::openGlApplyDebugSceneRotation(scene);
    if ( radianceMethod != nullptr ) {
        if ( radianceMethod->className == GALERKIN ) {
            GalerkinOpenGLRenderer::renderScene(scene, renderOptions);
        } else {
            java::System::err.println("OpenGL supports only rendering of Galerkin patches");
            java::System::exit(1);
        }
    } else if ( renderOptions->frustumCulling ) {
        Opengl::openGlRenderWorldOctree(scene, Opengl::openGlRenderPatchCallBack, renderOptions);
    } else {
        for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
            Opengl::openGlRenderPatchCallBack(scene->patchList->get(i), scene->camera, renderOptions);
        }
    }
    glPopMatrix();
}

void
Opengl::openGlRenderRadiance(const Scene *scene, const RadianceMethod *radianceMethod, const RenderOptions *renderOptions) {
    if ( renderOptions->smoothShading ) {
        glShadeModel(GL_SMOOTH);
    } else {
        glShadeModel(GL_FLAT);
    }

    Opengl::openGlRenderSetCamera(scene->camera, scene->geometryList);

    if ( renderOptions->backfaceCulling ) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }

    Opengl::openGlReallyRender(scene, radianceMethod, renderOptions);

    if ( renderOptions->drawBoundingBoxes ) {
        Render::renderBoundingBoxHierarchy(scene->camera, scene->geometryList, renderOptions);
    }

    if ( renderOptions->drawClusters ) {
        Render::renderClusterHierarchy(scene->camera, scene->clusteredGeometryList, renderOptions);
    }
}
#endif

/**
Renders the whole scene
*/
void
Opengl::openGlRenderScene(
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions)
{
#ifdef OPEN_GL_ENABLED
    Opengl::openGlRenderSetLineWidth(renderOptions->lineWidth);

    Canvas::canvasPushMode();

    if ( !renderOptions->renderRayTracedImage ) {
        Opengl::openGlRenderRadiance(scene, radianceMethod, renderOptions);
    }

    // Call installed render hooks, that want to render something in the scene
    RenderHookList::renderHooks();

    glFinish();

    Canvas::canvasPullMode();
#endif
}

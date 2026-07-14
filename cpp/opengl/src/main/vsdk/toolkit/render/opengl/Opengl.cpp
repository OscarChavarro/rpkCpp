#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/common/logging/Logger.h"

#ifdef OPEN_GL_ENABLED
    #ifdef __APPLE__
        #include <OpenGL/glu.h>
    #else
        #include <GL/glu.h>
    #endif

    #include "vsdk/toolkit/render/RenderHookList.h"
    #include "vsdk/toolkit/render/opengl/visualDebugTools/GlutDebugState.h"
#endif

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/tonemap/ToneMap.h"
#include "vsdk/toolkit/render/Canvas.h"
#include "vsdk/toolkit/render/opengl/GalerkinOpenGLRenderer.h"
#include "vsdk/toolkit/render/OctreeChild.h"
#include "vsdk/toolkit/render/opengl/OpenGlRenderTraversalCallback.h"
#include "vsdk/toolkit/render/opengl/Opengl.h"
#include "vsdk/toolkit/render/opengl/RenderOpenGL.h"
#include "java/lang/System.h"

const ToneMappingContext *Opengl::activeToneMapOptions = nullptr;
static bool openGlMissingToneMapWarningShown = false;

#ifdef OPEN_GL_ENABLED
void
Opengl::openGlRenderClearWindow(const Camera *camera) {
    glClearColor(camera->background.getR(), camera->background.getG(), camera->background.getB(), 0.0);
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
    glPolygonOffset(1.0F, 1.0F);
#endif
}

/**
Sets the current color for line or outline drawing
*/
void
Opengl::openGlRenderSetColor(const ColorRgb *rgb, const RendererConfiguration *renderOptions) {
    (void) renderOptions;
    ColorRgbMutable correctedRgb{};

    correctedRgb = static_cast<ColorRgbMutable>(*rgb);
    if ( Opengl::activeToneMapOptions != nullptr ) {
        ToneMap::toneMappingGammaCorrection(correctedRgb, *Opengl::activeToneMapOptions);
    } else if ( !openGlMissingToneMapWarningShown ) {
        Logger::warning("Opengl::openGlRenderSetColor", "Tone mapping context not set in active scene, using uncorrected color");
        openGlMissingToneMapWarningShown = true;
    }
#ifdef OPEN_GL_ENABLED
    glColor3fv(reinterpret_cast<GLfloat *>(&correctedRgb));
#endif
}

void
Opengl::openGlRenderSetColor(const ColorRgbMutable *rgb, const RendererConfiguration *renderOptions) {
    const ColorRgb color(*rgb);
    Opengl::openGlRenderSetColor(&color, renderOptions);
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
    const ColorRgbMutable *verticesColors,
    const RendererConfiguration *renderOptions)
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
        return Vector3D(0.0F, 0.0F, 0.0F);
    }

    if ( scene->clusteredRootGeometry != nullptr && scene->clusteredRootGeometry->bounded ) {
        return scene->clusteredRootGeometry->boundingBox.center();
    }

    if ( scene->geometryList != nullptr && scene->geometryList->size() > 0 ) {
        AxisAlignedBoundingBox sceneBounds;
        Geometry::listBounds(scene->geometryList, &sceneBounds);
        return sceneBounds.center();
    }

    return Vector3D(0.0F, 0.0F, 0.0F);
}

void
Opengl::viewportAxesInWorld(const Scene *scene, Vector3D *axisU, Vector3D *axisV) {
    if ( axisU == nullptr || axisV == nullptr ) {
        return;
    }

    axisU->set(1.0F, 0.0F, 0.0F);
    axisV->set(0.0F, 1.0F, 0.0F);

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
            upDirection.set(0.0F, 0.0F, 1.0F);
        } else {
            upDirection.normalize(Numeric::EPSILON_FLOAT);
        }

        cameraU.crossProduct(viewDirection, upDirection);
        if ( cameraU.norm2() < Numeric::EPSILON_FLOAT ) {
            upDirection.set(0.0F, 1.0F, 0.0F);
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
Opengl::openGlRenderPatchFlat(const Patch *patch, const RendererConfiguration *renderOptions) {
    Opengl::openGlRenderSetColor(&patch->getColor(), renderOptions);
    switch ( patch->getNumberOfVertices() ) {
        case 3:
            glBegin(GL_TRIANGLES);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->getVertices()[0]->point));
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->getVertices()[1]->point));
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->getVertices()[2]->point));
            glEnd();
            break;
        case 4:
            glBegin(GL_QUADS);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->getVertices()[0]->point));
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->getVertices()[1]->point));
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->getVertices()[2]->point));
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->getVertices()[3]->point));
            glEnd();
            break;
        default:
            glBegin(GL_POLYGON);
            for ( int i = 0; i < patch->getNumberOfVertices(); i++ ) {
                glVertex3fv(reinterpret_cast<GLfloat *>(patch->getVertices()[i]->point));
            }
            glEnd();
    }
}

void
Opengl::openGlRenderPatchSmooth(const Patch *patch, const RendererConfiguration *renderOptions) {
    switch ( patch->getNumberOfVertices() ) {
        case 3:
            glBegin(GL_TRIANGLES);
            Opengl::openGlRenderSetColor(&patch->getVertices()[0]->color, renderOptions);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->getVertices()[0]->point));
            Opengl::openGlRenderSetColor(&patch->getVertices()[1]->color, renderOptions);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->getVertices()[1]->point));
            Opengl::openGlRenderSetColor(&patch->getVertices()[2]->color, renderOptions);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->getVertices()[2]->point));
            glEnd();
            break;
        case 4:
            glBegin(GL_QUADS);
            Opengl::openGlRenderSetColor(&patch->getVertices()[0]->color, renderOptions);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->getVertices()[0]->point));
            Opengl::openGlRenderSetColor(&patch->getVertices()[1]->color, renderOptions);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->getVertices()[1]->point));
            Opengl::openGlRenderSetColor(&patch->getVertices()[2]->color, renderOptions);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->getVertices()[2]->point));
            Opengl::openGlRenderSetColor(&patch->getVertices()[3]->color, renderOptions);
            glVertex3fv(reinterpret_cast<GLfloat *>(patch->getVertices()[3]->point));
            glEnd();
            break;
        default:
            glBegin(GL_POLYGON);
            for ( int i = 0; i < patch->getNumberOfVertices(); i++ ) {
                Opengl::openGlRenderSetColor(&patch->getVertices()[i]->color, renderOptions);
                glVertex3fv(reinterpret_cast<GLfloat *>(patch->getVertices()[i]->point));
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
    for ( int i = 0; i < patch->getNumberOfVertices(); i++ ) {
        glVertex3fv(reinterpret_cast<GLfloat *>(&patch->getVertices()[i]->point));
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
    const RendererConfiguration *renderOptions)
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
    const RendererConfiguration *renderOptions)
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
    const RendererConfiguration *renderOptions)
{
    Opengl::openGlReallyRenderOctreeLeaf(camera, geometry, renderPatchCallback, renderOptions);
}

bool
Opengl::openGlViewCullBounds(const Camera *camera, const AxisAlignedBoundingBox *bounds) {
    for ( int i = 0; i < Camera::NUMBER_OF_VIEW_PLANES; i++ ) {
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
Opengl::openGlBoundsDistance2(Vector3D p, const AxisAlignedBoundingBox *boundingBox) {
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
    const RendererConfiguration *renderOptions)
{
    OctreeChild octree_children[8];
    java::ArrayList<Geometry *> *children = Geometry::primitiveListCopy(geometry);

    int i = 0;
    for ( int j = 0; children != nullptr && j < children->size(); j++ ) {
        Geometry *child = children->get(j);
        if ( child->isCompound() ) {
            if ( i >= 8 ) {
                Logger::error("openGlRenderOctreeNonLeaf", "Invalid octree geometry node (more than 8 compound children)");
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
    const RendererConfiguration *renderOptions)
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

/**
Renders the all the patches using default colors
*/
void
Opengl::openGlRenderPatchCallBack(const Patch *patch, const Camera *camera, const RendererConfiguration *renderOptions) {
#ifdef OPEN_GL_ENABLED
    if ( !renderOptions->isNoShading() ) {
        if ( renderOptions->isSmoothShading() ) {
            Opengl::openGlRenderPatchSmooth(patch, renderOptions);
        } else {
            Opengl::openGlRenderPatchFlat(patch, renderOptions);
        }
    }

    if ( renderOptions->isDrawOutlines() &&
         (patch->getNormal().dotProduct(camera->eyePosition) + patch->getPlaneConstant() > Numeric::EPSILON) ) {
        const ColorRgb &outlineColor = renderOptions->getOutlineColor();
        Opengl::openGlRenderSetColor(&outlineColor, renderOptions);
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
    RenderOpenGL::renderGetNearFar(camera, sceneGeometries);

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
Opengl::openGlApplyDebugSceneRotation(const Scene *scene, const GlutDebugState *debugState) {
    if ( debugState == nullptr ) {
        return;
    }

    const bool hasRotation =
        debugState->angleAroundViewportU != 0.0F ||
        debugState->angleAroundViewportV != 0.0F;
    if ( !hasRotation ) {
        return;
    }

    const Vector3D pivot = Opengl::sceneRotationPivot(scene);
    Vector3D axisU;
    Vector3D axisV;
    Opengl::viewportAxesInWorld(scene, &axisU, &axisV);

    glTranslated(pivot.x, pivot.y, pivot.z);
    glRotated(debugState->angleAroundViewportU, axisU.x, axisU.y, axisU.z);
    glRotated(debugState->angleAroundViewportV, axisV.x, axisV.y, axisV.z);
    glTranslated(-pivot.x, -pivot.y, -pivot.z);
}

void
Opengl::openGlReallyRender(
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RendererConfiguration *renderOptions,
    const GlutDebugState *debugState)
{
    glPushMatrix();
    Opengl::openGlApplyDebugSceneRotation(scene, debugState);
    if ( radianceMethod != nullptr ) {
        if ( radianceMethod->className == GALERKIN ) {
            GalerkinOpenGLRenderer::renderScene(scene, renderOptions, debugState);
        } else {
            java::System::err.println("OpenGL supports only rendering of Galerkin patches");
            java::System::exit(1);
        }
    } else if ( renderOptions->isFrustumCulling() ) {
        Opengl::openGlRenderWorldOctree(scene, Opengl::openGlRenderPatchCallBack, renderOptions);
    } else {
        for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
            Opengl::openGlRenderPatchCallBack(scene->patchList->get(i), scene->camera, renderOptions);
        }
    }
    glPopMatrix();
}

void
Opengl::openGlRenderRadiance(
    const Scene *scene,
    const RadianceMethod *radianceMethod,
    const RendererConfiguration *renderOptions,
    const GlutDebugState *debugState)
{
    if ( renderOptions->isSmoothShading() ) {
        glShadeModel(GL_SMOOTH);
    } else {
        glShadeModel(GL_FLAT);
    }

    Opengl::openGlRenderSetCamera(scene->camera, scene->geometryList);

    if ( renderOptions->isBackfaceCulling() ) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }

    Opengl::openGlReallyRender(scene, radianceMethod, renderOptions, debugState);

    if ( renderOptions->isDrawBoundingBoxes() ) {
        RenderOpenGL::renderBoundingBoxHierarchy(scene->camera, scene->geometryList, renderOptions);
    }

    if ( renderOptions->isDrawClusters() ) {
        RenderOpenGL::renderClusterHierarchy(scene->camera, scene->clusteredGeometryList, renderOptions);
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
    const ToneMappingContext *toneMapOptions,
    const RendererConfiguration *renderOptions,
    const GlutDebugState *debugState)
{
#ifdef OPEN_GL_ENABLED
    if ( scene == nullptr ) {
        Logger::fatal(-1, "Opengl::openGlRenderScene", "Scene not provided");
    }
    Opengl::activeToneMapOptions = toneMapOptions;
    if ( toneMapOptions == nullptr && !openGlMissingToneMapWarningShown ) {
        Logger::warning("Opengl::openGlRenderScene", "Tone mapping context not provided, using uncorrected color");
        openGlMissingToneMapWarningShown = true;
    }

    Opengl::openGlRenderSetLineWidth(renderOptions->getLineWidth());

    Canvas::canvasPushMode();

    if ( !renderOptions->isRenderRayTracedImage() ) {
        Opengl::openGlRenderRadiance(scene, radianceMethod, renderOptions, debugState);
    }

    // Call installed render hooks, that want to render something in the scene
    RenderHookList::renderHooks();

    glFinish();

    Canvas::canvasPullMode();
    Opengl::activeToneMapOptions = nullptr;
#endif
}

#include "java/lang/Float.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
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
Opengl::openGlRenderLine(Vector3Df *x, Vector3Df *y) {
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
        Logger::reportMessage("Opengl", Logger::WARNING, "Opengl::openGlRenderSetColor", "Tone mapping context not set in active scene, using uncorrected color");
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
Opengl::openGlRenderPolygonFlat(int numberOfVertices, Vector3Df *vertices) {
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
    Vector3Df *vertices,
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

Vector3Df
Opengl::sceneRotationPivot(const Scene *scene) {
    if ( scene == nullptr ) {
        return Vector3Df(0.0F, 0.0F, 0.0F);
    }

    if ( scene->clusteredRootGeometry != nullptr && scene->clusteredRootGeometry->bounded ) {
        return scene->clusteredRootGeometry->boundingBox.center();
    }

    if ( scene->geometryList != nullptr && scene->geometryList->size() > 0 ) {
        RpkAxisAlignedBoundingBox sceneBounds;
        RpkGeometry::listBounds(scene->geometryList, &sceneBounds);
        return sceneBounds.center();
    }

    return Vector3Df(0.0F, 0.0F, 0.0F);
}

void
Opengl::viewportAxesInWorld(const Scene *scene, Vector3Df *axisU, Vector3Df *axisV) {
    if ( axisU == nullptr || axisV == nullptr ) {
        return;
    }

    *axisU = Vector3Df(1.0F, 0.0F, 0.0F);
    *axisV = Vector3Df(0.0F, 1.0F, 0.0F);

    if ( scene == nullptr || scene->camera == nullptr ) {
        return;
    }

    const Camera *camera = scene->camera;

    Vector3Df cameraU;
    cameraU = camera->X;
    Vector3Df cameraV;
    cameraV = camera->Y;

    if ( cameraU.lengthSquared() < Numeric::EPSILON_FLOAT || cameraV.lengthSquared() < Numeric::EPSILON_FLOAT ) {
        Vector3Df viewDirection;
        viewDirection = camera->lookPosition.subtract(camera->eyePosition);
        if ( viewDirection.lengthSquared() < Numeric::EPSILON_FLOAT ) {
            return;
        }
        viewDirection = viewDirection.normalized(Numeric::EPSILON_FLOAT);

        Vector3Df upDirection;
        upDirection = camera->upDirection;
        if ( upDirection.lengthSquared() < Numeric::EPSILON_FLOAT ) {
            upDirection = Vector3Df(0.0F, 0.0F, 1.0F);
        } else {
            upDirection = upDirection.normalized(Numeric::EPSILON_FLOAT);
        }

        cameraU = viewDirection.crossProduct(upDirection);
        if ( cameraU.lengthSquared() < Numeric::EPSILON_FLOAT ) {
            upDirection = Vector3Df(0.0F, 1.0F, 0.0F);
            cameraU = viewDirection.crossProduct(upDirection);
        }
        if ( cameraU.lengthSquared() < Numeric::EPSILON_FLOAT ) {
            return;
        }
        cameraU = cameraU.normalized(Numeric::EPSILON_FLOAT);
        cameraV = viewDirection.crossProduct(cameraU);
    }

    if ( cameraU.lengthSquared() < Numeric::EPSILON_FLOAT || cameraV.lengthSquared() < Numeric::EPSILON_FLOAT ) {
        return;
    }

    cameraU = cameraU.normalized(Numeric::EPSILON_FLOAT);
    cameraV = cameraV.normalized(Numeric::EPSILON_FLOAT);

    *axisU = cameraU;
    *axisV = cameraV;
}

void
Opengl::openGlRenderPatchFlat(const Patch *patch, const ColorRgbMutable &color, const RendererConfiguration *renderOptions) {
    Opengl::openGlRenderSetColor(&color, renderOptions);
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
    const RpkGeometry *geometry,
    const OpenGlRenderTraversalCallback &renderPatch,
    const RendererConfiguration *renderOptions)
{
    const java::ArrayList<Patch *> *patchList = RpkGeometry::patchListReference(geometry);
    for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
        Opengl::openGlInvokeRenderPatch(renderPatch, patchList->get(i), camera, renderOptions);
    }
}

void
Opengl::openGlRenderOctreeLeaf(
    const Camera *camera,
    const RpkGeometry *geometry,
    const OpenGlRenderTraversalCallback &renderPatchCallback,
    const RendererConfiguration *renderOptions)
{
    Opengl::openGlReallyRenderOctreeLeaf(camera, geometry, renderPatchCallback, renderOptions);
}

bool
Opengl::openGlViewCullBounds(const Camera *camera, const RpkAxisAlignedBoundingBox *bounds) {
    for ( int i = 0; i < Camera::NUMBER_OF_VIEW_PLANES; i++ ) {
        if ( bounds->behindPlane(camera->viewPlanes[i].normal, camera->viewPlanes[i].d) ) {
            return true;
        }
    }
    return false;
}

/**
Squared distance to midpoint (avoid taking square root)
*/
float
Opengl::openGlBoundsDistance2(Vector3Df p, const RpkAxisAlignedBoundingBox *boundingBox) {
    Vector3Df mid = boundingBox->center();
    Vector3Df d;
    d = mid.subtract(p);

    return d.lengthSquared();
}

/**
Geometry is a surface or a compound with 1 surface and up to 8 compound children geometries,
clusteredWorldGeom is such a geometry e.g.
*/
void
Opengl::openGlRenderOctreeNonLeaf(
    Camera *camera,
    const RpkGeometry *geometry,
    const OpenGlRenderTraversalCallback &renderPatchCallback,
    const RendererConfiguration *renderOptions)
{
    OctreeChild octree_children[8];
    java::ArrayList<RpkGeometry *> *children = RpkGeometry::primitiveListCopy(geometry);

    int i = 0;
    for ( int j = 0; children != nullptr && j < children->size(); j++ ) {
        RpkGeometry *child = children->get(j);
        if ( child->isCompound() ) {
            if ( i >= 8 ) {
                Logger::reportMessage("Opengl", Logger::ERROR, "openGlRenderOctreeNonLeaf", "Invalid octree geometry node (more than 8 compound children)");
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
            octree_children[i].distance = java::Float::MAX_VALUE;
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
        octree_children[closest].distance = java::Float::MAX_VALUE;
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
        // Default rendering: flat or smooth with the scene's face display colors
        callbackContext.callbackWithoutData = nullptr;
        callbackContext.callbackWithData = Opengl::openGlRenderPatchCallBack;
        callbackContext.callbackData = scene->getFaceColors();
    } else {
        callbackContext.callbackWithoutData = renderPatchCallback;
        callbackContext.callbackWithData = nullptr;
        callbackContext.callbackData = nullptr;
    }
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
Opengl::openGlRenderPatchCallBack(const Patch *patch, const Camera *camera, const RendererConfiguration *renderOptions, void *faceColors) {
#ifdef OPEN_GL_ENABLED
    if ( !renderOptions->isNoShading() ) {
        if ( renderOptions->isSmoothShading() ) {
            Opengl::openGlRenderPatchSmooth(patch, renderOptions);
        } else {
            const FaceColorTable *colors = static_cast<const FaceColorTable *>(faceColors);
            Opengl::openGlRenderPatchFlat(patch, colors->get(patch->getFaceIndex()), renderOptions);
        }
    }

    if ( renderOptions->isDrawOutlines() &&
         (patch->getNormal().dotProduct(camera->eyePosition) + patch->getPlaneConstant() > VSDK::EPSILON) ) {
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
Opengl::openGlRenderSetCamera(Camera *camera, const java::ArrayList<RpkGeometry *> *sceneGeometries) {
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
    gluLookAt(camera->eyePosition.x(), camera->eyePosition.y(), camera->eyePosition.z(),
              camera->lookPosition.x(), camera->lookPosition.y(), camera->lookPosition.z(),
              camera->upDirection.x(), camera->upDirection.y(), camera->upDirection.z());
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

    const Vector3Df pivot = Opengl::sceneRotationPivot(scene);
    Vector3Df axisU;
    Vector3Df axisV;
    Opengl::viewportAxesInWorld(scene, &axisU, &axisV);

    glTranslated(pivot.x(), pivot.y(), pivot.z());
    glRotated(debugState->angleAroundViewportU, axisU.x(), axisU.y(), axisU.z());
    glRotated(debugState->angleAroundViewportV, axisV.x(), axisV.y(), axisV.z());
    glTranslated(-pivot.x(), -pivot.y(), -pivot.z());
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
        Opengl::openGlRenderWorldOctree(scene, nullptr, renderOptions);
    } else {
        for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
            Opengl::openGlRenderPatchCallBack(scene->patchList->get(i), scene->camera, renderOptions, scene->getFaceColors());
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
        Logger::reportMessage("Opengl", Logger::FATAL_ERROR, "Opengl::openGlRenderScene", "Scene not provided");
    }
    Opengl::activeToneMapOptions = toneMapOptions;
    if ( toneMapOptions == nullptr && !openGlMissingToneMapWarningShown ) {
        Logger::reportMessage("Opengl", Logger::WARNING, "Opengl::openGlRenderScene", "Tone mapping context not provided, using uncorrected color");
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

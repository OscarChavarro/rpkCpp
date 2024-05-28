#include <GL/glu.h>

#include "java/util/ArrayList.txx"
#include "render/canvas.h"
#include "render/renderhook.h"
#include "render/render.h"
#include "render/opengl.h"
#include "render/glutDebugTools.h"
#include "raycasting/render/RayTracingRenderer.h"

/**
Sets line width for outlines, etc
*/
static void
openGlRenderSetLineWidth(float width) {
    glLineWidth(width);
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
openGlReallyRender(const Scene *scene, const RadianceMethod *radianceMethod, const RenderOptions *renderOptions) {
    glPushMatrix();
    glRotated(GLOBAL_render_glutDebugState.angle, 0, 0, 1);
    if ( radianceMethod != nullptr ) {
        radianceMethod->renderScene(scene, renderOptions);
    } else if ( renderOptions->frustumCulling ) {
        openGlRenderWorldOctree(scene, openGlRenderPatchCallBack, renderOptions);
    } else {
        for ( int i = 0; scene->patchList != nullptr && i < scene->patchList->size(); i++ ) {
            openGlRenderPatchCallBack(scene->patchList->get(i), scene->camera, renderOptions);
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
    const RayTracer *rayTracer,
    const RadianceMethod *radianceMethod,
    const RenderOptions *renderOptions)
{
    openGlRenderSetLineWidth(renderOptions->lineWidth);

    canvasPushMode();

    if ( !renderOptions->renderRayTracedImage || rayTracer == nullptr ) {
        openGlRenderRadiance(scene, radianceMethod, renderOptions);
    }

    // Call installed render hooks, that want to render something in the scene
    renderHooks();

    glFinish();

    glDrawBuffer(GL_FRONT_AND_BACK);

    canvasPullMode();
}

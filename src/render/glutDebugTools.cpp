#include <cstdio>
#include <cstdlib>

#define GL_SILENCE_DEPRECATION

#include <GL/glut.h>

#include "java/util/ArrayList.h"
#include "common/RenderOptions.h"
#include "render/opengl.h"
#include "render/glutDebugTools.h"

static int globalWidth = 1920;
static int globalHeight = 1200;
static java::ArrayList<Patch *> *globalScenePatches;
static java::ArrayList<Patch *> *globalLightPatches;
static java::ArrayList<Geometry *> *globalSceneGeometries;
static java::ArrayList<Geometry *> *globalSceneClusteredGeometries;
static Geometry *globalClusteredWorldGeom;
static Background *globalSceneBackground;
static RadianceMethod *globalRadianceMethod;
static VoxelGrid *globalVoxelGrid;

GlutDebugState GLOBAL_render_glutDebugState;

GlutDebugState::GlutDebugState(){
    showSelectedPathOnly = false;
    selectedPatch = 0;
    angle = 0;
}

static void
resizeCallback(int newWidth, int newHeight) {
    globalWidth = newWidth;
    globalHeight = newHeight;
}

static void
keypressCallback(unsigned char keyChar, int /*x*/, int /*y*/) {
    switch ( keyChar ) {
        case 27:
            exit(1);
        case '0':
            if ( GLOBAL_render_glutDebugState.showSelectedPathOnly ) {
                GLOBAL_render_glutDebugState.showSelectedPathOnly = false;
            } else {
                GLOBAL_render_glutDebugState.showSelectedPathOnly = true;
            }
            break;
        case '1':
            GLOBAL_render_glutDebugState.selectedPatch--;
            if ( GLOBAL_render_glutDebugState.selectedPatch < 0 ) {
                GLOBAL_render_glutDebugState.selectedPatch = 0;
            }
            break;
        case '2':
            GLOBAL_render_glutDebugState.selectedPatch++;
            if ( GLOBAL_render_glutDebugState.selectedPatch >= globalScenePatches->size() ) {
                GLOBAL_render_glutDebugState.selectedPatch = (int)globalScenePatches->size() - 1;
            }
            break;
        case ' ':
            globalRadianceMethod->doStep(
                globalSceneBackground,
                globalScenePatches,
                globalSceneGeometries,
                globalSceneClusteredGeometries,
                globalLightPatches,
                globalClusteredWorldGeom,
                globalVoxelGrid);
            break;
        default:
            return;
    }

    if ( GLOBAL_render_glutDebugState.showSelectedPathOnly ) {
        printf("Selected patch: %d\n", GLOBAL_render_glutDebugState.selectedPatch);
    } else {
        printf("Selected patch: ALL\n");
    }

    glutPostRedisplay();
}

static void
extendedKeypressCallback(int keyCode, int /*x*/, int /*y*/) {
    switch ( keyCode ) {
        case GLUT_KEY_LEFT:
            GLOBAL_render_glutDebugState.angle += 1.0;
            break;
        case GLUT_KEY_RIGHT:
            GLOBAL_render_glutDebugState.angle -= 1.0;
            break;
        default:
            return;
    }

    glutPostRedisplay();
}

static void
drawCallback() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BITS);
    glEnable(GL_DEPTH_TEST);

    GLOBAL_camera_mainCamera.xSize = globalWidth;
    GLOBAL_camera_mainCamera.ySize = globalHeight;

    glViewport(0, 0, globalWidth, globalHeight);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINES);
    openGlRenderScene(
        globalScenePatches,
        globalSceneClusteredGeometries,
        globalSceneGeometries,
        globalClusteredWorldGeom,
        nullptr,
        globalRadianceMethod);
    glutSwapBuffers();
}

void
executeGlutGui(
    int argc,
    char *argv[],
    java::ArrayList<Patch *> *scenePatches,
    java::ArrayList<Patch *> *lightPatches,
    java::ArrayList<Geometry *> *sceneGeometries,
    java::ArrayList<Geometry *> *sceneClusteredGeometries,
    Background *sceneBackground,
    Geometry *clusteredWorldGeom,
    RadianceMethod *radianceMethod,
    VoxelGrid *voxelGrid)
{
    globalLightPatches = lightPatches;
    globalScenePatches = scenePatches;
    globalRadianceMethod = radianceMethod;
    globalSceneGeometries = sceneGeometries;
    globalSceneClusteredGeometries = sceneClusteredGeometries;
    globalClusteredWorldGeom = clusteredWorldGeom;
    globalSceneBackground = sceneBackground;
    globalVoxelGrid = voxelGrid;

    glutInit(&argc, argv);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(globalWidth, globalHeight);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    int windowHandle = glutCreateWindow("RPK");
    if ( windowHandle == GL_FALSE ) {
        printf("ERROR: Can not open GLUT window, check X11 setup!\n");
        exit(1);
    }

    GLOBAL_render_renderOptions.frustumCulling = false;

    glutReshapeFunc(resizeCallback);
    glutKeyboardFunc(keypressCallback);
    glutSpecialFunc(extendedKeypressCallback);
    glutDisplayFunc(drawCallback);
    glutMainLoop();
}

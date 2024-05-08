#include <cstdio>
#include <cstdlib>

#define GL_SILENCE_DEPRECATION

#include <GL/glut.h>

#include "java/util/ArrayList.txx"
#include "common/RenderOptions.h"
#include "render/opengl.h"
#include "render/glutDebugTools.h"

static int globalWidth = 1920;
static int globalHeight = 1200;
static Scene *globalScene;
static RadianceMethod *globalRadianceMethod;
static RenderOptions *globalRenderOptions;

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
            if ( GLOBAL_render_glutDebugState.selectedPatch >= globalScene->patchList->size() ) {
                GLOBAL_render_glutDebugState.selectedPatch = (int)globalScene->patchList->size() - 1;
            }
            break;
        case ' ':
            globalRadianceMethod->doStep(globalScene, globalRenderOptions);
            break;
        case 'p':
            globalScene->print();
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
        case GLUT_KEY_F2:
            globalRenderOptions->drawOutlines = !globalRenderOptions->drawOutlines;
            break;
        case GLUT_KEY_LEFT:
            GLOBAL_render_glutDebugState.angle += 1.0f;
            break;
        case GLUT_KEY_RIGHT:
            GLOBAL_render_glutDebugState.angle -= 1.0f;
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

    globalScene->camera->xSize = globalWidth;
    globalScene->camera->ySize = globalHeight;

    glViewport(0, 0, globalWidth, globalHeight);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINES);
    openGlRenderScene(globalScene, nullptr, globalRadianceMethod, globalRenderOptions);
    glutSwapBuffers();
}

void
executeGlutGui(
    int argc,
    char *argv[],
    Scene *scene,
    RadianceMethod *radianceMethod,
    RenderOptions *renderOptions)
{
    globalScene = scene;
    globalRadianceMethod = radianceMethod;
    globalRenderOptions = renderOptions;

    glutInit(&argc, argv);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(globalWidth, globalHeight);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    int windowHandle = glutCreateWindow("RPK");
    if ( windowHandle == GL_FALSE ) {
        printf("ERROR: Can not open GLUT window, check X11 setup!\n");
        exit(1);
    }

    renderOptions->frustumCulling = false;

    glutReshapeFunc(resizeCallback);
    glutKeyboardFunc(keypressCallback);
    glutSpecialFunc(extendedKeypressCallback);
    glutDisplayFunc(drawCallback);
    glutMainLoop();
}

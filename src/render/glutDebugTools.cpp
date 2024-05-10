#include <cstdio>
#include <cstdlib>

#define GL_SILENCE_DEPRECATION

#include <GL/glut.h>

#include "java/util/ArrayList.txx"
#include "common/RenderOptions.h"
#include "render/opengl.h"
#include "render/glutDebugTools.h"
#include "GALERKIN/GalerkinElement.h"

static int globalWidth = 1920;
static int globalHeight = 1200;
static Scene *globalScene;
static RadianceMethod *globalRadianceMethod;
static RenderOptions *globalRenderOptions;
static void (*globalMemoryFreeCallBack)(MgfContext *mgfContext);
static MgfContext *globalMgfContext;

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
printElementHierarchy(const GalerkinElement *element, int level) {
    switch ( level ) {
        case 0:
            break;
        case 1:
            printf("  - ");
            break;
        case 2:
            printf("    . ");
            break;
        default:
            printf("      (%d) -> ", level);
            break;
    }
    const ColorRgb *c = element->radiance;
    long int numberOfInteractions = 0;
    if ( element->interactions != nullptr ) {
        numberOfInteractions = element->interactions->size();
    }

    if ( element->regularSubElements == nullptr ) {
        if ( c == nullptr ) {
            printf("Child element no radiance\n");
        } else {
            printf("Child element radiance <%0.4f, %0.4f, %0.4f>, interactions: %ld\n",
               c->r, c->g, c->b, numberOfInteractions);
        }
    } else {
        if ( c == nullptr ) {
            printf("Container element no radiance\n");
        } else {
            printf("Container element radiance <%0.4f, %0.4f, %0.4f>, interactions: %ld\n",
               c->r, c->g, c->b, numberOfInteractions);
        }
        for ( int i = 0; i < 4; i++ ) {
            const GalerkinElement *child = (GalerkinElement *)element->regularSubElements[i];
            if ( child != nullptr ) {
                printElementHierarchy(child, level + 1);
            }
        }
    }
}

static void
printGalerkinElementForPatch(const Scene *scene, int patchIndex) {
    printf("================================================================================\n");
    if ( scene->patchList == nullptr || patchIndex >= scene->patchList->size() ) {
        return;
    }
    const Patch *patch = scene->patchList->get(patchIndex);
    if  ( patch == nullptr || patch->radianceData == nullptr ) {
        return;
    }
    const GalerkinElement *element = galerkinGetElement(patch);
    printf("Galerkin element for patch[%d] %d\n", patchIndex, patch->id);
    printElementHierarchy(element, 0);
}

static void
keypressCallback(unsigned char keyChar, int /*x*/, int /*y*/) {
    switch ( keyChar ) {
        case 27:
            globalMemoryFreeCallBack(globalMgfContext);
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
        case 'e':
            printGalerkinElementForPatch(globalScene, GLOBAL_render_glutDebugState.selectedPatch);
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
        case GLUT_KEY_F3:
            globalRenderOptions->drawSurfaces = !globalRenderOptions->drawSurfaces;
            break;
        case GLUT_KEY_F4:
            globalRenderOptions->drawBoundingBoxes = !globalRenderOptions->drawBoundingBoxes;
            break;
        case GLUT_KEY_F5:
            globalRenderOptions->drawClusters = !globalRenderOptions->drawClusters;
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
    RenderOptions *renderOptions,
    void (*memoryFreeCallBack)(MgfContext *mgfContext),
    MgfContext *mgfContext)
{
    globalScene = scene;
    globalRadianceMethod = radianceMethod;
    globalRenderOptions = renderOptions;
    globalMemoryFreeCallBack = memoryFreeCallBack;
    globalMgfContext = mgfContext;

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

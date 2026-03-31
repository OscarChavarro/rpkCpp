#include "render/visualDebugTools/GlutDebugTools.h"
#include "render/visualDebugTools/GlutDebugToolsModel.h"
#include "render/visualDebugTools/GlutDebugToolsKeyControl.h"
#include "render/visualDebugTools/GlutDebugToolsMouseControl.h"
#include "render/visualDebugTools/GlutDebugPatchHierarchy.h"
#include "render/visualDebugTools/GlutHudConsole.h"
#include "galerkin/GalerkinElement.h"

#ifdef OPEN_GL_ENABLED


#ifdef __APPLE__
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

#include <cstdio>

#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "render/Opengl.h"

static GlutDebugToolsModel globalModel;
static bool globalFullScreenApplied = false;

void
GlutDebugTools::resizeCallback(int newWidth, int newHeight) {
    if ( newWidth <= 0 || newHeight <= 0 ) {
        return;
    }
    globalModel.width = newWidth;
    globalModel.height = newHeight;
    if ( !globalModel.fullScreen ) {
        globalModel.windowedWidth = newWidth;
        globalModel.windowedHeight = newHeight;
    }
}

void
GlutDebugTools::syncModelWindowSizeFromGlut() {
    const int currentWidth = glutGet(GLUT_WINDOW_WIDTH);
    const int currentHeight = glutGet(GLUT_WINDOW_HEIGHT);
    if ( currentWidth <= 0 || currentHeight <= 0 ) {
        return;
    }

    globalModel.width = currentWidth;
    globalModel.height = currentHeight;
    if ( !globalModel.fullScreen ) {
        globalModel.windowedWidth = currentWidth;
        globalModel.windowedHeight = currentHeight;
    }
}

void
GlutDebugTools::syncCameraToViewport() {
    if ( globalModel.scene == nullptr || globalModel.scene->camera == nullptr ) {
        return;
    }
    if ( globalModel.width <= 0 || globalModel.height <= 0 ) {
        return;
    }

    Camera *camera = globalModel.scene->camera;
    if ( camera->xSize == globalModel.width &&
         camera->ySize == globalModel.height &&
         camera->pixelWidth > Numeric::EPSILON_FLOAT &&
         camera->pixelHeight > Numeric::EPSILON_FLOAT ) {
        return;
    }

    camera->set(
        &camera->eyePosition,
        &camera->lookPosition,
        &camera->upDirection,
        camera->fieldOfVision,
        globalModel.width,
        globalModel.height,
        &camera->background);
}

void
GlutDebugTools::printElementHierarchy(const GalerkinElement *element, int level) {
    switch ( level ) {
        case 0:
            break;
        case 1:
            java::lang::System::out.printf("  - ");
            break;
        case 2:
            java::lang::System::out.printf("    . ");
            break;
        default:
            java::lang::System::out.printf("      (%d) ", level);
            for ( int i = 3; i < level; i++ ) {
                java::lang::System::out.printf(" ");
            }
            java::lang::System::out.printf("-> ");
            break;
    }
    const ColorRgb *c = element->radiance;
    long int numberOfInteractions = 0;
    if ( element->interactions != nullptr ) {
        numberOfInteractions = element->interactions->size();
    }

    if ( element->regularSubElements == nullptr ) {
        if ( c == nullptr ) {
            java::lang::System::out.printf("Child element no radiance\n");
        } else {
            java::lang::System::out.printf("Child element radiance <%0.4f, %0.4f, %0.4f>, interactions: %ld\n",
               c->r, c->g, c->b, numberOfInteractions);
        }
    } else {
        if ( c == nullptr ) {
            java::lang::System::out.printf("Container element no radiance\n");
        } else {
            java::lang::System::out.printf("Container element radiance <%0.4f, %0.4f, %0.4f>, interactions: %ld\n",
               c->r, c->g, c->b, numberOfInteractions);
        }
        for ( int i = 0; i < 4; i++ ) {
            const GalerkinElement *child = static_cast<GalerkinElement *>(element->regularSubElements[i]);
            if ( child != nullptr ) {
                GlutDebugTools::printElementHierarchy(child, level + 1);
            }
        }
    }
}

void
GlutDebugTools::printGalerkinElementForPatch(const Scene *scene, int patchIndex) {
    java::lang::System::out.printf("================================================================================\n");
    if ( patchIndex < 0 || scene->patchList == nullptr || patchIndex >= scene->patchList->size() ) {
        return;
    }
    const Patch *patch = scene->patchList->get(patchIndex);
    if  ( patch == nullptr || patch->radianceData == nullptr ) {
        return;
    }
    const GalerkinElement *element = GalerkinElement::fromPatch(patch);
    java::lang::System::out.printf("Galerkin element for patch[%d] %d\n", patchIndex, patch->id);
    GlutDebugTools::printElementHierarchy(element, 0);
}

void
GlutDebugTools::keypressCallback(unsigned char keyChar, int /*x*/, int /*y*/) {
    if ( GlutDebugToolsKeyControl::handleKeypress(
             keyChar,
             globalModel,
             GlutDebugTools::printGalerkinElementForPatch) ) {
        glutPostRedisplay();
    }
}

void
GlutDebugTools::extendedKeypressCallback(int keyCode, int /*x*/, int /*y*/) {
    if ( GlutDebugToolsKeyControl::handleExtendedKeypress(keyCode, globalModel) ) {
        glutPostRedisplay();
    }
}

void
GlutDebugTools::mouseButtonCallback(int button, int state, int x, int y) {
    GlutDebugTools::syncModelWindowSizeFromGlut();
    GlutDebugTools::syncCameraToViewport();
    if ( GlutDebugToolsMouseControl::handleMouseButton(button, state, x, y, globalModel) ) {
        glutPostRedisplay();
    }
}

void
GlutDebugTools::mouseMotionCallback(int x, int y) {
    GlutDebugTools::syncModelWindowSizeFromGlut();
    GlutDebugTools::syncCameraToViewport();
    if ( GlutDebugToolsMouseControl::handleMouseMotion(x, y, globalModel) ) {
        glutPostRedisplay();
    }
}

void
GlutDebugTools::drawCallback() {
    if ( globalModel.scene == nullptr || globalModel.renderOptions == nullptr ) {
        return;
    }

    GlutDebugTools::syncModelWindowSizeFromGlut();
    GlutDebugTools::syncCameraToViewport();

    if ( globalModel.fullScreen != globalFullScreenApplied ) {
        if ( globalModel.fullScreen ) {
            glutFullScreen();
            globalFullScreenApplied = true;
        } else {
            glutPositionWindow(0, 0);
            glutReshapeWindow(globalModel.windowedWidth, globalModel.windowedHeight);
            globalFullScreenApplied = false;
        }
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, globalModel.width, globalModel.height);

    int totalElements = 0;
    int selectedPatchIndex = GLOBAL_render_glutDebugState.primarySelectedPatch;
    int secondarySelectedPatchIndex = GLOBAL_render_glutDebugState.selectedSelectedPatch;
    if ( selectedPatchIndex < -1 ) {
        selectedPatchIndex = -1;
    }
    if ( secondarySelectedPatchIndex < -1 ) {
        secondarySelectedPatchIndex = -1;
    }
    if ( globalModel.scene->patchList != nullptr ) {
        totalElements = globalModel.scene->patchList->size();
        if ( selectedPatchIndex >= totalElements ) {
            selectedPatchIndex = totalElements - 1;
        }
        if ( secondarySelectedPatchIndex >= totalElements ) {
            secondarySelectedPatchIndex = totalElements - 1;
        }
        if ( totalElements <= 0 ) {
            selectedPatchIndex = -1;
            secondarySelectedPatchIndex = -1;
        }
    }

    if ( globalModel.mode == GlutDebugMode::RADIANCE_SCENE ) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINES);
        Opengl::openGlRenderScene(globalModel.scene, globalModel.radianceMethod, globalModel.renderOptions);
    } else if ( globalModel.mode == GlutDebugMode::GALERKIN_ELEMENT_HIERARCHY ) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        Opengl::openGlRenderSetCamera(globalModel.scene->camera, globalModel.scene->geometryList);
        glPushMatrix();
        Opengl::openGlApplyDebugSceneRotation(globalModel.scene);
        GlutDebugPatchHierarchy::renderSelectedPatchAtLevel(
            globalModel.scene,
            globalModel.renderOptions,
            selectedPatchIndex,
            secondarySelectedPatchIndex,
            globalModel.selectedHierarchyLevel);
        GlutDebugPatchHierarchy::renderSecondarySelectedPatchMarker(
            globalModel.scene,
            globalModel.renderOptions,
            secondarySelectedPatchIndex);
        glPopMatrix();
    }

    char hudModeText[256];
    std::snprintf(
        hudModeText,
        sizeof(hudModeText),
        "MODE: %s [m]",
        glutDebugModeName(globalModel.mode));
    GlutHudConsole::printTextLine(hudModeText, 0, 0, globalModel.width, globalModel.height);

    char hudSelectedElementText[256];
    if ( selectedPatchIndex >= 0 ) {
        std::snprintf(
            hudSelectedElementText,
            sizeof(hudSelectedElementText),
            "Element %d/%d [1, 2, mouse click]",
            selectedPatchIndex + 1,
            totalElements);
    } else {
        std::snprintf(
            hudSelectedElementText,
            sizeof(hudSelectedElementText),
            "Element none/%d [1, 2, mouse click]",
            totalElements);
    }
    GlutHudConsole::printTextLine(hudSelectedElementText, 0, 1, globalModel.width, globalModel.height);

    if ( globalModel.mode == GlutDebugMode::GALERKIN_ELEMENT_HIERARCHY ) {
        char hudSecondarySelectedElementText[256];
        if ( secondarySelectedPatchIndex >= 0 ) {
            std::snprintf(
                hudSecondarySelectedElementText,
                sizeof(hudSecondarySelectedElementText),
                "Secondary element %d/%d [5, 6, shift-click]",
                secondarySelectedPatchIndex + 1,
                totalElements);
        } else {
            std::snprintf(
                hudSecondarySelectedElementText,
                sizeof(hudSecondarySelectedElementText),
                "Secondary element none/%d [5, 6, shift-click]",
                totalElements);
        }
        GlutHudConsole::printTextLine(hudSecondarySelectedElementText, 0, 2, globalModel.width, globalModel.height);

        int maxHierarchyLevel = 0;
        if ( selectedPatchIndex >= 0 && globalModel.scene != nullptr ) {
            maxHierarchyLevel = GlutDebugPatchHierarchy::maxLevelForSelectedPatch(
                globalModel.scene,
                selectedPatchIndex);
        }

        int currentHierarchyLevel = globalModel.selectedHierarchyLevel;
        if ( currentHierarchyLevel < 0 ) {
            currentHierarchyLevel = 0;
        }
        if ( currentHierarchyLevel > maxHierarchyLevel ) {
            currentHierarchyLevel = maxHierarchyLevel;
        }

        char currentLevelLabel[32];
        if ( selectedPatchIndex < 0 ) {
            std::snprintf(currentLevelLabel, sizeof(currentLevelLabel), "none");
        } else if ( currentHierarchyLevel == 0 ) {
            std::snprintf(currentLevelLabel, sizeof(currentLevelLabel), "base");
        } else {
            std::snprintf(currentLevelLabel, sizeof(currentLevelLabel), "%d", currentHierarchyLevel);
        }

        char hudSubdivisionText[256];
        std::snprintf(
            hudSubdivisionText,
            sizeof(hudSubdivisionText),
            "Patch subdivision level: %s/%d",
            currentLevelLabel,
            maxHierarchyLevel);
        GlutHudConsole::printTextLine(hudSubdivisionText, 0, 3, globalModel.width, globalModel.height);
    }

    glutSwapBuffers();
}

void
GlutDebugTools::executeGlutGui(
    int argc,
    char *argv[],
    Scene *scene,
    RadianceMethod *radianceMethod,
    RenderOptions *renderOptions,
    void (*memoryFreeCallBack)(ParseSession *mgfContext),
    ParseSession *mgfContext)
{
    globalModel.mode = GlutDebugMode::RADIANCE_SCENE;
    globalModel.fullScreen = false;
    globalModel.selectedHierarchyLevel = 0;
    globalModel.width = 1920;
    globalModel.height = 1200;
    globalModel.windowedWidth = globalModel.width;
    globalModel.windowedHeight = globalModel.height;
    globalFullScreenApplied = false;
    globalModel.scene = scene;
    globalModel.radianceMethod = radianceMethod;
    globalModel.renderOptions = renderOptions;
    globalModel.memoryFreeCallBack = memoryFreeCallBack;
    globalModel.mgfContext = mgfContext;

    glutInit(&argc, argv);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(globalModel.width, globalModel.height);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    int windowHandle = glutCreateWindow("RPK");
    if ( windowHandle == GL_FALSE ) {
        java::lang::System::out.printf("ERROR: Can not open GLUT window, check OpenGL/GLUT setup.\n");
        java::lang::System::exit(1);
    }

    globalModel.renderOptions->frustumCulling = false;

    glutReshapeFunc(GlutDebugTools::resizeCallback);
    glutKeyboardFunc(GlutDebugTools::keypressCallback);
    glutSpecialFunc(GlutDebugTools::extendedKeypressCallback);
    glutMouseFunc(GlutDebugTools::mouseButtonCallback);
    glutMotionFunc(GlutDebugTools::mouseMotionCallback);
    glutDisplayFunc(GlutDebugTools::drawCallback);
    glutMainLoop();
}

#endif

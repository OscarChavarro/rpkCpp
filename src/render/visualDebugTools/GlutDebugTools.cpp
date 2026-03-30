#include "render/visualDebugTools/GlutDebugTools.h"
#include "render/visualDebugTools/GlutDebugToolsModel.h"
#include "render/visualDebugTools/GlutDebugToolsKeyControl.h"
#include "app/GalerkinDebugRenderer.h"

#ifdef OPEN_GL_ENABLED


#ifdef __APPLE__
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "render/Opengl.h"

static GlutDebugToolsModel globalModel;

void
GlutDebugTools::resizeCallback(int newWidth, int newHeight) {
    globalModel.width = newWidth;
    globalModel.height = newHeight;
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
    if ( scene->patchList == nullptr || patchIndex >= scene->patchList->size() ) {
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
GlutDebugTools::drawCallback() {
    if ( globalModel.scene == nullptr || globalModel.renderOptions == nullptr ) {
        return;
    }

    globalModel.scene->camera->xSize = globalModel.width;
    globalModel.scene->camera->ySize = globalModel.height;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, globalModel.width, globalModel.height);

    if ( globalModel.mode == GlutDebugMode::RADIANCE_SCENE ) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINES);
        Opengl::openGlRenderScene(globalModel.scene, globalModel.radianceMethod, globalModel.renderOptions);
    } else if ( globalModel.mode == GlutDebugMode::GALERKIN_ELEMENT_HIERARCHY ) {
        Opengl::openGlRenderSetCamera(globalModel.scene->camera, globalModel.scene->geometryList);
        glPushMatrix();
        glRotated(GLOBAL_render_glutDebugState.angle, 0, 0, 1);
        GalerkinDebugRenderer::renderGalerkinElementHierarchy(globalModel.scene, globalModel.renderOptions);
        glPopMatrix();
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
    globalModel.width = 1920;
    globalModel.height = 1200;
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
    glutDisplayFunc(GlutDebugTools::drawCallback);
    glutMainLoop();
}

#endif

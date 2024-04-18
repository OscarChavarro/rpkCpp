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
static RadianceMethod *globalRadianceMethod;
static Scene *globalScene;

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

static char *globalCompoundType = (char *)"Compound";
static char *globalMeshSurfaceType = (char *)"MeshSurface";
static char *globalPatchSetType = (char *)"PatchSet";
static char *globalUnknownType = (char *)"<unknown>";

static char *
printGeometryType(GeometryClassId id) {
    char *response = globalUnknownType;
    if ( id == GeometryClassId::SURFACE_MESH ) {
        response = globalMeshSurfaceType;
    } else if ( id == GeometryClassId::COMPOUND ) {
        response = globalCompoundType;
    } else if ( id == GeometryClassId::PATCH_SET ) {
        response = globalPatchSetType;
    }
    return response;
}

static void
printDataStructures() {
    printf("= globalSceneGeometries ================================================================\n");
    printf("Geometries on list: %ld\n", globalScene->geometryList->size());
    for ( int i = 0; i < globalScene->geometryList->size(); i++ ) {
        Geometry *geometry = globalScene->geometryList->get(i);
        printf("  - [%d] / [%s]\n", i, printGeometryType(geometry->className));
        printf("    . Id: %d\n", geometry->id);
        printf("    . %s\n", geometry->isDuplicate ? "Duplicate" : "Original");
        if ( geometry->className == GeometryClassId::SURFACE_MESH ) {
            MeshSurface* mesh = (MeshSurface *)geometry;
            printf("    . Inner id: %d\n", mesh->meshId);
            // Note that mesh is not used anymore after initial MGF read process, all needed is patches,
            // compounds/clusters and materials.
            //printf("    . Vertices: %ld, positions: %ld, normals: %ld, faces: %ld\n",
            //   mesh->vertices->size(), mesh->positions->size(), mesh->normals->size(), mesh->faces->size());
        } else if ( geometry->className == GeometryClassId::COMPOUND ) {
            Compound *compound = (Compound *)geometry;
            if ( compound->compoundData->children != nullptr ) {
                printf("    . Outer children: %ld\n", compound->compoundData->children->size());
            }
            if ( compound->children != nullptr ) {
                printf("    . Inner children: %ld\n", compound->children->size());
            }
        }
    }
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
            globalRadianceMethod->doStep(
                globalScene->camera,
                globalScene->background,
                globalScene->patchList,
                globalScene->geometryList,
                globalScene->clusteredGeometryList,
                globalScene->lightSourcePatchList,
                globalScene->clusteredRootGeometry,
                globalScene->voxelGrid);
            break;
        case 'p':
            printDataStructures();
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

    globalScene->camera->xSize = globalWidth;
    globalScene->camera->ySize = globalHeight;

    glViewport(0, 0, globalWidth, globalHeight);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINES);
    openGlRenderScene(
        globalScene->camera,
        globalScene->patchList,
        globalScene->clusteredGeometryList,
        globalScene->geometryList,
        globalScene->clusteredRootGeometry,
        nullptr,
        globalRadianceMethod);
    glutSwapBuffers();
}

void
executeGlutGui(
    int argc,
    char *argv[],
    Scene *scene,
    RadianceMethod *radianceMethod)
{
    globalScene = scene;
    globalRadianceMethod = radianceMethod;

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

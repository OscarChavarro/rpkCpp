/**
Hierarchical object names tracking
*/

#include <cstring>

#include "java/util/ArrayList.txx"
#include "scene/scene.h"
#include "io/mgf/words.h"
#include "io/mgf/mgfHandlerObject.h"
#include "io/mgf/mgfDefinitions.h"
#include "io/mgf/mgfHandlerMaterial.h"

int GLOBAL_mgf_objectNames; // Depth of name hierarchy
char **GLOBAL_mgf_objectNamesList; // Name list
java::ArrayList<Geometry *> **GLOBAL_mgf_geometryStackPtr = nullptr;
int GLOBAL_mgf_inSurface = false; // True if busy creating a new surface
java::ArrayList<Geometry *> *GLOBAL_mgf_geometryStack[MAXIMUM_GEOMETRY_STACK_DEPTH];

static int globalObjectMaxName; // Allocated list size

// List increment ( > 1 )
#define ALLOC_INC 16

static void
pushCurrentGeometryList(MgfContext *context) {
    if ( GLOBAL_mgf_geometryStackPtr - GLOBAL_mgf_geometryStack >= MAXIMUM_GEOMETRY_STACK_DEPTH ) {
        doError(
                "Objects are nested too deep for this program. Recompile with larger MAXIMUM_GEOMETRY_STACK_DEPTH constant in read mgf", context);
        return;
    } else {
        *GLOBAL_mgf_geometryStackPtr = GLOBAL_mgf_currentGeometryList;
        GLOBAL_mgf_geometryStackPtr++;
        GLOBAL_mgf_currentGeometryList = nullptr;
    }
}

static void
popCurrentGeometryList(MgfContext *context) {
    if ( GLOBAL_mgf_geometryStackPtr <= GLOBAL_mgf_geometryStack ) {
        doError("Object stack underflow ... unbalanced 'o' contexts?", context);
        GLOBAL_mgf_currentGeometryList = nullptr;
        return;
    } else {
        GLOBAL_mgf_geometryStackPtr--;
        GLOBAL_mgf_currentGeometryList = *GLOBAL_mgf_geometryStackPtr;
    }
}

void
newSurface() {
    GLOBAL_mgf_currentPointList = new java::ArrayList<Vector3D *>();
    GLOBAL_mgf_currentNormalList = new java::ArrayList<Vector3D *>();
    GLOBAL_mgf_currentVertexList = new java::ArrayList<Vertex *>();
    GLOBAL_mgf_currentFaceList = new java::ArrayList<Patch *>();
    GLOBAL_mgf_inSurface = true;
}

/**
Handle an object entity statement
*/
static int
handleObject2Entity(int ac, char **av)
{
    if ( ac == 1 ) {
        // just pop top object
        if ( GLOBAL_mgf_objectNames < 1 ) {
            return MGF_ERROR_UNMATCHED_CONTEXT_CLOSE;
        }
        free(GLOBAL_mgf_objectNamesList[--GLOBAL_mgf_objectNames]);
        GLOBAL_mgf_objectNamesList[GLOBAL_mgf_objectNames] = nullptr;
        return MGF_OK;
    }
    if ( ac != 2 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if ( !isNameWords(av[1])) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( GLOBAL_mgf_objectNames >= globalObjectMaxName - 1 ) {
        // enlarge array
        if ( !globalObjectMaxName ) {
            GLOBAL_mgf_objectNamesList = (char **) malloc(
                    (globalObjectMaxName = ALLOC_INC) * sizeof(char *));
        } else {
            GLOBAL_mgf_objectNamesList = (char **) realloc((void *) GLOBAL_mgf_objectNamesList, (globalObjectMaxName += ALLOC_INC) * sizeof(char *));
        }
        if ( GLOBAL_mgf_objectNamesList == nullptr) {
            return MGF_ERROR_OUT_OF_MEMORY;
        }
    }

    // allocate new entry
    GLOBAL_mgf_objectNamesList[GLOBAL_mgf_objectNames] = (char *) malloc(strlen(av[1]) + 1);
    if ( GLOBAL_mgf_objectNamesList[GLOBAL_mgf_objectNames] == nullptr) {
        return MGF_ERROR_OUT_OF_MEMORY;
    }
    strcpy(GLOBAL_mgf_objectNamesList[GLOBAL_mgf_objectNames++], av[1]);
    GLOBAL_mgf_objectNamesList[GLOBAL_mgf_objectNames] = nullptr;
    return MGF_OK;
}

void
surfaceDone() {
    if ( GLOBAL_mgf_currentGeometryList == nullptr ) {
        GLOBAL_mgf_currentGeometryList = new java::ArrayList<Geometry *>();
    }

    if ( GLOBAL_mgf_currentFaceList != nullptr ) {
        Geometry *newGeometry = new MeshSurface(
                GLOBAL_mgf_currentMaterial,
                GLOBAL_mgf_currentPointList,
                GLOBAL_mgf_currentNormalList,
                nullptr, // null texture coordinate list
                GLOBAL_mgf_currentVertexList,
                GLOBAL_mgf_currentFaceList,
                MaterialColorFlags::NO_COLORS);
        GLOBAL_mgf_currentGeometryList->add(0, newGeometry);
    }
    GLOBAL_mgf_inSurface = false;
}

int
handleObjectEntity(int argc, char **argv, MgfContext *context) {
    int i;

    if ( argc > 1 ) {
        // Beginning of a new object
        for ( i = 0; i < GLOBAL_mgf_geometryStackPtr - GLOBAL_mgf_geometryStack; i++ ) {
            fprintf(stderr, "\t");
        }
        fprintf(stderr, "%s ...\n", argv[1]);

        if ( GLOBAL_mgf_inSurface ) {
            surfaceDone();
        }

        pushCurrentGeometryList(context);

        newSurface();
    } else {
        // End of object definition
        Geometry *theGeometry = nullptr;

        if ( GLOBAL_mgf_inSurface ) {
            surfaceDone();
        }

        long listSize = 0;
        if ( GLOBAL_mgf_currentGeometryList != nullptr ) {
            listSize += GLOBAL_mgf_currentGeometryList->size();
        }

        if ( listSize > 0 ) {
            theGeometry = geomCreateCompound(new Compound(GLOBAL_mgf_currentGeometryList));
        }

        popCurrentGeometryList(context);

        if ( theGeometry ) {
            GLOBAL_mgf_currentGeometryList->add(0, theGeometry);
            GLOBAL_scene_geometries = GLOBAL_mgf_currentGeometryList;
        }

        newSurface();
    }

    return handleObject2Entity(argc, argv);
}

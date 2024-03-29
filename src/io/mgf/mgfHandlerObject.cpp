/**
Hierarchical object names tracking
*/

#include <cstring>

#include "java/util/ArrayList.txx"
#include "scene/scene.h"
#include "io/mgf/words.h"
#include "io/mgf/mgfHandlerObject.h"
#include "io/mgf/mgfDefinitions.h"

static char **globalObjectNamesList; // Name list (names in hierarchy)
static int globalObjectMaxName; // Allocated list size
static int globalObjectNames; // Depth of name hierarchy

// List increment ( > 1 )
#define ALLOC_INC 16

static void
pushCurrentGeometryList(MgfContext *context) {
    if ( context->geometryStackPtr - context->geometryStack >= MAXIMUM_GEOMETRY_STACK_DEPTH ) {
        doError(
                "Objects are nested too deep for this program. Recompile with larger MAXIMUM_GEOMETRY_STACK_DEPTH constant in read mgf", context);
        return;
    } else {
        *context->geometryStackPtr = context->currentGeometryList;
        context->geometryStackPtr++;
        context->currentGeometryList = nullptr;
    }
}

static void
popCurrentGeometryList(MgfContext *context) {
    if ( context->geometryStackPtr <= context->geometryStack ) {
        doError("Object stack underflow ... unbalanced 'o' contexts?", context);
        context->currentGeometryList = nullptr;
        return;
    } else {
        context->geometryStackPtr--;
        context->currentGeometryList = *context->geometryStackPtr;
    }
}

void
newSurface(MgfContext *context) {
    context->currentPointList = new java::ArrayList<Vector3D *>();
    context->currentNormalList = new java::ArrayList<Vector3D *>();
    context->currentVertexList = new java::ArrayList<Vertex *>();
    context->currentFaceList = new java::ArrayList<Patch *>();
    context->inSurface = true;
}

/**
Handle an object entity statement
*/
static int
handleObject2Entity(int ac, char **av)
{
    if ( ac == 1 ) {
        // Just pop top object
        if ( globalObjectNames < 1 ) {
            return MGF_ERROR_UNMATCHED_CONTEXT_CLOSE;
        }
        free(globalObjectNamesList[--globalObjectNames]);
        globalObjectNamesList[globalObjectNames] = nullptr;
        return MGF_OK;
    }
    if ( ac != 2 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if ( !isNameWords(av[1])) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( globalObjectNames >= globalObjectMaxName - 1 ) {
        // Enlarge array
        if ( !globalObjectMaxName ) {
            globalObjectNamesList = (char **) malloc(
                    (globalObjectMaxName = ALLOC_INC) * sizeof(char *));
        } else {
            globalObjectNamesList = (char **) realloc((void *) globalObjectNamesList, (globalObjectMaxName += ALLOC_INC) * sizeof(char *));
        }
        if ( globalObjectNamesList == nullptr) {
            return MGF_ERROR_OUT_OF_MEMORY;
        }
    }

    // allocate new entry
    globalObjectNamesList[globalObjectNames] = (char *) malloc(strlen(av[1]) + 1);
    if ( globalObjectNamesList[globalObjectNames] == nullptr) {
        return MGF_ERROR_OUT_OF_MEMORY;
    }
    strcpy(globalObjectNamesList[globalObjectNames++], av[1]);
    globalObjectNamesList[globalObjectNames] = nullptr;
    return MGF_OK;
}

void
surfaceDone(MgfContext *context) {
    if ( context->currentGeometryList == nullptr ) {
        context->currentGeometryList = new java::ArrayList<Geometry *>();
    }

    if ( context->currentFaceList != nullptr ) {
        Geometry *newGeometry = new MeshSurface(
            context->currentMaterial,
            context->currentPointList,
            context->currentNormalList,
            nullptr, // null texture coordinate list
            context->currentVertexList,
            context->currentFaceList,
            MaterialColorFlags::NO_COLORS);
        context->currentGeometryList->add(0, newGeometry);
    }
    context->inSurface = false;
}

int
handleObjectEntity(int argc, char **argv, MgfContext *context) {
    int i;

    if ( argc > 1 ) {
        // Beginning of a new object
        for ( i = 0; i < context->geometryStackPtr - context->geometryStack; i++ ) {
            fprintf(stderr, "\t");
        }
        fprintf(stderr, "%s ...\n", argv[1]);

        if ( context->inSurface ) {
            surfaceDone(context);
        }

        pushCurrentGeometryList(context);

        newSurface(context);
    } else {
        // End of object definition
        Geometry *theGeometry = nullptr;

        if ( context->inSurface ) {
            surfaceDone(context);
        }

        long listSize = 0;
        if ( context->currentGeometryList != nullptr ) {
            listSize += context->currentGeometryList->size();
        }

        if ( listSize > 0 ) {
            theGeometry = geomCreateCompound(new Compound(context->currentGeometryList));
        }

        popCurrentGeometryList(context);

        if ( theGeometry ) {
            context->currentGeometryList->add(0, theGeometry);
            context->geometries = context->currentGeometryList;
        }

        newSurface(context);
    }

    return handleObject2Entity(argc, argv);
}

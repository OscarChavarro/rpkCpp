/**
Hierarchical object names tracking
*/

#include <cstring>

#include "java/util/ArrayList.txx"
#include "common/CppReAlloc.h"
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
    if ( context->geometryStackHeadIndex >= MAXIMUM_GEOMETRY_STACK_DEPTH ) {
        doError("Objects are nested too deep for this program. Recompile with larger MAXIMUM_GEOMETRY_STACK_DEPTH constant in read mgf", context);
        return;
    } else {
        context->geometryStack[context->geometryStackHeadIndex] = context->currentGeometryList;
        context->geometryStackHeadIndex++;
        context->currentGeometryList = nullptr;
    }
}

static void
popCurrentGeometryList(MgfContext *context) {
    if ( context->geometryStackHeadIndex < 0 ) {
        doError("Object stack underflow ... unbalanced 'o' contexts?", context);
        context->currentGeometryList = nullptr;
        return;
    } else {
        context->geometryStackHeadIndex--;
        context->currentGeometryList = context->geometryStack[context->geometryStackHeadIndex];
    }
}

void
mgfObjectNewSurface(MgfContext *context) {
    // Note: lists created here will be transferred to new MeshSurface,
    // should not be deleted from MgfContext
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
handleObject2Entity(int ac, const char **av) {
    if ( ac == 1 ) {
        // Just pop top object
        if ( globalObjectNames < 1 ) {
            return MgfErrorCode::MGF_ERROR_UNMATCHED_CONTEXT_CLOSE;
        }
        free(globalObjectNamesList[--globalObjectNames]);
        globalObjectNamesList[globalObjectNames] = nullptr;
        return MgfErrorCode::MGF_OK;
    }
    if ( ac != 2 ) {
        return MgfErrorCode::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if ( !isNameWords(av[1]) ) {
        return MgfErrorCode::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( globalObjectNames >= globalObjectMaxName - 1 ) {
        // Enlarge array
        if ( globalObjectMaxName == 0 ) {
            globalObjectMaxName = ALLOC_INC;
            globalObjectNamesList = new char *[globalObjectMaxName];
        } else {
            CppReAlloc<char *> memoryManager;
            globalObjectMaxName += ALLOC_INC;
            globalObjectNamesList = memoryManager.reAlloc(globalObjectNamesList, globalObjectMaxName);
            if ( globalObjectNamesList == nullptr ) {
                fprintf(stderr, "Memory error\n");
                exit(1);
            }
        }
        if ( globalObjectNamesList == nullptr ) {
            return MgfErrorCode::MGF_ERROR_OUT_OF_MEMORY;
        }
    }

    // Allocate new entry
    globalObjectNamesList[globalObjectNames] = (char *)malloc(strlen(av[1]) + 1);
    if ( globalObjectNamesList[globalObjectNames] == nullptr) {
        return MgfErrorCode::MGF_ERROR_OUT_OF_MEMORY;
    }
    strcpy(globalObjectNamesList[globalObjectNames++], av[1]);
    globalObjectNamesList[globalObjectNames] = nullptr;
    return MgfErrorCode::MGF_OK;
}

void
mgfObjectSurfaceDone(MgfContext *context) {
    if ( context->currentGeometryList == nullptr ) {
        context->currentGeometryList = new java::ArrayList<Geometry *>();
    }

    if ( context->readerContext != nullptr ) {
        const char *head = context->readerContext->inputLine;
        if ( head[0] == 'o' && head[1] == ' ' ) {
            char *tail = &context->readerContext->inputLine[2];
            if ( strlen(tail) > 0 ) {
                if ( context->currentObjectName != nullptr ) {
                    delete[] context->currentObjectName;
                }
                context->currentObjectName = nullptr;
                tail[strlen(tail) - 1] = '\0';
                context->currentObjectName = new char[strlen(tail) + 1];
                strcpy(context->currentObjectName, tail);
            }
        }
    }

    if ( context->currentFaceList != nullptr && context->currentFaceList->size() > 0 ) {
        Geometry *newGeometry = new MeshSurface(
            context->currentObjectName,
            context->currentMaterial,
            context->currentPointList,
            context->currentNormalList,
            nullptr, // null texture coordinate list
            context->currentVertexList,
            context->currentFaceList,
            MaterialColorFlags::NO_COLORS);
        context->currentGeometryList->add(newGeometry);
        context->allGeometries->add(newGeometry);
        context->currentObjectName = nullptr;
    }
    context->inSurface = false;
}

int
handleObjectEntity(int argc, const char **argv, MgfContext *context) {
    if ( argc > 1 ) {
        // Beginning of a new object
        if ( context->inSurface ) {
            mgfObjectSurfaceDone(context);
        }

        pushCurrentGeometryList(context);

        mgfObjectNewSurface(context);
    } else {
        // End of object definition
        Geometry *newGeometry = nullptr;

        if ( context->inSurface ) {
            mgfObjectSurfaceDone(context);
        }

        long listSize = 0;
        if ( context->currentGeometryList != nullptr ) {
            listSize += context->currentGeometryList->size();
        }

        if ( listSize > 0 ) {
            Compound *newCompound = new Compound(context->currentGeometryList);
            newGeometry = new Geometry(nullptr, newCompound, GeometryClassId::COMPOUND);
        }

        popCurrentGeometryList(context);

        if ( newGeometry != nullptr && context->currentGeometryList ) {
            context->currentGeometryList->add(newGeometry);
            context->geometries = context->currentGeometryList;
            context->allGeometries->add(newGeometry);
            mgfObjectNewSurface(context);
        }
    }

    return handleObject2Entity(argc, argv);
}

void
mgfObjectFreeMemory() {
    delete[] globalObjectNamesList;
}

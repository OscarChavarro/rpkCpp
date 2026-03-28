/**
Hierarchical object names tracking
*/

#include <cstring>

#include "java/lang/System.h"
#include "java/util/ArrayList.txx"

#include "common/CppReAlloc.h"

#include "numericalAnalysis/MeshSurfaceVisitor.h"

#include "io/context/WordsContext.h"

#include "io/mgf/mgfDefinitions.h"
#include "io/mgf/mgfHandlerObject.h"

static char **globalObjectNamesList; // Name list (names in hierarchy)
static int globalObjectMaxName; // Allocated list size
static int globalObjectNames; // Depth of name hierarchy

// List increment ( > 1 )
static constexpr int ALLOC_INC = 16;

static void
disposeCurrentSurfaceLists(BaseContext *context) {
    if ( context->currentPointList != nullptr ) {
        for ( int i = 0; i < context->currentPointList->size(); i++ ) {
            delete context->currentPointList->get(i);
        }
        context->currentPointList->dispose();
        delete context->currentPointList;
        context->currentPointList = nullptr;
    }
    if ( context->currentNormalList != nullptr ) {
        for ( int i = 0; i < context->currentNormalList->size(); i++ ) {
            delete context->currentNormalList->get(i);
        }
        context->currentNormalList->dispose();
        delete context->currentNormalList;
        context->currentNormalList = nullptr;
    }
    if ( context->currentVertexList != nullptr ) {
        for ( int i = 0; i < context->currentVertexList->size(); i++ ) {
            delete context->currentVertexList->get(i);
        }
        context->currentVertexList->dispose();
        delete context->currentVertexList;
        context->currentVertexList = nullptr;
    }
    if ( context->currentFaceList != nullptr ) {
        for ( int i = 0; i < context->currentFaceList->size(); i++ ) {
            delete context->currentFaceList->get(i);
        }
        context->currentFaceList->dispose();
        delete context->currentFaceList;
        context->currentFaceList = nullptr;
    }
}

static void
pushCurrentGeometryList(BaseContext *context) {
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
popCurrentGeometryList(BaseContext *context) {
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
mgfObjectNewSurface(BaseContext *context) {
    // Note: lists created here will be transferred to new MeshSurface,
    // should not be deleted from BaseContext
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
            return ErrorCodeContext::MGF_ERROR_UNMATCHED_CONTEXT_CLOSE;
        }
        delete[] globalObjectNamesList[--globalObjectNames];
        globalObjectNamesList[globalObjectNames] = nullptr;
        return ErrorCodeContext::MGF_OK;
    }
    if ( ac != 2 ) {
        return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if ( !WordsContext::isName(av[1]) ) {
        return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( globalObjectNames >= globalObjectMaxName - 1 ) {
        // Enlarge array
        if ( globalObjectMaxName == 0 ) {
            globalObjectMaxName = ALLOC_INC;
            globalObjectNamesList = new char *[globalObjectMaxName];
        } else {
            const int oldObjectMaxName = globalObjectMaxName;
            globalObjectMaxName += ALLOC_INC;
            globalObjectNamesList = CppReAlloc::reAlloc(
                globalObjectNamesList,
                oldObjectMaxName,
                globalObjectMaxName);
            if ( globalObjectNamesList == nullptr ) {
                java::lang::System::err.println("Memory error");
                exit(1);
            }
        }
        if ( globalObjectNamesList == nullptr ) {
            return ErrorCodeContext::MGF_ERROR_OUT_OF_MEMORY;
        }
    }

    // Allocate new entry
    globalObjectNamesList[globalObjectNames] = new char[strlen(av[1]) + 1];
    if ( globalObjectNamesList[globalObjectNames] == nullptr) {
        return ErrorCodeContext::MGF_ERROR_OUT_OF_MEMORY;
    }
    strcpy(globalObjectNamesList[globalObjectNames++], av[1]);
    globalObjectNamesList[globalObjectNames] = nullptr;
    return ErrorCodeContext::MGF_OK;
}

void
mgfObjectSurfaceDone(BaseContext *context) {
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
        MeshSurfaceVisitor::initializeFacesDefaults(dynamic_cast<MeshSurface *>(newGeometry));
        context->currentGeometryList->add(newGeometry);
        context->allGeometries->add(newGeometry);
        context->currentObjectName = nullptr;
        context->currentPointList = nullptr;
        context->currentNormalList = nullptr;
        context->currentVertexList = nullptr;
        context->currentFaceList = nullptr;
    } else {
        disposeCurrentSurfaceLists(context);
    }
    context->inSurface = false;
}

int
handleObjectEntity(int argc, const char **argv, BaseContext *context) {
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
            newGeometry = new Compound(context->currentGeometryList);
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
    for ( int i = 0; i < globalObjectNames; i++ ) {
        delete[] globalObjectNamesList[i];
    }
    delete[] globalObjectNamesList;
    globalObjectNamesList = nullptr;
    globalObjectMaxName = 0;
    globalObjectNames = 0;
}

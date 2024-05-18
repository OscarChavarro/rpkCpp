#ifndef __MGF_CONTEXT__
#define __MGF_CONTEXT__

#include "scene/RadianceMethod.h"
#include "io/mgf/MgfEntity.h"
#include "io/mgf/MgfReaderContext.h"

// Error codes
enum MgfErrorCode {
    MGF_OK = 0, // normal return value
    MGF_ERROR_UNKNOWN_ENTITY = 1,
    MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS = 2,
    MGF_ERROR_ARGUMENT_TYPE = 3,
    MGF_ERROR_ILLEGAL_ARGUMENT_VALUE = 4,
    MGF_ERROR_UNDEFINED_REFERENCE = 5,
    MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE = 6,
    MGF_ERROR_IN_INCLUDED_FILE = 7,
    MGF_ERROR_OUT_OF_MEMORY = 8,
    MGF_ERROR_FILE_SEEK_ERROR = 9,
    MGF_ERROR_LINE_TOO_LONG = 11,
    MGF_ERROR_UNMATCHED_CONTEXT_CLOSE = 12,
    MGF_NUMBER_OF_ERRORS = 13
};

// Objects 'o' contexts can be nested this deep
#define MAXIMUM_GEOMETRY_STACK_DEPTH 100

class MgfTransformContext;
class MgfColorContext;
class LookUpTable;

class MgfContext {
  public:
    // Parameters received from main program
    RadianceMethod *radianceMethod;
    bool singleSided;
    char *currentVertexName;
    int numberOfQuarterCircleDivisions;
    bool monochrome;
    Material *currentMaterial;

    // Internal variables on the MGF reader context
    char entityNames[TOTAL_NUMBER_OF_ENTITIES][MGF_MAXIMUM_ENTITY_NAME_LENGTH];
    const char *errorCodeMessages[MGF_NUMBER_OF_ERRORS];
    MgfReaderContext *readerContext;
    int (*handleCallbacks[TOTAL_NUMBER_OF_ENTITIES])(int argc, const char **argv, MgfContext *context);
    int (*supportCallbacks[TOTAL_NUMBER_OF_ENTITIES])(int argc, const char **argv, MgfContext *context);
    char *currentMaterialName;
    int geometryStackHeadIndex;
    java::ArrayList<Geometry *> *geometryStack[MAXIMUM_GEOMETRY_STACK_DEPTH];

    // Those lists are transferred to MeshSurface objects, should not be deleted from context
    java::ArrayList<Vector3D *> *currentPointList;
    java::ArrayList<Vector3D *> *currentNormalList;
    java::ArrayList<Vertex *> *currentVertexList;
    java::ArrayList<Patch *> *currentFaceList;
    java::ArrayList<Geometry *> *currentGeometryList;
    char *currentObjectName;

    MgfTransformContext *transformContext;
    MgfColorContext *unNamedColorContext;
    MgfColorContext *currentColor;
    bool inSurface;
    bool inComplex;
    LookUpTable *vertexLookUpTable;
    java::ArrayList<Geometry *> *allGeometries;

    // Return model
    java::ArrayList<Geometry *> *geometries;
    java::ArrayList<Material *> *materials;

    MgfContext();
    ~MgfContext();
};

#include "io/mgf/MgfTransformContext.h"
#include "io/mgf/MgfColorContext.h"
#include "io/mgf/lookup.h"

#endif

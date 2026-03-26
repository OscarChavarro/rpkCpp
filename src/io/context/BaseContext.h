#ifndef __BASE_CONTEXT__
#define __BASE_CONTEXT__

#include "scene/RadianceMethod.h"
#include "io/mgf/MgfEntity.h"
#include "io/mgf/MgfErrorCode.h"
#include "io/context/ReaderContext.h"

// Objects 'o' contexts can be nested this deep
constexpr int MAXIMUM_GEOMETRY_STACK_DEPTH = 100;

class TransformContext;
class ColorContext;
class LookUpTable;
class MgfEntityHandler;
class PersistedSceneModel;

class BaseContext {
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
    const char *errorCodeMessages[MgfErrorCode::MGF_NUMBER_OF_ERRORS];
    ReaderContext *readerContext;
    MgfEntityHandler *handleCallbacks[TOTAL_NUMBER_OF_ENTITIES];
    MgfEntityHandler *supportCallbacks[TOTAL_NUMBER_OF_ENTITIES];
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

    TransformContext *transformContext;
    ColorContext *unNamedColorContext;
    ColorContext *currentColor;
    bool inSurface;
    bool inComplex;
    LookUpTable *vertexLookUpTable;
    java::ArrayList<Geometry *> *allGeometries;

    // Return model
    java::ArrayList<Geometry *> *geometries;
    java::ArrayList<Material *> *materials;
    PersistedSceneModel *model;

    BaseContext();
    ~BaseContext();
};

#include "io/context/TransformContext.h"
#include "io/context/ColorContext.h"
#include "io/mgf/LookUpTable.h"

#endif

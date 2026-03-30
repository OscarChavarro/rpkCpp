#ifndef __MGF_PARSE_SESSION__
#define __MGF_PARSE_SESSION__

#include "scene/RadianceMethod.h"
#include "io/context/TransformStackContext.h"
#include "io/context/ColorContext.h"
#include "io/context/LookUpTable.h"

#include "io/context/ParserConfig.h"
#include "io/context/ReaderStackState.h"
#include "io/context/GeometryBuildState.h"
#include "io/context/MaterialState.h"
#include "io/context/ColorRepository.h"
#include "io/context/MaterialRepository.h"
#include "io/context/VertexRepository.h"
#include "io/context/ObjectHierarchyState.h"
#include "io/context/TransformStack.h"

namespace java {
    template <class T>
    class ArrayList;
}

class Geometry;
class Material;
class EntityHandler;
class Patch;
class PersistedSceneModel;
class ReaderContext;
class Vector3D;
class Vertex;

class ParseSession {
  public:
    ParserConfig parserConfig;
    ReaderStackState readerStackState;
    GeometryBuildState geometryBuildState;
    MaterialState materialState;
    ColorRepository colorRepository;
    MaterialRepository materialRepository;
    VertexRepository vertexRepository;
    ObjectHierarchyState objectHierarchyState;
    TransformStack transformStack;

    PersistedSceneModel *model;

    // Transitional aliases to keep current code building while the call sites
    // migrate to explicit sub-state access.
    using EntityNamesArray = char[TOTAL_NUMBER_OF_ENTITIES][MGF_MAXIMUM_ENTITY_NAME_LENGTH];
    using ErrorMessagesArray = const char *[ErrorCodeContext::MGF_NUMBER_OF_ERRORS];
    using HandlerArray = EntityHandler *[TOTAL_NUMBER_OF_ENTITIES];
    using GeometryStackArray = java::ArrayList<Geometry *> *[MAXIMUM_GEOMETRY_STACK_DEPTH];

    RadianceMethod *&radianceMethod;
    bool &singleSided;
    char *&currentVertexName;
    int &numberOfQuarterCircleDivisions;
    bool &monochrome;
    Material *&currentMaterial;
    EntityNamesArray &entityNames;
    ErrorMessagesArray &errorCodeMessages;
    LookUpTable &entityLookUpTable;
    int &nextFileContextId;
    ReaderContext *&readerContext;
    HandlerArray &handleCallbacks;
    HandlerArray &supportCallbacks;
    char *&currentMaterialName;
    int &geometryStackHeadIndex;
    GeometryStackArray &geometryStack;
    java::ArrayList<Vector3D *> *&currentPointList;
    java::ArrayList<Vector3D *> *&currentNormalList;
    java::ArrayList<Vertex *> *&currentVertexList;
    java::ArrayList<Patch *> *&currentFaceList;
    java::ArrayList<Geometry *> *&currentGeometryList;
    char *&currentObjectName;
    TransformStackContext *&transformContext;
    ColorContext *&unNamedColorContext;
    ColorContext *&currentColor;
    bool &inSurface;
    bool &inComplex;
    bool &warpConeEnds;
    LookUpTable *&vertexLookUpTable;
    java::ArrayList<Geometry *> *&allGeometries;
    java::ArrayList<Geometry *> *&geometries;
    java::ArrayList<Material *> *&materials;

    ParseSession();
    ~ParseSession();

    ParseSession(const ParseSession &) = delete;
    ParseSession &operator=(const ParseSession &) = delete;
};

#endif

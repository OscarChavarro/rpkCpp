#ifndef __MGF_PARSE_SESSION__
#define __MGF_PARSE_SESSION__

#include "scene/RadianceMethod.h"
#include "common/linealAlgebra/Vector3D.h"
#include "io/context/TransformStackContext.h"
#include "io/context/ColorContext.h"
#include "io/context/ParseContext.h"
#include "io/context/LookUpTable.h"
#include "io/context/PersistedSceneModel.h"
#include "io/context/ReaderContext.h"
#include "io/context/EntityContextInfo.h"

#include "io/context/ParserConfig.h"
#include "io/context/ReaderStackState.h"
#include "io/context/GeometryBuildState.h"
#include "io/context/MaterialState.h"
#include "io/context/ColorRepository.h"
#include "io/context/MaterialRepository.h"
#include "io/context/VertexRepository.h"
#include "io/context/ObjectHierarchyState.h"
#include "io/context/TransformStack.h"
#include "material/Material.h"
#include "skin/Geometry.h"
#include "skin/Patch.h"
#include "skin/Vertex.h"

class ParseSession final : public ParseContext {
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
    using EntityNamesArray = char[TOTAL_NUMBER_OF_ENTITIES][EntityContextInfo::MGF_MAXIMUM_ENTITY_NAME_LENGTH];
    using ErrorMessagesArray = const char *[ErrorCodeContext::MGF_NUMBER_OF_ERRORS];
    using GeometryStackArray = java::ArrayList<Geometry *> *[GeometryBuildState::MAXIMUM_GEOMETRY_STACK_DEPTH];

    RadianceMethod *&radianceMethod;
    bool &singleSided;
    char *&currentVertexName;
    int &numberOfQuarterCircleDivisions;
    bool &monochrome;
    Material *&currentMaterial;
    EntityNamesArray &entityNames;
    ErrorMessagesArray &errorCodeMessages;
    LookUpTable<char *> &entityLookUpTable;
    int &nextFileContextId;
    ReaderContext *&readerContext;
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
    LookUpTable<char *> *&vertexLookUpTable;
    java::ArrayList<Geometry *> *&allGeometries;
    java::ArrayList<Geometry *> *&geometries;
    java::ArrayList<Material *> *&materials;

    ParseSession();
    ~ParseSession();

    ParseSession(const ParseSession &) = delete;
    ParseSession &operator=(const ParseSession &) = delete;
};

#endif

#ifndef MGF_PARSE_SESSION__
#define MGF_PARSE_SESSION__

#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/io/context/TransformStackContext.h"
#include "vsdk/toolkit/io/context/ColorContext.h"
#include "vsdk/toolkit/io/context/ParseContext.h"
#include "vsdk/toolkit/common/dataStructures/LookUpTable.h"
#include "vsdk/toolkit/io/context/ParseSnapshotContext.h"
#include "vsdk/toolkit/io/context/ReaderContext.h"
#include "vsdk/toolkit/io/context/EntityNamingContext.h"

#include "vsdk/toolkit/io/context/ParseOptionsContext.h"
#include "vsdk/toolkit/io/context/ReaderDispatchContext.h"
#include "vsdk/toolkit/io/context/GeometryAssemblyContext.h"
#include "vsdk/toolkit/io/context/MaterialSelectionContext.h"
#include "vsdk/toolkit/io/context/ColorRegistryContext.h"
#include "vsdk/toolkit/io/context/MaterialRegistryContext.h"
#include "vsdk/toolkit/io/context/VertexRegistryContext.h"
#include "vsdk/toolkit/io/context/ObjectScopeContext.h"
#include "vsdk/toolkit/io/context/TransformScopeContext.h"
#include "vsdk/toolkit/material/Material.h"
#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/Vertex.h"

class ParseRuntimeContext final : public ParseContext {
  public:
    ParseOptionsContext parserConfig;
    ReaderDispatchContext readerStackState;
    GeometryAssemblyContext geometryBuildState;
    MaterialSelectionContext materialState;
    ColorRegistryContext colorRepository;
    MaterialRegistryContext materialRepository;
    VertexRegistryContext vertexRepository;
    ObjectScopeContext objectHierarchyState;
    TransformScopeContext transformStack;

    ParseSnapshotContext *model;

    // Transitional aliases to keep current code building while the call sites
    // migrate to explicit sub-state access.
    using EntityNamesArray = char[TOTAL_NUMBER_OF_ENTITIES][EntityNamingContext::MGF_MAXIMUM_ENTITY_NAME_LENGTH];
    using ErrorMessagesArray = const char *[ParseErrorContext::MGF_NUMBER_OF_ERRORS];
    using GeometryStackArray = java::ArrayList<Geometry *> *[GeometryAssemblyContext::MAXIMUM_GEOMETRY_STACK_DEPTH];

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

    ParseRuntimeContext();
    ~ParseRuntimeContext();

    ParseRuntimeContext(const ParseRuntimeContext &) = delete;
    ParseRuntimeContext &operator=(const ParseRuntimeContext &) = delete;
};

#endif

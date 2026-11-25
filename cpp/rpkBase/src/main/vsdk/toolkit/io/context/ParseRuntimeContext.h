#ifndef __PARSE_RUNTIME_CONTEXT__
#define __PARSE_RUNTIME_CONTEXT__

#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
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
#include "vsdk/toolkit/material/RpkMaterial.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"

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
    using GeometryStackArray = java::ArrayList<RpkGeometry *> *[GeometryAssemblyContext::MAXIMUM_GEOMETRY_STACK_DEPTH];

    RadianceMethod *&radianceMethod;
    bool &singleSided;
    char *&currentVertexName;
    int &numberOfQuarterCircleDivisions;
    bool &monochrome;
    RpkMaterial *&currentMaterial;
    EntityNamesArray &entityNames;
    ErrorMessagesArray &errorCodeMessages;
    LookUpTable<char *> &entityLookUpTable;
    int &nextFileContextId;
    ReaderContext *&readerContext;
    char *&currentMaterialName;
    int &geometryStackHeadIndex;
    GeometryStackArray &geometryStack;
    java::ArrayList<Vector3Df *> *&currentPointList;
    java::ArrayList<Vector3Df *> *&currentNormalList;
    java::ArrayList<RpkVertex *> *&currentVertexList;
    java::ArrayList<Patch *> *&currentFaceList;
    java::ArrayList<RpkGeometry *> *&currentGeometryList;
    char *&currentObjectName;
    TransformStackContext *&transformContext;
    ColorContext *&unNamedColorContext;
    ColorContext *&currentColor;
    bool &inSurface;
    bool &inComplex;
    bool &warpConeEnds;
    LookUpTable<char *> *&vertexLookUpTable;
    java::ArrayList<RpkGeometry *> *&allGeometries;
    java::ArrayList<RpkGeometry *> *&geometries;
    java::ArrayList<RpkMaterial *> *&materials;

    ParseRuntimeContext();
    ~ParseRuntimeContext();

    ParseRuntimeContext(const ParseRuntimeContext &) = delete;
    ParseRuntimeContext &operator=(const ParseRuntimeContext &) = delete;
};

#endif

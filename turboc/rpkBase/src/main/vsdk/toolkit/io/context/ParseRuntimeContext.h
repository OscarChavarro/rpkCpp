#ifndef __MGF_PARSE_SESSION__
#define __MGF_PARSE_SESSION__

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
#include "vsdk/toolkit/material/Material.h"
#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/Vertex.h"

class ParseRuntimeContext: public ParseContext{ public:
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

    typedef char EntityNamesArray[TOTAL_NUMBER_OF_ENTITIES][EntityNamingContext::MGF_MAXIMUM_ENTITY_NAME_LENGTH];
    typedef const char *ErrorMessagesArray[MGF_NUMBER_OF_ERRORS];
    typedef ArrayList<Geometry *> *GeometryStackArray[GeometryAssemblyContext::MAXIMUM_GEOMETRY_STACK_DEPTH];

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
    ArrayList<Vector3Df *> *&currentPointList;
    ArrayList<Vector3Df *> *&currentNormalList;
    ArrayList<Vertex *> *&currentVertexList;
    ArrayList<Patch *> *&currentFaceList;
    ArrayList<Geometry *> *&currentGeometryList;
    char *&currentObjectName;
    TransformStackContext *&transformContext;
    ColorContext *&unNamedColorContext;
    ColorContext *&currentColor;
    bool &inSurface;
    bool &inComplex;
    bool &warpConeEnds;
    LookUpTable<char *> *&vertexLookUpTable;
    ArrayList<Geometry *> *&allGeometries;
    ArrayList<Geometry *> *&geometries;
    ArrayList<Material *> *&materials;

    ParseRuntimeContext();
    ~ParseRuntimeContext();

  private:
    ParseRuntimeContext(const ParseRuntimeContext &);
    ParseRuntimeContext &operator=(const ParseRuntimeContext &);
};

#endif

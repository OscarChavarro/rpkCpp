#include "io/context/ParseSession.h"
#include "io/context/PersistedSceneModel.h"

ParseSession::ParseSession():
    parserConfig(),
    readerStackState(),
    geometryBuildState(),
    materialState(),
    colorRepository(),
    materialRepository(),
    vertexRepository(),
    objectHierarchyState(),
    transformStack(),
    model(nullptr),
    radianceMethod(parserConfig.radianceMethod),
    singleSided(parserConfig.singleSided),
    currentVertexName(geometryBuildState.currentVertexName),
    numberOfQuarterCircleDivisions(parserConfig.numberOfQuarterCircleDivisions),
    monochrome(parserConfig.monochrome),
    currentMaterial(materialState.currentMaterial),
    entityNames(readerStackState.entityNames),
    errorCodeMessages(readerStackState.errorCodeMessages),
    entityLookUpTable(readerStackState.entityLookUpTable),
    nextFileContextId(readerStackState.nextFileContextId),
    readerContext(readerStackState.readerContext),
    handleCallbacks(readerStackState.handleCallbacks),
    supportCallbacks(readerStackState.supportCallbacks),
    currentMaterialName(materialState.currentMaterialName),
    geometryStackHeadIndex(geometryBuildState.geometryStackHeadIndex),
    geometryStack(geometryBuildState.geometryStack),
    currentPointList(geometryBuildState.currentPointList),
    currentNormalList(geometryBuildState.currentNormalList),
    currentVertexList(geometryBuildState.currentVertexList),
    currentFaceList(geometryBuildState.currentFaceList),
    currentGeometryList(geometryBuildState.currentGeometryList),
    currentObjectName(geometryBuildState.currentObjectName),
    transformContext(transformStack.transformContext),
    unNamedColorContext(colorRepository.unNamedColorContext),
    currentColor(colorRepository.currentColor),
    inSurface(geometryBuildState.inSurface),
    inComplex(geometryBuildState.inComplex),
    warpConeEnds(geometryBuildState.warpConeEnds),
    vertexLookUpTable(vertexRepository.vertexLookUpTable),
    allGeometries(geometryBuildState.allGeometries),
    geometries(geometryBuildState.geometries),
    materials(materialState.materials)
{
}

ParseSession::~ParseSession() {
    if ( model != nullptr ) {
        delete model;
        model = nullptr;
    }
}

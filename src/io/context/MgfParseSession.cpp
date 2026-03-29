#include "io/context/MgfParseSession.h"
#include "io/context/PersistedSceneModel.h"

MgfParseSession::MgfParseSession():
    parserConfig(),
    readerStackState(),
    geometryBuildState(),
    materialState(),
    colorState(),
    transformState(),
    model(nullptr),
    radianceMethod(parserConfig.radianceMethod),
    singleSided(parserConfig.singleSided),
    currentVertexName(geometryBuildState.currentVertexName),
    numberOfQuarterCircleDivisions(parserConfig.numberOfQuarterCircleDivisions),
    monochrome(parserConfig.monochrome),
    currentMaterial(materialState.currentMaterial),
    entityNames(readerStackState.entityNames),
    errorCodeMessages(readerStackState.errorCodeMessages),
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
    transformContext(transformState.transformContext),
    unNamedColorContext(colorState.unNamedColorContext),
    currentColor(colorState.currentColor),
    inSurface(geometryBuildState.inSurface),
    inComplex(geometryBuildState.inComplex),
    vertexLookUpTable(geometryBuildState.vertexLookUpTable),
    allGeometries(geometryBuildState.allGeometries),
    geometries(geometryBuildState.geometries),
    materials(materialState.materials)
{
}

MgfParseSession::~MgfParseSession() {
    if ( model != nullptr ) {
        delete model;
        model = nullptr;
    }
}

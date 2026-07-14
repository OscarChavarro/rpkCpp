#ifndef PERSISTED_SCENE_MODEL__
#define PERSISTED_SCENE_MODEL__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/io/context/ColorContext.h"
#include "vsdk/toolkit/io/context/ReaderContext.h"
#include "vsdk/toolkit/io/context/TransformStackContext.h"
#include "vsdk/toolkit/material/Material.h"
#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/Vertex.h"

class ParseSnapshotContext {
  public:
    // Snapshot of MGF parsing state and outputs (non-owning references).
    ColorContext *currentColor;
    java::ArrayList<Patch *> *currentFaceList;
    java::ArrayList<Geometry *> *currentGeometryList;
    char *currentMaterialName;
    java::ArrayList<Vector3D *> *currentNormalList;
    char *currentObjectName;
    java::ArrayList<Vector3D *> *currentPointList;
    java::ArrayList<Vertex *> *currentVertexList;
    char *currentVertexName;
    java::ArrayList<Geometry *> *geometries;
    int geometryStackHeadIndex;
    bool inComplex;
    bool inSurface;
    java::ArrayList<Material *> *materials;
    bool monochrome;
    ReaderContext *readerContext;
    TransformStackContext *transformContext;

    ParseSnapshotContext();
};

#endif

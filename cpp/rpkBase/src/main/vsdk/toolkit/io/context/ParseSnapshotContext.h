#ifndef __PARSE_SNAPSHOT_CONTEXT__
#define __PARSE_SNAPSHOT_CONTEXT__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/io/context/ColorContext.h"
#include "vsdk/toolkit/io/context/ReaderContext.h"
#include "vsdk/toolkit/io/context/TransformStackContext.h"
#include "vsdk/toolkit/material/RpkMaterial.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"

class ParseSnapshotContext {
  public:
    // Snapshot of MGF parsing state and outputs (non-owning references).
    ColorContext *currentColor;
    java::ArrayList<Patch *> *currentFaceList;
    java::ArrayList<RpkGeometry *> *currentGeometryList;
    char *currentMaterialName;
    java::ArrayList<Vector3Df *> *currentNormalList;
    char *currentObjectName;
    java::ArrayList<Vector3Df *> *currentPointList;
    java::ArrayList<RpkVertex *> *currentVertexList;
    char *currentVertexName;
    java::ArrayList<RpkGeometry *> *geometries;
    int geometryStackHeadIndex;
    bool inComplex;
    bool inSurface;
    java::ArrayList<RpkMaterial *> *materials;
    bool monochrome;
    ReaderContext *readerContext;
    TransformStackContext *transformContext;

    ParseSnapshotContext();
};

#endif

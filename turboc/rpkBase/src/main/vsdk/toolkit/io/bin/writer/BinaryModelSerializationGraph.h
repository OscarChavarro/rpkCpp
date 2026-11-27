#ifndef BNRY_MDL_WRTR_SRLZT_CNTXT
#define BNRY_MDL_WRTR_SRLZT_CNTXT

#include "java/util/ArrayList.h"
#include "java/util/HashMap.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/io/context/ColorContext.h"
#include "vsdk/toolkit/io/context/ParseSnapshotContext.h"
#include "vsdk/toolkit/io/context/ReaderContext.h"
#include "vsdk/toolkit/io/context/TransformSequenceContext.h"
#include "vsdk/toolkit/io/context/TransformStackContext.h"
#include "vsdk/toolkit/material/RpkMaterial.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"

class BinaryModelSerializationGraph {
  public:
    HashMap<const Vector3Dd *, int> vectorIndices;
    ArrayList<const Vector3Dd *> vectors;

    HashMap<const RpkVertex *, int> vertexIndices;
    ArrayList<const RpkVertex *> vertices;

    HashMap<const Patch *, int> patchIndices;
    ArrayList<const Patch *> patches;

    HashMap<const RpkMaterial *, int> materialIndices;
    ArrayList<const RpkMaterial *> materials;

    HashMap<const RpkGeometry *, int> geometryIndices;
    ArrayList<const RpkGeometry *> geometries;

    HashMap<const ColorContext *, int> colorContextIndices;
    ArrayList<const ColorContext *> colorContexts;

    HashMap<const ReaderContext *, int> readerContextIndices;
    ArrayList<const ReaderContext *> readerContexts;

    HashMap<const TransformSequenceContext *, int> transformArrayIndices;
    ArrayList<const TransformSequenceContext *> transformArrays;

    HashMap<const TransformStackContext *, int> transformContextIndices;
    ArrayList<const TransformStackContext *> transformContexts;

    bool ensureVector(const Vector3Dd *value);
    bool ensureMaterial(const RpkMaterial *value);
    bool ensureVertex(const RpkVertex *value);
    bool ensurePatch(const Patch *value);
    bool ensureGeometry(const RpkGeometry *value);
    bool ensureColorContext(const ColorContext *value);
    bool ensureReaderContext(const ReaderContext *value);
    bool ensureTransformArray(const TransformSequenceContext *value);
    bool ensureTransformContext(const TransformStackContext *value);

    bool collectVectorList(const ArrayList<Vector3Dd *> *list);
    bool collectVertexList(const ArrayList<RpkVertex *> *list);
    bool collectPatchList(const ArrayList<Patch *> *list);
    bool collectMaterialList(const ArrayList<RpkMaterial *> *list);
    bool collectGeometryList(const ArrayList<RpkGeometry *> *list);
    bool collectModel(const ParseSnapshotContext *model);
};

#endif

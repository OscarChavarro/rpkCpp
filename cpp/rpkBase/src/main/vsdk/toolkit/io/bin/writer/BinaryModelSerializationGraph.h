#ifndef __BINARY_MODEL_SERIALIZATION_GRAPH__
#define __BINARY_MODEL_SERIALIZATION_GRAPH__

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
    java::HashMap<const Vector3Dd *, int> vectorIndices;
    java::ArrayList<const Vector3Dd *> vectors;

    java::HashMap<const RpkVertex *, int> vertexIndices;
    java::ArrayList<const RpkVertex *> vertices;

    java::HashMap<const Patch *, int> patchIndices;
    java::ArrayList<const Patch *> patches;

    java::HashMap<const RpkMaterial *, int> materialIndices;
    java::ArrayList<const RpkMaterial *> materials;

    java::HashMap<const RpkGeometry *, int> geometryIndices;
    java::ArrayList<const RpkGeometry *> geometries;

    java::HashMap<const ColorContext *, int> colorContextIndices;
    java::ArrayList<const ColorContext *> colorContexts;

    java::HashMap<const ReaderContext *, int> readerContextIndices;
    java::ArrayList<const ReaderContext *> readerContexts;

    java::HashMap<const TransformSequenceContext *, int> transformArrayIndices;
    java::ArrayList<const TransformSequenceContext *> transformArrays;

    java::HashMap<const TransformStackContext *, int> transformContextIndices;
    java::ArrayList<const TransformStackContext *> transformContexts;

    bool ensureVector(const Vector3Dd *value);
    bool ensureMaterial(const RpkMaterial *value);
    bool ensureVertex(const RpkVertex *value);
    bool ensurePatch(const Patch *value);
    bool ensureGeometry(const RpkGeometry *value);
    bool ensureColorContext(const ColorContext *value);
    bool ensureReaderContext(const ReaderContext *value);
    bool ensureTransformArray(const TransformSequenceContext *value);
    bool ensureTransformContext(const TransformStackContext *value);

    bool collectVectorList(const java::ArrayList<Vector3Dd *> *list);
    bool collectVertexList(const java::ArrayList<RpkVertex *> *list);
    bool collectPatchList(const java::ArrayList<Patch *> *list);
    bool collectMaterialList(const java::ArrayList<RpkMaterial *> *list);
    bool collectGeometryList(const java::ArrayList<RpkGeometry *> *list);
    bool collectModel(const ParseSnapshotContext *model);
};

#endif

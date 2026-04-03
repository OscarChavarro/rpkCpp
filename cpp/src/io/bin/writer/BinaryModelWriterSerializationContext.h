#ifndef __BINARY_MODEL_WRITER_SERIALIZATION_CONTEXT__
#define __BINARY_MODEL_WRITER_SERIALIZATION_CONTEXT__

#include "java/util/ArrayList.h"
#include "java/util/HashMap.h"
#include "common/linealAlgebra/Vector3D.h"
#include "io/context/ColorContext.h"
#include "io/context/ParseSnapshotContext.h"
#include "io/context/ReaderContext.h"
#include "io/context/TransformSequenceContext.h"
#include "io/context/TransformStackContext.h"
#include "material/Material.h"
#include "skin/Geometry.h"
#include "skin/Patch.h"
#include "skin/Vertex.h"

class BinaryModelWriterSerializationContext {
  public:
    java::HashMap<const Vector3D *, int> vectorIndices;
    java::ArrayList<const Vector3D *> vectors;

    java::HashMap<const Vertex *, int> vertexIndices;
    java::ArrayList<const Vertex *> vertices;

    java::HashMap<const Patch *, int> patchIndices;
    java::ArrayList<const Patch *> patches;

    java::HashMap<const Material *, int> materialIndices;
    java::ArrayList<const Material *> materials;

    java::HashMap<const Geometry *, int> geometryIndices;
    java::ArrayList<const Geometry *> geometries;

    java::HashMap<const ColorContext *, int> colorContextIndices;
    java::ArrayList<const ColorContext *> colorContexts;

    java::HashMap<const ReaderContext *, int> readerContextIndices;
    java::ArrayList<const ReaderContext *> readerContexts;

    java::HashMap<const TransformSequenceContext *, int> transformArrayIndices;
    java::ArrayList<const TransformSequenceContext *> transformArrays;

    java::HashMap<const TransformStackContext *, int> transformContextIndices;
    java::ArrayList<const TransformStackContext *> transformContexts;

    bool ensureVector(const Vector3D *value);
    bool ensureMaterial(const Material *value);
    bool ensureVertex(const Vertex *value);
    bool ensurePatch(const Patch *value);
    bool ensureGeometry(const Geometry *value);
    bool ensureColorContext(const ColorContext *value);
    bool ensureReaderContext(const ReaderContext *value);
    bool ensureTransformArray(const TransformSequenceContext *value);
    bool ensureTransformContext(const TransformStackContext *value);

    bool collectVectorList(const java::ArrayList<Vector3D *> *list);
    bool collectVertexList(const java::ArrayList<Vertex *> *list);
    bool collectPatchList(const java::ArrayList<Patch *> *list);
    bool collectMaterialList(const java::ArrayList<Material *> *list);
    bool collectGeometryList(const java::ArrayList<Geometry *> *list);
    bool collectModel(const ParseSnapshotContext *model);
};

#endif

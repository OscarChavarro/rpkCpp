#ifndef __BINARY_MODEL_READER_CLEANUP__
#define __BINARY_MODEL_READER_CLEANUP__

#include "java/util/ArrayList.h"
#include "common/linealAlgebra/Vector3D.h"
#include "io/bin/reader/BinaryModelGeometryRecordData.h"
#include "io/bin/reader/BinaryModelSnapshotRecordData.h"
#include "io/bin/reader/BinaryModelVertexRecordData.h"
#include "io/context/ColorContext.h"
#include "io/context/ParseSnapshotContext.h"
#include "io/context/ReaderContext.h"
#include "io/context/TransformSequenceContext.h"
#include "io/context/TransformStackContext.h"
#include "material/Material.h"
#include "skin/Geometry.h"
#include "environment/geometry/elements/Patch.h"
#include "environment/geometry/elements/Vertex.h"

class BinaryModelReadCleanup {
  public:
    static void cleanupPartialModel(
        java::ArrayList<Vector3D *> &vectors,
        java::ArrayList<Vertex *> &vertices,
        java::ArrayList<Patch *> &patches,
        java::ArrayList<Material *> &materials,
        java::ArrayList<Geometry *> &geometries,
        java::ArrayList<ColorContext *> &colorContexts,
        java::ArrayList<ReaderContext *> &readerContexts,
        java::ArrayList<TransformSequenceContext *> &transformArrays,
        java::ArrayList<TransformStackContext *> &transformContexts,
        ParseSnapshotContext *model);
    static void releaseVertexRecordIndexLists(java::ArrayList<BinaryModelVertexRecordData> &vertexRecords);
    static void releaseGeometryRecordIndexLists(java::ArrayList<BinaryModelGeometryRecordData> &geometryRecords);
    static void releaseModelRecordIndexLists(BinaryModelSnapshotRecordData *modelRecord);
};

#endif

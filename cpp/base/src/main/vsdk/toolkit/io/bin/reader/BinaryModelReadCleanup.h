#ifndef BINARY_MODEL_READER_CLEANUP__
#define BINARY_MODEL_READER_CLEANUP__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelGeometryRecordData.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelSnapshotRecordData.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelVertexRecordData.h"
#include "vsdk/toolkit/io/context/ColorContext.h"
#include "vsdk/toolkit/io/context/ParseSnapshotContext.h"
#include "vsdk/toolkit/io/context/ReaderContext.h"
#include "vsdk/toolkit/io/context/TransformSequenceContext.h"
#include "vsdk/toolkit/io/context/TransformStackContext.h"
#include "vsdk/toolkit/material/Material.h"
#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/Vertex.h"

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

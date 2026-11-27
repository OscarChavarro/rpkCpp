#ifndef __BINARY_MODEL_READ_CLEANUP__
#define __BINARY_MODEL_READ_CLEANUP__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelGeometryRecordData.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelSnapshotRecordData.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelVertexRecordData.h"
#include "vsdk/toolkit/io/context/ColorContext.h"
#include "vsdk/toolkit/io/context/ParseSnapshotContext.h"
#include "vsdk/toolkit/io/context/ReaderContext.h"
#include "vsdk/toolkit/io/context/TransformSequenceContext.h"
#include "vsdk/toolkit/io/context/TransformStackContext.h"
#include "vsdk/toolkit/material/RpkMaterial.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"

class BinaryModelReadCleanup {
  public:
    static void cleanupPartialModel(
        ArrayList<Vector3Dd *> &vectors,
        ArrayList<RpkVertex *> &vertices,
        ArrayList<Patch *> &patches,
        ArrayList<RpkMaterial *> &materials,
        ArrayList<RpkGeometry *> &geometries,
        ArrayList<ColorContext *> &colorContexts,
        ArrayList<ReaderContext *> &readerContexts,
        ArrayList<TransformSequenceContext *> &transformArrays,
        ArrayList<TransformStackContext *> &transformContexts,
        ParseSnapshotContext *model);
    static void releaseVertexRecordIndexLists(ArrayList<BinaryModelVertexRecordData> &vertexRecords);
    static void releaseGeometryRecordIndexLists(ArrayList<BinaryModelGeometryRecordData> &geometryRecords);
    static void releaseModelRecordIndexLists(BinaryModelSnapshotRecordData *modelRecord);
};

#endif

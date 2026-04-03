#ifndef __BINARY_MODEL_READER_CLEANUP__
#define __BINARY_MODEL_READER_CLEANUP__

#include "java/util/ArrayList.h"
#include "common/linealAlgebra/Vector3D.h"
#include "io/bin/reader/BinaryModelReaderGeometryRecord.h"
#include "io/bin/reader/BinaryModelReaderModelRecord.h"
#include "io/bin/reader/BinaryModelReaderVertexRecord.h"
#include "io/context/ColorContext.h"
#include "io/context/PersistedSceneModel.h"
#include "io/context/ReaderContext.h"
#include "io/context/TransformArray.h"
#include "io/context/TransformStackContext.h"
#include "material/Material.h"
#include "skin/Geometry.h"
#include "skin/Patch.h"
#include "skin/Vertex.h"

class BinaryModelReaderCleanup {
  public:
    static void cleanupPartialModel(
        java::ArrayList<Vector3D *> &vectors,
        java::ArrayList<Vertex *> &vertices,
        java::ArrayList<Patch *> &patches,
        java::ArrayList<Material *> &materials,
        java::ArrayList<Geometry *> &geometries,
        java::ArrayList<ColorContext *> &colorContexts,
        java::ArrayList<ReaderContext *> &readerContexts,
        java::ArrayList<TransformArray *> &transformArrays,
        java::ArrayList<TransformStackContext *> &transformContexts,
        PersistedSceneModel *model);
    static void releaseVertexRecordIndexLists(java::ArrayList<BinaryModelReaderVertexRecord> &vertexRecords);
    static void releaseGeometryRecordIndexLists(java::ArrayList<BinaryModelReaderGeometryRecord> &geometryRecords);
    static void releaseModelRecordIndexLists(BinaryModelReaderModelRecord *modelRecord);
};

#endif

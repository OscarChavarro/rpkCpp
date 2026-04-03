#include "java/util/ArrayList.txx"

#include "common/linealAlgebra/Vector3D.h"
#include "material/Material.h"
#include "skin/Geometry.h"
#include "skin/Patch.h"
#include "skin/Vertex.h"
#include "io/context/ColorContext.h"
#include "io/context/PersistedSceneModel.h"
#include "io/context/ReaderContext.h"
#include "io/context/TransformArray.h"
#include "io/context/TransformStackContext.h"
#include "io/bin/reader/BinaryModelReaderVertexRecord.h"
#include "io/bin/reader/BinaryModelReaderGeometryRecord.h"
#include "io/bin/reader/BinaryModelReaderModelRecord.h"
#include "io/bin/reader/BinaryModelReaderSupport.h"
#include "io/bin/reader/BinaryModelReaderCleanup.h"

void
BinaryModelReaderCleanup::cleanupPartialModel(
    java::ArrayList<Vector3D *> &vectors,
    java::ArrayList<Vertex *> &vertices,
    java::ArrayList<Patch *> &patches,
    java::ArrayList<Material *> &materials,
    java::ArrayList<Geometry *> &geometries,
    java::ArrayList<ColorContext *> &colorContexts,
    java::ArrayList<ReaderContext *> &readerContexts,
    java::ArrayList<TransformArray *> &transformArrays,
    java::ArrayList<TransformStackContext *> &transformContexts,
    PersistedSceneModel *model)
{
    const bool hasGeometry = geometries.size() > 0;
    bool hasSurfaceGeometry = false;
    for ( long int i = 0; i < geometries.size(); i++ ) {
        Geometry *geometry = geometries.get(i);
        if ( geometry != nullptr && geometry->className == GeometryClassId::SURFACE_MESH ) {
            hasSurfaceGeometry = true;
            break;
        }
    }

    for ( long int i = 0; i < geometries.size(); i++ ) {
        delete geometries.get(i);
    }

    if ( !hasGeometry || !hasSurfaceGeometry ) {
        for ( long int i = 0; i < patches.size(); i++ ) {
            delete patches.get(i);
        }
        for ( long int i = 0; i < vertices.size(); i++ ) {
            delete vertices.get(i);
        }
        for ( long int i = 0; i < vectors.size(); i++ ) {
            delete vectors.get(i);
        }
    }

    for ( long int i = 0; i < materials.size(); i++ ) {
        delete materials.get(i);
    }
    for ( long int i = 0; i < colorContexts.size(); i++ ) {
        delete colorContexts.get(i);
    }
    for ( long int i = 0; i < readerContexts.size(); i++ ) {
        delete readerContexts.get(i);
    }
    for ( long int i = 0; i < transformArrays.size(); i++ ) {
        delete transformArrays.get(i);
    }
    for ( long int i = 0; i < transformContexts.size(); i++ ) {
        delete transformContexts.get(i);
    }

    if ( model != nullptr ) {
        delete[] model->currentMaterialName;
        delete[] model->currentObjectName;
        delete[] model->currentVertexName;
        delete model;
    }
}

void
BinaryModelReaderCleanup::releaseVertexRecordIndexLists(java::ArrayList<BinaryModelReaderVertexRecord> &vertexRecords) {
    for ( long int i = 0; i < vertexRecords.size(); i++ ) {
        BinaryModelReaderSupport::releaseIndexListRecord(&vertexRecords[i].patchIndices);
    }
}

void
BinaryModelReaderCleanup::releaseGeometryRecordIndexLists(java::ArrayList<BinaryModelReaderGeometryRecord> &geometryRecords) {
    for ( long int i = 0; i < geometryRecords.size(); i++ ) {
        BinaryModelReaderGeometryRecord &record = geometryRecords[i];
        if ( record.objectName != nullptr ) {
            delete[] record.objectName;
            record.objectName = nullptr;
        }
        record.hasObjectName = false;
        BinaryModelReaderSupport::releaseIndexListRecord(&record.positions);
        BinaryModelReaderSupport::releaseIndexListRecord(&record.normals);
        BinaryModelReaderSupport::releaseIndexListRecord(&record.vertices);
        BinaryModelReaderSupport::releaseIndexListRecord(&record.faces);
        BinaryModelReaderSupport::releaseIndexListRecord(&record.children);
        BinaryModelReaderSupport::releaseIndexListRecord(&record.patchSetPatches);
    }
}

void
BinaryModelReaderCleanup::releaseModelRecordIndexLists(BinaryModelReaderModelRecord *modelRecord) {
    if ( modelRecord == nullptr ) {
        return;
    }
    if ( modelRecord->currentMaterialName != nullptr ) {
        delete[] modelRecord->currentMaterialName;
        modelRecord->currentMaterialName = nullptr;
    }
    if ( modelRecord->currentObjectName != nullptr ) {
        delete[] modelRecord->currentObjectName;
        modelRecord->currentObjectName = nullptr;
    }
    if ( modelRecord->currentVertexName != nullptr ) {
        delete[] modelRecord->currentVertexName;
        modelRecord->currentVertexName = nullptr;
    }
    modelRecord->hasCurrentMaterialName = false;
    modelRecord->hasCurrentObjectName = false;
    modelRecord->hasCurrentVertexName = false;
    BinaryModelReaderSupport::releaseIndexListRecord(&modelRecord->currentFaceList);
    BinaryModelReaderSupport::releaseIndexListRecord(&modelRecord->currentGeometryList);
    BinaryModelReaderSupport::releaseIndexListRecord(&modelRecord->currentNormalList);
    BinaryModelReaderSupport::releaseIndexListRecord(&modelRecord->currentPointList);
    BinaryModelReaderSupport::releaseIndexListRecord(&modelRecord->currentVertexList);
    BinaryModelReaderSupport::releaseIndexListRecord(&modelRecord->geometries);
    BinaryModelReaderSupport::releaseIndexListRecord(&modelRecord->materials);
}

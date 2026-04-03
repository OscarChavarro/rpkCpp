#include "java/util/ArrayList.txx"

#include "common/linealAlgebra/Vector3D.h"
#include "material/Material.h"
#include "skin/Geometry.h"
#include "skin/Patch.h"
#include "skin/Vertex.h"
#include "io/context/ColorContext.h"
#include "io/context/ParseSnapshotContext.h"
#include "io/context/ReaderContext.h"
#include "io/context/TransformSequenceContext.h"
#include "io/context/TransformStackContext.h"
#include "io/bin/reader/BinaryModelVertexRecordData.h"
#include "io/bin/reader/BinaryModelGeometryRecordData.h"
#include "io/bin/reader/BinaryModelSnapshotRecordData.h"
#include "io/bin/reader/BinaryModelReadPrimitives.h"
#include "io/bin/reader/BinaryModelReadCleanup.h"

void
BinaryModelReadCleanup::cleanupPartialModel(
    java::ArrayList<Vector3D *> &vectors,
    java::ArrayList<Vertex *> &vertices,
    java::ArrayList<Patch *> &patches,
    java::ArrayList<Material *> &materials,
    java::ArrayList<Geometry *> &geometries,
    java::ArrayList<ColorContext *> &colorContexts,
    java::ArrayList<ReaderContext *> &readerContexts,
    java::ArrayList<TransformSequenceContext *> &transformArrays,
    java::ArrayList<TransformStackContext *> &transformContexts,
    ParseSnapshotContext *model)
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
BinaryModelReadCleanup::releaseVertexRecordIndexLists(java::ArrayList<BinaryModelVertexRecordData> &vertexRecords) {
    for ( long int i = 0; i < vertexRecords.size(); i++ ) {
        BinaryModelReadPrimitives::releaseIndexListRecord(&vertexRecords[i].patchIndices);
    }
}

void
BinaryModelReadCleanup::releaseGeometryRecordIndexLists(java::ArrayList<BinaryModelGeometryRecordData> &geometryRecords) {
    for ( long int i = 0; i < geometryRecords.size(); i++ ) {
        BinaryModelGeometryRecordData &record = geometryRecords[i];
        if ( record.objectName != nullptr ) {
            delete[] record.objectName;
            record.objectName = nullptr;
        }
        record.hasObjectName = false;
        BinaryModelReadPrimitives::releaseIndexListRecord(&record.positions);
        BinaryModelReadPrimitives::releaseIndexListRecord(&record.normals);
        BinaryModelReadPrimitives::releaseIndexListRecord(&record.vertices);
        BinaryModelReadPrimitives::releaseIndexListRecord(&record.faces);
        BinaryModelReadPrimitives::releaseIndexListRecord(&record.children);
        BinaryModelReadPrimitives::releaseIndexListRecord(&record.patchSetPatches);
    }
}

void
BinaryModelReadCleanup::releaseModelRecordIndexLists(BinaryModelSnapshotRecordData *modelRecord) {
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
    BinaryModelReadPrimitives::releaseIndexListRecord(&modelRecord->currentFaceList);
    BinaryModelReadPrimitives::releaseIndexListRecord(&modelRecord->currentGeometryList);
    BinaryModelReadPrimitives::releaseIndexListRecord(&modelRecord->currentNormalList);
    BinaryModelReadPrimitives::releaseIndexListRecord(&modelRecord->currentPointList);
    BinaryModelReadPrimitives::releaseIndexListRecord(&modelRecord->currentVertexList);
    BinaryModelReadPrimitives::releaseIndexListRecord(&modelRecord->geometries);
    BinaryModelReadPrimitives::releaseIndexListRecord(&modelRecord->materials);
}

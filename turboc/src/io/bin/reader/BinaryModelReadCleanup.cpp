#include "java/util/ArrayList.txx"

#include "common/linealAlgebra/Vector3D.h"
#include "material/Material.h"
#include "skin/Geometry.h"
#include "environment/geometry/elements/Patch.h"
#include "environment/geometry/elements/Vertex.h"
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
    ArrayList<Vector3D *> &vectors,
    ArrayList<Vertex *> &vertices,
    ArrayList<Patch *> &patches,
    ArrayList<Material *> &materials,
    ArrayList<Geometry *> &geometries,
    ArrayList<ColorContext *> &colorContexts,
    ArrayList<ReaderContext *> &readerContexts,
    ArrayList<TransformSequenceContext *> &transformArrays,
    ArrayList<TransformStackContext *> &transformContexts,
    ParseSnapshotContext *model)
{
    const bool hasGeometry = geometries.size() > 0;
    bool hasSurfaceGeometry = false;
    for ( long int i = 0; i < geometries.size(); i++ ) {
        Geometry *geometry = geometries.get(i);
        if ( geometry != NULL && geometry->className == SURFACE_MESH ) {
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

    if ( model != NULL ) {
        delete[] model->currentMaterialName;
        delete[] model->currentObjectName;
        delete[] model->currentVertexName;
        delete model;
    }
}

void
BinaryModelReadCleanup::releaseVertexRecordIndexLists(ArrayList<BinaryModelVertexRecordData> &vertexRecords) {
    for ( long int i = 0; i < vertexRecords.size(); i++ ) {
        BinaryModelReadPrimitives::releaseIndexListRecord(&vertexRecords[i].patchIndices);
    }
}

void
BinaryModelReadCleanup::releaseGeometryRecordIndexLists(ArrayList<BinaryModelGeometryRecordData> &geometryRecords) {
    for ( long int i = 0; i < geometryRecords.size(); i++ ) {
        BinaryModelGeometryRecordData &record = geometryRecords[i];
        if ( record.objectName != NULL ) {
            delete[] record.objectName;
            record.objectName = NULL;
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
    if ( modelRecord == NULL ) {
        return;
    }
    if ( modelRecord->currentMaterialName != NULL ) {
        delete[] modelRecord->currentMaterialName;
        modelRecord->currentMaterialName = NULL;
    }
    if ( modelRecord->currentObjectName != NULL ) {
        delete[] modelRecord->currentObjectName;
        modelRecord->currentObjectName = NULL;
    }
    if ( modelRecord->currentVertexName != NULL ) {
        delete[] modelRecord->currentVertexName;
        modelRecord->currentVertexName = NULL;
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

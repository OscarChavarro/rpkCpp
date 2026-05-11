package vsdk.toolkit.io.bin.reader;

import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.io.context.ColorContext;
import vsdk.toolkit.io.context.ParseSnapshotContext;
import vsdk.toolkit.io.context.ReaderContext;
import vsdk.toolkit.io.context.TransformSequenceContext;
import vsdk.toolkit.io.context.TransformStackContext;
import vsdk.toolkit.material.Material;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.GeometryClassId;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.Vertex;

public class BinaryModelReadCleanup {
    public static void cleanupPartialModel(
        ArrayList<Vector3D> vectors,
        ArrayList<Vertex> vertices,
        ArrayList<Patch> patches,
        ArrayList<Material> materials,
        ArrayList<Geometry> geometries,
        ArrayList<ColorContext> colorContexts,
        ArrayList<ReaderContext> readerContexts,
        ArrayList<TransformSequenceContext> transformArrays,
        ArrayList<TransformStackContext> transformContexts,
        ParseSnapshotContext model) {
        boolean hasGeometry = geometries != null && !geometries.isEmpty();
        boolean hasSurfaceGeometry = false;
        for (int i = 0; geometries != null && i < geometries.size(); i++) {
            Geometry geometry = geometries.get(i);
            if (geometry != null && geometry.className == GeometryClassId.SURFACE_MESH) {
                hasSurfaceGeometry = true;
                break;
            }
        }

        for (int i = 0; geometries != null && i < geometries.size(); i++) {
            Geometry geometry = geometries.get(i);
            if (geometry != null) {
                geometry.destroy();
            }
        }

        if (!hasGeometry || !hasSurfaceGeometry) {
            for (int i = 0; patches != null && i < patches.size(); i++) {
                Patch patch = patches.get(i);
                if (patch != null) {
                    patch.destroy();
                }
            }
            for (int i = 0; vertices != null && i < vertices.size(); i++) {
                Vertex vertex = vertices.get(i);
                if (vertex != null) {
                    vertex.destroy();
                }
            }
        }

        if (vectors != null) {
            vectors.clear();
        }
        if (vertices != null) {
            vertices.clear();
        }
        if (patches != null) {
            patches.clear();
        }
        if (materials != null) {
            materials.clear();
        }
        if (geometries != null) {
            geometries.clear();
        }
        if (colorContexts != null) {
            colorContexts.clear();
        }
        if (readerContexts != null) {
            readerContexts.clear();
        }
        if (transformArrays != null) {
            transformArrays.clear();
        }
        if (transformContexts != null) {
            transformContexts.clear();
        }

        if (model != null) {
            model.currentMaterialName = null;
            model.currentObjectName = null;
            model.currentVertexName = null;
        }
    }

    public static void releaseVertexRecordIndexLists(ArrayList<BinaryModelVertexRecordData> vertexRecords) {
        for (int i = 0; vertexRecords != null && i < vertexRecords.size(); i++) {
            BinaryModelReadPrimitives.releaseIndexListRecord(vertexRecords.get(i).patchIndices);
        }
    }

    public static void releaseGeometryRecordIndexLists(ArrayList<BinaryModelGeometryRecordData> geometryRecords) {
        for (int i = 0; geometryRecords != null && i < geometryRecords.size(); i++) {
            BinaryModelGeometryRecordData record = geometryRecords.get(i);
            record.objectName = null;
            record.hasObjectName = false;
            BinaryModelReadPrimitives.releaseIndexListRecord(record.positions);
            BinaryModelReadPrimitives.releaseIndexListRecord(record.normals);
            BinaryModelReadPrimitives.releaseIndexListRecord(record.vertices);
            BinaryModelReadPrimitives.releaseIndexListRecord(record.faces);
            BinaryModelReadPrimitives.releaseIndexListRecord(record.children);
            BinaryModelReadPrimitives.releaseIndexListRecord(record.patchSetPatches);
        }
    }

    public static void releaseModelRecordIndexLists(BinaryModelSnapshotRecordData modelRecord) {
        if (modelRecord == null) {
            return;
        }
        modelRecord.currentMaterialName = null;
        modelRecord.currentObjectName = null;
        modelRecord.currentVertexName = null;
        modelRecord.hasCurrentMaterialName = false;
        modelRecord.hasCurrentObjectName = false;
        modelRecord.hasCurrentVertexName = false;
        BinaryModelReadPrimitives.releaseIndexListRecord(modelRecord.currentFaceList);
        BinaryModelReadPrimitives.releaseIndexListRecord(modelRecord.currentGeometryList);
        BinaryModelReadPrimitives.releaseIndexListRecord(modelRecord.currentNormalList);
        BinaryModelReadPrimitives.releaseIndexListRecord(modelRecord.currentPointList);
        BinaryModelReadPrimitives.releaseIndexListRecord(modelRecord.currentVertexList);
        BinaryModelReadPrimitives.releaseIndexListRecord(modelRecord.geometries);
        BinaryModelReadPrimitives.releaseIndexListRecord(modelRecord.materials);
    }
}

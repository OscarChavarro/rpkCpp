import { Vector3D } from "../../../common/linealAlgebra/Vector3D";
import { ColorContext } from "../../context/ColorContext";
import { ParseSnapshotContext } from "../../context/ParseSnapshotContext";
import { ReaderContext } from "../../context/ReaderContext";
import { TransformSequenceContext } from "../../context/TransformSequenceContext";
import { TransformStackContext } from "../../context/TransformStackContext";
import { Material } from "../../../material/Material";
import { Geometry } from "../../../skin/Geometry";
import { GeometryClassId } from "../../../skin/GeometryClassId";
import { Patch } from "../../../environment/geometry/elements/Patch";
import { Vertex } from "../../../environment/geometry/elements/Vertex";
import { BinaryModelGeometryRecordData } from "./BinaryModelGeometryRecordData";
import { BinaryModelReadPrimitives } from "./BinaryModelReadPrimitives";
import { BinaryModelSnapshotRecordData } from "./BinaryModelSnapshotRecordData";
import { BinaryModelVertexRecordData } from "./BinaryModelVertexRecordData";

export class BinaryModelReadCleanup {
  public static cleanupPartialModel(
    vectors: Vector3D[] | null,
    vertices: Vertex[] | null,
    patches: Patch[] | null,
    materials: Material[] | null,
    geometries: Geometry[] | null,
    colorContexts: ColorContext[] | null,
    readerContexts: ReaderContext[] | null,
    transformArrays: TransformSequenceContext[] | null,
    transformContexts: TransformStackContext[] | null,
    model: ParseSnapshotContext | null
  ): void {
    const hasGeometry = geometries !== null && geometries.length > 0;
    let hasSurfaceGeometry = false;

    for (let i = 0; geometries !== null && i < geometries.length; i++) {
      const geometry = geometries[i];
      if (geometry !== null && geometry.className === GeometryClassId.SURFACE_MESH) {
        hasSurfaceGeometry = true;
        break;
      }
    }

    for (let i = 0; geometries !== null && i < geometries.length; i++) {
      const geometry = geometries[i];
      if (geometry !== null) {
        geometry.destroy();
      }
    }

    if (!hasGeometry || !hasSurfaceGeometry) {
      for (let i = 0; patches !== null && i < patches.length; i++) {
        const patch = patches[i];
        if (patch !== null) {
          patch.destroy();
        }
      }
      for (let i = 0; vertices !== null && i < vertices.length; i++) {
        const vertex = vertices[i];
        if (vertex !== null) {
          vertex.destroy();
        }
      }
    }

    if (vectors !== null) {
      vectors.length = 0;
    }
    if (vertices !== null) {
      vertices.length = 0;
    }
    if (patches !== null) {
      patches.length = 0;
    }
    if (materials !== null) {
      materials.length = 0;
    }
    if (geometries !== null) {
      geometries.length = 0;
    }
    if (colorContexts !== null) {
      colorContexts.length = 0;
    }
    if (readerContexts !== null) {
      readerContexts.length = 0;
    }
    if (transformArrays !== null) {
      transformArrays.length = 0;
    }
    if (transformContexts !== null) {
      transformContexts.length = 0;
    }

    if (model !== null) {
      model.currentMaterialName = null;
      model.currentObjectName = null;
      model.currentVertexName = null;
    }
  }

  public static releaseVertexRecordIndexLists(vertexRecords: BinaryModelVertexRecordData[] | null): void {
    for (let i = 0; vertexRecords !== null && i < vertexRecords.length; i++) {
      BinaryModelReadPrimitives.releaseIndexListRecord(vertexRecords[i].patchIndices);
    }
  }

  public static releaseGeometryRecordIndexLists(geometryRecords: BinaryModelGeometryRecordData[] | null): void {
    for (let i = 0; geometryRecords !== null && i < geometryRecords.length; i++) {
      const record = geometryRecords[i];
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

  public static releaseModelRecordIndexLists(modelRecord: BinaryModelSnapshotRecordData | null): void {
    if (modelRecord === null) {
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

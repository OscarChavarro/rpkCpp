import { Logger } from "../../../common/logging/Logger";
import { Vector3D } from "../../../common/linealAlgebra/Vector3D";
import { ColorContext } from "../../context/ColorContext";
import { ParseSnapshotContext } from "../../context/ParseSnapshotContext";
import { ReaderContext } from "../../context/ReaderContext";
import { TransformSequenceContext } from "../../context/TransformSequenceContext";
import { TransformStackContext } from "../../context/TransformStackContext";
import { Material } from "../../../material/Material";
import { Compound } from "../../../skin/Compound";
import { Geometry } from "../../../skin/Geometry";
import { GeometryClassId } from "../../../skin/GeometryClassId";
import { MeshSurface } from "../../../skin/MeshSurface";
import { Patch } from "../../../environment/geometry/elements/Patch";
import { PatchSet } from "../../../environment/geometry/elements/PatchSet";
import { Vertex } from "../../../environment/geometry/elements/Vertex";

export class BinaryModelSerializationGraph {
  public vectorIndices: Map<Vector3D, number>;
  public vectors: Vector3D[];

  public vertexIndices: Map<Vertex, number>;
  public vertices: Vertex[];

  public patchIndices: Map<Patch, number>;
  public patches: Patch[];

  public materialIndices: Map<Material, number>;
  public materials: Material[];

  public geometryIndices: Map<Geometry, number>;
  public geometries: Geometry[];

  public colorContextIndices: Map<ColorContext, number>;
  public colorContexts: ColorContext[];

  public readerContextIndices: Map<ReaderContext, number>;
  public readerContexts: ReaderContext[];

  public transformArrayIndices: Map<TransformSequenceContext, number>;
  public transformArrays: TransformSequenceContext[];

  public transformContextIndices: Map<TransformStackContext, number>;
  public transformContexts: TransformStackContext[];

  public constructor() {
    this.vectorIndices = new Map<Vector3D, number>();
    this.vectors = [];

    this.vertexIndices = new Map<Vertex, number>();
    this.vertices = [];

    this.patchIndices = new Map<Patch, number>();
    this.patches = [];

    this.materialIndices = new Map<Material, number>();
    this.materials = [];

    this.geometryIndices = new Map<Geometry, number>();
    this.geometries = [];

    this.colorContextIndices = new Map<ColorContext, number>();
    this.colorContexts = [];

    this.readerContextIndices = new Map<ReaderContext, number>();
    this.readerContexts = [];

    this.transformArrayIndices = new Map<TransformSequenceContext, number>();
    this.transformArrays = [];

    this.transformContextIndices = new Map<TransformStackContext, number>();
    this.transformContexts = [];
  }

  public ensureVector(value: Vector3D | null): boolean {
    if (value === null) {
      return true;
    }
    if (this.vectorIndices.has(value)) {
      return true;
    }

    const index = this.vectors.length;
    this.vectors.push(value);
    this.vectorIndices.set(value, index);
    return true;
  }

  public ensureMaterial(value: Material | null): boolean {
    if (value === null) {
      return true;
    }
    if (this.materialIndices.has(value)) {
      return true;
    }

    const index = this.materials.length;
    this.materials.push(value);
    this.materialIndices.set(value, index);
    return true;
  }

  public ensureVertex(value: Vertex | null): boolean {
    if (value === null) {
      return true;
    }
    if (this.vertexIndices.has(value)) {
      return true;
    }

    if (value.radianceData !== null) {
      Logger.error(
        "BinaryModelSerializationGraph::ensureVertex",
        "Vertex radianceData is not supported by BinaryModelWritter"
      );
      return false;
    }

    const index = this.vertices.length;
    this.vertices.push(value);
    this.vertexIndices.set(value, index);

    if (!this.ensureVector(value.point)) {
      return false;
    }
    if (!this.ensureVector(value.normal)) {
      return false;
    }
    if (!this.ensureVector(value.textureCoordinates)) {
      return false;
    }
    if (!this.ensureVertex(value.back)) {
      return false;
    }

    if (value.patches !== null) {
      for (let i = 0; i < value.patches.length; i++) {
        if (!this.ensurePatch(value.patches[i]!)) {
          return false;
        }
      }
    }

    return true;
  }

  public ensurePatch(value: Patch | null): boolean {
    if (value === null) {
      return true;
    }
    if (this.patchIndices.has(value)) {
      return true;
    }

    if (value.radianceData !== null) {
      Logger.error(
        "BinaryModelSerializationGraph::ensurePatch",
        "Patch radianceData is not supported by BinaryModelWritter"
      );
      return false;
    }

    const index = this.patches.length;
    this.patches.push(value);
    this.patchIndices.set(value, index);

    for (let i = 0; i < Patch.MAXIMUM_VERTICES_PER_PATCH; i++) {
      if (!this.ensureVertex(value.vertex[i]!)) {
        return false;
      }
    }

    if (!this.ensurePatch(value.twin)) {
      return false;
    }
    if (!this.ensureMaterial(value.material)) {
      return false;
    }

    return true;
  }

  public ensureGeometry(value: Geometry | null): boolean {
    if (value === null) {
      return true;
    }
    if (this.geometryIndices.has(value)) {
      return true;
    }

    if (value.radianceData !== null) {
      Logger.error(
        "BinaryModelSerializationGraph::ensureGeometry",
        "Geometry radianceData is not supported by BinaryModelWritter"
      );
      return false;
    }

    const index = this.geometries.length;
    this.geometries.push(value);
    this.geometryIndices.set(value, index);

    if (value.className === GeometryClassId.SURFACE_MESH) {
      const surface = value as MeshSurface;
      if (!this.ensureMaterial(surface.material)) {
        return false;
      }
      if (!this.collectVectorList(surface.positions)) {
        return false;
      }
      if (!this.collectVectorList(surface.normals)) {
        return false;
      }
      if (!this.collectVertexList(surface.vertices)) {
        return false;
      }
      if (!this.collectPatchList(surface.faces)) {
        return false;
      }
    }
    else if (value.className === GeometryClassId.COMPOUND) {
      const compound = value as Compound;
      if (!this.collectGeometryList(compound.children)) {
        return false;
      }
    }
    else if (value.className === GeometryClassId.PATCH_SET) {
      const patchSet = value as PatchSet;
      if (!this.collectPatchList(patchSet.getPatchList())) {
        return false;
      }
    }
    else {
      Logger.error(
        "BinaryModelSerializationGraph::ensureGeometry",
        "Unsupported geometry class for BinaryModelWritter"
      );
      return false;
    }

    return true;
  }

  public ensureColorContext(value: ColorContext | null): boolean {
    if (value === null) {
      return true;
    }
    if (this.colorContextIndices.has(value)) {
      return true;
    }

    const index = this.colorContexts.length;
    this.colorContexts.push(value);
    this.colorContextIndices.set(value, index);
    return true;
  }

  public ensureReaderContext(value: ReaderContext | null): boolean {
    if (value === null) {
      return true;
    }
    if (this.readerContextIndices.has(value)) {
      return true;
    }

    const index = this.readerContexts.length;
    this.readerContexts.push(value);
    this.readerContextIndices.set(value, index);
    return this.ensureReaderContext(value.prev);
  }

  public ensureTransformArray(value: TransformSequenceContext | null): boolean {
    if (value === null) {
      return true;
    }
    if (this.transformArrayIndices.has(value)) {
      return true;
    }

    const index = this.transformArrays.length;
    this.transformArrays.push(value);
    this.transformArrayIndices.set(value, index);
    return true;
  }

  public ensureTransformContext(value: TransformStackContext | null): boolean {
    if (value === null) {
      return true;
    }
    if (this.transformContextIndices.has(value)) {
      return true;
    }

    const index = this.transformContexts.length;
    this.transformContexts.push(value);
    this.transformContextIndices.set(value, index);

    if (!this.ensureTransformArray(value.transformationArray)) {
      return false;
    }
    return this.ensureTransformContext(value.prev);
  }

  public collectVectorList(list: Vector3D[] | null): boolean {
    if (list === null) {
      return true;
    }

    for (let i = 0; i < list.length; i++) {
      if (!this.ensureVector(list[i]!)) {
        return false;
      }
    }

    return true;
  }

  public collectVertexList(list: Vertex[] | null): boolean {
    if (list === null) {
      return true;
    }

    for (let i = 0; i < list.length; i++) {
      if (!this.ensureVertex(list[i]!)) {
        return false;
      }
    }

    return true;
  }

  public collectPatchList(list: Patch[] | null): boolean {
    if (list === null) {
      return true;
    }

    for (let i = 0; i < list.length; i++) {
      if (!this.ensurePatch(list[i]!)) {
        return false;
      }
    }

    return true;
  }

  public collectMaterialList(list: Material[] | null): boolean {
    if (list === null) {
      return true;
    }

    for (let i = 0; i < list.length; i++) {
      if (!this.ensureMaterial(list[i]!)) {
        return false;
      }
    }

    return true;
  }

  public collectGeometryList(list: Geometry[] | null): boolean {
    if (list === null) {
      return true;
    }

    for (let i = 0; i < list.length; i++) {
      if (!this.ensureGeometry(list[i]!)) {
        return false;
      }
    }

    return true;
  }

  public collectModel(model: ParseSnapshotContext | null): boolean {
    if (model === null) {
      return true;
    }

    if (!this.ensureColorContext(model.currentColor)) {
      return false;
    }
    if (!this.collectPatchList(model.currentFaceList)) {
      return false;
    }
    if (!this.collectGeometryList(model.currentGeometryList)) {
      return false;
    }
    if (!this.collectMaterialList(model.materials)) {
      return false;
    }
    if (!this.collectVectorList(model.currentNormalList)) {
      return false;
    }
    if (!this.collectVectorList(model.currentPointList)) {
      return false;
    }
    if (!this.collectVertexList(model.currentVertexList)) {
      return false;
    }
    if (!this.collectGeometryList(model.geometries)) {
      return false;
    }
    if (!this.ensureReaderContext(model.readerContext)) {
      return false;
    }

    return this.ensureTransformContext(model.transformContext);
  }
}

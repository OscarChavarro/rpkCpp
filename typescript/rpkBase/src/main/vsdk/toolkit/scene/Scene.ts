import { Compound } from "../skin/Compound";
import { Geometry } from "../skin/Geometry";
import { GeometryClassId } from "../skin/GeometryClassId";
import { MeshSurface } from "../skin/MeshSurface";
import { Patch } from "../environment/geometry/elements/Patch";
import { PatchSet } from "../environment/geometry/elements/PatchSet";
import { String as JavaString } from "vitral/dist/java/lang/String";
import { Logger as VsdkLogger } from "vitral/dist/vsdk/toolkit/common/logging/Logger";
import { Background } from "./Background";
import { Camera } from "./Camera";
import { DirectPotentialTable } from "./DirectPotentialTable";
import { FaceBoundsTable } from "./FaceBoundsTable";
import { FaceColorTable } from "./FaceColorTable";
import { FaceVisibilityTable } from "./FaceVisibilityTable";
import { RadianceElementTable } from "./RadianceElementTable";
import type { VisibilityQueryRecorder } from "./VisibilityQueryRecorder";
import { SceneAccelerationMethod } from "./SceneAccelerationMethod";
import { SceneQuery, SceneQueryWorkspace } from "./SceneQuery";

export class Scene {
  private static readonly compoundType = "Compound";
  private static readonly meshSurfaceType = "MeshSurface";
  private static readonly patchSetType = "PatchSet";
  private static readonly unknownType = "<unknown>";

  // Face table: every patch of the scene, in depth-first order of
  // clusteredRootGeometry, indexed by Patch.getFaceIndex(); patches not
  // owned. Each geometry and cluster covers a contiguous range.
  private faceTable: Patch[];
  private numberOfFaces: number;
  private faceBounds: FaceBoundsTable | null; // Bounds of the faceTable entries
  private directPotentials: DirectPotentialTable | null; // Direct potential of the faceTable entries
  private faceVisibility: FaceVisibilityTable | null; // Direct visibility of the faceTable entries
  private faceColors: FaceColorTable | null; // Display color of the faceTable entries
  private radianceElements: RadianceElementTable | null; // Top-level radiance element of the faceTable entries
  private accelerationMethod: SceneAccelerationMethod; // Index of sceneQuery; WALD2007 unless an option says otherwise
  private sceneQuery: SceneQuery | null; // Built by buildSceneQuery
  private sceneQueryWorkspace: SceneQueryWorkspace | null; // Query scratch of the single-threaded callers

  public background: Background | null;
  public camera: Camera | null;
  public geometryList: Geometry[] | null;
  public clusteredGeometryList: Geometry[] | null;
  public clusteredRootGeometry: Geometry | null;
  public visibilityQueryRecorder: VisibilityQueryRecorder | null; // Ray capture runs only, otherwise null
  public patchList: Patch[] | null;
  public lightSourcePatchList: Patch[] | null;

  public constructor() {
    this.faceTable = [];
    this.numberOfFaces = 0;
    this.faceBounds = null;
    this.directPotentials = null;
    this.faceVisibility = null;
    this.faceColors = null;
    this.radianceElements = null;
    this.accelerationMethod = SceneAccelerationMethod.WALD2007_ACCELERATION;
    this.sceneQuery = null;
    this.sceneQueryWorkspace = null;
    this.background = null;
    this.camera = new Camera();
    this.geometryList = null;
    this.clusteredGeometryList = [];
    this.clusteredRootGeometry = null;
    this.visibilityQueryRecorder = null;
    this.patchList = null;
    this.lightSourcePatchList = null;
  }

  /**
  Deletes the top-level radiance elements still bound to faces and the table.
  Radiance methods unbind (and destroy) their elements when they are replaced;
  the elements of the method in use at teardown are released here, as the
  faces' destructors did when they held them
  */
  private deleteRadianceElements(): void {
    if (this.radianceElements !== null) {
      for (let i = 0; i < this.radianceElements.getNumberOfFaces(); i++) {
        this.radianceElements.set(i, null);
      }
    }
    this.radianceElements = null;
  }

  public destroy(): void {
    this.faceBounds = null;
    this.directPotentials = null;
    this.faceVisibility = null;
    this.faceColors = null;
    this.deleteRadianceElements();
    this.faceTable = [];
    this.numberOfFaces = 0;
    if (this.lightSourcePatchList !== null) {
      this.lightSourcePatchList = null;
    }
    if (this.clusteredGeometryList !== null) {
      this.clusteredGeometryList = null;
    }
    if (this.clusteredRootGeometry !== null) {
      this.clusteredRootGeometry = null;
    }
    if (this.patchList !== null) {
      this.patchList = null;
    }
    this.sceneQueryWorkspace = null;
    if (this.visibilityQueryRecorder !== null) {
      this.visibilityQueryRecorder.destroy();
      this.visibilityQueryRecorder = null;
    }
    if (this.sceneQuery !== null) {
      this.sceneQuery.destroy();
      this.sceneQuery = null;
    }
    if (this.background !== null) {
      this.background = null;
    }
    if (this.camera !== null) {
      this.camera = null;
    }
  }

  private static printGeometryType(id: number): string {
    let response = Scene.unknownType;
    if (id === GeometryClassId.SURFACE_MESH) {
      response = Scene.meshSurfaceType;
    }
    else if (id === GeometryClassId.COMPOUND) {
      response = Scene.compoundType;
    }
    else if (id === GeometryClassId.PATCH_SET) {
      response = Scene.patchSetType;
    }
    return response;
  }

  private static printPatchSet(patchSet: PatchSet): void {
    if (patchSet.getPatchList() !== null) {
      process.stdout.write(`  - Patches: ${patchSet.getPatchList()!.length}\n`);
    }
    else {
      process.stdout.write("  - Patches: null list!\n");
    }
  }

  private static printCompound(compound: Compound): void {
    if (compound.children !== null) {
      process.stdout.write(`    . Outer children: ${compound.children.length}\n`);
      for (let i = 0; i < compound.children.length; i++) {
        const child = compound.children[i]!;
        process.stdout.write(`    . Child [${i}] / [${Scene.printGeometryType(child.className)}]\n`);
        if (child.className === GeometryClassId.SURFACE_MESH) {
          Scene.printSurfaceMesh(child as MeshSurface, 6);
        }
      }
    }
  }

  private static printSurfaceMesh(mesh: MeshSurface, level: number): void {
    let spaces = "";
    for (let i = 0; i < level; i++) {
      spaces += " ";
    }

    process.stdout.write(`${spaces}  - Object name: ${mesh.objectName}\n`);
    process.stdout.write(`${spaces}  - Type: SurfaceMesh\n`);
    process.stdout.write(`${spaces}    . Id: ${mesh.id}\n`);
    process.stdout.write(`${spaces}    . Inner id: ${mesh.meshId}\n`);
    process.stdout.write(
      `${spaces}    . Vertices: ${mesh.vertices === null ? 0 : mesh.vertices.length}, positions: ${mesh.positions === null ? 0 : mesh.positions.length}, normals: ${mesh.normals === null ? 0 : mesh.normals.length}, faces: ${mesh.faces === null ? 0 : mesh.faces.length}\n`
    );
  }

  private printGeometries(): void {
    process.stdout.write("= geometryList ================================================================\n");
    process.stdout.write(`Geometries on list: ${this.geometryList!.length}\n`);
    for (let i = 0; i < this.geometryList!.length; i++) {
      const geometry = this.geometryList![i]!;
      process.stdout.write(
        `  - Index: [${i + 1} of ${this.geometryList!.length}] / [${Scene.printGeometryType(geometry.className)}]\n`
      );
      process.stdout.write(`    . Id: ${geometry.id}\n`);
      process.stdout.write(`    . ${geometry.isDuplicate ? "Duplicate" : "Original"}\n`);

      if (geometry.className === GeometryClassId.SURFACE_MESH) {
        Scene.printSurfaceMesh(geometry as MeshSurface, 0);
      }
      else if (geometry.className === GeometryClassId.COMPOUND) {
        Scene.printCompound(geometry as Compound);
      }
      else if (geometry.className === GeometryClassId.PATCH_SET) {
        Scene.printPatchSet(geometry as PatchSet);
      }
    }
  }

  private printClusteredGeometries(): void {
    process.stdout.write("= clusteredGeometryList ================================================================\n");
    process.stdout.write(`Geometry clusters on list: ${this.clusteredGeometryList!.length}\n`);
    for (let i = 0; i < this.clusteredGeometryList!.length; i++) {
      const geometry = this.clusteredGeometryList![i]!;
      process.stdout.write(
        `  - Index: [${i + 1} of ${this.clusteredGeometryList!.length}] / [${Scene.printGeometryType(geometry.className)}]\n`
      );
      process.stdout.write(`    . Id: ${geometry.id}\n`);
      process.stdout.write(`    . ${geometry.isDuplicate ? "Duplicate" : "Original"}\n`);

      if (geometry.className === GeometryClassId.SURFACE_MESH) {
        Scene.printSurfaceMesh(geometry as MeshSurface, 0);
      }
      else if (geometry.className === GeometryClassId.COMPOUND) {
        Scene.printCompound(geometry as Compound);
      }
      else if (geometry.className === GeometryClassId.PATCH_SET) {
        Scene.printPatchSet(geometry as PatchSet);
      }
    }
  }

  private printPatches(): void {
    process.stdout.write("= patchList ================================================================\n");
    if (this.patchList === null) {
      process.stdout.write("Patches on top level scene list: NULL\n");
      return;
    }
    process.stdout.write(`Patches on top level scene list: ${this.patchList.length}\n`);
    for (let i = 0; i < this.patchList.length; i++) {
      const patch = this.patchList[i]!;
      process.stdout.write(`  - patch[${i}]: vertices: ${patch.numberOfVertices}, area: ${patch.getArea().toFixed(3)}\n`);
    }
  }

  private static printClusterHierarchy(node: Geometry, level: number, elementCount: number[]): void {
    if (level === 0) {
      process.stdout.write("= clusteredRootGeometry ================================================================\n");
    }

    switch (level) {
      case 0:
        break;
      case 1:
        process.stdout.write("* ");
        break;
      case 2:
        process.stdout.write("  - ");
        break;
      case 3:
        process.stdout.write("    . ");
        break;
      default:
        process.stdout.write("   ");
        for (let j = 0; j < level; j++) {
          process.stdout.write(" ");
        }
        process.stdout.write(`[${level}] `);
        break;
    }

    if (node.className === GeometryClassId.SURFACE_MESH) {
      process.stdout.write(`Mesh (${elementCount[0]!})\n`);
      elementCount[0]!++;
    }
    else if (node.className === GeometryClassId.COMPOUND) {
      const compound = node as Compound;
      process.stdout.write(`Compound ${compound.id} (${elementCount[0]!})\n`);
      elementCount[0]!++;

      for (let i = 0; compound.children !== null && i < compound.children.length; i++) {
        Scene.printClusterHierarchy(compound.children[i]!, level + 1, elementCount);
      }
    }
    else if (node.className === GeometryClassId.PATCH_SET) {
      const patchSet = node as PatchSet;
      if (patchSet.getPatchList() === null) {
        process.stdout.write(`empty PatchSet (${elementCount[0]!})\n`);
      }
      else {
        process.stdout.write(`PatchSet ${patchSet.id} with ${patchSet.getPatchList()!.length} patches (${elementCount[0]!})\n`);
      }
      elementCount[0]!++;
    }
  }

  private printSceneQuery(): void {
    process.stdout.write(`= scene query (${SceneQuery.methodName(this.accelerationMethod)}) ===========================================================\n`);
  }

  /** The index buildSceneQuery uses; set while the scene is being built */
  public setAccelerationMethod(method: SceneAccelerationMethod): void {
    this.accelerationMethod = method;
  }

  public getAccelerationMethod(): SceneAccelerationMethod {
    return this.accelerationMethod;
  }

  /**
  Builds the scene query over the face table with the acceleration method,
  and the scene's workspace for it (reporting to visibilityQueryRecorder in
  ray capture runs). Called once, after buildFaceTable.
  */
  public buildSceneQuery(): void {
    if (this.sceneQuery !== null) {
      this.sceneQuery.destroy();
    }
    this.sceneQuery = new SceneQuery(this, this.accelerationMethod);
    this.sceneQueryWorkspace = new SceneQueryWorkspace(this.sceneQuery, this.visibilityQueryRecorder);
  }

  public getSceneQuery(): SceneQuery | null {
    return this.sceneQuery;
  }

  public getSceneQueryWorkspace(): SceneQueryWorkspace | null {
    return this.sceneQueryWorkspace;
  }

  public getNumberOfFaces(): number {
    return this.numberOfFaces;
  }

  public getFace(faceIndex: number): Patch {
    return this.faceTable[faceIndex]!;
  }

  public getFaceBounds(): FaceBoundsTable | null {
    return this.faceBounds;
  }

  // Per-face solver data: written by Potential.updateDirectPotential, read
  // by the importance-driven solvers
  public getDirectPotentials(): DirectPotentialTable | null {
    return this.directPotentials;
  }

  // Written by Potential.updateDirectVisibility, read by the importance-driven
  // stochastic radiosity
  public getFaceVisibility(): FaceVisibilityTable | null {
    return this.faceVisibility;
  }

  // Written by the radiance methods after each step, read by the displays
  public getFaceColors(): FaceColorTable | null {
    return this.faceColors;
  }

  // Filled by Radiance.setRadianceMethod, read by the radiance methods
  public getRadianceElements(): RadianceElementTable | null {
    return this.radianceElements;
  }

  private static collectFaces(geometry: Geometry | null, table: Patch[], capacity: number, count: number[], conflicts: number[]): void {
    if (geometry === null) {
      return;
    }
    // Faces are numbered depth-first, so the faces below this geometry are
    // exactly the interval [first, count - 1] once its subtree is visited
    const first = count[0]!;
    if (geometry.className === GeometryClassId.COMPOUND) {
      const children = (geometry as Compound).children;
      if (children !== null) {
        for (let i = 0; i < children.length; i++) {
          Scene.collectFaces(children[i]!, table, capacity, count, conflicts);
        }
      }
      geometry.setFaceIndexRange(first, count[0]! - 1);
      return;
    }
    const patches = Geometry.patchListReference(geometry);
    for (let i = 0; patches !== null && i < patches.length; i++) {
      const patch = patches[i]!;
      if (patch.getFaceIndex() >= 0 || count[0]! >= capacity) {
        conflicts[0]!++;
        continue;
      }
      patch.setFaceIndex(count[0]!);
      table[count[0]!] = patch;
      count[0]!++;
    }
    geometry.setFaceIndexRange(first, count[0]! - 1);
  }

  /**
  Assigns the dense face indices and builds the face table and the face
  bounds. Called once, after the cluster hierarchy exists and before the
  scene query. A patch reachable from two geometries, or a patch of
  patchList that clusteredRootGeometry does not reach, is a fatal error.
  */
  public buildFaceTable(): void {
    this.faceBounds = null;
    this.directPotentials = null;
    this.faceVisibility = null;
    this.faceColors = null;
    this.deleteRadianceElements();
    this.faceTable = [];
    this.numberOfFaces = 0;
    const capacity = this.patchList !== null ? this.patchList.length : 0;
    for (let i = 0; i < capacity; i++) {
      this.patchList![i]!.setFaceIndex(-1);
    }
    const count = [0];
    const conflicts = [0];
    Scene.collectFaces(this.clusteredRootGeometry, this.faceTable, capacity, count, conflicts);
    this.numberOfFaces = count[0]!;

    let unreached = 0;
    for (let i = 0; i < capacity; i++) {
      if (this.patchList![i]!.getFaceIndex() < 0) {
        unreached++;
      }
    }
    if (conflicts[0] !== 0 || unreached !== 0 || this.numberOfFaces !== capacity) {
      VsdkLogger.reportMessage(this, VsdkLogger.FATAL_ERROR, "buildFaceTable", JavaString.vformat(
        "Face table: %d patches, %d indexed, %d reached twice, %d unreachable from the cluster hierarchy",
        [capacity, this.numberOfFaces, conflicts[0]!, unreached]));
    }
    this.faceBounds = new FaceBoundsTable(this.faceTable, this.numberOfFaces);
    this.directPotentials = new DirectPotentialTable(this.numberOfFaces);
    this.faceVisibility = new FaceVisibilityTable(this.numberOfFaces);
    this.faceColors = new FaceColorTable(this.numberOfFaces);
    this.radianceElements = new RadianceElementTable(this.numberOfFaces);
  }

  public print(): void {
    this.printGeometries();
    this.printClusteredGeometries();
    this.printPatches();
    const elementCount = [0];
    Scene.printClusterHierarchy(this.clusteredRootGeometry as Geometry, 0, elementCount);
    this.printSceneQuery();
    process.stdout.write(`*** Total number of geometry elements on cluster hierarchy: ${elementCount[0]!}\n`);
  }
}

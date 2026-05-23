import { Compound } from "../skin/Compound";
import { Geometry } from "../skin/Geometry";
import { GeometryClassId } from "../skin/GeometryClassId";
import { MeshSurface } from "../skin/MeshSurface";
import { Patch } from "../environment/geometry/elements/Patch";
import { PatchSet } from "../environment/geometry/elements/PatchSet";
import { Background } from "./Background";
import { Camera } from "./Camera";
import { VoxelGrid } from "./VoxelGrid";

export class Scene {
  private static readonly compoundType = "Compound";
  private static readonly meshSurfaceType = "MeshSurface";
  private static readonly patchSetType = "PatchSet";
  private static readonly unknownType = "<unknown>";

  public background: Background | null;
  public camera: Camera | null;
  public geometryList: Geometry[] | null;
  public clusteredGeometryList: Geometry[] | null;
  public clusteredRootGeometry: Geometry | null;
  public voxelGrid: VoxelGrid | null;
  public patchList: Patch[] | null;
  public lightSourcePatchList: Patch[] | null;

  public constructor() {
    this.background = null;
    this.camera = new Camera();
    this.geometryList = null;
    this.clusteredGeometryList = [];
    this.clusteredRootGeometry = null;
    this.voxelGrid = null;
    this.patchList = null;
    this.lightSourcePatchList = null;
  }

  public destroy(): void {
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
    if (this.voxelGrid !== null) {
      this.voxelGrid = null;
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
      process.stdout.write(`  - patch[${i}]: vertices: ${patch.numberOfVertices}, area: ${patch.area.toFixed(3)}\n`);
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

  private printVoxelGrid(): void {
    process.stdout.write("= voxelGrid ================================================================\n");
    this.voxelGrid!.print();
  }

  public print(): void {
    this.printGeometries();
    this.printClusteredGeometries();
    this.printPatches();
    const elementCount = [0];
    Scene.printClusterHierarchy(this.clusteredRootGeometry as Geometry, 0, elementCount);
    this.printVoxelGrid();
    process.stdout.write(`*** Total number of geometry elements on cluster hierarchy: ${elementCount[0]!}\n`);
  }
}

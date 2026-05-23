import { Numeric } from "../common/linealAlgebra/Numeric";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { BoundingBox } from "../skin/AxisAlignedBoundingBox";
import { Compound } from "../skin/Compound";
import { Geometry } from "../skin/Geometry";
import { Patch } from "../environment/geometry/elements/Patch";
import { PatchSet } from "../environment/geometry/elements/PatchSet";

export class PatchClusterOctreeNode {
  private static readonly XYZ_EQUAL_MASK = 0x08;
  private static readonly MINIMUM_NUMBER_OF_PATCHES_PER_CLUSTER = 3;

  private patches: Patch[] | null;
  private children: Array<PatchClusterOctreeNode | null>;
  private boundingBox: BoundingBox;
  private boundingBoxCentroid: Vector3D;
  private static clusterNodeGeometriesToDelete: Geometry[] | null = null;

  public constructor(inPatches?: Patch[] | null) {
    this.boundingBox = new BoundingBox();
    this.boundingBoxCentroid = new Vector3D();
    this.boundingBoxCentroid.set(0.0, 0.0, 0.0);
    this.patches = [];

    this.children = new Array<PatchClusterOctreeNode | null>(8);
    for (let i = 0; i < 8; i++) {
      this.children[i] = null;
    }

    for (let i = 0; inPatches !== undefined && inPatches !== null && i < inPatches.length; i++) {
      this.clusterAddPatch(inPatches[i]!);
    }

    if (inPatches !== undefined) {
      this.boundingBoxCentroid = this.boundingBox.center();
    }
  }

  public destroy(): void {
    for (let i = 0; i < 8; i++) {
      if (this.children[i] !== null) {
        this.children[i]!.destroy();
        this.children[i] = null;
      }
    }
    this.patches = null;
  }

  private static addToDeletionCache(geometry: Geometry): void {
    if (PatchClusterOctreeNode.clusterNodeGeometriesToDelete === null) {
      PatchClusterOctreeNode.clusterNodeGeometriesToDelete = [];
    }
    PatchClusterOctreeNode.clusterNodeGeometriesToDelete.push(geometry);
  }

  public static deleteCachedGeometries(): void {
    if (PatchClusterOctreeNode.clusterNodeGeometriesToDelete === null) {
      return;
    }
    for (let i = 0; i < PatchClusterOctreeNode.clusterNodeGeometriesToDelete.length; i++) {
      const geometry = PatchClusterOctreeNode.clusterNodeGeometriesToDelete[i]!;
      if (geometry !== null) {
        geometry.isDuplicate = false;
        geometry.radianceData = null;
        Geometry.destroy(geometry);
      }
    }
    PatchClusterOctreeNode.clusterNodeGeometriesToDelete.length = 0;
    PatchClusterOctreeNode.clusterNodeGeometriesToDelete = null;
  }

  private clusterAddPatch(patch: Patch | null): void {
    if (patch !== null && this.patches !== null) {
      this.patches.push(patch);

      const patchBoundingBox = new BoundingBox();
      if (patch.boundingBox !== null) {
        patchBoundingBox.copyFrom(patch.boundingBox);
      }
      else {
        patch.computeAndGetBoundingBox(patchBoundingBox);
      }
      this.boundingBox.enlarge(patchBoundingBox);
    }
  }

  private octantIndex(p: Vector3D): number {
    const c = this.boundingBox.center();
    let index = 0;

    if (p.x > c.x) {
      index |= 1;
    }
    if (p.y > c.y) {
      index |= 2;
    }
    if (p.z > c.z) {
      index |= 4;
    }
    return index;
  }

  private movePatchToSubOctantCluster(patchIndexOnParent: number): boolean {
    const patch = (this.patches as Patch[])[patchIndexOnParent]!;
    const patchBoundingBox = patch.boundingBox as BoundingBox;

    const smallestBoxDimension = 10.0 * Numeric.EPSILON_FLOAT;
    if (
      (patchBoundingBox.dx() > smallestBoxDimension && patchBoundingBox.dx() > this.boundingBox.dx() * 0.5) ||
      (patchBoundingBox.dy() > smallestBoxDimension && patchBoundingBox.dy() > this.boundingBox.dy() * 0.5) ||
      (patchBoundingBox.dz() > smallestBoxDimension && patchBoundingBox.dz() > this.boundingBox.dz() * 0.5)
    ) {
      return false;
    }

    const midPatch = patchBoundingBox.center();
    const selectedChildOctantIndex = this.octantIndex(midPatch);

    if (selectedChildOctantIndex === PatchClusterOctreeNode.XYZ_EQUAL_MASK) {
      return false;
    }

    const selectedChildCluster = this.children[selectedChildOctantIndex] as PatchClusterOctreeNode;

    (this.patches as Patch[]).splice(patchIndexOnParent, 1);
    (selectedChildCluster.patches as Patch[]).push(patch);
    selectedChildCluster.boundingBox.enlarge(patchBoundingBox);
    return true;
  }

  public splitCluster(): void {
    if (this.patches !== null && this.patches.length <= PatchClusterOctreeNode.MINIMUM_NUMBER_OF_PATCHES_PER_CLUSTER) {
      return;
    }

    for (let i = 0; i < 8; i++) {
      this.children[i] = new PatchClusterOctreeNode();
    }

    for (let i = 0; this.patches !== null && i < this.patches.length; i++) {
      if (this.movePatchToSubOctantCluster(i)) {
        i--;
      }
    }

    for (let i = 0; i < 8; i++) {
      if ((this.children[i] as PatchClusterOctreeNode).patches!.length === 0) {
        this.children[i] = null;
      }
      else {
        (this.children[i] as PatchClusterOctreeNode).boundingBoxCentroid =
          (this.children[i] as PatchClusterOctreeNode).boundingBox.center();
        (this.children[i] as PatchClusterOctreeNode).splitCluster();
      }
    }
  }

  public convertClusterToGeometry(): Geometry {
    let parentPatchesGeometry: Geometry | null = null;
    if (this.patches !== null && this.patches.length > 0) {
      parentPatchesGeometry = new PatchSet(this.patches);
      PatchClusterOctreeNode.addToDeletionCache(parentPatchesGeometry);
    }

    const patchesGeometryList: Geometry[] = [];
    if (parentPatchesGeometry !== null) {
      patchesGeometryList.push(parentPatchesGeometry);
    }

    for (let i = 0; i < 8; i++) {
      let child: Geometry | null = null;
      if (this.children[i] !== null) {
        child = this.children[i]!.convertClusterToGeometry();
      }
      if (child !== null) {
        patchesGeometryList.push(child);
      }
    }

    const newGeometry = new Compound(patchesGeometryList);
    PatchClusterOctreeNode.addToDeletionCache(newGeometry);
    return newGeometry;
  }

  public print(level: number): void {
    switch (level) {
      case 0:
        process.stdout.write("= PatchClusterOctreeNode ================================================================\n");
        break;
      case 1:
        process.stdout.write("  - ");
        break;
      case 2:
        process.stdout.write("    . ");
        break;
      default:
        process.stdout.write(`      (${level}) `);
        for (let i = 3; i < level; i++) {
          process.stdout.write(" ");
        }
        process.stdout.write("-> ");
        break;
    }

    process.stdout.write(`${(this.patches as Patch[]).length} patches: `);
    for (let i = 0; i < (this.patches as Patch[]).length; i++) {
      process.stdout.write(`[${(this.patches as Patch[])[i]!.id}]`);
    }
    process.stdout.write("\n");

    for (let i = 0; i < 8; i++) {
      if (this.children[i] !== null) {
        this.children[i]!.print(level + 1);
      }
    }
  }
}

import type { BinnedSahLeafVisitor } from "../boundingVolumeHierarchy/BinnedSahLeafVisitor.js";
import type { Ray } from "../element/Ray.js";
import { RayHit } from "../element/RayHit.js";
import type { TriangleMesh } from "./TriangleMesh.js";

/**
Closest hit over the meshes of a group: the leaf test of the top level
hierarchy and, called for every mesh in order, the linear loop. Keeps the
first (lowest index) mesh among equal distances, as the linear loop does.
*/
export class _TriangleMeshGroupClosestHit implements BinnedSahLeafVisitor {
    public readonly meshes: TriangleMesh[];
    public readonly ray: Ray;
    public bestHit: RayHit | null;
    public hasBestHit: boolean;
    public bestMesh: number;
    public bestTriangle: number;

    public constructor(inMeshes: TriangleMesh[], inRay: Ray) {
        this.meshes = inMeshes;
        this.ray = inRay;
        this.bestHit = null;
        this.hasBestHit = false;
        this.bestMesh = -1;
        this.bestTriangle = -1;
    }

    public reset(): void {
        this.hasBestHit = false;
        this.bestMesh = -1;
        this.bestTriangle = -1;
    }

    public visit(mesh: number, inOutBestT: number[]): void {
        const meshHit = new RayHit();
        const triangle: number[] = [-1];
        if (!this.meshes[mesh]!.doIntersectionFirstHit(this.ray, meshHit, triangle) || meshHit.getRay() === null) {
            return;
        }
        const t = meshHit.getRay()!.getT();
        if (t < inOutBestT[0]! || (t === inOutBestT[0] && this.hasBestHit && mesh < this.bestMesh)) {
            inOutBestT[0] = t;
            this.bestHit = new RayHit(meshHit);
            this.bestMesh = mesh;
            this.bestTriangle = triangle[0]!;
            this.hasBestHit = true;
        }
    }
}

import { Surface } from "./Surface.js";
import { Ray } from "../element/Ray.js";
import { RayHit } from "../element/RayHit.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { ArrayList } from "../../../../../java/util/ArrayList.js";
import { BinnedSahHierarchy } from "../boundingVolumeHierarchy/BinnedSahHierarchy.js";
import { TriangleAccelerationStructure } from "../boundingVolumeHierarchy/TriangleAccelerationStructure.js";
import { TriangleMeshAccelerationMethod } from "../boundingVolumeHierarchy/TriangleMeshAccelerationMethod.js";
import { _TriangleMeshGroupClosestHit } from "./_TriangleMeshGroupClosestHit.js";
export interface TriangleMeshLike {
    getMinMax(): Float64Array | number[];
    doIntersectionFirstHit(ray: Ray, hit: RayHit, triangleInformation?: Int32Array | number[]): boolean;
    doContainmentTest(p: Vector3Dd, tolerance: number): number;
    prepareAccelerationStructure(method: TriangleMeshAccelerationMethod): void;
    getAccelerationStructure(): TriangleAccelerationStructure | null;
    getPreparedMinMax(outMinMax: number[] | Float64Array): boolean;
}

export class TriangleMeshGroup extends Surface<Ray, RayHit> {
    private meshes: TriangleMeshLike[] = [];
    private information: [number, number] = [-1, -1];

    // Top level ray acceleration over the meshes (the TLAS role of the
    // Vulkan raytracer), see prepareAccelerationStructures. Valid while
    // preparedMeshCount == meshes.length; -1 when there is none.
    private readonly meshHierarchy: BinnedSahHierarchy = new BinnedSahHierarchy();
    private preparedMeshCount = -1;
    private readonly preparedBounds: number[] = [0, 0, 0, 0, 0, 0];

    public constructor(meshes?: TriangleMeshLike[]) {
        super();
        if (meshes !== undefined) this.meshes = meshes;
    }
    public getMeshes() {
        return this.meshes;
    }

    /**
    `TriangleMesh.prepareAccelerationStructure` on every mesh and, unless
    the method is `BRUTE_FORCE`, a `BinnedSahHierarchy` ([WALD2007]) over the
    mesh boxes, so a ray visits the meshes nearest first instead of all of
    them. Single threaded and idempotent: a second call with the same method
    and unchanged meshes only reads. The hit does not depend on the method.
    `setMeshes` / `addMesh` drop the top level; changes made to the meshes
    through `getMeshes` / `getMeshAt` require calling this method again.
    */
    public prepareAccelerationStructures(method: TriangleMeshAccelerationMethod): void {
        let changed = false;
        for (let i = 0; i < this.meshes.length; i++) {
            const before = this.meshes[i]!.getAccelerationStructure();
            this.meshes[i]!.prepareAccelerationStructure(method);
            if (this.meshes[i]!.getAccelerationStructure() !== before) changed = true;
        }
        if (method === TriangleMeshAccelerationMethod.BRUTE_FORCE) {
            if (this.preparedMeshCount !== -1) this.preparedMeshCount = -1;
            return;
        }
        // Nothing written when already prepared: safe from tracing threads.
        if (!changed && this.preparedMeshCount === this.meshes.length) return;
        this.buildMeshHierarchy();
    }

    private buildMeshHierarchy(): void {
        this.preparedMeshCount = -1;
        const bounds: number[] = [0, 0, 0, 0, 0, 0];
        if (!this.getPreparedMinMax(bounds)) return; // some mesh has no structure
        // [WALD2007] top level: one padded box per mesh, from its cached bounds.
        const boxes = new ArrayList<number>(6 * (this.meshes.length > 0 ? this.meshes.length : 1));
        for (let i = 0; i < this.meshes.length; i++) {
            const mm: number[] = [0, 0, 0, 0, 0, 0];
            this.meshes[i]!.getPreparedMinMax(mm);
            const lo: number[] = [mm[0]!, mm[1]!, mm[2]!];
            const hi: number[] = [mm[3]!, mm[4]!, mm[5]!];
            TriangleAccelerationStructure.padBox(lo, hi);
            for (let k = 0; k < 3; k++) boxes.add(lo[k]!);
            for (let k = 0; k < 3; k++) boxes.add(hi[k]!);
        }
        this.meshHierarchy.build(boxes);
        for (let k = 0; k < 6; k++) this.preparedBounds[k] = bounds[k]!;
        this.preparedMeshCount = this.meshes.length;
    }

    /** Top level hierarchy over the meshes, or null when not prepared */
    public getMeshHierarchy(): BinnedSahHierarchy | null {
        return this.preparedMeshCount === this.meshes.length ? this.meshHierarchy : null;
    }

    /** `getMinMax` from the prepared structures, when every mesh has one */
    public getPreparedMinMax(outMinMax: number[] | Float64Array): boolean {
        if (this.preparedMeshCount >= 0 && this.preparedMeshCount === this.meshes.length) {
            for (let k = 0; k < 6; k++) outMinMax[k] = this.preparedBounds[k]!;
            return true;
        }
        // Same combination as getMinMax of this port (C++ and Java follow
        // their computeMinMaxPositions), from cached mesh bounds.
        if (this.meshes.length === 0) return false;
        const m: number[] = [Infinity, Infinity, Infinity, -Infinity, -Infinity, -Infinity];
        const mm: number[] = [0, 0, 0, 0, 0, 0];
        for (let i = 0; i < this.meshes.length; i++) {
            if (!this.meshes[i]!.getPreparedMinMax(mm)) return false;
            for (let k = 0; k < 3; k++) {
                m[k] = Math.min(m[k]!, mm[k]!);
                m[k + 3] = Math.max(m[k + 3]!, mm[k + 3]!);
            }
        }
        for (let k = 0; k < 6; k++) outMinMax[k] = m[k]!;
        return true;
    }

    public setMeshes(x: TriangleMeshLike[]) {
        this.meshes = x;
        this.preparedMeshCount = -1;
    }
    public addMesh(x: TriangleMeshLike) {
        this.meshes.push(x);
        this.preparedMeshCount = -1;
    }
    public getMeshAt(i: number) {
        return this.meshes[i]!;
    }
    public getMinMax(): Float64Array {
        const m = new Float64Array([Infinity, Infinity, Infinity, -Infinity, -Infinity, -Infinity]);
        for (const mesh of this.meshes) {
            const x = mesh.getMinMax();
            for (let i = 0; i < 3; i++) {
                m[i] = Math.min(m[i]!, x[i]!);
                m[i + 3] = Math.max(m[i + 3]!, x[i + 3]!);
            }
        }
        return m;
    }
    public calculateMinMaxPositions(): void {
        this.getMinMax();
    }
    private setIntersectionIndexes(mesh: number, triangle: number): void {
        // Legacy last-hit state written by every ray: skip writes that change
        // nothing.
        if (this.information[0] !== mesh) this.information[0] = mesh;
        if (this.information[1] !== triangle) this.information[1] = triangle;
    }
    public doIntersectionFirstHit(ray: Ray, out: RayHit): boolean {
        const closest = new _TriangleMeshGroupClosestHit(this.meshes, ray);
        const minT = [1e308];
        let traversed = false;
        if (this.preparedMeshCount >= 0 && this.preparedMeshCount === this.meshes.length) {
            traversed = this.meshHierarchy.traverseClosest(ray, closest, minT);
            if (!traversed) closest.reset(); // stack overflow: linear loop below
        }
        if (!traversed) {
            minT[0] = 1e308;
            for (let i = 0; i < this.meshes.length; i++) closest.visit(i, minT);
        }

        if (!closest.hasBestHit) {
            this.setIntersectionIndexes(-1, -1);
            return false;
        }

        this.setIntersectionIndexes(closest.bestMesh, closest.bestTriangle);
        const bestHit: RayHit = closest.bestHit!;

        out.clone(bestHit);
        return true;
    }
    public override doExtraInformation(ray: Ray, t: number, out: RayHit): void {
        this.doIntersectionFirstHit(ray.withT(t), out);
    }
    public doIntersectionInformation(): [number, number] {
        return [...this.information] as [number, number];
    }
    public override doContainmentTest(p: Vector3Dd, t: number): number {
        for (const mesh of this.meshes) {
            const status = mesh.doContainmentTest(p, t);
            if (status !== TriangleMeshGroup.OUTSIDE) return status;
        }
        return TriangleMeshGroup.OUTSIDE;
    }
    public exportToTriangleMeshGroup() {
        return this;
    }
    public override toString() {
        return `TriangleMeshGroup < #Mesh: ${this.meshes.length} >`;
    }
}

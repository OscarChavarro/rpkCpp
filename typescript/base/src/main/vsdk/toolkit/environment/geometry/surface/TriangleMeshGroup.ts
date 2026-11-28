import { Surface } from "./Surface.js";
import type { Ray } from "../element/Ray.js";
import { RayHit } from "../element/RayHit.js";
import type { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { ArrayList } from "../../../../../java/util/ArrayList.js";
import { IndexOutOfBoundsException } from "../../../../../java/lang/IndexOutOfBoundsException.js";
import { Double } from "../../../../../java/lang/Double.js";
import { BinnedSahHierarchy } from "../boundingVolumeHierarchy/BinnedSahHierarchy.js";
import { TriangleAccelerationStructure } from "../boundingVolumeHierarchy/TriangleAccelerationStructure.js";
import { TriangleMeshAccelerationMethod } from "../boundingVolumeHierarchy/TriangleMeshAccelerationMethod.js";
import { _TriangleMeshGroupClosestHit } from "./_TriangleMeshGroupClosestHit.js";
import type { TriangleMesh } from "./TriangleMesh.js";

/**
Group of triangle meshes. Java's `ArrayList<TriangleMesh> meshes` is an
array here; `getMeshAt` checks the index as `ArrayList.get` does. Java keeps
the last-hit indexes in a `ThreadLocal`; JavaScript workers do not share
this object, so it is a plain field.
*/
export class TriangleMeshGroup extends Surface<Ray, RayHit> {
    private meshes: TriangleMesh[];
    private readonly intersectionInformation: number[] = [-1, -1];

    // Top level ray acceleration over the meshes (the TLAS role of the
    // Vulkan raytracer), see prepareAccelerationStructures. Valid while
    // preparedMeshCount == meshes.size(); -1 when there is none.
    private meshHierarchy: BinnedSahHierarchy;
    private preparedMeshCount: number;
    private readonly preparedBounds: number[] = [0, 0, 0, 0, 0, 0];

    public constructor();
    public constructor(meshes: TriangleMesh[]);
    public constructor(group: TriangleMeshGroup);
    public constructor(a?: TriangleMesh[] | TriangleMeshGroup) {
        super();
        if (a === undefined) {
            this.meshes = [];
            this.meshHierarchy = new BinnedSahHierarchy();
            this.preparedMeshCount = -1;
        } else if (a instanceof TriangleMeshGroup) {
            const group: TriangleMeshGroup = a;
            this.meshes = group.getMeshes();
            this.meshHierarchy = new BinnedSahHierarchy(group.meshHierarchy);
            this.preparedMeshCount = group.preparedMeshCount;
            for (let i = 0; i < 6; i++) this.preparedBounds[i] = group.preparedBounds[i]!;
        } else {
            this.meshes = a;
            this.meshHierarchy = new BinnedSahHierarchy();
            this.preparedMeshCount = -1;
        }
    }

    public getMeshes(): TriangleMesh[] {
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
    @param method acceleration method to use
    */
    public prepareAccelerationStructures(method: TriangleMeshAccelerationMethod): void {
        let changed = false;
        for (let i = 0; i < this.meshes.length; i++) {
            const before: TriangleAccelerationStructure | null = this.meshes[i]!.getAccelerationStructure();
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

    /**
    @return top level hierarchy over the meshes, or null when not prepared
    */
    public getMeshHierarchy(): BinnedSahHierarchy | null {
        return this.preparedMeshCount === this.meshes.length ? this.meshHierarchy : null;
    }

    /**
    `getMinMax` from the prepared structures, when every mesh has one.
    @param outMinMax six values array to fill
    @return true when `outMinMax` was written
    */
    public getPreparedMinMax(outMinMax: number[] | Float64Array): boolean {
        if (this.preparedMeshCount >= 0 && this.preparedMeshCount === this.meshes.length) {
            for (let k = 0; k < 6; k++) outMinMax[k] = this.preparedBounds[k]!;
            return true;
        }
        // Same combination as computeMinMaxPositions, from cached mesh bounds.
        if (this.meshes.length === 0) return false;
        let minX = 0,
            minY = 0,
            minZ = 0,
            maxX = 0,
            maxY = 0,
            maxZ = 0;
        const mm: number[] = [0, 0, 0, 0, 0, 0];
        for (let i = 0; i < this.meshes.length; i++) {
            if (!this.meshes[i]!.getPreparedMinMax(mm)) return false;
            if (i === 0) {
                minX = mm[0]!;
                minY = mm[1]!;
                minZ = mm[2]!;
                maxX = mm[3]!;
                maxY = mm[4]!;
                maxZ = mm[5]!;
            }
            if (mm[0]! < minX) minX = mm[0]!;
            if (mm[1]! < minY) minY = mm[1]!;
            if (mm[2]! < minZ) minZ = mm[2]!;
            if (mm[3]! > maxX) maxX = mm[3]!;
            if (mm[4]! > maxY) maxY = mm[4]!;
            if (mm[5]! > maxZ) maxZ = mm[5]!;
        }
        outMinMax[0] = minX;
        outMinMax[1] = minY;
        outMinMax[2] = minZ;
        outMinMax[3] = maxX;
        outMinMax[4] = maxY;
        outMinMax[5] = maxZ;
        return true;
    }

    public setMeshes(meshes: TriangleMesh[]): void {
        this.meshes = meshes;
        this.preparedMeshCount = -1;
    }

    public addMesh(mesh: TriangleMesh): void {
        this.meshes.push(mesh);
        this.preparedMeshCount = -1;
    }

    public getMeshAt(index: number): TriangleMesh {
        if (index < 0 || index >= this.meshes.length) {
            throw new IndexOutOfBoundsException("Index " + index + " out of bounds for length " + this.meshes.length);
        }
        return this.meshes[index]!;
    }

    /**
    @return a new 6 valued double array containing the coordinates of a min-max
    bounding box for current geometry.
    */
    public override getMinMax(): number[] {
        return this.computeMinMaxPositions();
    }

    /**
    Kept for source and binary compatibility. MinMax values are calculated
    without storing state by getMinMax().
    */
    public calculateMinMaxPositions(): void {
        this.computeMinMaxPositions();
    }

    private computeMinMaxPositions(): number[] {
        const minMax: number[] = [0, 0, 0, 0, 0, 0];
        let first = true;
        let minX = Double.MAX_VALUE,
            minY = Double.MAX_VALUE,
            minZ = Double.MAX_VALUE;
        let maxX = Double.MIN_VALUE,
            maxY = Double.MIN_VALUE,
            maxZ = Double.MIN_VALUE;

        for (let i = 0; i < this.meshes.length; i++) {
            const minmaxMesh = this.meshes[i]!.getMinMax();
            const x = minmaxMesh[0]!;
            const y = minmaxMesh[1]!;
            const z = minmaxMesh[2]!;
            const X = minmaxMesh[3]!;
            const Y = minmaxMesh[4]!;
            const Z = minmaxMesh[5]!;
            if (first) {
                minX = x;
                maxX = X;
                minY = y;
                maxY = Y;
                minZ = z;
                maxZ = Z;
                first = false;
            }
            if (x < minX) {
                minX = x;
            }
            if (y < minY) {
                minY = y;
            }
            if (z < minZ) {
                minZ = z;
            }
            if (X > maxX) {
                maxX = X;
            }
            if (Y > maxY) {
                maxY = Y;
            }
            if (Z > maxZ) {
                maxZ = Z;
            }
        }
        minMax[0] = minX;
        minMax[1] = minY;
        minMax[2] = minZ;
        minMax[3] = maxX;
        minMax[4] = maxY;
        minMax[5] = maxZ;
        return minMax;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doIntersectionFirstHit. Java overload
    `doIntersectionFirstHit(Ray inOut_Ray)`.
    */
    public doIntersectionFirstHit(inOutRay: Ray): Ray | null;
    public override doIntersectionFirstHit(inRay: Ray, outHit: RayHit): boolean;
    public override doIntersectionFirstHit(inRay: Ray, outHit: RayHit | null): boolean;
    public override doIntersectionFirstHit(inRay: Ray, outHit?: RayHit | null): Ray | null | boolean {
        if (outHit === undefined) {
            const hit = new RayHit(RayHit.DETAIL_NONE, true);
            if (this.doIntersectionFirstHit(inRay, hit)) {
                return hit.getRay();
            }
            return null;
        }
        const closest = new _TriangleMeshGroupClosestHit(this.meshes, inRay);
        const minT: number[] = [1e308];
        let traversed = false;
        if (this.preparedMeshCount >= 0 && this.preparedMeshCount === this.meshes.length) {
            traversed = this.meshHierarchy.traverseClosest(inRay, closest, minT);
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

        if (outHit !== null) {
            outHit.clone(bestHit);
        }
        return true;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doExtraInformation.
    */
    public override doExtraInformation(inRay: Ray, inT: number, outData: RayHit): void {
        if ((outData as RayHit | null) === null) {
            return;
        }
        this.doIntersectionFirstHit(inRay.withT(inT), outData);
    }

    private setIntersectionIndexes(mesh: number, triangle: number): void {
        // Legacy last-hit state written by every ray: skip writes that change
        // nothing.
        const info = this.intersectionInformation;
        if (info[0] !== mesh) info[0] = mesh;
        if (info[1] !== triangle) info[1] = triangle;
    }

    public doIntersectionInformation(): number[] {
        const info = this.intersectionInformation;
        return [info[0]!, info[1]!];
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doContainmentTest.
    \todo  Check efficiency for this implementation. Note that for the
    special application of volume rendering generation, it is better
    to provide another method, to add voxels after a path following
    over the line.
    @return INSIDE, OUTSIDE or LIMIT constant value
    */
    public override doContainmentTest(p: Vector3Dd, distanceTolerance: number): number {
        let mesh: TriangleMesh;
        let status: number;
        let i: number;

        // Chain of responsability behavior design pattern with TriangleMesh
        for (i = 0; i < this.meshes.length; i++) {
            mesh = this.meshes[i]!;
            status = mesh.doContainmentTest(p, distanceTolerance);
            if (status !== TriangleMeshGroup.OUTSIDE) {
                return status;
            }
        }
        return TriangleMeshGroup.OUTSIDE;
    }

    /**
    Provides an object to text report convertion, optimized for human
    readability and debugging. Do not use for serialization or persistence
    purposes.
    @return human readable report from current mesh group
    */
    public override toString(): string {
        return "TriangleMeshGroup < #Mesh: " + this.meshes.length + " >";
    }

    public exportToTriangleMeshGroup(): TriangleMeshGroup {
        return this;
    }
}

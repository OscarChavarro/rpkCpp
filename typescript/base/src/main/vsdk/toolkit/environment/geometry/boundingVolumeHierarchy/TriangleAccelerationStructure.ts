import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { RaytraceStatistics } from "../../../common/statistics/RaytraceStatistics.js";
import type { Ray } from "../element/Ray.js";
import { Triangle } from "../element/Triangle.js";
import { TriangleMeshAccelerationMethod } from "./TriangleMeshAccelerationMethod.js";

/**
Ray acceleration structure over the triangles of an indexed triangle mesh
(`TriangleMesh` arrays: three coordinates per vertex, three vertex indexes
per triangle). Every implementation answers exactly what the brute force
loop answers (`firstHitBruteForce`): the closest hit of
`Triangle.intersectRay`, ties resolved by the lowest triangle index, with
the same `t`, `u`, `v` values.

Built once, single threaded, before tracing; queries are read only and safe
from several threads. The structure is a snapshot of the mesh: it must be
rebuilt when the vertices or triangles change (`TriangleMesh` releases it
from its own mutators and checks the counts before using it).

Output parameters (`outT`, `outU`, `outV`, `outTriangle`, `t`, `u`, `v`) are
one-element holders, as in `Triangle.intersectRay`. The C++ owning
`TriangleAccelerationStructureSlot` is not needed with a garbage collector:
holders keep a nullable reference and copy it with `copy()`.
*/
export abstract class TriangleAccelerationStructure {
    // Relative and absolute padding of the triangle boxes: Moller-Trumbore can
    // accept points a few ulps outside the exact triangle, the slab test must
    // not reject them.
    private static readonly RELATIVE_BOX_PADDING = 1e-9;
    private static readonly ABSOLUTE_BOX_PADDING = 1e-12;

    /**
    TypeScript only: an ES module can not import a subclass from the module
    of its base class (the cycle leaves the base class undefined while the
    subclass is evaluated), so `create` reaches `BinnedSahTriangleHierarchy`
    through this constructor, which its module installs.
    */
    private static binnedSahTriangleHierarchyFactory: (() => TriangleAccelerationStructure) | null = null;
    /** As `binnedSahTriangleHierarchyFactory`, for `GridTriangleHierarchy` */
    private static gridTriangleHierarchyFactory: (() => TriangleAccelerationStructure) | null = null;

    protected builtVertexCount: number;
    protected builtTriangleCount: number;
    protected readonly vertexBounds: number[] = [0, 0, 0, 0, 0, 0];

    protected constructor();
    /** Copy of the base state, for the subclasses' `copy` (cpp implicit copy
    constructor) */
    protected constructor(other: TriangleAccelerationStructure);
    protected constructor(other?: TriangleAccelerationStructure) {
        if (other !== undefined) {
            this.builtVertexCount = other.builtVertexCount;
            this.builtTriangleCount = other.builtTriangleCount;
            for (let i = 0; i < 6; i++) {
                this.vertexBounds[i] = other.vertexBounds[i]!;
            }
            return;
        }
        this.builtVertexCount = -1;
        this.builtTriangleCount = -1;
        for (let a = 0; a < 3; a++) {
            this.vertexBounds[a] = 1e308;
            this.vertexBounds[a + 3] = -1e308;
        }
    }

    /** Stores the sizes and the vertex bounds; every `build` calls it */
    protected recordBuiltMesh(positions: ArrayLike<number>, indexes: ArrayLike<number>): void {
        this.builtVertexCount = Math.trunc(positions.length / 3);
        this.builtTriangleCount = Math.trunc(indexes.length / 3);
        // Same operations and order as TriangleMesh.getMinMax of this port
        // (C++ and Java follow their calculateMinMaxPositions), so the cached
        // bounds are bit identical to getMinMax().
        let minX = Infinity,
            minY = Infinity,
            minZ = Infinity;
        let maxX = -Infinity,
            maxY = -Infinity,
            maxZ = -Infinity;
        for (let i = 0; i < this.builtVertexCount; i++) {
            const x = positions[i * 3]!,
                y = positions[i * 3 + 1]!,
                z = positions[i * 3 + 2]!;
            minX = Math.min(minX, x);
            minY = Math.min(minY, y);
            minZ = Math.min(minZ, z);
            maxX = Math.max(maxX, x);
            maxY = Math.max(maxY, y);
            maxZ = Math.max(maxZ, z);
        }
        this.vertexBounds[0] = minX;
        this.vertexBounds[1] = minY;
        this.vertexBounds[2] = minZ;
        this.vertexBounds[3] = maxX;
        this.vertexBounds[4] = maxY;
        this.vertexBounds[5] = maxZ;
    }

    public abstract getMethod(): TriangleMeshAccelerationMethod;

    public abstract build(positions: ArrayLike<number>, indexes: ArrayLike<number>): void;

    /**
    Closest hit of the ray against the triangles given to `build` (the same
    arrays must be passed). Returns false on a miss.
    */
    public abstract firstHit(
        ray: Ray,
        positions: ArrayLike<number>,
        indexes: ArrayLike<number>,
        outT: number[],
        outU: number[],
        outV: number[],
        outTriangle: number[],
    ): boolean;

    /** New copy */
    public abstract copy(): TriangleAccelerationStructure;

    /** Number of nodes, for reports */
    public abstract getNodeCount(): number;

    /** Depth of the hierarchy (a single leaf has depth 1), for reports */
    public abstract getDepth(): number;

    /** True when built for meshes of these sizes */
    public matches(vertexCount: number, triangleCount: number): boolean {
        return this.builtVertexCount === vertexCount && this.builtTriangleCount === triangleCount;
    }

    /**
    Minimum and maximum vertex coordinates when built (x, y, z, X, Y, Z),
    computed exactly as `TriangleMesh.getMinMax`, which walks every vertex
    on each call: callers testing a ray against the mesh bounds use this
    copy instead.
    */
    public getVertexBounds(outMinMax: number[] | Float64Array): void {
        for (let i = 0; i < 6; i++) {
            outMinMax[i] = this.vertexBounds[i]!;
        }
    }

    private static absoluteValue(x: number): number {
        return x < 0 ? -x : x;
    }

    /** Enlarges `[lo, hi]` a little, so that rounding never hides a hit of
    `Triangle.intersectRay` inside it */
    public static padBox(lo: number[], hi: number[]): void {
        for (let a = 0; a < 3; a++) {
            const absoluteValue = TriangleAccelerationStructure.absoluteValue;
            const magnitude =
                absoluteValue(lo[a]!) > absoluteValue(hi[a]!) ? absoluteValue(lo[a]!) : absoluteValue(hi[a]!);
            const pad =
                magnitude * TriangleAccelerationStructure.RELATIVE_BOX_PADDING +
                TriangleAccelerationStructure.ABSOLUTE_BOX_PADDING;
            lo[a] = lo[a]! - pad;
            hi[a] = hi[a]! + pad;
        }
    }

    /** Box of one triangle, padded with `padBox` */
    public static triangleBox(
        positions: ArrayLike<number>,
        indexes: ArrayLike<number>,
        triangle: number,
        lo: number[],
        hi: number[],
    ): void {
        for (let a = 0; a < 3; a++) {
            lo[a] = 1e30;
            hi[a] = -1e30;
        }
        for (let k = 0; k < 3; k++) {
            const vertex = indexes[triangle * 3 + k]!;
            for (let a = 0; a < 3; a++) {
                const c = positions[vertex * 3 + a]!;
                lo[a] = c < lo[a]! ? c : lo[a]!;
                hi[a] = c > hi[a]! ? c : hi[a]!;
            }
        }
        TriangleAccelerationStructure.padBox(lo, hi);
    }

    /** `Triangle.intersectRay` over one triangle of the arrays */
    public static testTriangle(
        ray: Ray,
        positions: ArrayLike<number>,
        indexes: ArrayLike<number>,
        triangle: number,
        t: number[],
        u: number[],
        v: number[],
    ): boolean {
        const i0 = indexes[triangle * 3]!;
        const i1 = indexes[triangle * 3 + 1]!;
        const i2 = indexes[triangle * 3 + 2]!;
        const v0 = new Vector3Dd(positions[i0 * 3]!, positions[i0 * 3 + 1]!, positions[i0 * 3 + 2]!);
        const v1 = new Vector3Dd(positions[i1 * 3]!, positions[i1 * 3 + 1]!, positions[i1 * 3 + 2]!);
        const v2 = new Vector3Dd(positions[i2 * 3]!, positions[i2 * 3 + 1]!, positions[i2 * 3 + 2]!);
        return Triangle.intersectRay(ray, v0, v1, v2, t, u, v);
    }

    /** The reference result: every triangle is tested */
    public static firstHitBruteForce(
        ray: Ray,
        positions: ArrayLike<number>,
        indexes: ArrayLike<number>,
        outT: number[],
        outU: number[],
        outV: number[],
        outTriangle: number[],
    ): boolean {
        const triangleCount = Math.trunc(indexes.length / 3);
        // No nodes: every triangle is a primitive test.
        RaytraceStatistics.recordPrimitiveLevelTraversal(0, triangleCount);
        let bestT = 1e308;
        let bestU = 0;
        let bestV = 0;
        let best = -1;
        const t = [0],
            u = [0],
            v = [0];
        for (let i = 0; i < triangleCount; i++) {
            if (TriangleAccelerationStructure.testTriangle(ray, positions, indexes, i, t, u, v) && t[0]! < bestT) {
                bestT = t[0]!;
                bestU = u[0]!;
                bestV = v[0]!;
                best = i;
            }
        }
        if (best < 0) {
            return false;
        }
        outT[0] = bestT;
        outU[0] = bestU;
        outV[0] = bestV;
        outTriangle[0] = best;
        return true;
    }

    /** New, unbuilt structure for the method; null for `BRUTE_FORCE` */
    public static create(method: TriangleMeshAccelerationMethod): TriangleAccelerationStructure | null {
        switch (method) {
            case TriangleMeshAccelerationMethod.WALD2007_BINNED_SAH:
                return TriangleAccelerationStructure.binnedSahTriangleHierarchyFactory !== null
                    ? TriangleAccelerationStructure.binnedSahTriangleHierarchyFactory()
                    : null;
            case TriangleMeshAccelerationMethod.VOXEL_GRID:
                return TriangleAccelerationStructure.gridTriangleHierarchyFactory !== null
                    ? TriangleAccelerationStructure.gridTriangleHierarchyFactory()
                    : null;
            case TriangleMeshAccelerationMethod.BRUTE_FORCE:
                return null;
        }
        return null;
    }

    /** @internal Called once by the module of `BinnedSahTriangleHierarchy` */
    public static installBinnedSahTriangleHierarchyFactory(factory: () => TriangleAccelerationStructure): void {
        TriangleAccelerationStructure.binnedSahTriangleHierarchyFactory = factory;
    }

    /** @internal Called once by the module of `GridTriangleHierarchy` */
    public static installGridTriangleHierarchyFactory(factory: () => TriangleAccelerationStructure): void {
        TriangleAccelerationStructure.gridTriangleHierarchyFactory = factory;
    }
}

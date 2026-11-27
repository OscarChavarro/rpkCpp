//= References:                                                             =
//= [WALD2007] Wald, Ingo. "On fast Construction of SAH-based Bounding      =
//=     Volume Hierarchies". Proceedings of the 2007 IEEE Symposium on      =
//=     Interactive Ray Tracing, 2007, pp. 33-40.                           =

import { ArrayList } from "../../../../../java/util/ArrayList.js";
import { RaytraceStatistics } from "../../../common/statistics/RaytraceStatistics.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import type { Ray } from "../element/Ray.js";
import { Triangle } from "../element/Triangle.js";
import { BinnedSahHierarchy } from "./BinnedSahHierarchy.js";
import { TriangleAccelerationStructure } from "./TriangleAccelerationStructure.js";
import { TriangleMeshAccelerationMethod } from "./TriangleMeshAccelerationMethod.js";

/**
`TriangleMeshAccelerationMethod.WALD2007_BINNED_SAH` for the triangles of
one mesh (the bottom level): a `BinnedSahHierarchy` ([WALD2007].3) over the
padded triangle boxes.

Exactness: the leaves keep a copy of their triangle vertices in leaf order
(as the triangle buffer of the shader, for memory locality), the leaf test
is `Triangle.intersectRay` over the same values, a node is skipped only
when its entry is strictly beyond the closest hit, and ties go to the
lowest triangle index: the result is the brute force one.
*/
export class BinnedSahTriangleHierarchy extends TriangleAccelerationStructure {
    private readonly hierarchy: BinnedSahHierarchy;
    private readonly leafVertices: ArrayList<number>;

    public constructor();
    /** Deep copy (cpp implicit copy constructor) */
    public constructor(other: BinnedSahTriangleHierarchy);
    public constructor(other?: BinnedSahTriangleHierarchy) {
        if (other === undefined) {
            super();
            this.hierarchy = new BinnedSahHierarchy();
            this.leafVertices = new ArrayList<number>();
            return;
        }
        super(other);
        this.hierarchy = new BinnedSahHierarchy(other.hierarchy);
        this.leafVertices = new ArrayList<number>(other.leafVertices);
    }

    public getMethod(): TriangleMeshAccelerationMethod {
        return TriangleMeshAccelerationMethod.WALD2007_BINNED_SAH;
    }

    public build(positions: ArrayLike<number>, indexes: ArrayLike<number>): void {
        const primitiveCount = Math.trunc(indexes.length / 3);
        this.recordBuiltMesh(positions, indexes);

        // [WALD2007].3.2 Initial setup: the box of every triangle, computed once.
        const boxes = new ArrayList<number>(6 * (primitiveCount > 0 ? primitiveCount : 1));
        const lo: number[] = [0, 0, 0];
        const hi: number[] = [0, 0, 0];
        for (let i = 0; i < primitiveCount; i++) {
            TriangleAccelerationStructure.triangleBox(positions, indexes, i, lo, hi);
            for (let a = 0; a < 3; a++) {
                boxes.add(lo[a]!);
            }
            for (let a = 0; a < 3; a++) {
                boxes.add(hi[a]!);
            }
        }
        this.hierarchy.build(boxes);

        // Triangle vertices in leaf order, as the triangle buffer of the shader.
        const order = this.hierarchy.getPrimitiveOrder();
        this.leafVertices.clear();
        for (let i = 0; i < primitiveCount; i++) {
            const triangle = order.get(i);
            for (let k = 0; k < 3; k++) {
                const vertex = indexes[triangle * 3 + k]!;
                this.leafVertices.add(positions[vertex * 3]!);
                this.leafVertices.add(positions[vertex * 3 + 1]!);
                this.leafVertices.add(positions[vertex * 3 + 2]!);
            }
        }
    }

    public firstHit(
        ray: Ray,
        positions: ArrayLike<number>,
        indexes: ArrayLike<number>,
        outT: number[],
        outU: number[],
        outV: number[],
        outTriangle: number[],
    ): boolean {
        const nodes = this.hierarchy.getNodes();
        if (this.builtTriangleCount <= 0 || nodes.size() === 0) {
            return false;
        }
        const slab = new BinnedSahHierarchy.RaySlab(ray);
        const orderArray = this.hierarchy.getPrimitiveOrder();

        // Same initial bound and tie rule as the brute force loop.
        let bestT = 1e308;
        let bestU = 0;
        let bestV = 0;
        let best = -1;

        const rootDistance = slab.entry(nodes.get(0), bestT);
        if (rootDistance < 0.0) {
            RaytraceStatistics.recordPrimitiveLevelTraversal(0, 0);
            return false;
        }
        // Counted locally, reported once: no shared write per node.
        let nodeVisits = 0;
        let triangleTests = 0;
        // Traversal of casTraverseBlas (computeAccelerationStructure.glsl),
        // specialized for triangles: no virtual call per leaf entry.
        const nodeStack = new Int32Array(BinnedSahHierarchy.STACK_SIZE);
        const distanceStack = new Float64Array(BinnedSahHierarchy.STACK_SIZE);
        let sp = 0;
        nodeStack[sp] = 0;
        distanceStack[sp] = rootDistance;
        sp++;

        const t: number[] = [0];
        const u: number[] = [0];
        const v: number[] = [0];
        while (sp > 0) {
            sp--;
            // Strictly farther than the closest hit: nothing there can win, not
            // even a tie (which needs t == bestT).
            if (distanceStack[sp]! > bestT) {
                continue;
            }
            const node = nodes.get(nodeStack[sp]!);
            nodeVisits++;
            if (node.count > 0) {
                triangleTests += node.count;
                for (let i = node.leftOrFirst; i < node.leftOrFirst + node.count; i++) {
                    const p = 9 * i;
                    const v0 = new Vector3Dd(
                        this.leafVertices.get(p),
                        this.leafVertices.get(p + 1),
                        this.leafVertices.get(p + 2),
                    );
                    const v1 = new Vector3Dd(
                        this.leafVertices.get(p + 3),
                        this.leafVertices.get(p + 4),
                        this.leafVertices.get(p + 5),
                    );
                    const v2 = new Vector3Dd(
                        this.leafVertices.get(p + 6),
                        this.leafVertices.get(p + 7),
                        this.leafVertices.get(p + 8),
                    );
                    if (!Triangle.intersectRay(ray, v0, v1, v2, t, u, v)) {
                        continue;
                    }
                    const triangle = orderArray.get(i);
                    if (t[0]! < bestT || (t[0] === bestT && best >= 0 && triangle < best)) {
                        bestT = t[0]!;
                        bestU = u[0]!;
                        bestV = v[0]!;
                        best = triangle;
                    }
                }
                continue;
            }
            let left = node.leftOrFirst;
            let right = left + 1;
            let dl = slab.entry(nodes.get(left), bestT);
            let dr = slab.entry(nodes.get(right), bestT);
            // Push the farther child first, so the nearer one is visited first.
            if (dl >= 0.0 && dr >= 0.0 && dl < dr) {
                const swapNode = left;
                left = right;
                right = swapNode;
                const swapDistance = dl;
                dl = dr;
                dr = swapDistance;
            }
            if (sp + 2 > BinnedSahHierarchy.STACK_SIZE) {
                // Unreachable with MAXIMUM_DEPTH < STACK_SIZE; never give a
                // wrong answer if it happens.
                RaytraceStatistics.recordPrimitiveLevelTraversal(nodeVisits, triangleTests);
                return TriangleAccelerationStructure.firstHitBruteForce(
                    ray,
                    positions,
                    indexes,
                    outT,
                    outU,
                    outV,
                    outTriangle,
                );
            }
            if (dl >= 0.0) {
                nodeStack[sp] = left;
                distanceStack[sp] = dl;
                sp++;
            }
            if (dr >= 0.0) {
                nodeStack[sp] = right;
                distanceStack[sp] = dr;
                sp++;
            }
        }
        RaytraceStatistics.recordPrimitiveLevelTraversal(nodeVisits, triangleTests);
        if (best < 0) {
            return false;
        }
        outT[0] = bestT;
        outU[0] = bestU;
        outV[0] = bestV;
        outTriangle[0] = best;
        return true;
    }

    public copy(): TriangleAccelerationStructure {
        return new BinnedSahTriangleHierarchy(this);
    }

    public getNodeCount(): number {
        return this.hierarchy.getNodeCount();
    }

    public getDepth(): number {
        return this.hierarchy.getDepth();
    }

    public getHierarchy(): BinnedSahHierarchy {
        return this.hierarchy;
    }
}

TriangleAccelerationStructure.installBinnedSahTriangleHierarchyFactory(() => new BinnedSahTriangleHierarchy());

//= References:                                                             =
//= [WALD2007] Wald, Ingo. "On fast Construction of SAH-based Bounding      =
//=     Volume Hierarchies". Proceedings of the 2007 IEEE Symposium on      =
//=     Interactive Ray Tracing, 2007, pp. 33-40.                           =

import { ArrayList } from "../../../../../java/util/ArrayList.js";
import { BinnedSahHierarchy } from "../boundingVolumeHierarchy/BinnedSahHierarchy.js";
import type { BinnedSahStoppingLeafVisitor } from "../boundingVolumeHierarchy/BinnedSahStoppingLeafVisitor.js";
import type { Ray } from "../element/Ray.js";
import type { PrimitiveRayTest } from "./PrimitiveRayTest.js";
import { SpatialIndex } from "./SpatialIndex.js";
import type { SpatialIndexWorkspace } from "./SpatialIndexWorkspace.js";
import { SpatialQueryMode } from "./SpatialQueryMode.js";
import { SpatialQueryResult } from "./SpatialQueryResult.js";

/** Leaf visitor of one query: runs the caller's exact test */
class _QueryVisitor implements BinnedSahStoppingLeafVisitor {
    private readonly test: PrimitiveRayTest;
    private readonly minimumT: number;
    private readonly anyHit: boolean;
    private result: SpatialQueryResult;
    private tests: number;
    private readonly hitT: number[] = [0];

    public constructor(inTest: PrimitiveRayTest, inMinimumT: number, inAnyHit: boolean) {
        this.test = inTest;
        this.minimumT = inMinimumT;
        this.anyHit = inAnyHit;
        this.result = new SpatialQueryResult();
        this.tests = 0;
    }

    public visit(primitive: number, inOutBestT: number[]): boolean {
        this.tests++;
        if (this.test.test(primitive, this.minimumT, inOutBestT[0]!, this.hitT)) {
            inOutBestT[0] = this.hitT[0]!;
            this.result = new SpatialQueryResult(primitive, this.hitT[0]!);
            return this.anyHit;
        }
        return false;
    }

    public getResult(): SpatialQueryResult {
        return this.result;
    }

    public getTests(): number {
        return this.tests;
    }
}

/**
`SpatialIndex` over a `BinnedSahHierarchy` ([WALD2007]): the binned SAH
builder and its nearest-first traversal, through
`BinnedSahHierarchy.traverse`, which can stop at the first accepted hit and
counts into the workspace instead of `RaytraceStatistics`.

Unbounded primitives are kept out of the hierarchy, on an explicit list that
every query tests first, in list order. Candidates of the hierarchy come in
traversal order, so with hits at equal parameters the accepted one depends on
the caller's tie rule; ANY_HIT stops at the first accepted candidate.
*/
export class BinnedSahSpatialIndex extends SpatialIndex {
    private readonly hierarchy: BinnedSahHierarchy;
    private readonly unboundedPrimitives: ArrayList<number>;
    private readonly unbounded: ArrayList<number>;
    private readonly numberOfPrimitives: number;

    /**
    @param boxes six values per primitive (lo x, y, z, hi x, y, z), padded by
    the caller; the box of an unbounded primitive is ignored
    @param inUnboundedPrimitives ids of the primitives without a box
    @param maximumLeafSize as in `BinnedSahHierarchy.build`
    */
    public constructor(
        boxes: ArrayList<number>,
        inUnboundedPrimitives: ArrayList<number>,
        maximumLeafSize: number = BinnedSahHierarchy.DEFAULT_MAXIMUM_LEAF_SIZE,
    ) {
        super();
        this.hierarchy = new BinnedSahHierarchy();
        this.unboundedPrimitives = new ArrayList<number>();
        this.unbounded = new ArrayList<number>();
        this.numberOfPrimitives = Math.trunc(boxes.size() / 6);
        for (let i = 0; i < this.numberOfPrimitives; i++) {
            this.unbounded.add(0);
        }
        for (let i = 0; i < inUnboundedPrimitives.size(); i++) {
            const primitive = inUnboundedPrimitives.get(i);
            if (primitive >= 0 && primitive < this.numberOfPrimitives && this.unbounded.get(primitive) === 0) {
                this.unbounded.set(primitive, 1);
                this.unboundedPrimitives.add(primitive);
            }
        }

        // An unbounded primitive gets an empty box: the hierarchy never reaches it
        const boundedBoxes = new ArrayList<number>();
        for (let i = 0; i < this.numberOfPrimitives; i++) {
            for (let k = 0; k < 6; k++) {
                boundedBoxes.add(this.unbounded.get(i) !== 0 ? (k < 3 ? 1.0 : -1.0) : boxes.get(6 * i + k));
            }
        }
        this.hierarchy.build(boundedBoxes, maximumLeafSize);
    }

    public getNumberOfPrimitives(): number {
        return this.numberOfPrimitives;
    }

    public getWorkspaceSize(): number {
        return 0;
    }

    public getHierarchy(): BinnedSahHierarchy {
        return this.hierarchy;
    }

    public query(
        ray: Ray,
        minimumT: number,
        maximumT: number,
        mode: SpatialQueryMode,
        test: PrimitiveRayTest,
        workspace: SpatialIndexWorkspace,
    ): SpatialQueryResult {
        const anyHit = mode === SpatialQueryMode.ANY_HIT;
        const visitor = new _QueryVisitor(test, minimumT, anyHit);
        const bestT: number[] = [maximumT];

        let stopped = false;
        for (let i = 0; i < this.unboundedPrimitives.size() && !stopped; i++) {
            stopped = visitor.visit(this.unboundedPrimitives.get(i), bestT);
        }

        if (!stopped) {
            const nodeVisits: number[] = [0];
            const entryTests: number[] = [0];
            if (!this.hierarchy.traverse(ray, visitor, bestT, nodeVisits, entryTests)) {
                // Stack overflow (not reachable while MAXIMUM_DEPTH < STACK_SIZE):
                // the bounded primitives are tested linearly instead
                for (let primitive = 0; primitive < this.numberOfPrimitives; primitive++) {
                    if (this.unbounded.get(primitive) === 0 && visitor.visit(primitive, bestT)) {
                        break;
                    }
                }
            }
            workspace.addNodeVisits(nodeVisits[0]!);
        }

        workspace.addPrimitiveTests(visitor.getTests());
        return visitor.getResult();
    }
}

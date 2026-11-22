import type { Ray } from "../element/Ray.js";
import type { PrimitiveRayTest } from "./PrimitiveRayTest.js";
import { SpatialIndex } from "./SpatialIndex.js";
import type { SpatialIndexWorkspace } from "./SpatialIndexWorkspace.js";
import { SpatialQueryMode } from "./SpatialQueryMode.js";
import { SpatialQueryResult } from "./SpatialQueryResult.js";

/**
Reference `SpatialIndex`: no pruning at all. Every primitive is tested, in
increasing id order, so its answer is the one every conservative index must
agree with on visibility; with a tie rule that prefers the lowest id it is
also the closest-hit answer of brute force.
*/
export class LinearSpatialIndex extends SpatialIndex {
    private readonly numberOfPrimitives: number;

    public constructor(inNumberOfPrimitives: number) {
        super();
        this.numberOfPrimitives = inNumberOfPrimitives > 0 ? inNumberOfPrimitives : 0;
    }

    public getNumberOfPrimitives(): number {
        return this.numberOfPrimitives;
    }

    public getWorkspaceSize(): number {
        return 0;
    }

    public query(
        _ray: Ray,
        minimumT: number,
        maximumT: number,
        mode: SpatialQueryMode,
        test: PrimitiveRayTest,
        workspace: SpatialIndexWorkspace,
    ): SpatialQueryResult {
        let result = new SpatialQueryResult();
        let bestT = maximumT;
        let tests = 0;
        const hitT: number[] = [0];
        for (let primitive = 0; primitive < this.numberOfPrimitives; primitive++) {
            tests++;
            if (test.test(primitive, minimumT, bestT, hitT)) {
                bestT = hitT[0]!;
                result = new SpatialQueryResult(primitive, hitT[0]!);
                if (mode === SpatialQueryMode.ANY_HIT) {
                    break;
                }
            }
        }
        workspace.addPrimitiveTests(tests);
        return result;
    }
}

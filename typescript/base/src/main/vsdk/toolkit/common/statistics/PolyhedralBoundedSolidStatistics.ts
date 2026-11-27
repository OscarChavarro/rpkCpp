type StatisticsFlags = { readonly raytrace?: boolean; readonly polyhedral?: boolean };
const statisticsFlags = (globalThis as { __vitralStatistics?: StatisticsFlags }).__vitralStatistics;

/** Opt-in browser-safe counterpart of the Java system-property polyhedral counters. */
export class PolyhedralBoundedSolidStatistics {
    private static readonly enabled = statisticsFlags?.polyhedral === true;
    private static lmev = 0n;
    private static lkev = 0n;
    private static lkef = 0n;
    private static lmef = 0n;
    private static lkemr = 0n;
    private static lmekr = 0n;
    private static lringmv = 0n;
    private static setOperations = 0n;
    private static unions = 0n;
    private static intersections = 0n;
    private static subtracts = 0n;
    private static splits = 0n;
    private static splitNoNullEdges = 0n;
    private static splitAbove = 0n;
    private static splitBelow = 0n;
    private static joins = 0n;
    private static incompleteJoins = 0n;
    private static equalHalfEdges = 0n;
    private static invalidHalfEdges = 0n;
    private static consistencyWarnings = 0n;
    private static operationFailures = 0n;
    public static isEnabled(): boolean {
        return this.enabled;
    }
    public static recordLmevCall(): void {
        if (this.enabled) this.lmev++;
    }
    public static recordLkevCall(): void {
        if (this.enabled) this.lkev++;
    }
    public static recordLkefCall(): void {
        if (this.enabled) this.lkef++;
    }
    public static recordLmefCall(): void {
        if (this.enabled) this.lmef++;
    }
    public static recordLkemrCall(): void {
        if (this.enabled) this.lkemr++;
    }
    public static recordLmekrCall(): void {
        if (this.enabled) this.lmekr++;
    }
    public static recordLringmvCall(): void {
        if (this.enabled) this.lringmv++;
    }
    public static recordSetOpCall(operation: number): void {
        if (!this.enabled) return;
        this.setOperations++;
        if (operation === 1) this.unions++;
        else if (operation === 2) this.intersections++;
        else if (operation === 3) this.subtracts++;
    }
    public static recordSplitCall(): void {
        if (this.enabled) this.splits++;
    }
    public static recordSplitNoNullEdgesCase(): void {
        if (this.enabled) this.splitNoNullEdges++;
    }
    public static recordSplitProducedSolids(above: number, below: number): void {
        if (!this.enabled) return;
        if (above > 0) this.splitAbove += BigInt(above);
        if (below > 0) this.splitBelow += BigInt(below);
    }
    public static recordJoinCall(): void {
        if (this.enabled) this.joins++;
    }
    public static recordJoinIncompleteCase(): void {
        if (this.enabled) this.incompleteJoins++;
    }
    public static recordHe1EqualsHe2Case(): void {
        if (this.enabled) this.equalHalfEdges++;
    }
    public static recordInvalidHalfEdgeInputCase(): void {
        if (this.enabled) this.invalidHalfEdges++;
    }
    public static recordConsistencyWarningCase(): void {
        if (this.enabled) this.consistencyWarnings++;
    }
    public static recordOperationFailureCase(): void {
        if (this.enabled) this.operationFailures++;
    }
    public static reset(): void {
        if (!this.enabled) return;
        this.lmev =
            this.lkev =
            this.lkef =
            this.lmef =
            this.lkemr =
            this.lmekr =
            this.lringmv =
            this.setOperations =
            this.unions =
            this.intersections =
            this.subtracts =
            this.splits =
            this.splitNoNullEdges =
            this.splitAbove =
            this.splitBelow =
            this.joins =
            this.incompleteJoins =
            this.equalHalfEdges =
            this.invalidHalfEdges =
            this.consistencyWarnings =
            this.operationFailures =
                0n;
    }
    public static getOperationFailureCases(): bigint {
        return this.operationFailures;
    }
    public static getConsistencyWarningCases(): bigint {
        return this.consistencyWarnings;
    }
    public static getHe1EqualsHe2Cases(): bigint {
        return this.equalHalfEdges;
    }
    public static getInvalidHalfEdgeInputCases(): bigint {
        return this.invalidHalfEdges;
    }
    public static getJoinIncompleteCases(): bigint {
        return this.incompleteJoins;
    }
    public static getSetOpCalls(): bigint {
        return this.setOperations;
    }
    public static printSummary(): void {
        if (!this.enabled) return;
        const eulerTotal = this.lmev + this.lkev + this.lkef + this.lmef + this.lkemr + this.lmekr + this.lringmv;
        console.log(
            `PolyhedralBoundedSolid statistics (enabled=true):\n  Euler ops total: ${eulerTotal}\n    lmev: ${this.lmev}\n    lkev: ${this.lkev}\n    lkef: ${this.lkef}\n    lmef: ${this.lmef}\n    lkemr: ${this.lkemr}\n    lmekr: ${this.lmekr}\n    lringmv: ${this.lringmv}\n  Boolean setOp calls: ${this.setOperations}\n    union: ${this.unions}\n    intersection: ${this.intersections}\n    subtract: ${this.subtracts}\n  Slicing split calls: ${this.splits}\n    no-null-edges fast-path: ${this.splitNoNullEdges}\n    produced above solids: ${this.splitAbove}\n    produced below solids: ${this.splitBelow}\n  Join calls: ${this.joins}\n  Join incomplete cases: ${this.incompleteJoins}\n  Borderline he1 == he2 cases: ${this.equalHalfEdges}\n  Invalid half-edge inputs: ${this.invalidHalfEdges}\n  Consistency warnings: ${this.consistencyWarnings}\n  Operation failure cases: ${this.operationFailures}`,
        );
    }
}

package vsdk.toolkit.common.statistics;

import java.util.concurrent.atomic.LongAdder;

public final class PolyhedralBoundedSolidStatistics {
    private static final boolean ENABLED =
        Boolean.getBoolean("vsdk.polyhedral.stats");

    private static final LongAdder eulerLmevCalls = new LongAdder();
    private static final LongAdder eulerLkevCalls = new LongAdder();
    private static final LongAdder eulerLkefCalls = new LongAdder();
    private static final LongAdder eulerLmefCalls = new LongAdder();
    private static final LongAdder eulerLkemrCalls = new LongAdder();
    private static final LongAdder eulerLmekrCalls = new LongAdder();
    private static final LongAdder eulerLringmvCalls = new LongAdder();

    private static final LongAdder setOpCalls = new LongAdder();
    private static final LongAdder setOpUnionCalls = new LongAdder();
    private static final LongAdder setOpIntersectionCalls = new LongAdder();
    private static final LongAdder setOpSubtractCalls = new LongAdder();

    private static final LongAdder splitCalls = new LongAdder();
    private static final LongAdder splitNoNullEdgesCases = new LongAdder();
    private static final LongAdder splitProducedAboveSolids = new LongAdder();
    private static final LongAdder splitProducedBelowSolids = new LongAdder();

    private static final LongAdder joinCalls = new LongAdder();
    private static final LongAdder joinIncompleteCases = new LongAdder();

    private static final LongAdder he1EqualsHe2Cases = new LongAdder();
    private static final LongAdder invalidHalfEdgeInputCases = new LongAdder();
    private static final LongAdder consistencyWarningCases = new LongAdder();
    private static final LongAdder operationFailureCases = new LongAdder();

    private PolyhedralBoundedSolidStatistics() {
    }

    public static boolean isEnabled() {
        return ENABLED;
    }

    public static void recordLmevCall() {
        if (ENABLED) {
            eulerLmevCalls.increment();
        }
    }

    public static void recordLkevCall() {
        if (ENABLED) {
            eulerLkevCalls.increment();
        }
    }

    public static void recordLkefCall() {
        if (ENABLED) {
            eulerLkefCalls.increment();
        }
    }

    public static void recordLmefCall() {
        if (ENABLED) {
            eulerLmefCalls.increment();
        }
    }

    public static void recordLkemrCall() {
        if (ENABLED) {
            eulerLkemrCalls.increment();
        }
    }

    public static void recordLmekrCall() {
        if (ENABLED) {
            eulerLmekrCalls.increment();
        }
    }

    public static void recordLringmvCall() {
        if (ENABLED) {
            eulerLringmvCalls.increment();
        }
    }

    public static void recordSetOpCall(int op) {
        if (!ENABLED) {
            return;
        }
        setOpCalls.increment();
        if (op == 1) {
            setOpUnionCalls.increment();
        }
        else if (op == 2) {
            setOpIntersectionCalls.increment();
        }
        else if (op == 3) {
            setOpSubtractCalls.increment();
        }
    }

    public static void recordSplitCall() {
        if (ENABLED) {
            splitCalls.increment();
        }
    }

    public static void recordSplitNoNullEdgesCase() {
        if (ENABLED) {
            splitNoNullEdgesCases.increment();
        }
    }

    public static void recordSplitProducedSolids(int above, int below) {
        if (!ENABLED) {
            return;
        }
        if (above > 0) {
            splitProducedAboveSolids.add(above);
        }
        if (below > 0) {
            splitProducedBelowSolids.add(below);
        }
    }

    public static void recordJoinCall() {
        if (ENABLED) {
            joinCalls.increment();
        }
    }

    public static void recordJoinIncompleteCase() {
        if (ENABLED) {
            joinIncompleteCases.increment();
        }
    }

    public static void recordHe1EqualsHe2Case() {
        if (ENABLED) {
            he1EqualsHe2Cases.increment();
        }
    }

    public static void recordInvalidHalfEdgeInputCase() {
        if (ENABLED) {
            invalidHalfEdgeInputCases.increment();
        }
    }

    public static void recordConsistencyWarningCase() {
        if (ENABLED) {
            consistencyWarningCases.increment();
        }
    }

    public static void recordOperationFailureCase() {
        if (ENABLED) {
            operationFailureCases.increment();
        }
    }

    public static void reset() {
        if (!ENABLED) {
            return;
        }
        eulerLmevCalls.reset();
        eulerLkevCalls.reset();
        eulerLkefCalls.reset();
        eulerLmefCalls.reset();
        eulerLkemrCalls.reset();
        eulerLmekrCalls.reset();
        eulerLringmvCalls.reset();

        setOpCalls.reset();
        setOpUnionCalls.reset();
        setOpIntersectionCalls.reset();
        setOpSubtractCalls.reset();

        splitCalls.reset();
        splitNoNullEdgesCases.reset();
        splitProducedAboveSolids.reset();
        splitProducedBelowSolids.reset();

        joinCalls.reset();
        joinIncompleteCases.reset();

        he1EqualsHe2Cases.reset();
        invalidHalfEdgeInputCases.reset();
        consistencyWarningCases.reset();
        operationFailureCases.reset();
    }

    public static long getOperationFailureCases() {
        return operationFailureCases.sum();
    }

    public static long getConsistencyWarningCases() {
        return consistencyWarningCases.sum();
    }

    public static long getHe1EqualsHe2Cases() {
        return he1EqualsHe2Cases.sum();
    }

    public static long getInvalidHalfEdgeInputCases() {
        return invalidHalfEdgeInputCases.sum();
    }

    public static long getJoinIncompleteCases() {
        return joinIncompleteCases.sum();
    }

    public static long getSetOpCalls() {
        return setOpCalls.sum();
    }

    public static void printSummary() {
        long lmev = eulerLmevCalls.sum();
        long lkev = eulerLkevCalls.sum();
        long lkef = eulerLkefCalls.sum();
        long lmef = eulerLmefCalls.sum();
        long lkemr = eulerLkemrCalls.sum();
        long lmekr = eulerLmekrCalls.sum();
        long lringmv = eulerLringmvCalls.sum();
        long eulerTotal = lmev + lkev + lkef + lmef + lkemr + lmekr + lringmv;

        System.out.println("PolyhedralBoundedSolid statistics (enabled=" +
            ENABLED + "):");
        System.out.println("  Euler ops total: " + eulerTotal);
        System.out.println("    lmev: " + lmev);
        System.out.println("    lkev: " + lkev);
        System.out.println("    lkef: " + lkef);
        System.out.println("    lmef: " + lmef);
        System.out.println("    lkemr: " + lkemr);
        System.out.println("    lmekr: " + lmekr);
        System.out.println("    lringmv: " + lringmv);

        System.out.println("  Boolean setOp calls: " + setOpCalls.sum());
        System.out.println("    union: " + setOpUnionCalls.sum());
        System.out.println("    intersection: " + setOpIntersectionCalls.sum());
        System.out.println("    subtract: " + setOpSubtractCalls.sum());

        System.out.println("  Slicing split calls: " + splitCalls.sum());
        System.out.println("    no-null-edges fast-path: " + splitNoNullEdgesCases.sum());
        System.out.println("    produced above solids: " + splitProducedAboveSolids.sum());
        System.out.println("    produced below solids: " + splitProducedBelowSolids.sum());

        System.out.println("  Join calls: " + joinCalls.sum());
        System.out.println("  Join incomplete cases: " + joinIncompleteCases.sum());

        System.out.println("  Borderline he1 == he2 cases: " + he1EqualsHe2Cases.sum());
        System.out.println("  Invalid half-edge inputs: " + invalidHalfEdgeInputCases.sum());
        System.out.println("  Consistency warnings: " + consistencyWarningCases.sum());
        System.out.println("  Operation failure cases: " + operationFailureCases.sum());
    }
}

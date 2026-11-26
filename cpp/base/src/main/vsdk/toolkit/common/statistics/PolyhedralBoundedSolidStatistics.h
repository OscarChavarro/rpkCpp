#ifndef __POLYHEDRAL_BOUNDED_SOLID_STATISTICS__
#define __POLYHEDRAL_BOUNDED_SOLID_STATISTICS__

/**
Counters of the operations over `PolyhedralBoundedSolid`s (Euler operators,
set operations, splits and their problematic cases), enabled with the
system property `vsdk.polyhedral.stats` (see `java::System::setProperty`).

C++ counterpart of Java's `PolyhedralBoundedSolidStatistics`. C++ port
note: the property is read the first time the statistics are used (Java
reads it when the class is loaded).
*/
class PolyhedralBoundedSolidStatistics {
public:
    static bool isEnabled();

    static void recordLmevCall();
    static void recordLkevCall();
    static void recordLkefCall();
    static void recordLmefCall();
    static void recordLkemrCall();
    static void recordLmekrCall();
    static void recordLringmvCall();
    static void recordSplitCall();
    static void recordSplitNoNullEdgesCase();
    static void recordJoinCall();
    static void recordJoinIncompleteCase();
    static void recordHe1EqualsHe2Case();
    static void recordInvalidHalfEdgeInputCase();
    static void recordConsistencyWarningCase();
    static void recordOperationFailureCase();
    static void recordSetOpCall(int op);
    static void recordSplitProducedSolids(int above, int below);

    static void reset();

    static long long getOperationFailureCases();
    static long long getConsistencyWarningCases();
    static long long getHe1EqualsHe2Cases();
    static long long getInvalidHalfEdgeInputCases();
    static long long getJoinIncompleteCases();
    static long long getSetOpCalls();

    static void printSummary();

private:
    PolyhedralBoundedSolidStatistics() {}
};

#endif

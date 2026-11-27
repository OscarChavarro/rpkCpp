#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/concurrent/atomic/AtomicLong.h"
#include <stdio.h>

#include "java/lang/Boolean.h"
#include "vsdk/toolkit/common/statistics/PolyhedralBoundedSolidStatistics.h"


static AtomicLong eulerLmevCalls(0);
static AtomicLong eulerLkevCalls(0);
static AtomicLong eulerLkefCalls(0);
static AtomicLong eulerLmefCalls(0);
static AtomicLong eulerLkemrCalls(0);
static AtomicLong eulerLmekrCalls(0);
static AtomicLong eulerLringmvCalls(0);
static AtomicLong setOpCalls(0);
static AtomicLong setOpUnionCalls(0);
static AtomicLong setOpIntersectionCalls(0);
static AtomicLong setOpSubtractCalls(0);
static AtomicLong splitCalls(0);
static AtomicLong splitNoNullEdgesCases(0);
static AtomicLong splitProducedAboveSolids(0);
static AtomicLong splitProducedBelowSolids(0);
static AtomicLong joinCalls(0);
static AtomicLong joinIncompleteCases(0);
static AtomicLong he1EqualsHe2Cases(0);
static AtomicLong invalidHalfEdgeInputCases(0);
static AtomicLong consistencyWarningCases(0);
static AtomicLong operationFailureCases(0);


BOOLEAN PolyhedralBoundedSolidStatistics::isEnabled()
{
    static const BOOLEAN ENABLED = Boolean::getBoolean("vsdk.polyhedral.stats");
    return ENABLED;
}

void PolyhedralBoundedSolidStatistics::recordLmevCall()
{
    if ( isEnabled() ) {
        eulerLmevCalls.incrementAndGet();
    }
}

void PolyhedralBoundedSolidStatistics::recordLkevCall()
{
    if ( isEnabled() ) {
        eulerLkevCalls.incrementAndGet();
    }
}

void PolyhedralBoundedSolidStatistics::recordLkefCall()
{
    if ( isEnabled() ) {
        eulerLkefCalls.incrementAndGet();
    }
}

void PolyhedralBoundedSolidStatistics::recordLmefCall()
{
    if ( isEnabled() ) {
        eulerLmefCalls.incrementAndGet();
    }
}

void PolyhedralBoundedSolidStatistics::recordLkemrCall()
{
    if ( isEnabled() ) {
        eulerLkemrCalls.incrementAndGet();
    }
}

void PolyhedralBoundedSolidStatistics::recordLmekrCall()
{
    if ( isEnabled() ) {
        eulerLmekrCalls.incrementAndGet();
    }
}

void PolyhedralBoundedSolidStatistics::recordLringmvCall()
{
    if ( isEnabled() ) {
        eulerLringmvCalls.incrementAndGet();
    }
}

void PolyhedralBoundedSolidStatistics::recordSplitCall()
{
    if ( isEnabled() ) {
        splitCalls.incrementAndGet();
    }
}

void PolyhedralBoundedSolidStatistics::recordSplitNoNullEdgesCase()
{
    if ( isEnabled() ) {
        splitNoNullEdgesCases.incrementAndGet();
    }
}

void PolyhedralBoundedSolidStatistics::recordJoinCall()
{
    if ( isEnabled() ) {
        joinCalls.incrementAndGet();
    }
}

void PolyhedralBoundedSolidStatistics::recordJoinIncompleteCase()
{
    if ( isEnabled() ) {
        joinIncompleteCases.incrementAndGet();
    }
}

void PolyhedralBoundedSolidStatistics::recordHe1EqualsHe2Case()
{
    if ( isEnabled() ) {
        he1EqualsHe2Cases.incrementAndGet();
    }
}

void PolyhedralBoundedSolidStatistics::recordInvalidHalfEdgeInputCase()
{
    if ( isEnabled() ) {
        invalidHalfEdgeInputCases.incrementAndGet();
    }
}

void PolyhedralBoundedSolidStatistics::recordConsistencyWarningCase()
{
    if ( isEnabled() ) {
        consistencyWarningCases.incrementAndGet();
    }
}

void PolyhedralBoundedSolidStatistics::recordOperationFailureCase()
{
    if ( isEnabled() ) {
        operationFailureCases.incrementAndGet();
    }
}

void PolyhedralBoundedSolidStatistics::recordSetOpCall(int op)
{
    if ( !isEnabled() ) {
        return;
    }
    setOpCalls.incrementAndGet();
    if ( op == 1 ) {
        setOpUnionCalls.incrementAndGet();
    }
    else if ( op == 2 ) {
        setOpIntersectionCalls.incrementAndGet();
    }
    else if ( op == 3 ) {
        setOpSubtractCalls.incrementAndGet();
    }
}

void PolyhedralBoundedSolidStatistics::recordSplitProducedSolids(int above,
                                                                 int below)
{
    if ( !isEnabled() ) {
        return;
    }
    if ( above > 0 ) {
        splitProducedAboveSolids.addAndGet(above);
    }
    if ( below > 0 ) {
        splitProducedBelowSolids.addAndGet(below);
    }
}

void PolyhedralBoundedSolidStatistics::reset()
{
    if ( !isEnabled() ) {
        return;
    }
    eulerLmevCalls.set(0);
    eulerLkevCalls.set(0);
    eulerLkefCalls.set(0);
    eulerLmefCalls.set(0);
    eulerLkemrCalls.set(0);
    eulerLmekrCalls.set(0);
    eulerLringmvCalls.set(0);
    setOpCalls.set(0);
    setOpUnionCalls.set(0);
    setOpIntersectionCalls.set(0);
    setOpSubtractCalls.set(0);
    splitCalls.set(0);
    splitNoNullEdgesCases.set(0);
    splitProducedAboveSolids.set(0);
    splitProducedBelowSolids.set(0);
    joinCalls.set(0);
    joinIncompleteCases.set(0);
    he1EqualsHe2Cases.set(0);
    invalidHalfEdgeInputCases.set(0);
    consistencyWarningCases.set(0);
    operationFailureCases.set(0);
}

long long PolyhedralBoundedSolidStatistics::getOperationFailureCases()
{
    return operationFailureCases.get();
}

long long PolyhedralBoundedSolidStatistics::getConsistencyWarningCases()
{
    return consistencyWarningCases.get();
}

long long PolyhedralBoundedSolidStatistics::getHe1EqualsHe2Cases()
{
    return he1EqualsHe2Cases.get();
}

long long PolyhedralBoundedSolidStatistics::getInvalidHalfEdgeInputCases()
{
    return invalidHalfEdgeInputCases.get();
}

long long PolyhedralBoundedSolidStatistics::getJoinIncompleteCases()
{
    return joinIncompleteCases.get();
}

long long PolyhedralBoundedSolidStatistics::getSetOpCalls()
{
    return setOpCalls.get();
}

void PolyhedralBoundedSolidStatistics::printSummary()
{
    long long lmev = eulerLmevCalls.get();
    long long lkev = eulerLkevCalls.get();
    long long lkef = eulerLkefCalls.get();
    long long lmef = eulerLmefCalls.get();
    long long lkemr = eulerLkemrCalls.get();
    long long lmekr = eulerLmekrCalls.get();
    long long lringmv = eulerLringmvCalls.get();
    long long eulerTotal = lmev + lkev + lkef + lmef + lkemr + lmekr + lringmv;

    printf("PolyhedralBoundedSolid statistics (enabled=%s):\n",
           isEnabled() ? "true" : "false");
    printf("  Euler ops total: %lld\n", eulerTotal);
    printf("    lmev: %lld\n", lmev);
    printf("    lkev: %lld\n", lkev);
    printf("    lkef: %lld\n", lkef);
    printf("    lmef: %lld\n", lmef);
    printf("    lkemr: %lld\n", lkemr);
    printf("    lmekr: %lld\n", lmekr);
    printf("    lringmv: %lld\n", lringmv);

    printf("  Boolean setOp calls: %lld\n", setOpCalls.get());
    printf("    union: %lld\n", setOpUnionCalls.get());
    printf("    intersection: %lld\n", setOpIntersectionCalls.get());
    printf("    subtract: %lld\n", setOpSubtractCalls.get());

    printf("  Slicing split calls: %lld\n", splitCalls.get());
    printf("    no-null-edges fast-path: %lld\n", splitNoNullEdgesCases.get());
    printf("    produced above solids: %lld\n", splitProducedAboveSolids.get());
    printf("    produced below solids: %lld\n", splitProducedBelowSolids.get());

    printf("  Join calls: %lld\n", joinCalls.get());
    printf("  Join incomplete cases: %lld\n", joinIncompleteCases.get());

    printf("  Borderline he1 == he2 cases: %lld\n", he1EqualsHe2Cases.get());
    printf("  Invalid half-edge inputs: %lld\n", invalidHalfEdgeInputCases.get());
    printf("  Consistency warnings: %lld\n", consistencyWarningCases.get());
    printf("  Operation failure cases: %lld\n", operationFailureCases.get());
}

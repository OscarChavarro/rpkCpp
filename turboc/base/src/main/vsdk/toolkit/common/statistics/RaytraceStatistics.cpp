#include <stdio.h>

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Boolean.h"
#include "java/util/concurrent/atomic/AtomicLong.h"
#include "vsdk/toolkit/common/statistics/RaytraceStatistics.h"

// C++ port note: file scope statics instead of an unnamed namespace
static AtomicLong primaryRays(0);
static AtomicLong shadowRays(0);
static AtomicLong reflectionRays(0);
static AtomicLong sceneTraversals(0);
static AtomicLong objectIntersectionTests(0);
static AtomicLong raysWithT(0);
static AtomicLong rayHitInstances(0);
static AtomicLong hitInfoClones(0);
static AtomicLong geometryDetailComputations(0);
static AtomicLong instanceLevelTraversals(0);
static AtomicLong instanceLevelNodeVisits(0);
static AtomicLong instanceLevelEntryTests(0);
static AtomicLong primitiveLevelTraversals(0);
static AtomicLong primitiveLevelNodeVisits(0);
static AtomicLong primitiveLevelPrimitiveTests(0);

// -1: not decided yet (read the property), 0: disabled, 1: enabled
static int enabledState = -1;

BOOLEAN RaytraceStatistics::isEnabled()
{
    if ( enabledState < 0 ) {
        enabledState = Boolean::getBoolean("vsdk.raytrace.stats") ? 1 : 0;
    }
    return enabledState == 1;
}

void RaytraceStatistics::setEnabled(BOOLEAN enabled)
{
    enabledState = enabled ? 1 : 0;
}

void RaytraceStatistics::reset()
{
    primaryRays.set(0);
    shadowRays.set(0);
    reflectionRays.set(0);
    sceneTraversals.set(0);
    objectIntersectionTests.set(0);
    raysWithT.set(0);
    rayHitInstances.set(0);
    hitInfoClones.set(0);
    geometryDetailComputations.set(0);
    instanceLevelTraversals.set(0);
    instanceLevelNodeVisits.set(0);
    instanceLevelEntryTests.set(0);
    primitiveLevelTraversals.set(0);
    primitiveLevelNodeVisits.set(0);
    primitiveLevelPrimitiveTests.set(0);
}

void RaytraceStatistics::recordPrimaryRay()
{
    if ( isEnabled() ) primaryRays.incrementAndGet();
}

void RaytraceStatistics::recordShadowRay()
{
    if ( isEnabled() ) shadowRays.incrementAndGet();
}

void RaytraceStatistics::recordReflectionRay()
{
    if ( isEnabled() ) reflectionRays.incrementAndGet();
}

void RaytraceStatistics::recordSceneTraversal()
{
    if ( isEnabled() ) sceneTraversals.incrementAndGet();
}

void RaytraceStatistics::recordObjectIntersectionTest()
{
    if ( isEnabled() ) objectIntersectionTests.incrementAndGet();
}

void RaytraceStatistics::recordRayWithT()
{
    if ( isEnabled() ) raysWithT.incrementAndGet();
}

void RaytraceStatistics::recordRayHitInstance()
{
    if ( isEnabled() ) rayHitInstances.incrementAndGet();
}

void RaytraceStatistics::recordHitInfoClone()
{
    if ( isEnabled() ) hitInfoClones.incrementAndGet();
}

void RaytraceStatistics::recordGeometryDetailComputation()
{
    if ( isEnabled() ) geometryDetailComputations.incrementAndGet();
}

void RaytraceStatistics::recordInstanceLevelTraversal(long int nodeVisits, long int entryTests)
{
    if ( !isEnabled() ) return;
    instanceLevelTraversals.incrementAndGet();
    if ( nodeVisits != 0 ) instanceLevelNodeVisits.addAndGet(nodeVisits);
    if ( entryTests != 0 ) instanceLevelEntryTests.addAndGet(entryTests);
}

void RaytraceStatistics::recordPrimitiveLevelTraversal(long int nodeVisits, long int primitiveTests)
{
    if ( !isEnabled() ) return;
    primitiveLevelTraversals.incrementAndGet();
    if ( nodeVisits != 0 ) primitiveLevelNodeVisits.addAndGet(nodeVisits);
    if ( primitiveTests != 0 ) primitiveLevelPrimitiveTests.addAndGet(primitiveTests);
}

long long RaytraceStatistics::getPrimaryRays() { return primaryRays.get(); }
long long RaytraceStatistics::getShadowRays() { return shadowRays.get(); }
long long RaytraceStatistics::getInstanceLevelTraversals() { return instanceLevelTraversals.get(); }
long long RaytraceStatistics::getInstanceLevelNodeVisits() { return instanceLevelNodeVisits.get(); }
long long RaytraceStatistics::getInstanceLevelEntryTests() { return instanceLevelEntryTests.get(); }
long long RaytraceStatistics::getPrimitiveLevelTraversals() { return primitiveLevelTraversals.get(); }
long long RaytraceStatistics::getPrimitiveLevelNodeVisits() { return primitiveLevelNodeVisits.get(); }
long long RaytraceStatistics::getPrimitiveLevelPrimitiveTests() { return primitiveLevelPrimitiveTests.get(); }

void RaytraceStatistics::printSummary()
{
    printf("Raytrace statistics (enabled=%s):\n", isEnabled() ? "true" : "false");
    printf("  Primary rays: %lld\n", primaryRays.get());
    printf("  Shadow rays: %lld\n", shadowRays.get());
    printf("  Reflection rays: %lld\n", reflectionRays.get());
    printf("  Scene traversals: %lld\n", sceneTraversals.get());
    printf("  Object intersection tests: %lld\n", objectIntersectionTests.get());
    printf("  Rays with t: %lld\n", raysWithT.get());
    printf("  RayHit instances: %lld\n", rayHitInstances.get());
    printf("  Hit info clones: %lld\n", hitInfoClones.get());
    printf("  Geometry detail computations: %lld\n", geometryDetailComputations.get());
    printf("  Instance level: %lld traversals, %lld node visits, %lld entry tests\n",
        instanceLevelTraversals.get(), instanceLevelNodeVisits.get(),
        instanceLevelEntryTests.get());
    printf("  Primitive level: %lld traversals, %lld node visits, %lld primitive tests\n",
        primitiveLevelTraversals.get(), primitiveLevelNodeVisits.get(),
        primitiveLevelPrimitiveTests.get());
}

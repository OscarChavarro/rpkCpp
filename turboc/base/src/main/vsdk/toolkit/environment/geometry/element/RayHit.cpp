#include <stddef.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/statistics/RaytraceStatistics.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
static const Vector3Dd ZERO_VECTOR;

RayHit::RayHit()
    : point(ZERO_VECTOR), normal(ZERO_VECTOR), tangent(ZERO_VECTOR),
      u(0), v(0),
      material(NULL), texture(NULL), normalMap(NULL),
      ray(),
      hasRay(FALSE),
      requiredDetailMask(DETAIL_ALL),
      storeRay(TRUE),
      hitDistance(0),
      hitDistanceKnown(FALSE)
{
    clear();
    RaytraceStatistics::recordRayHitInstance();
}

RayHit::RayHit(int requiredDetailMask)
    : point(ZERO_VECTOR), normal(ZERO_VECTOR), tangent(ZERO_VECTOR),
      u(0), v(0),
      material(NULL), texture(NULL), normalMap(NULL),
      ray(),
      hasRay(FALSE),
      requiredDetailMask(requiredDetailMask),
      storeRay(TRUE),
      hitDistance(0),
      hitDistanceKnown(FALSE)
{
    clear();
    RaytraceStatistics::recordRayHitInstance();
}

RayHit::RayHit(int requiredDetailMask, BOOLEAN storeRay)
    : point(ZERO_VECTOR), normal(ZERO_VECTOR), tangent(ZERO_VECTOR),
      u(0), v(0),
      material(NULL), texture(NULL), normalMap(NULL),
      ray(),
      hasRay(FALSE),
      requiredDetailMask(requiredDetailMask),
      storeRay(storeRay),
      hitDistance(0),
      hitDistanceKnown(FALSE)
{
    clear();
    RaytraceStatistics::recordRayHitInstance();
}

RayHit::RayHit(const RayHit& other)
    : point(ZERO_VECTOR), normal(ZERO_VECTOR), tangent(ZERO_VECTOR),
      u(0), v(0),
      material(NULL), texture(NULL), normalMap(NULL),
      ray(),
      hasRay(FALSE),
      requiredDetailMask(other.requiredDetailMask),
      storeRay(other.storeRay),
      hitDistance(0),
      hitDistanceKnown(FALSE)
{
    clone(other);
}

RayHit::~RayHit()
{
}

void RayHit::clear()
{
    point = ZERO_VECTOR;
    normal = ZERO_VECTOR;
    tangent = ZERO_VECTOR;
    u = 0;
    v = 0;
    material = NULL;
    texture = NULL;
    normalMap = NULL;
    hasRay = FALSE;
    availableDetailMask = DETAIL_NONE;
    detailAvailabilityKnown = FALSE;
    hitDistance = 0;
    hitDistanceKnown = FALSE;
}

void RayHit::reset(int newRequiredDetailMask)
{
    requiredDetailMask = newRequiredDetailMask;
    clear();
}

void RayHit::resetForDistanceOnly()
{
    requiredDetailMask = DETAIL_NONE;
    hasRay = FALSE;
    availableDetailMask = DETAIL_NONE;
    detailAvailabilityKnown = FALSE;
    hitDistance = 0;
    hitDistanceKnown = FALSE;
}

void RayHit::clone(const RayHit& other)
{
    RaytraceStatistics::recordHitInfoClone();
    requiredDetailMask = other.requiredDetailMask;
    storeRay = other.storeRay;
    availableDetailMask = other.availableDetailMask;
    detailAvailabilityKnown = other.detailAvailabilityKnown;
    hitDistance = other.hitDistance;
    hitDistanceKnown = other.hitDistanceKnown;
    point = other.point;
    normal = other.normal;
    tangent = other.tangent;
    u = other.u;
    v = other.v;
    material = other.material;
    texture = other.texture;
    normalMap = other.normalMap;

    hasRay = other.hasRay;
    if ( hasRay ) {
        ray = other.ray;
    }
}

int RayHit::getRequiredDetailMask() const
{
    return requiredDetailMask;
}

void RayHit::setRequiredDetailMask(int value)
{
    requiredDetailMask = value;
}

BOOLEAN RayHit::shouldStoreRay() const
{
    return storeRay;
}

void RayHit::setStoreRay(BOOLEAN value)
{
    storeRay = value;
}

BOOLEAN RayHit::needsPoint() const
{
    return (requiredDetailMask & DETAIL_POINT) != 0;
}

BOOLEAN RayHit::needsNormal() const
{
    return (requiredDetailMask & DETAIL_NORMAL) != 0;
}

BOOLEAN RayHit::needsTextureCoordinates() const
{
    return (requiredDetailMask & DETAIL_UV) != 0;
}

BOOLEAN RayHit::needsTangent() const
{
    return (requiredDetailMask & DETAIL_TANGENT) != 0;
}

BOOLEAN RayHit::needsAnySurfaceData() const
{
    return requiredDetailMask != DETAIL_NONE;
}

const Ray* RayHit::getRay() const
{
    return hasRay ? &ray : NULL;
}

void RayHit::setRay(const Ray& value)
{
    ray = value;
    hasRay = TRUE;
    hitDistance = value.getT();
    hitDistanceKnown = TRUE;
}

BOOLEAN RayHit::hasHitDistance() const
{
    return hitDistanceKnown;
}

double RayHit::getHitDistance() const
{
    if ( hitDistanceKnown ) {
        return hitDistance;
    }
    if ( hasRay ) {
        return ray.getT();
    }
    return 0;
}

void RayHit::setHitDistance(double value)
{
    hitDistance = value;
    hitDistanceKnown = TRUE;
}

// Integral constants bound to references (i.e. by `?:`) need a
// definition out of the class in C++98

BOOLEAN RayHit::isDetailAvailabilityKnown() const { return detailAvailabilityKnown; }
int RayHit::getAvailableDetailMask() const { return availableDetailMask; }
void RayHit::beginDetailEvaluation() { availableDetailMask = DETAIL_NONE; detailAvailabilityKnown = TRUE; }
void RayHit::markDetailAvailable(int mask) { availableDetailMask |= mask & DETAIL_ALL; }

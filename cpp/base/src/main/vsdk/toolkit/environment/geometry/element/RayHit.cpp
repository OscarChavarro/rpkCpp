#include "vsdk/toolkit/common/statistics/RaytraceStatistics.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
static const Vector3Dd ZERO_VECTOR;

RayHit::RayHit() : RayHit(DETAIL_ALL, true) {}

RayHit::RayHit(int requiredDetailMask) : RayHit(requiredDetailMask, true) {}

RayHit::RayHit(int requiredDetailMask, bool storeRay)
    : point(ZERO_VECTOR), normal(ZERO_VECTOR), tangent(ZERO_VECTOR),
      u(0), v(0),
      material(nullptr), texture(nullptr), normalMap(nullptr),
      ray(),
      hasRay(false),
      requiredDetailMask(requiredDetailMask),
      storeRay(storeRay),
      hitDistance(0),
      hitDistanceKnown(false)
{
    clear();
    RaytraceStatistics::recordRayHitInstance();
}

RayHit::RayHit(const RayHit& other)
    : point(ZERO_VECTOR), normal(ZERO_VECTOR), tangent(ZERO_VECTOR),
      u(0), v(0),
      material(nullptr), texture(nullptr), normalMap(nullptr),
      ray(),
      hasRay(false),
      requiredDetailMask(other.requiredDetailMask),
      storeRay(other.storeRay),
      hitDistance(0),
      hitDistanceKnown(false)
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
    material = nullptr;
    texture = nullptr;
    normalMap = nullptr;
    hasRay = false;
    availableDetailMask = DETAIL_NONE;
    detailAvailabilityKnown = false;
    hitDistance = 0;
    hitDistanceKnown = false;
}

void RayHit::reset(int newRequiredDetailMask)
{
    requiredDetailMask = newRequiredDetailMask;
    clear();
}

void RayHit::resetForDistanceOnly()
{
    requiredDetailMask = DETAIL_NONE;
    hasRay = false;
    availableDetailMask = DETAIL_NONE;
    detailAvailabilityKnown = false;
    hitDistance = 0;
    hitDistanceKnown = false;
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

bool RayHit::shouldStoreRay() const
{
    return storeRay;
}

void RayHit::setStoreRay(bool value)
{
    storeRay = value;
}

bool RayHit::needsPoint() const
{
    return (requiredDetailMask & DETAIL_POINT) != 0;
}

bool RayHit::needsNormal() const
{
    return (requiredDetailMask & DETAIL_NORMAL) != 0;
}

bool RayHit::needsTextureCoordinates() const
{
    return (requiredDetailMask & DETAIL_UV) != 0;
}

bool RayHit::needsTangent() const
{
    return (requiredDetailMask & DETAIL_TANGENT) != 0;
}

bool RayHit::needsAnySurfaceData() const
{
    return requiredDetailMask != DETAIL_NONE;
}

const Ray* RayHit::getRay() const
{
    return hasRay ? &ray : nullptr;
}

void RayHit::setRay(const Ray& value)
{
    ray = value;
    hasRay = true;
    hitDistance = value.getT();
    hitDistanceKnown = true;
}

bool RayHit::hasHitDistance() const
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
    hitDistanceKnown = true;
}


bool RayHit::isDetailAvailabilityKnown() const { return detailAvailabilityKnown; }
int RayHit::getAvailableDetailMask() const { return availableDetailMask; }
void RayHit::beginDetailEvaluation() { availableDetailMask = DETAIL_NONE; detailAvailabilityKnown = true; }
void RayHit::markDetailAvailable(int mask) { availableDetailMask |= mask & DETAIL_ALL; }

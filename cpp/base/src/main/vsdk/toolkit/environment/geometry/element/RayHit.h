#ifndef __RAY_HIT__
#define __RAY_HIT__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/SurfaceDetail.h"
class SimpleMaterial;
class Image;
class NormalMap;

/**
RayHit describes the result of a ray/geometry intersection.
*/
class RayHit {
public:
    static const int DETAIL_NONE = SurfaceDetail::NONE;
    static const int DETAIL_POINT = SurfaceDetail::POINT;
    static const int DETAIL_NORMAL = SurfaceDetail::NORMAL;
    static const int DETAIL_UV = SurfaceDetail::UV;
    static const int DETAIL_TANGENT = SurfaceDetail::TANGENT;
    static const int DETAIL_ALL = SurfaceDetail::ALL;

    Vector3Dd point; // Intersection point coordinates
    Vector3Dd normal; // Surface normal at intersection point
    Vector3Dd tangent; // Surface tangent at intersection point
    // Note that surface binormal at intersection point must be calculated
    // by the application as the cross product (n x t).

    double u; // Texture coordinate of intersection point
    double v;

    // This can be null.
    SimpleMaterial* material; // Internal geometry selected material

    // This can be null.
    Image* texture; // Internal geometry selected texture map

    // This can be null.
    NormalMap* normalMap; // Internal geometry selected normal map

private:
    Ray ray;
    bool hasRay;

    int requiredDetailMask;
    int availableDetailMask;
    bool detailAvailabilityKnown;
    bool storeRay;
    double hitDistance;
    bool hitDistanceKnown;

public:
    RayHit();
    explicit RayHit(int requiredDetailMask);
    RayHit(int requiredDetailMask, bool storeRay);
    RayHit(const RayHit& other);
    ~RayHit();

    void clear();
    void reset(int newRequiredDetailMask);
    void resetForDistanceOnly();

    void clone(const RayHit& other);

    // Legacy producers may leave availability unknown; requested is not available.
    bool isDetailAvailabilityKnown() const;
    int getAvailableDetailMask() const;
    void beginDetailEvaluation();
    void markDetailAvailable(int mask);

    int getRequiredDetailMask() const;
    void setRequiredDetailMask(int value);

    bool shouldStoreRay() const;
    void setStoreRay(bool value);

    bool needsPoint() const;
    bool needsNormal() const;
    bool needsTextureCoordinates() const;
    bool needsTangent() const;
    bool needsAnySurfaceData() const;

    const Ray* getRay() const;
    void setRay(const Ray& value);

    bool hasHitDistance() const;
    double getHitDistance() const;
    void setHitDistance(double value);

};

#endif

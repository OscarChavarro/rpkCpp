#ifndef __GEOMETRY__
#define __GEOMETRY__

#include "vsdk/toolkit/common/Entity.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/processing/Containment.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingSink.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingSupport.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidQueryStatus.h"
#include "vsdk/toolkit/environment/geometry/element/RayFirstHitStatus.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryContainment.h"
class GeometryBounds;
class RayBoundary;
class RaySolidClassification;
class Ray;
class RayHit;

class Geometry : public Entity {
public:
    static constexpr int INSIDE = static_cast<int>(Containment::INSIDE);
    static constexpr int LIMIT = static_cast<int>(Containment::LIMIT);
    static constexpr int OUTSIDE = static_cast<int>(Containment::OUTSIDE);

    virtual ~Geometry() {}

    // An independent mathematical copy, or nullptr when copying is unsupported.
    virtual Geometry* copyGeometry() const { return nullptr; }

    // Unsupported is distinct from an empty sequence of crossings.
    virtual RayCrossingSupport emitRayCrossings(
        const Ray&, const RayCrossingQuery&, RayCrossingSink&) const
    { return RayCrossingSupport::Unsupported; }

    // Full solid membership over the query interval; never infer interior from
    // surface hits alone. Unsupported geometries leave result empty.
    virtual RaySolidQueryStatus classifyRay(const Ray&, const RayCrossingQuery&,
        RaySolidClassification& result) const;

    // Inclusive explicit interval; unsupported implementations clear output.
    virtual RayFirstHitStatus queryFirstHit(const Ray&, const RayCrossingQuery&, RayHit*,
        RayBoundary* boundary = nullptr) const;

    // Resolve detail for a boundary in this geometry's local coordinates.
    // Hit means evaluated; unsupported and numerical failures remain explicit.
    virtual RayFirstHitStatus querySurfaceDetail(const Ray&, const RayBoundary&, RayHit*) const;

    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit) = 0;
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outHit);
    virtual int computeQuantitativeInvisibility(const Vector3Dd& origin, const Vector3Dd& p);
    // Unsupported is explicit; legacy getMinMax remains available.
    virtual void getBounds(GeometryBounds& result) const;
    virtual double* getMinMax() = 0;
    // Typed containment; unsupported is explicit here.
    virtual GeometryContainment classifyPoint(const Vector3Dd&, double distanceTolerance) const;
    // Certified local-space classification of the closed ball of radius
    // distanceTolerance around the point. Inside/Outside: the whole ball lies
    // strictly in the interior/exterior. Boundary: a boundary point lies within
    // the ball. Anything not proven, including at zero tolerance, is Indeterminate.
    virtual GeometryContainment classifyPointCertified(const Vector3Dd&, double distanceTolerance) const;
    // Certified classification of the closed axis-aligned local box
    // center +/- halfWidths: Inside/Outside only when the whole box lies strictly
    // in the interior/exterior; never Boundary. Other results are Indeterminate.
    virtual GeometryContainment classifyBoxCertified(const Vector3Dd& center, const Vector3Dd& halfWidths) const;
    // Legacy integer predicate kept for parity with the Java port. It is not
    // the typed contract: its tolerance band is family specific, the default
    // reports OUTSIDE for unsupported geometries, and surfaces report LIMIT only
    // on the surface. Typed scene and CSG queries never consult it. Adapters for
    // solids introduced with the typed contract raise when a typed result has
    // no integer representation.
    virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance);
};

#endif

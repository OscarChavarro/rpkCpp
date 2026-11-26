#ifndef __CONE__
#define __CONE__

#include "vsdk/toolkit/environment/geometry/volume/Solid.h"
class PolyhedralBoundedSolid;
class Ray;
class RayHit;

class Cone : public Solid {
private:
    double bottomRadius; // Radius at the base
    double topRadius;    // Radius at the top
    double height;       // Height

    static const int DEFAULT_CIRCUMFERENCE_DIVISIONS = 36;
    static const int DEFAULT_HEIGHT_DIVISIONS = 1;
    static const int MIN_CIRCUMFERENCE_DIVISIONS = 3;
    static const int MIN_HEIGHT_DIVISIONS = 1;

    static double sq(double v);
    static bool approxEq(double a, double b);

    Ray* doIntersectionCylinder(const Ray& inOutRay, double inR, double inH, RayHit* outInfo);
    Ray* doIntersectionCone(const Ray& inOutRay, double inR, double inH, RayHit* outInfo);
    Ray* doIntersectionTap(const Ray& inOutRay, double inR, double inH, RayHit* outInfo);

    static void closeTopFaceToApex(PolyhedralBoundedSolid* solid, double apexZ);

    /**
    Splits face `faceId`, a single loop polygon, into a fan of triangles
    towards a new vertex at (0, 0, `apexZ`); the face keeps one of the
    triangles.
    */
    static void closeFaceToApex(PolyhedralBoundedSolid* solid, int faceId,
        double apexZ);

    PolyhedralBoundedSolid* buildPolyhedralBoundedSolid(int nsides,
        int heightDivisions);

    /**
    Open lamina of the lateral surface (see `exportToPolyhedralBoundedSolid`
    with `withCaps` false).
    */
    PolyhedralBoundedSolid* buildOpenLamina(int nsides);

public:
    Cone(double bottomRadius, double topRadius, double height);
    virtual ~Cone() {}

    double getBottomRadius() const;
    double getTopRadius() const;
    double getHeight() const;
    void setBottomRadius(double value);
    void setTopRadius(double value);
    void setHeight(double value);

    Ray* doIntersectionFirstHit(const Ray& inOutRay);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit) override;
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData) override;
    virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance) override;
    virtual double* getMinMax() override;
    // Finite solid bounds: |x|,|y| <= max radius and 0 <= z <= height.
    void getBounds(GeometryBounds& result) const override;

    /**
    @return a new boundary representation of the cone with the default
    divisions, owned by the caller
    */
    virtual PolyhedralBoundedSolid* exportToPolyhedralBoundedSolid() override;

    /**
    Boundary representation of the cone (a cylinder when both radii are
    equal), with its base on the plane z = 0 and its axis on +Z. The rings
    of vertices start at +X and go counterclockwise seen from +Z.

    With `withCaps` true (default) it is the closed solid: the lateral
    surface plus the base disk and, when the top radius is not zero, the top
    disk.

    With `withCaps` false it is a valid boundary representation of the
    lateral surface alone, an open curved surface without caps: a lamina
    ([MANT1988] 12.2, generalized from the planar circular lamina) with two
    coincident sides of opposite orientation, so it encloses no volume.
      - When both radii are not zero (cylinder or frustum) the lamina is
        built from a washer (`Tube::createWasherLamina`) whose inner ring is
        moved to z = height with the top radius, and both of its faces are
        split into the planar quadrilaterals between the two rings (a
        `lmekr` joins both loops of each face and `smef` splits it). The
        lamina has the topology of a torus (genus 1). Faces 1 and
        3 to `circumferenceDivisions` + 1 are the outer side (normals away
        from the axis); faces 2 and `circumferenceDivisions` + 2 to
        2 * `circumferenceDivisions` are the inner side.
      - When the top radius is zero (cone with apex) it is a circular lamina
        whose two faces are split into triangle fans, each one towards its
        own apex vertex (both at the same point), so the lamina has the
        topology of a sphere. Face 1 and faces 3 to
        `circumferenceDivisions` + 1 are the outer side; face 2 and the
        rest are the inner side.
    The open lamina has a single band of faces along the axis:
    `heightDivisions` is not used.

    @param circumferenceDivisions sides of the cone base (at least 3)
    @param heightDivisions vertical subdivisions of the closed solid (at
    least 1)
    @param withCaps true for the closed solid, false for the open lamina of
    the lateral surface
    @return a new boundary representation of the cone, owned by the caller
    */
    PolyhedralBoundedSolid* exportToPolyhedralBoundedSolid(
        int circumferenceDivisions, int heightDivisions, bool withCaps = true);

};

#endif

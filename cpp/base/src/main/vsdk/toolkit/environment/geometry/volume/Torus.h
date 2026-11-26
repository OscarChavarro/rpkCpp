#ifndef __TORUS__
#define __TORUS__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/volume/Solid.h"
class Ray;
class RayHit;
class PolyhedralBoundedSolid;

class Torus : public Solid {
private:
    /// Default number of sections of the boundary representation along the
    /// major circle (around the Z axis)
    static const int DEFAULT_MAJOR_DIVISIONS = 24;
    /// Default number of sides of each section (around the tube)
    static const int DEFAULT_MINOR_DIVISIONS = 12;
    static const int MIN_MAJOR_DIVISIONS = 3;
    static const int MIN_MINOR_DIVISIONS = 3;

    double majorRadius;
    double minorRadius;

    /**
    Finds the real roots of a polynomial, in ascending order, isolating them
    with the roots of its derivative (that separate the intervals where the
    polynomial is monotonic) and bisecting the intervals where it changes its
    sign. Roots of even multiplicity (tangencies) are found only if they are
    a root of the derivative too.
    @param polynomial coefficients, in ascending order of the powers
    @return the real roots
    */
    static java::ArrayList<double> findRealRoots(
        const java::ArrayList<double>& polynomial);
    static double evaluate(const java::ArrayList<double>& polynomial,
                           int degree, double x);

    /**
    Checks a root of the intersection equation: the solver of the equation can
    give real roots that do not correspond to any point of the torus.
    @param origin origin of the ray
    @param direction unit direction of the ray
    @param t distance along the ray to check
    @return true if the point of the ray at the distance is on the surface of
    the torus
    */
    bool isOnSurface(const Vector3Dd& origin, const Vector3Dd& direction,
                     double t) const;

    /**
    Builds a "polyhedral torus" approximation by a closed rotational sweep of
    a circular lamina, following the idea of section [MANT1988].12.5 and
    program [MANT1988].12.11, but expressed with the generalized sweep of
    `PolyhedralBoundedSolidEulerConstructions` (each step is a rotation about
    the Z axis instead of a translation):
      - A circular lamina of `minorDivisions` vertices and radius
        `minorRadius` is placed on the XZ plane, centered at
        (`majorRadius`, 0, 0), with its face 1 looking along the sweep (+Y).
      - Face 1 is swept `majorDivisions` times, each one rotated
        2*PI/`majorDivisions` about Z. After the last step its vertices
        coincide with the ones of the other face of the lamina.
      - The two coincident faces are merged as two loops of one face
        (`lkfmrh`) and glued (`loopGlue`, [MANT1988].12.4.2), closing the
        handle of the genus 1 solid.

    The side faces are planar: the four vertices of each one are two points
    of a section and their rotated copies, an isosceles trapezoid symmetric
    about the bisector plane of both sections.
    */
    PolyhedralBoundedSolid* buildPolyhedralBoundedSolid(
        int majorDivisions, int minorDivisions);

public:
    Torus(double inMajorRadius, double inMinorRadius);
    virtual ~Torus() {}

    double getMajorRadius() const;
    void setMajorRadius(double rMajor);
    double getMinorRadius() const;
    void setMinorRadius(double rMinor);

    Ray* doIntersectionFirstHit(const Ray& inOutRay);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outHit);
    virtual double* getMinMax();

    virtual PolyhedralBoundedSolid* exportToPolyhedralBoundedSolid() override;

    /**
    @param majorDivisions number of sections along the major circle, around
    the Z axis (at least 3)
    @param minorDivisions number of sides of each section, around the tube
    (at least 3)
    @return a new polyhedral approximation of current torus
    */
    PolyhedralBoundedSolid* exportToPolyhedralBoundedSolid(
        int majorDivisions, int minorDivisions);
};

#endif

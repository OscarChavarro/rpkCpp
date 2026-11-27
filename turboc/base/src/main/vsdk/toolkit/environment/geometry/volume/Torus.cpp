#include <stddef.h>
#include <math.h>

#include "java/util/ArrayList.txx"

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/volume/Torus.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerConstructions.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidTopologyEditing.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
#include "vsdk/toolkit/common/Cpp98Support.h"
#include "java/lang/Math.h"
Torus::Torus(double inMajorRadius, double inMinorRadius)
    : majorRadius(inMajorRadius), minorRadius(inMinorRadius)
{
    // Turbo C++ 3.0 crashes with the new of a template class as an argument
    MemberEntityControlAccessor<Torus, double> *accessor;
    accessor = new MemberEntityControlAccessor<Torus, double>(
        &Torus::getMajorRadius, &Torus::setMajorRadius);
    addControlSpecification("double;majorRadius;(0, INFINITE)", accessor);
    accessor = new MemberEntityControlAccessor<Torus, double>(
        &Torus::getMinorRadius, &Torus::setMinorRadius);
    addControlSpecification("double;minorRadius;(0, INFINITE)", accessor);
}

double Torus::getMajorRadius() const { return majorRadius; }
void Torus::setMajorRadius(double rMajor) { majorRadius = rMajor; }
double Torus::getMinorRadius() const { return minorRadius; }
void Torus::setMinorRadius(double rMinor) { minorRadius = rMinor; }

/// The roots the quartic solver gives for a ray are accepted only if the
/// point they define is on the surface of the torus, within this distance
/// (relative to the major radius): verified roots have errors around 1e-11,
/// and spurious ones (the solver gives some) errors of the order of 0.1
static const double ROOT_VALIDATION_TOLERANCE = 1.0e-6;

Ray* Torus::doIntersectionFirstHit(const Ray& inOutRay) {
    Vector3Dd origin = inOutRay.getOrigin();

    Ray ray = inOutRay.withDirection(inOutRay.getDirection().normalized());
    Vector3Dd d = ray.getDirection();

    // Distance from the original origin to the point nearest to the center
    double shift = -origin.dotProduct(d);
    Vector3Dd p = origin.add(d.multiply(shift));

    double alpha, beta, gama;

    alpha = d.dotProduct(d);
    beta = 2 * p.dotProduct(d);
    gama = p.dotProduct(p) - (minorRadius * minorRadius) - (majorRadius * majorRadius);

    double a4, a3, a2, a1, a0;

    a4 = alpha * alpha;
    a3 = 2 * alpha * beta;
    a2 = (beta * beta) + 2 * alpha * gama + 4 * (majorRadius * majorRadius) * (d.z() * d.z());
    a1 = 2 * beta * gama + 8 * (majorRadius * majorRadius) * p.z() * d.z();
    a0 = (gama * gama) + 4 * (majorRadius * majorRadius) * (p.z() * p.z()) - (4 * (majorRadius * majorRadius) * (minorRadius * minorRadius));

    ArrayList<double> polynomial;
    polynomial.add(a0);
    polynomial.add(a1);
    polynomial.add(a2);
    polynomial.add(a3);
    polynomial.add(a4);
    ArrayList<double> roots = findRealRoots(polynomial);
    double mRoot = 0;
    int count = 0;

    for ( long i = 0; i < roots.size(); i++ ) {
        // Distance along the ray that was given
        double t = shift + roots.get(i);

        if ( t > 0 && isOnSurface(origin, d, t) ) {
            if ( count == 0 || t < mRoot ) {
                mRoot = t;
                count++;
            }
        }
    }

    if ( count == 0 ) {
        return NULL;
    }
    return new Ray(ray.withT(mRoot));
}

ArrayList<double> Torus::findRealRoots(
    const ArrayList<double>& polynomial)
{
    int degree = (int)polynomial.size() - 1;
    ArrayList<double> found;

    while ( degree > 0 && polynomial.get(degree) == 0.0 ) {
        degree--;
    }
    if ( degree <= 0 ) {
        return found;
    }
    if ( degree == 1 ) {
        found.add(-polynomial.get(0) / polynomial.get(1));
        return found;
    }

    ArrayList<double> derivative;

    long k;
    for ( k = 1; k <= degree; k++ ) {
        derivative.add(k * polynomial.get(k));
    }
    ArrayList<double> critical = findRealRoots(derivative);

    // Cauchy bound: every root is inside it
    double bound = 0;

    for ( k = 0; k < degree; k++ ) {
        bound = Math::max(bound, fabs(polynomial.get(k) / polynomial.get(degree)));
    }
    bound += 1;

    ArrayList<double> limits;

    limits.add(-bound);
    for ( k = 0; k < critical.size(); k++ ) {
        limits.add(Math::max(-bound, Math::min(bound, critical.get(k))));
    }
    limits.add(bound);

    for ( k = 0; k < limits.size() - 1 && found.size() < degree; k++ ) {
        double low = limits.get(k);
        double high = limits.get(k + 1);
        double valueAtLow = evaluate(polynomial, degree, low);
        double valueAtHigh = evaluate(polynomial, degree, high);

        if ( valueAtLow == 0.0 ) {
            if ( found.size() == 0 || found.get(found.size() - 1) != low ) {
                found.add(low);
            }
        }
        else if ( valueAtHigh != 0.0 && (valueAtLow < 0.0) != (valueAtHigh < 0.0) ) {
            for ( int iteration = 0; iteration < 200; iteration++ ) {
                double middle = 0.5 * (low + high);

                if ( middle <= low || middle >= high ) {
                    break;
                }
                if ( (evaluate(polynomial, degree, middle) < 0.0) == (valueAtLow < 0.0) ) {
                    low = middle;
                }
                else {
                    high = middle;
                }
            }
            found.add(0.5 * (low + high));
        }
    }
    if ( found.size() < degree && evaluate(polynomial, degree, bound) == 0.0 ) {
        found.add(bound);
    }
    return found;
}

double Torus::evaluate(const ArrayList<double>& polynomial, int degree,
                       double x)
{
    double value = 0;

    for ( int k = degree; k >= 0; k-- ) {
        value = value * x + polynomial.get(k);
    }
    return value;
}

BOOLEAN Torus::isOnSurface(const Vector3Dd& origin, const Vector3Dd& direction,
                        double t) const
{
    Vector3Dd hit = origin.add(direction.multiply(t));
    double distanceToCircle = Cpp98Support::hypot(Cpp98Support::hypot(hit.x(), hit.y()) - majorRadius, hit.z());
    double tolerance = ROOT_VALIDATION_TOLERANCE * Math::max(majorRadius, minorRadius);

    return fabs(distanceToCircle - minorRadius) <= tolerance;
}

BOOLEAN Torus::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit) {
    Ray* hit = doIntersectionFirstHit(inRay);
    if (hit == NULL) return FALSE;

    if (outHit != NULL) {
        outHit->setRay(*hit);
        doExtraInformation(*hit, hit->getT(), outHit);
        outHit->setRay(*hit);
    }
    delete hit;
    return TRUE;
}

void Torus::doExtraInformation(const Ray& inRay, double inT, RayHit* outHit) {
    if (outHit == NULL) return;

    Vector3Dd p = inRay.getOrigin().add(inRay.getDirection().multiply(inT));
    if (outHit->needsPoint()) {
        outHit->point = p;
    }

    if (outHit->needsNormal() || outHit->needsTextureCoordinates() || outHit->needsTangent()) {
        double x = p.x(), y = p.y(), z = p.z();
        double sum = x*x + y*y + z*z + majorRadius*majorRadius - minorRadius*minorRadius;
        Vector3Dd grad(
            4*x*sum - 8*majorRadius*majorRadius*x,
            4*y*sum - 8*majorRadius*majorRadius*y,
            4*z*sum);
        Vector3Dd n = grad.normalized();
        if (outHit->needsNormal()) outHit->normal = n;

        if (outHit->needsTextureCoordinates()) {
            double theta = atan2(y, x);
            double ring = sqrt(x*x + y*y);
            double phi = atan2(z, ring - majorRadius);
            outHit->u = (theta + M_PI) / (2.0 * M_PI);
            outHit->v = (phi + M_PI) / (2.0 * M_PI);
        }

        if (outHit->needsTangent()) {
            double theta = atan2(y, x);
            outHit->tangent = Vector3Dd(-sin(theta), cos(theta), 0);
        }
    }
}

double* Torus::getMinMax() {
    double* m = new double[6];
    double ext = majorRadius + minorRadius;
    m[0] = -ext; m[1] = -ext; m[2] = -minorRadius;
    m[3] =  ext; m[4] =  ext; m[5] =  minorRadius;
    return m;
}

PolyhedralBoundedSolid* Torus::exportToPolyhedralBoundedSolid()
{
    return buildPolyhedralBoundedSolid(DEFAULT_MAJOR_DIVISIONS,
        DEFAULT_MINOR_DIVISIONS);
}

PolyhedralBoundedSolid* Torus::exportToPolyhedralBoundedSolid(
    int majorDivisions, int minorDivisions)
{
    int normalizedMajorDivisions = Math::max((int)MIN_MAJOR_DIVISIONS,
        majorDivisions);
    int normalizedMinorDivisions = Math::max((int)MIN_MINOR_DIVISIONS,
        minorDivisions);

    if ( normalizedMajorDivisions == DEFAULT_MAJOR_DIVISIONS &&
         normalizedMinorDivisions == DEFAULT_MINOR_DIVISIONS ) {
        return exportToPolyhedralBoundedSolid();
    }

    return buildPolyhedralBoundedSolid(normalizedMajorDivisions,
        normalizedMinorDivisions);
}

PolyhedralBoundedSolid* Torus::buildPolyhedralBoundedSolid(
    int majorDivisions, int minorDivisions)
{
    PolyhedralBoundedSolid* solid;
    Matrix4x4d T;
    Matrix4x4d R;
    Matrix4x4d M;
    Matrix4x4d step;
    int i;

    //- Section lamina, from the XY plane to the XZ plane -------------
    solid = PolyhedralBoundedSolidEulerConstructions::createCircularLamina(
        0.0, 0.0, minorRadius, 0.0, minorDivisions);
    T = Matrix4x4d();
    T = T.translation(majorRadius, 0.0, 0.0);
    R = Matrix4x4d();
    R = R.axisRotation(-M_PI/2.0, 1, 0, 0);
    M = T.multiply(R);
    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        solid->getVerticesList().get(i)->position =
            M.multiply(solid->getVerticesList().get(i)->position);
    }

    //- Rotational sweep of face 1 about the Z axis -------------------
    step = Matrix4x4d();
    step = step.axisRotation(2.0*M_PI / ((double)majorDivisions),
        0, 0, 1);
    for ( i = 0; i < majorDivisions; i++ ) {
        PolyhedralBoundedSolidEulerConstructions::translationalSweepExtrudeFace(
            solid, solid->findFace(1), step);
    }

    //- Close the ring: glue the swept face to the starting one -------
    _PolyhedralBoundedSolidFace* headFace = solid->findFace(2);
    _PolyhedralBoundedSolidFace* tailFace = solid->findFace(1);
    PolyhedralBoundedSolidEulerOperators::lkfmrh(solid, headFace, tailFace);
    PolyhedralBoundedSolidTopologyEditing::loopGlue(solid, headFace->id);
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);

    return solid;
}

const Class Torus::CLASS = { "Torus", &Solid::CLASS };

const Class *
Torus::getClass() const
{
    return &CLASS;
}

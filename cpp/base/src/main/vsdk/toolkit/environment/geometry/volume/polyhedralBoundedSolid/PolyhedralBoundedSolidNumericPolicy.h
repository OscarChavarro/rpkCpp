#ifndef __POLYHEDRAL_BOUNDED_SOLID_NUMERIC_POLICY__
#define __POLYHEDRAL_BOUNDED_SOLID_NUMERIC_POLICY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class PolyhedralBoundedSolid;
class _PolyhedralBoundedSolidFace;

/**
Tolerances of the geometric predicates of the boundary representations,
scaled with the size of the model (a solid, a pair of solids, a face or a
set of points).

C++ counterpart of Java's `PolyhedralBoundedSolidNumericPolicy`.
*/
class PolyhedralBoundedSolidNumericPolicy {
public:
    static const double BREP_EPSILON;
    static const double BREP_BIG_EPSILON;

    class ToleranceContext {
    private:
        double modelScale_;
        double epsilon_;
        double bigEpsilon_;
        double unitVectorTolerance_;
        double angleTolerance_;
        double coplanarDotTolerance_;
        double unitIntervalTolerance_;
    public:
        /**
        C++ port note: Java has no default constructor; this one builds the
        default context (`defaultContext()`).
        */
        ToleranceContext();
        ToleranceContext(double modelScale, double epsilon, double bigEpsilon,
            double unitVectorTolerance, double angleTolerance,
            double coplanarDotTolerance, double unitIntervalTolerance);

        double modelScale() const;
        double epsilon() const;
        double bigEpsilon() const;
        double unitVectorTolerance() const;
        double angleTolerance() const;
        double coplanarDotTolerance() const;
        double unitIntervalTolerance() const;
    };

    static ToleranceContext defaultContext();
    static ToleranceContext fromScale(double modelScale);
    static ToleranceContext forSolid(PolyhedralBoundedSolid* solid);
    static ToleranceContext forSolids(PolyhedralBoundedSolid* a,
                                      PolyhedralBoundedSolid* b);
    static ToleranceContext forFace(_PolyhedralBoundedSolidFace* face);
    static ToleranceContext forPoints(const java::ArrayList<Vector3Dd>& points);

    static int compare(double a, double b, double tolerance);
    static int compare(double a, double b, const ToleranceContext& context);
    static int compareToZero(double value, const ToleranceContext& context);
    static int compareToZeroBig(double value, const ToleranceContext& context);
    static bool isZero(double value, const ToleranceContext& context);
    static bool isZeroBig(double value, const ToleranceContext& context);
    static bool pointsCoincident(const Vector3Dd& a, const Vector3Dd& b, const ToleranceContext& context);
    static bool pointsSeparated(const Vector3Dd& a, const Vector3Dd& b, const ToleranceContext& context);
    static int testPointInside(_PolyhedralBoundedSolidFace* face,
                               const Vector3Dd& point,
                               const ToleranceContext& context);
    static bool vectorsColinear(const Vector3Dd& a, const Vector3Dd& b,
                                const ToleranceContext& context);
    static bool unitVectorsParallel(const Vector3Dd& a, const Vector3Dd& b,
                                    const ToleranceContext& context);
    static bool angleIntervalsOverlap(double upperA, double lowerB,
                                      const ToleranceContext& context);
    static bool unitIntervalContainsStrictly(double t,
                                             const ToleranceContext& context);
    static double orientationTolerance2D(const Vector2Dd& a, const Vector2Dd& b,
                                         const Vector2Dd& c,
                                         const ToleranceContext& context);
    static double linearTolerance2D(const ToleranceContext& context);
    static double areaTolerance2D(const ToleranceContext& context);

private:
    static double sanitizeScale(double modelScale);
    static double diagonalSize(double minX, double minY, double minZ,
        double maxX, double maxY, double maxZ);
    static double estimateSolidScale(PolyhedralBoundedSolid* solid);
    static double estimateFaceScale(_PolyhedralBoundedSolidFace* face);
    static double estimatePointsScale(const java::ArrayList<Vector3Dd>& points);
    static double clamp(double value, double min, double max);
};

#endif

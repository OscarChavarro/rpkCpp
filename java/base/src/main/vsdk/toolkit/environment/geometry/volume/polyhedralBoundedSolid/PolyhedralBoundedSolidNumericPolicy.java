//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid;

import java.util.ArrayList;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidFace;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidHalfEdge;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidLoop;

/**
Numerical tolerances for geometric predicates used to implement the face
equations and intersection predicates of chapter [MANT1988].13 and the robust
case distinctions required by chapter [MANT1988].15.
*/
public class PolyhedralBoundedSolidNumericPolicy
{
    public static final double BREP_EPSILON = VSDK.EPSILON;
    public static final double BREP_BIG_EPSILON = 10.0 * BREP_EPSILON;

    /** Scale of an empty or degenerate model (no extent to measure). */
    private static final double DEFAULT_SCALE = 1.0;
    /**
    Smallest model scale. Tolerances follow the model down to this size, so
    a part modelled in metres at 10^-3 gets 10^-3 times the tolerances of
    the same part at unit size. They used to stop at scale 1: below it the
    tolerances were absolute and, at 10^-3, a tenth of the features.
    */
    private static final double MIN_SCALE = 1.0e-9;
    private static final double MAX_UNIT_INTERVAL_TOLERANCE = 1.0e-3;
    private static final ToleranceContext DEFAULT_CONTEXT =
        fromScale(DEFAULT_SCALE);

    private PolyhedralBoundedSolidNumericPolicy()
    {
    }

    public static final class ToleranceContext
    {
        private final double modelScale;
        private final double epsilon;
        private final double bigEpsilon;
        private final double unitVectorTolerance;
        private final double angleTolerance;
        private final double coplanarDotTolerance;
        private final double unitIntervalTolerance;

        private ToleranceContext(double modelScale, double epsilon, double bigEpsilon,
                                 double unitVectorTolerance, double angleTolerance,
                                 double coplanarDotTolerance,
                                 double unitIntervalTolerance)
        {
            this.modelScale = modelScale;
            this.epsilon = epsilon;
            this.bigEpsilon = bigEpsilon;
            this.unitVectorTolerance = unitVectorTolerance;
            this.angleTolerance = angleTolerance;
            this.coplanarDotTolerance = coplanarDotTolerance;
            this.unitIntervalTolerance = unitIntervalTolerance;
        }

        public double modelScale()
        {
            return modelScale;
        }

        public double epsilon()
        {
            return epsilon;
        }

        public double bigEpsilon()
        {
            return bigEpsilon;
        }

        public double unitVectorTolerance()
        {
            return unitVectorTolerance;
        }

        public double angleTolerance()
        {
            return angleTolerance;
        }

        public double coplanarDotTolerance()
        {
            return coplanarDotTolerance;
        }

        public double unitIntervalTolerance()
        {
            return unitIntervalTolerance;
        }
    }

    public static ToleranceContext defaultContext()
    {
        return DEFAULT_CONTEXT;
    }

    public static ToleranceContext fromScale(double modelScale)
    {
        double safeScale = sanitizeScale(modelScale);
        double eps = BREP_EPSILON * safeScale;
        double bigEps = BREP_BIG_EPSILON * safeScale;
        double unitTol = BREP_BIG_EPSILON;
        double angleTol = BREP_BIG_EPSILON;
        double coplanarDotTol = 10.0 * BREP_BIG_EPSILON;
        double unitIntervalTol = clamp(bigEps / safeScale,
            BREP_BIG_EPSILON, MAX_UNIT_INTERVAL_TOLERANCE);
        return new ToleranceContext(safeScale, eps, bigEps, unitTol, angleTol,
            coplanarDotTol, unitIntervalTol);
    }

    public static ToleranceContext forSolid(PolyhedralBoundedSolid solid)
    {
        return fromScale(estimateSolidScale(solid));
    }

    public static ToleranceContext forSolids(PolyhedralBoundedSolid a,
                                             PolyhedralBoundedSolid b)
    {
        return fromScale(Math.max(estimateSolidScale(a), estimateSolidScale(b)));
    }

    public static ToleranceContext forFace(_PolyhedralBoundedSolidFace face)
    {
        return fromScale(estimateFaceScale(face));
    }

    public static ToleranceContext forPoints(ArrayList<Vector3Dd> points)
    {
        return fromScale(estimatePointsScale(points));
    }

    private static double sanitizeScale(double scale)
    {
        if ( !Double.isFinite(scale) || scale <= 0.0 ) {
            return DEFAULT_SCALE;
        }
        if ( scale < MIN_SCALE ) {
            return MIN_SCALE;
        }
        return scale;
    }

    private static double estimateSolidScale(PolyhedralBoundedSolid solid)
    {
        if ( solid == null ) {
            return DEFAULT_SCALE;
        }

        double[] minMax = solid.getMinMax();
        if ( minMax == null || minMax.length < 6 ) {
            return DEFAULT_SCALE;
        }
        return diagonalSize(minMax[0], minMax[1], minMax[2],
                            minMax[3], minMax[4], minMax[5]);
    }

    private static double estimateFaceScale(_PolyhedralBoundedSolidFace face)
    {
        if ( face == null || face.boundariesList == null ) {
            return DEFAULT_SCALE;
        }

        double minX = Double.POSITIVE_INFINITY;
        double minY = Double.POSITIVE_INFINITY;
        double minZ = Double.POSITIVE_INFINITY;
        double maxX = -Double.POSITIVE_INFINITY;
        double maxY = -Double.POSITIVE_INFINITY;
        double maxZ = -Double.POSITIVE_INFINITY;
        boolean found = false;

        int i;
        int j;
        for ( i = 0; i < face.boundariesList.size(); i++ ) {
            _PolyhedralBoundedSolidLoop loop = face.boundariesList.get(i);
            if ( loop == null ) {
                continue;
            }
            for ( j = 0; j < loop.halfEdgesList.size(); j++ ) {
                _PolyhedralBoundedSolidHalfEdge he = loop.halfEdgesList.get(j);
                if ( he == null || he.startingVertex == null ) {
                    continue;
                }
                Vector3Dd p = he.startingVertex.position;
                if ( p == null ) {
                    continue;
                }
                found = true;
                if ( p.x() < minX ) minX = p.x();
                if ( p.y() < minY ) minY = p.y();
                if ( p.z() < minZ ) minZ = p.z();
                if ( p.x() > maxX ) maxX = p.x();
                if ( p.y() > maxY ) maxY = p.y();
                if ( p.z() > maxZ ) maxZ = p.z();
            }
        }

        if ( !found ) {
            return DEFAULT_SCALE;
        }
        return diagonalSize(minX, minY, minZ, maxX, maxY, maxZ);
    }

    private static double estimatePointsScale(ArrayList<Vector3Dd> points)
    {
        if ( points == null || points.size() < 2 ) {
            return DEFAULT_SCALE;
        }

        double minX = Double.POSITIVE_INFINITY;
        double minY = Double.POSITIVE_INFINITY;
        double minZ = Double.POSITIVE_INFINITY;
        double maxX = -Double.POSITIVE_INFINITY;
        double maxY = -Double.POSITIVE_INFINITY;
        double maxZ = -Double.POSITIVE_INFINITY;
        boolean found = false;

        int i;
        for ( i = 0; i < points.size(); i++ ) {
            Vector3Dd p = points.get(i);
            if ( p == null ) {
                continue;
            }
            found = true;
            if ( p.x() < minX ) minX = p.x();
            if ( p.y() < minY ) minY = p.y();
            if ( p.z() < minZ ) minZ = p.z();
            if ( p.x() > maxX ) maxX = p.x();
            if ( p.y() > maxY ) maxY = p.y();
            if ( p.z() > maxZ ) maxZ = p.z();
        }

        if ( !found ) {
            return DEFAULT_SCALE;
        }
        return diagonalSize(minX, minY, minZ, maxX, maxY, maxZ);
    }

    private static double diagonalSize(double minX, double minY, double minZ,
                                       double maxX, double maxY, double maxZ)
    {
        double dx = maxX - minX;
        double dy = maxY - minY;
        double dz = maxZ - minZ;
        double diag = Math.sqrt(dx*dx + dy*dy + dz*dz);
        return sanitizeScale(diag);
    }

    public static int compare(double a, double b, double tolerance)
    {
        return PolyhedralBoundedSolid.compareValue(a, b, tolerance);
    }

    public static int compare(double a, double b, ToleranceContext context)
    {
        return compare(a, b, context.epsilon());
    }

    public static int compareToZero(double value, ToleranceContext context)
    {
        return compare(value, 0.0, context.epsilon());
    }

    public static int compareToZeroBig(double value, ToleranceContext context)
    {
        return compare(value, 0.0, context.bigEpsilon());
    }

    public static boolean isZero(double value, ToleranceContext context)
    {
        return Math.abs(value) <= context.epsilon();
    }

    public static boolean isZeroBig(double value, ToleranceContext context)
    {
        return Math.abs(value) <= context.bigEpsilon();
    }

    public static boolean pointsCoincident(Vector3Dd a, Vector3Dd b,
                                           ToleranceContext context)
    {
        return Vector3Dd.distance(a, b) <= context.bigEpsilon();
    }

    public static boolean pointsSeparated(Vector3Dd a, Vector3Dd b,
                                          ToleranceContext context)
    {
        return Vector3Dd.distance(a, b) > context.bigEpsilon();
    }

    public static int testPointInside(_PolyhedralBoundedSolidFace face, Vector3Dd point,
                                      ToleranceContext context)
    {
        return face.testPointInside(point, context.bigEpsilon());
    }

    public static boolean vectorsColinear(Vector3Dd a, Vector3Dd b,
                                          ToleranceContext context)
    {
        // |a x b| = |a| |b| sin(angle): compare the sine, so that the test
        // does not depend on the length of the vectors (it used to accept
        // angles up to bigEpsilon / length, half a degree at 10^-3).
        return a.crossProduct(b).length() <=
            BREP_BIG_EPSILON * a.length() * b.length();
    }

    public static boolean unitVectorsParallel(Vector3Dd a, Vector3Dd b,
                                              ToleranceContext context)
    {
        return a.crossProduct(b).length() <= context.unitVectorTolerance();
    }

    public static boolean angleIntervalsOverlap(double upperA, double lowerB,
                                                ToleranceContext context)
    {
        double tolerance = context.angleTolerance();
        return upperA + tolerance > lowerB - tolerance;
    }

    public static boolean unitIntervalContainsStrictly(double t,
                                                       ToleranceContext context)
    {
        return t > context.unitIntervalTolerance() &&
               t < 1.0 - context.unitIntervalTolerance();
    }

    public static double orientationTolerance2D(Vector2Dd a, Vector2Dd b,
                                                Vector2Dd c,
                                                ToleranceContext context)
    {
        double lx = Math.max(Math.max(Math.abs(a.x() - b.x()), Math.abs(a.x() - c.x())),
                             Math.abs(b.x() - c.x()));
        double ly = Math.max(Math.max(Math.abs(a.y() - b.y()), Math.abs(a.y() - c.y())),
                             Math.abs(b.y() - c.y()));
        // The orientation is a cross product, |b - a| times the distance of
        // c from the line: a distance tolerance times the span. The span
        // used to be at least 1, which below unit size made the tolerance
        // an angle of bigEpsilon / span (half a degree at 10^-3).
        double span = Math.max(lx, ly);
        return context.bigEpsilon() * span;
    }

    public static double linearTolerance2D(ToleranceContext context)
    {
        return context.bigEpsilon();
    }

    public static double areaTolerance2D(ToleranceContext context)
    {
        double linearTolerance = context.bigEpsilon();
        return linearTolerance * linearTolerance;
    }

    private static double clamp(double value, double min, double max)
    {
        if ( value < min ) {
            return min;
        }
        if ( value > max ) {
            return max;
        }
        return value;
    }
}

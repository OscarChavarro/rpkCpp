//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.surface.InfinitePlane;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidFace;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidHalfEdge;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidLoop;

/**
Snapshot of the geometry of one face for read-only passes that query the
same unchanged faces many times (strict validation, point classification):
the containing plane, computed once, and the axis-aligned bounding box of
the face vertices (Fable5.2Analysis.md Stage G).

<p>Faces do not cache their plane, because vertex positions are public and
change outside the Euler operators (transformations, snapping). A snapshot
is only valid while its solid is not changed; build it at the start of the
pass and drop it at the end.</p>

<p>The box tests are conservative with respect to
`_PolyhedralBoundedSolidFace.testPointInside(point, tolerance, plane)`:
{@link #projectionExcludes} answers true only when that test would answer
OUTSIDE, so skipping the test then changes no result.</p>
 */
public final class PolyhedralBoundedSolidFaceGeometry
{
    private final InfinitePlane plane;
    private final int dominantCoordinate;
    private final double minX;
    private final double minY;
    private final double minZ;
    private final double maxX;
    private final double maxY;
    private final double maxZ;
    /** Largest distance from a face vertex to the plane. */
    private final double planeDeviation;
    private final double longestEdge;

    private PolyhedralBoundedSolidFaceGeometry(_PolyhedralBoundedSolidFace face)
    {
        _PolyhedralBoundedSolidLoop loop;
        _PolyhedralBoundedSolidHalfEdge he;
        _PolyhedralBoundedSolidHalfEdge start;
        double lowX = Double.POSITIVE_INFINITY;
        double lowY = Double.POSITIVE_INFINITY;
        double lowZ = Double.POSITIVE_INFINITY;
        double highX = Double.NEGATIVE_INFINITY;
        double highY = Double.NEGATIVE_INFINITY;
        double highZ = Double.NEGATIVE_INFINITY;
        double deviation = 0.0;
        double longest = 0.0;
        int i;

        plane = face.getContainingPlane();
        for ( i = 0; i < face.boundariesList.size(); i++ ) {
            loop = face.boundariesList.get(i);
            if ( loop == null || loop.boundaryStartHalfEdge == null ) {
                continue;
            }
            start = loop.boundaryStartHalfEdge;
            he = start;
            do {
                Vector3Dd p = he.startingVertex.position;
                lowX = Math.min(lowX, p.x());
                lowY = Math.min(lowY, p.y());
                lowZ = Math.min(lowZ, p.z());
                highX = Math.max(highX, p.x());
                highY = Math.max(highY, p.y());
                highZ = Math.max(highZ, p.z());
                if ( plane != null ) {
                    deviation = Math.max(deviation,
                        Math.abs(plane.pointDistance(p)));
                }
                if ( he.next() == null ) {
                    break;
                }
                longest = Math.max(longest, Vector3Dd.distance(p,
                    he.next().startingVertex.position));
                he = he.next();
            } while ( he != start );
        }
        minX = lowX;
        minY = lowY;
        minZ = lowZ;
        maxX = highX;
        maxY = highY;
        maxZ = highZ;
        planeDeviation = deviation;
        longestEdge = longest;
        dominantCoordinate = plane == null ? 0 :
            dominantCoordinateOf(plane.getNormal());
    }

    /** Same choice as `_PolyhedralBoundedSolidFace.testPointInsideDetailed`. */
    private static int dominantCoordinateOf(Vector3Dd n)
    {
        if ( Math.abs(n.x()) >= Math.abs(n.y()) &&
             Math.abs(n.x()) >= Math.abs(n.z()) ) {
            return 1;
        }
        if ( Math.abs(n.y()) >= Math.abs(n.x()) &&
             Math.abs(n.y()) >= Math.abs(n.z()) ) {
            return 2;
        }
        return 3;
    }

    public static PolyhedralBoundedSolidFaceGeometry of(
        _PolyhedralBoundedSolidFace face)
    {
        return new PolyhedralBoundedSolidFaceGeometry(face);
    }

    /** One snapshot per face, indexed like `solid.getPolygonsList()`. */
    public static PolyhedralBoundedSolidFaceGeometry[] of(
        PolyhedralBoundedSolid solid)
    {
        int n = solid.getPolygonsList().size();
        PolyhedralBoundedSolidFaceGeometry[] out =
            new PolyhedralBoundedSolidFaceGeometry[n];
        int i;

        for ( i = 0; i < n; i++ ) {
            out[i] = new PolyhedralBoundedSolidFaceGeometry(
                solid.getPolygonsList().get(i));
        }
        return out;
    }

    /** The containing plane, or null for a degenerate face. */
    public InfinitePlane getPlane()
    {
        return plane;
    }

    public double getPlaneDeviation()
    {
        return planeDeviation;
    }

    /**
    Margin that covers every tolerance of the point-in-face test: vertex
    proximity (2 tolerance), edge proximity (tolerance) and the parametric
    slack of the segment test (VSDK.EPSILON times the edge length).
    */
    private double pointTestMargin(double tolerance)
    {
        return 4.0 * tolerance + 2.0 * VSDK.EPSILON * longestEdge;
    }

    /**
    True when `point` is far enough from the face, in the two coordinates
    kept by the point-in-face projection, that
    `testPointInside(point, tolerance, getPlane())` must answer OUTSIDE.
    */
    public boolean projectionExcludes(Vector3Dd point, double tolerance)
    {
        double m = pointTestMargin(tolerance);
        boolean xOut = point.x() < minX - m || point.x() > maxX + m;
        boolean yOut = point.y() < minY - m || point.y() > maxY + m;
        boolean zOut = point.z() < minZ - m || point.z() > maxZ + m;

        switch ( dominantCoordinate ) {
          case 1: return yOut || zOut;
          case 2: return xOut || zOut;
          case 3: return xOut || yOut;
          default: return false;
        }
    }

    /**
    True when the boxes of the two faces, each grown by `margin`, do not
    overlap.
    */
    public boolean boxSeparatedFrom(PolyhedralBoundedSolidFaceGeometry other,
        double margin)
    {
        return maxX + margin < other.minX - margin ||
               other.maxX + margin < minX - margin ||
               maxY + margin < other.minY - margin ||
               other.maxY + margin < minY - margin ||
               maxZ + margin < other.minZ - margin ||
               other.maxZ + margin < minZ - margin;
    }

    /**
    True when the box of the segment from `from` to `to` and the box of
    the face, each grown by `margin`, do not overlap.
    */
    public boolean segmentSeparatedFrom(Vector3Dd from, Vector3Dd to,
        double margin)
    {
        return Math.max(from.x(), to.x()) + margin < minX - margin ||
               Math.min(from.x(), to.x()) - margin > maxX + margin ||
               Math.max(from.y(), to.y()) + margin < minY - margin ||
               Math.min(from.y(), to.y()) - margin > maxY + margin ||
               Math.max(from.z(), to.z()) + margin < minZ - margin ||
               Math.min(from.z(), to.z()) - margin > maxZ + margin;
    }

    /**
    Margin to grow this face's box by so that it holds every point that a
    tolerance-`tolerance` point-in-face test against it can accept, also in
    the coordinate dropped by the projection (where a non-planar face can
    reach `planeDeviation` beyond its plane; the factor 2 bounds the
    sqrt(3) of a dominant normal component of at least 1/sqrt(3)).
    */
    public double containmentMargin(double tolerance)
    {
        return pointTestMargin(tolerance) + 2.0 * (planeDeviation + tolerance);
    }
}

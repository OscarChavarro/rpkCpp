package vsdk.toolkit.skin;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

/**
A bounding box is represented as an array of 6 floating point numbers.
The meaning of the numbers is given by the constants MIN_X.
*/
public class RpkAxisAlignedBoundingBox {
    private final double[] coordinates;

    private static void setIfLess(double[] a, int idx, double b) {
        a[idx] = a[idx] < b ? a[idx] : b;
    }

    private static void setIfGreater(double[] a, int idx, double b) {
        a[idx] = a[idx] > b ? a[idx] : b;
    }

    public RpkAxisAlignedBoundingBox() {
        coordinates = new double[6];
        coordinates[BoundingBoxCoordinateIndex.MIN_X] = Double.MAX_VALUE;
        coordinates[BoundingBoxCoordinateIndex.MIN_Y] = Double.MAX_VALUE;
        coordinates[BoundingBoxCoordinateIndex.MIN_Z] = Double.MAX_VALUE;
        coordinates[BoundingBoxCoordinateIndex.MAX_X] = -Double.MAX_VALUE;
        coordinates[BoundingBoxCoordinateIndex.MAX_Y] = -Double.MAX_VALUE;
        coordinates[BoundingBoxCoordinateIndex.MAX_Z] = -Double.MAX_VALUE;
    }

    public double maxExtent() {
        double dx = coordinates[BoundingBoxCoordinateIndex.MAX_X] - coordinates[BoundingBoxCoordinateIndex.MIN_X];
        double dy = coordinates[BoundingBoxCoordinateIndex.MAX_Y] - coordinates[BoundingBoxCoordinateIndex.MIN_Y];
        double dz = coordinates[BoundingBoxCoordinateIndex.MAX_Z] - coordinates[BoundingBoxCoordinateIndex.MIN_Z];
        return dx > dy ? (dx > dz ? dx : dz) : (dy > dz ? dy : dz);
    }

    public boolean outOfBounds(Vector3Dd p) {
        return p.x() < coordinates[BoundingBoxCoordinateIndex.MIN_X] || p.x() > coordinates[BoundingBoxCoordinateIndex.MAX_X]
            || p.y() < coordinates[BoundingBoxCoordinateIndex.MIN_Y] || p.y() > coordinates[BoundingBoxCoordinateIndex.MAX_Y]
            || p.z() < coordinates[BoundingBoxCoordinateIndex.MIN_Z] || p.z() > coordinates[BoundingBoxCoordinateIndex.MAX_Z];
    }

    public Vector3Dd center() {
        return new Vector3Dd(
            0.5 * (coordinates[BoundingBoxCoordinateIndex.MIN_X] + coordinates[BoundingBoxCoordinateIndex.MAX_X]),
            0.5 * (coordinates[BoundingBoxCoordinateIndex.MIN_Y] + coordinates[BoundingBoxCoordinateIndex.MAX_Y]),
            0.5 * (coordinates[BoundingBoxCoordinateIndex.MIN_Z] + coordinates[BoundingBoxCoordinateIndex.MAX_Z]));
    }

    public void setAsUnion(RpkAxisAlignedBoundingBox a, RpkAxisAlignedBoundingBox b) {
        for (int i = BoundingBoxCoordinateIndex.MIN_X; i <= BoundingBoxCoordinateIndex.MIN_Z; i++) {
            coordinates[i] = a.coordinates[i] < b.coordinates[i] ? a.coordinates[i] : b.coordinates[i];
        }

        for (int i = BoundingBoxCoordinateIndex.MAX_X; i <= BoundingBoxCoordinateIndex.MAX_Z; i++) {
            coordinates[i] = a.coordinates[i] > b.coordinates[i] ? a.coordinates[i] : b.coordinates[i];
        }
    }

    /**
    True if the two given bounding boxes are disjoint.
    */
    public boolean disjointToOtherBoundingBox(RpkAxisAlignedBoundingBox other) {
        return (coordinates[BoundingBoxCoordinateIndex.MIN_X] > other.coordinates[BoundingBoxCoordinateIndex.MAX_X])
            || (other.coordinates[BoundingBoxCoordinateIndex.MIN_X] > coordinates[BoundingBoxCoordinateIndex.MAX_X])
            || (coordinates[BoundingBoxCoordinateIndex.MIN_Y] > other.coordinates[BoundingBoxCoordinateIndex.MAX_Y])
            || (other.coordinates[BoundingBoxCoordinateIndex.MIN_Y] > coordinates[BoundingBoxCoordinateIndex.MAX_Y])
            || (coordinates[BoundingBoxCoordinateIndex.MIN_Z] > other.coordinates[BoundingBoxCoordinateIndex.MAX_Z])
            || (other.coordinates[BoundingBoxCoordinateIndex.MIN_Z] > coordinates[BoundingBoxCoordinateIndex.MAX_Z]);
    }

    /**
    Returns true if the bounding box is behind the plane defined by norm and d.
    See F. Tampieri, Fast Vertex Radiosity Update, Graphics Gems II, p 303.
    */
    public boolean behindPlane(Vector3Dd normal, double distance) {
        Vector3Dd p = new Vector3Dd();

        p = p.withX(normal.x() > 0.0 ? coordinates[BoundingBoxCoordinateIndex.MAX_X] : coordinates[BoundingBoxCoordinateIndex.MIN_X]);
        p = p.withY(normal.y() > 0.0 ? coordinates[BoundingBoxCoordinateIndex.MAX_Y] : coordinates[BoundingBoxCoordinateIndex.MIN_Y]);
        p = p.withZ(normal.z() > 0.0 ? coordinates[BoundingBoxCoordinateIndex.MAX_Z] : coordinates[BoundingBoxCoordinateIndex.MIN_Z]);

        return normal.dotProduct(p) + distance <= 0.0;
    }

    public void copyFrom(RpkAxisAlignedBoundingBox other) {
        coordinates[BoundingBoxCoordinateIndex.MIN_X] = other.coordinates[BoundingBoxCoordinateIndex.MIN_X];
        coordinates[BoundingBoxCoordinateIndex.MIN_Y] = other.coordinates[BoundingBoxCoordinateIndex.MIN_Y];
        coordinates[BoundingBoxCoordinateIndex.MIN_Z] = other.coordinates[BoundingBoxCoordinateIndex.MIN_Z];
        coordinates[BoundingBoxCoordinateIndex.MAX_X] = other.coordinates[BoundingBoxCoordinateIndex.MAX_X];
        coordinates[BoundingBoxCoordinateIndex.MAX_Y] = other.coordinates[BoundingBoxCoordinateIndex.MAX_Y];
        coordinates[BoundingBoxCoordinateIndex.MAX_Z] = other.coordinates[BoundingBoxCoordinateIndex.MAX_Z];
    }

    /**
    Enlarge BoundingBox with other.
    */
    public void enlarge(RpkAxisAlignedBoundingBox other) {
        setIfLess(coordinates, BoundingBoxCoordinateIndex.MIN_X, other.coordinates[BoundingBoxCoordinateIndex.MIN_X]);
        setIfLess(coordinates, BoundingBoxCoordinateIndex.MIN_Y, other.coordinates[BoundingBoxCoordinateIndex.MIN_Y]);
        setIfLess(coordinates, BoundingBoxCoordinateIndex.MIN_Z, other.coordinates[BoundingBoxCoordinateIndex.MIN_Z]);
        setIfGreater(coordinates, BoundingBoxCoordinateIndex.MAX_X, other.coordinates[BoundingBoxCoordinateIndex.MAX_X]);
        setIfGreater(coordinates, BoundingBoxCoordinateIndex.MAX_Y, other.coordinates[BoundingBoxCoordinateIndex.MAX_Y]);
        setIfGreater(coordinates, BoundingBoxCoordinateIndex.MAX_Z, other.coordinates[BoundingBoxCoordinateIndex.MAX_Z]);
    }

    public void enlargeToIncludePoint(Vector3Dd point) {
        setIfLess(coordinates, BoundingBoxCoordinateIndex.MIN_X, point.x());
        setIfLess(coordinates, BoundingBoxCoordinateIndex.MIN_Y, point.y());
        setIfLess(coordinates, BoundingBoxCoordinateIndex.MIN_Z, point.z());
        setIfGreater(coordinates, BoundingBoxCoordinateIndex.MAX_X, point.x());
        setIfGreater(coordinates, BoundingBoxCoordinateIndex.MAX_Y, point.y());
        setIfGreater(coordinates, BoundingBoxCoordinateIndex.MAX_Z, point.z());
    }

    public void enlargeTinyBit() {
        double dx = (coordinates[BoundingBoxCoordinateIndex.MAX_X] - coordinates[BoundingBoxCoordinateIndex.MIN_X]) * 1e-4;
        double dy = (coordinates[BoundingBoxCoordinateIndex.MAX_Y] - coordinates[BoundingBoxCoordinateIndex.MIN_Y]) * 1e-4;
        double dz = (coordinates[BoundingBoxCoordinateIndex.MAX_Z] - coordinates[BoundingBoxCoordinateIndex.MIN_Z]) * 1e-4;

        if (dx < Numeric.EPSILON) {
            dx = Numeric.EPSILON;
        }
        if (dy < Numeric.EPSILON) {
            dy = Numeric.EPSILON;
        }
        if (dz < Numeric.EPSILON) {
            dz = Numeric.EPSILON;
        }

        coordinates[BoundingBoxCoordinateIndex.MIN_X] -= dx;
        coordinates[BoundingBoxCoordinateIndex.MAX_X] += dx;
        coordinates[BoundingBoxCoordinateIndex.MIN_Y] -= dy;
        coordinates[BoundingBoxCoordinateIndex.MAX_Y] += dy;
        coordinates[BoundingBoxCoordinateIndex.MIN_Z] -= dz;
        coordinates[BoundingBoxCoordinateIndex.MAX_Z] += dz;
    }

    public void computeContributionFlags(RpkAxisAlignedBoundingBox other, boolean[] hasMinMax1, boolean[] hasMinMax2) {
        for (int i = BoundingBoxCoordinateIndex.MIN_X; i <= BoundingBoxCoordinateIndex.MIN_Z; i++) {
            if (coordinates[i] < other.coordinates[i]) {
                hasMinMax1[i] = true;
            }
            else if (!Numeric.doubleEqual(coordinates[i], other.coordinates[i], VSDK.EPSILON)) {
                hasMinMax2[i] = true;
            }
        }

        for (int i = BoundingBoxCoordinateIndex.MAX_X; i <= BoundingBoxCoordinateIndex.MAX_Z; i++) {
            if (coordinates[i] > other.coordinates[i]) {
                hasMinMax1[i] = true;
            }
            else if (!Numeric.doubleEqual(coordinates[i], other.coordinates[i], VSDK.EPSILON)) {
                hasMinMax2[i] = true;
            }
        }
    }

    public double dx() {
        return coordinates[BoundingBoxCoordinateIndex.MAX_X] - coordinates[BoundingBoxCoordinateIndex.MIN_X];
    }

    public double dy() {
        return coordinates[BoundingBoxCoordinateIndex.MAX_Y] - coordinates[BoundingBoxCoordinateIndex.MIN_Y];
    }

    public double dz() {
        return coordinates[BoundingBoxCoordinateIndex.MAX_Z] - coordinates[BoundingBoxCoordinateIndex.MIN_Z];
    }

    public Vector3Dd voxelSize(short nx, short ny, short nz) {
        return new Vector3Dd(
            dx() / (double)nx,
            dy() / (double)ny,
            dz() / (double)nz);
    }

    public void enlargeByFactor(double factor) {
        double fdx = dx() * factor;
        double fdy = dy() * factor;
        double fdz = dz() * factor;

        if (fdx < Numeric.EPSILON) {
            fdx = Numeric.EPSILON;
        }
        if (fdy < Numeric.EPSILON) {
            fdy = Numeric.EPSILON;
        }
        if (fdz < Numeric.EPSILON) {
            fdz = Numeric.EPSILON;
        }

        coordinates[BoundingBoxCoordinateIndex.MIN_X] -= fdx;
        coordinates[BoundingBoxCoordinateIndex.MAX_X] += fdx;
        coordinates[BoundingBoxCoordinateIndex.MIN_Y] -= fdy;
        coordinates[BoundingBoxCoordinateIndex.MAX_Y] += fdy;
        coordinates[BoundingBoxCoordinateIndex.MIN_Z] -= fdz;
        coordinates[BoundingBoxCoordinateIndex.MAX_Z] += fdz;
    }

    public Vector3Dd minPoint() {
        return new Vector3Dd(
            coordinates[BoundingBoxCoordinateIndex.MIN_X],
            coordinates[BoundingBoxCoordinateIndex.MIN_Y],
            coordinates[BoundingBoxCoordinateIndex.MIN_Z]);
    }

    public Vector3Dd maxPoint() {
        return new Vector3Dd(
            coordinates[BoundingBoxCoordinateIndex.MAX_X],
            coordinates[BoundingBoxCoordinateIndex.MAX_Y],
            coordinates[BoundingBoxCoordinateIndex.MAX_Z]);
    }

    public double minX() {
        return coordinates[BoundingBoxCoordinateIndex.MIN_X];
    }

    public double minY() {
        return coordinates[BoundingBoxCoordinateIndex.MIN_Y];
    }

    public double minZ() {
        return coordinates[BoundingBoxCoordinateIndex.MIN_Z];
    }

    public double maxX() {
        return coordinates[BoundingBoxCoordinateIndex.MAX_X];
    }

    public double maxY() {
        return coordinates[BoundingBoxCoordinateIndex.MAX_Y];
    }

    public double maxZ() {
        return coordinates[BoundingBoxCoordinateIndex.MAX_Z];
    }

    public double[] rawCoordinates() {
        return coordinates;
    }

    public double valueAt(int idx) {
        switch (idx) {
            case BoundingBoxCoordinateIndex.MIN_X: return minX();
            case BoundingBoxCoordinateIndex.MIN_Y: return minY();
            case BoundingBoxCoordinateIndex.MIN_Z: return minZ();
            case BoundingBoxCoordinateIndex.MAX_X: return maxX();
            case BoundingBoxCoordinateIndex.MAX_Y: return maxY();
            case BoundingBoxCoordinateIndex.MAX_Z: return maxZ();
            default: return 0.0;
        }
    }

    public void corners(Vector3Dd[] out) {
        double minX = coordinates[BoundingBoxCoordinateIndex.MIN_X];
        double minY = coordinates[BoundingBoxCoordinateIndex.MIN_Y];
        double minZ = coordinates[BoundingBoxCoordinateIndex.MIN_Z];
        double maxX = coordinates[BoundingBoxCoordinateIndex.MAX_X];
        double maxY = coordinates[BoundingBoxCoordinateIndex.MAX_Y];
        double maxZ = coordinates[BoundingBoxCoordinateIndex.MAX_Z];

        out[0] = new Vector3Dd(minX, minY, minZ);
        out[1] = new Vector3Dd(maxX, minY, minZ);
        out[2] = new Vector3Dd(minX, maxY, minZ);
        out[3] = new Vector3Dd(maxX, maxY, minZ);
        out[4] = new Vector3Dd(minX, minY, maxZ);
        out[5] = new Vector3Dd(maxX, minY, maxZ);
        out[6] = new Vector3Dd(minX, maxY, maxZ);
        out[7] = new Vector3Dd(maxX, maxY, maxZ);
    }

    private static boolean clipAxisSlab(
        double minimumBound,
        double maximumBound,
        double origin,
        double direction,
        double toleranceScale,
        double[] nearDistance,
        double[] farDistance) {
        if (direction == 0.0) {
            return !(origin < minimumBound || origin > maximumBound);
        }

        double invDirection = 1.0 / direction;
        double entryDistance = (minimumBound - origin) * invDirection;
        double exitDistance = (maximumBound - origin) * invDirection;
        if (entryDistance > exitDistance) {
            double swapValue = entryDistance;
            entryDistance = exitDistance;
            exitDistance = swapValue;
        }

        if (exitDistance < nearDistance[0]) {
            return false;
        }
        if (entryDistance > nearDistance[0]) {
            nearDistance[0] = entryDistance;
        }
        if (exitDistance < farDistance[0]) {
            farDistance[0] = exitDistance;
        }
        return nearDistance[0] <= (farDistance[0] * toleranceScale);
    }

    /**
    Intersects the ray with this bounding box directly (no separate ray-intersection
    box object needed), narrowing [tMin, tMax] to the overlap with the ray segment.
    */
    // Scratch storage reused across bounding box tests on the hot ray
    // intersection paths (single threaded rendering core; the C++ port keeps
    // the equivalents on the stack)
    private static final double[] intersectingSegmentScratchNear = new double[1];
    private static final double[] intersectingSegmentScratchFar = new double[1];
    private static final double[] intersectScratchTMin = new double[1];
    private static final double[] intersectScratchTMax = new double[1];

    public boolean intersectingSegment(Ray ray, double[] tMin, double[] tMax) {
        if (ray == null || tMin == null || tMin.length == 0 || tMax == null || tMax.length == 0) {
            return false;
        }

        double minimumDistance = tMin[0];
        double maximumDistance = tMax[0];
        double[] nearDistance = intersectingSegmentScratchNear;
        double[] farDistance = intersectingSegmentScratchFar;
        nearDistance[0] = minimumDistance;
        farDistance[0] = maximumDistance;
        double toleranceScale = 1.0 + Numeric.EPSILON;

        if (!clipAxisSlab(
            coordinates[BoundingBoxCoordinateIndex.MIN_X], coordinates[BoundingBoxCoordinateIndex.MAX_X],
            ray.getOrigin().x(), ray.getDirection().x(),
            toleranceScale,
            nearDistance, farDistance)) {
            return false;
        }
        if (!clipAxisSlab(
            coordinates[BoundingBoxCoordinateIndex.MIN_Y], coordinates[BoundingBoxCoordinateIndex.MAX_Y],
            ray.getOrigin().y(), ray.getDirection().y(),
            toleranceScale,
            nearDistance, farDistance)) {
            return false;
        }
        if (!clipAxisSlab(
            coordinates[BoundingBoxCoordinateIndex.MIN_Z], coordinates[BoundingBoxCoordinateIndex.MAX_Z],
            ray.getOrigin().z(), ray.getDirection().z(),
            toleranceScale,
            nearDistance, farDistance)) {
            return false;
        }

        tMin[0] = nearDistance[0];
        tMax[0] = farDistance[0];

        if (nearDistance[0] == minimumDistance) {
            return farDistance[0] < maximumDistance;
        }
        return nearDistance[0] < maximumDistance;
    }

    public boolean intersect(Ray ray, double minimumDistance, double[] maximumDistance) {
        if (maximumDistance == null || maximumDistance.length == 0) {
            return false;
        }

        double[] tMin = intersectScratchTMin;
        double[] tMax = intersectScratchTMax;
        tMin[0] = minimumDistance;
        tMax[0] = maximumDistance[0];
        boolean hit = intersectingSegment(ray, tMin, tMax);
        if (hit) {
            if (tMin[0] == minimumDistance) {
                if (tMax[0] < maximumDistance[0]) {
                    maximumDistance[0] = tMax[0];
                }
            }
            else if (tMin[0] < maximumDistance[0]) {
                maximumDistance[0] = tMin[0];
            }
        }
        return hit;
    }
}

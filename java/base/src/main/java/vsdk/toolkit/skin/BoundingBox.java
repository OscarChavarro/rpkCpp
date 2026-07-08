package vsdk.toolkit.skin;

import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3D;

/**
A bounding box is represented as an array of 6 floating point numbers.
The meaning of the numbers is given by the constants MIN_X.
*/
public class BoundingBox {
    private final float[] coordinates;

    private static void setIfLess(float[] a, int idx, float b) {
        a[idx] = a[idx] < b ? a[idx] : b;
    }

    private static void setIfGreater(float[] a, int idx, float b) {
        a[idx] = a[idx] > b ? a[idx] : b;
    }

    public BoundingBox() {
        coordinates = new float[6];
        coordinates[BoundingBoxCoordinateIndex.MIN_X] = Numeric.HUGE_FLOAT_VALUE;
        coordinates[BoundingBoxCoordinateIndex.MIN_Y] = Numeric.HUGE_FLOAT_VALUE;
        coordinates[BoundingBoxCoordinateIndex.MIN_Z] = Numeric.HUGE_FLOAT_VALUE;
        coordinates[BoundingBoxCoordinateIndex.MAX_X] = -Numeric.HUGE_FLOAT_VALUE;
        coordinates[BoundingBoxCoordinateIndex.MAX_Y] = -Numeric.HUGE_FLOAT_VALUE;
        coordinates[BoundingBoxCoordinateIndex.MAX_Z] = -Numeric.HUGE_FLOAT_VALUE;
    }

    public float maxExtent() {
        float dx = coordinates[BoundingBoxCoordinateIndex.MAX_X] - coordinates[BoundingBoxCoordinateIndex.MIN_X];
        float dy = coordinates[BoundingBoxCoordinateIndex.MAX_Y] - coordinates[BoundingBoxCoordinateIndex.MIN_Y];
        float dz = coordinates[BoundingBoxCoordinateIndex.MAX_Z] - coordinates[BoundingBoxCoordinateIndex.MIN_Z];
        return dx > dy ? (dx > dz ? dx : dz) : (dy > dz ? dy : dz);
    }

    public boolean outOfBounds(Vector3D p) {
        return p.x < coordinates[BoundingBoxCoordinateIndex.MIN_X] || p.x > coordinates[BoundingBoxCoordinateIndex.MAX_X]
            || p.y < coordinates[BoundingBoxCoordinateIndex.MIN_Y] || p.y > coordinates[BoundingBoxCoordinateIndex.MAX_Y]
            || p.z < coordinates[BoundingBoxCoordinateIndex.MIN_Z] || p.z > coordinates[BoundingBoxCoordinateIndex.MAX_Z];
    }

    public Vector3D center() {
        return new Vector3D(
            0.5f * (coordinates[BoundingBoxCoordinateIndex.MIN_X] + coordinates[BoundingBoxCoordinateIndex.MAX_X]),
            0.5f * (coordinates[BoundingBoxCoordinateIndex.MIN_Y] + coordinates[BoundingBoxCoordinateIndex.MAX_Y]),
            0.5f * (coordinates[BoundingBoxCoordinateIndex.MIN_Z] + coordinates[BoundingBoxCoordinateIndex.MAX_Z]));
    }

    public void setAsUnion(BoundingBox a, BoundingBox b) {
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
    public boolean disjointToOtherBoundingBox(BoundingBox other) {
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
    public boolean behindPlane(Vector3D normal, float distance) {
        Vector3D p = new Vector3D();

        p.x = normal.x > 0.0f ? coordinates[BoundingBoxCoordinateIndex.MAX_X] : coordinates[BoundingBoxCoordinateIndex.MIN_X];
        p.y = normal.y > 0.0f ? coordinates[BoundingBoxCoordinateIndex.MAX_Y] : coordinates[BoundingBoxCoordinateIndex.MIN_Y];
        p.z = normal.z > 0.0f ? coordinates[BoundingBoxCoordinateIndex.MAX_Z] : coordinates[BoundingBoxCoordinateIndex.MIN_Z];

        return normal.dotProduct(p) + distance <= 0.0f;
    }

    public void copyFrom(BoundingBox other) {
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
    public void enlarge(BoundingBox other) {
        setIfLess(coordinates, BoundingBoxCoordinateIndex.MIN_X, other.coordinates[BoundingBoxCoordinateIndex.MIN_X]);
        setIfLess(coordinates, BoundingBoxCoordinateIndex.MIN_Y, other.coordinates[BoundingBoxCoordinateIndex.MIN_Y]);
        setIfLess(coordinates, BoundingBoxCoordinateIndex.MIN_Z, other.coordinates[BoundingBoxCoordinateIndex.MIN_Z]);
        setIfGreater(coordinates, BoundingBoxCoordinateIndex.MAX_X, other.coordinates[BoundingBoxCoordinateIndex.MAX_X]);
        setIfGreater(coordinates, BoundingBoxCoordinateIndex.MAX_Y, other.coordinates[BoundingBoxCoordinateIndex.MAX_Y]);
        setIfGreater(coordinates, BoundingBoxCoordinateIndex.MAX_Z, other.coordinates[BoundingBoxCoordinateIndex.MAX_Z]);
    }

    public void enlargeToIncludePoint(Vector3D point) {
        setIfLess(coordinates, BoundingBoxCoordinateIndex.MIN_X, point.x);
        setIfLess(coordinates, BoundingBoxCoordinateIndex.MIN_Y, point.y);
        setIfLess(coordinates, BoundingBoxCoordinateIndex.MIN_Z, point.z);
        setIfGreater(coordinates, BoundingBoxCoordinateIndex.MAX_X, point.x);
        setIfGreater(coordinates, BoundingBoxCoordinateIndex.MAX_Y, point.y);
        setIfGreater(coordinates, BoundingBoxCoordinateIndex.MAX_Z, point.z);
    }

    public void enlargeTinyBit() {
        float dx = (coordinates[BoundingBoxCoordinateIndex.MAX_X] - coordinates[BoundingBoxCoordinateIndex.MIN_X]) * 1e-4f;
        float dy = (coordinates[BoundingBoxCoordinateIndex.MAX_Y] - coordinates[BoundingBoxCoordinateIndex.MIN_Y]) * 1e-4f;
        float dz = (coordinates[BoundingBoxCoordinateIndex.MAX_Z] - coordinates[BoundingBoxCoordinateIndex.MIN_Z]) * 1e-4f;

        if (dx < Numeric.EPSILON_FLOAT) {
            dx = Numeric.EPSILON_FLOAT;
        }
        if (dy < Numeric.EPSILON_FLOAT) {
            dy = Numeric.EPSILON_FLOAT;
        }
        if (dz < Numeric.EPSILON_FLOAT) {
            dz = Numeric.EPSILON_FLOAT;
        }

        coordinates[BoundingBoxCoordinateIndex.MIN_X] -= dx;
        coordinates[BoundingBoxCoordinateIndex.MAX_X] += dx;
        coordinates[BoundingBoxCoordinateIndex.MIN_Y] -= dy;
        coordinates[BoundingBoxCoordinateIndex.MAX_Y] += dy;
        coordinates[BoundingBoxCoordinateIndex.MIN_Z] -= dz;
        coordinates[BoundingBoxCoordinateIndex.MAX_Z] += dz;
    }

    public void computeContributionFlags(BoundingBox other, boolean[] hasMinMax1, boolean[] hasMinMax2) {
        for (int i = BoundingBoxCoordinateIndex.MIN_X; i <= BoundingBoxCoordinateIndex.MIN_Z; i++) {
            if (coordinates[i] < other.coordinates[i]) {
                hasMinMax1[i] = true;
            }
            else if (!Numeric.doubleEqual(coordinates[i], other.coordinates[i], Numeric.EPSILON)) {
                hasMinMax2[i] = true;
            }
        }

        for (int i = BoundingBoxCoordinateIndex.MAX_X; i <= BoundingBoxCoordinateIndex.MAX_Z; i++) {
            if (coordinates[i] > other.coordinates[i]) {
                hasMinMax1[i] = true;
            }
            else if (!Numeric.doubleEqual(coordinates[i], other.coordinates[i], Numeric.EPSILON)) {
                hasMinMax2[i] = true;
            }
        }
    }

    public float dx() {
        return coordinates[BoundingBoxCoordinateIndex.MAX_X] - coordinates[BoundingBoxCoordinateIndex.MIN_X];
    }

    public float dy() {
        return coordinates[BoundingBoxCoordinateIndex.MAX_Y] - coordinates[BoundingBoxCoordinateIndex.MIN_Y];
    }

    public float dz() {
        return coordinates[BoundingBoxCoordinateIndex.MAX_Z] - coordinates[BoundingBoxCoordinateIndex.MIN_Z];
    }

    public Vector3D voxelSize(short nx, short ny, short nz) {
        return new Vector3D(
            dx() / (float)nx,
            dy() / (float)ny,
            dz() / (float)nz);
    }

    public void enlargeByFactor(float factor) {
        float fdx = dx() * factor;
        float fdy = dy() * factor;
        float fdz = dz() * factor;

        if (fdx < Numeric.EPSILON_FLOAT) {
            fdx = Numeric.EPSILON_FLOAT;
        }
        if (fdy < Numeric.EPSILON_FLOAT) {
            fdy = Numeric.EPSILON_FLOAT;
        }
        if (fdz < Numeric.EPSILON_FLOAT) {
            fdz = Numeric.EPSILON_FLOAT;
        }

        coordinates[BoundingBoxCoordinateIndex.MIN_X] -= fdx;
        coordinates[BoundingBoxCoordinateIndex.MAX_X] += fdx;
        coordinates[BoundingBoxCoordinateIndex.MIN_Y] -= fdy;
        coordinates[BoundingBoxCoordinateIndex.MAX_Y] += fdy;
        coordinates[BoundingBoxCoordinateIndex.MIN_Z] -= fdz;
        coordinates[BoundingBoxCoordinateIndex.MAX_Z] += fdz;
    }

    public Vector3D minPoint() {
        return new Vector3D(
            coordinates[BoundingBoxCoordinateIndex.MIN_X],
            coordinates[BoundingBoxCoordinateIndex.MIN_Y],
            coordinates[BoundingBoxCoordinateIndex.MIN_Z]);
    }

    public Vector3D maxPoint() {
        return new Vector3D(
            coordinates[BoundingBoxCoordinateIndex.MAX_X],
            coordinates[BoundingBoxCoordinateIndex.MAX_Y],
            coordinates[BoundingBoxCoordinateIndex.MAX_Z]);
    }

    public float minX() {
        return coordinates[BoundingBoxCoordinateIndex.MIN_X];
    }

    public float minY() {
        return coordinates[BoundingBoxCoordinateIndex.MIN_Y];
    }

    public float minZ() {
        return coordinates[BoundingBoxCoordinateIndex.MIN_Z];
    }

    public float maxX() {
        return coordinates[BoundingBoxCoordinateIndex.MAX_X];
    }

    public float maxY() {
        return coordinates[BoundingBoxCoordinateIndex.MAX_Y];
    }

    public float maxZ() {
        return coordinates[BoundingBoxCoordinateIndex.MAX_Z];
    }

    public float[] rawCoordinates() {
        return coordinates;
    }

    public float valueAt(int idx) {
        switch (idx) {
            case BoundingBoxCoordinateIndex.MIN_X: return minX();
            case BoundingBoxCoordinateIndex.MIN_Y: return minY();
            case BoundingBoxCoordinateIndex.MIN_Z: return minZ();
            case BoundingBoxCoordinateIndex.MAX_X: return maxX();
            case BoundingBoxCoordinateIndex.MAX_Y: return maxY();
            case BoundingBoxCoordinateIndex.MAX_Z: return maxZ();
            default: return 0.0f;
        }
    }

    public void corners(Vector3D[] out) {
        float minX = coordinates[BoundingBoxCoordinateIndex.MIN_X];
        float minY = coordinates[BoundingBoxCoordinateIndex.MIN_Y];
        float minZ = coordinates[BoundingBoxCoordinateIndex.MIN_Z];
        float maxX = coordinates[BoundingBoxCoordinateIndex.MAX_X];
        float maxY = coordinates[BoundingBoxCoordinateIndex.MAX_Y];
        float maxZ = coordinates[BoundingBoxCoordinateIndex.MAX_Z];

        out[0].set(minX, minY, minZ);
        out[1].set(maxX, minY, minZ);
        out[2].set(minX, maxY, minZ);
        out[3].set(maxX, maxY, minZ);
        out[4].set(minX, minY, maxZ);
        out[5].set(maxX, minY, maxZ);
        out[6].set(minX, maxY, maxZ);
        out[7].set(maxX, maxY, maxZ);
    }

    private static boolean clipAxisSlab(
        float minimumBound,
        float maximumBound,
        float origin,
        float direction,
        float toleranceScale,
        float[] nearDistance,
        float[] farDistance) {
        if (direction == 0.0f) {
            return !(origin < minimumBound || origin > maximumBound);
        }

        float invDirection = 1.0f / direction;
        float entryDistance = (minimumBound - origin) * invDirection;
        float exitDistance = (maximumBound - origin) * invDirection;
        if (entryDistance > exitDistance) {
            float swapValue = entryDistance;
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
    public boolean intersectingSegment(Ray ray, float[] tMin, float[] tMax) {
        if (ray == null || tMin == null || tMin.length == 0 || tMax == null || tMax.length == 0) {
            return false;
        }

        float minimumDistance = tMin[0];
        float maximumDistance = tMax[0];
        float[] nearDistance = new float[] {minimumDistance};
        float[] farDistance = new float[] {maximumDistance};
        float toleranceScale = 1.0f + Numeric.EPSILON_FLOAT;

        if (!clipAxisSlab(
            coordinates[BoundingBoxCoordinateIndex.MIN_X], coordinates[BoundingBoxCoordinateIndex.MAX_X],
            ray.position.x, ray.direction.x,
            toleranceScale,
            nearDistance, farDistance)) {
            return false;
        }
        if (!clipAxisSlab(
            coordinates[BoundingBoxCoordinateIndex.MIN_Y], coordinates[BoundingBoxCoordinateIndex.MAX_Y],
            ray.position.y, ray.direction.y,
            toleranceScale,
            nearDistance, farDistance)) {
            return false;
        }
        if (!clipAxisSlab(
            coordinates[BoundingBoxCoordinateIndex.MIN_Z], coordinates[BoundingBoxCoordinateIndex.MAX_Z],
            ray.position.z, ray.direction.z,
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

    public boolean intersect(Ray ray, float minimumDistance, float[] maximumDistance) {
        if (maximumDistance == null || maximumDistance.length == 0) {
            return false;
        }

        float[] tMin = new float[] {minimumDistance};
        float[] tMax = new float[] {maximumDistance[0]};
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

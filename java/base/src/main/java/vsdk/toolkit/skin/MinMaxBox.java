package vsdk.toolkit.skin;

import vsdk.toolkit.environment.geometry.elements.*;

import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;

public class MinMaxBox {
    private final BoundingBox boundingBox;

    public MinMaxBox(BoundingBox sourceBoundingBox) {
        boundingBox = new BoundingBox();
        if (sourceBoundingBox != null) {
            boundingBox.copyFrom(sourceBoundingBox);
        }
    }

    public void updateFromBoundingBox(BoundingBox sourceBoundingBox) {
        if (sourceBoundingBox == null) {
            return;
        }
        boundingBox.copyFrom(sourceBoundingBox);
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

    public boolean intersectingSegment(Ray ray, float[] tMin, float[] tMax) {
        if (ray == null || tMin == null || tMin.length == 0 || tMax == null || tMax.length == 0) {
            return false;
        }

        float minimumDistance = tMin[0];
        float maximumDistance = tMax[0];
        float[] box = boundingBox.rawCoordinates();
        float[] nearDistance = new float[] {minimumDistance};
        float[] farDistance = new float[] {maximumDistance};
        float toleranceScale = 1.0f + Numeric.EPSILON_FLOAT;

        if (!clipAxisSlab(
            box[BoundingBoxCoordinateIndex.MIN_X], box[BoundingBoxCoordinateIndex.MAX_X],
            ray.position.x, ray.direction.x,
            toleranceScale,
            nearDistance, farDistance)) {
            return false;
        }
        if (!clipAxisSlab(
            box[BoundingBoxCoordinateIndex.MIN_Y], box[BoundingBoxCoordinateIndex.MAX_Y],
            ray.position.y, ray.direction.y,
            toleranceScale,
            nearDistance, farDistance)) {
            return false;
        }
        if (!clipAxisSlab(
            box[BoundingBoxCoordinateIndex.MIN_Z], box[BoundingBoxCoordinateIndex.MAX_Z],
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

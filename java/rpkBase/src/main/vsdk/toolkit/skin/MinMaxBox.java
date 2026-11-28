package vsdk.toolkit.skin;

import vsdk.toolkit.environment.geometry.element.Ray;

public class MinMaxBox {
    private final RpkAxisAlignedBoundingBox boundingBox;

    public MinMaxBox(RpkAxisAlignedBoundingBox sourceBoundingBox) {
        boundingBox = new RpkAxisAlignedBoundingBox();
        if (sourceBoundingBox != null) {
            boundingBox.copyFrom(sourceBoundingBox);
        }
    }

    public void updateFromBoundingBox(RpkAxisAlignedBoundingBox sourceBoundingBox) {
        if (sourceBoundingBox == null) {
            return;
        }
        boundingBox.copyFrom(sourceBoundingBox);
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

    // Scratch storage reused across bounding box tests (single threaded
    // rendering core; the C++ port keeps the equivalents on the stack)
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
        double[] box = boundingBox.rawCoordinates();
        double[] nearDistance = intersectingSegmentScratchNear;
        double[] farDistance = intersectingSegmentScratchFar;
        nearDistance[0] = minimumDistance;
        farDistance[0] = maximumDistance;
        double toleranceScale = 1.0 + Numeric.EPSILON;

        if (!clipAxisSlab(
            box[BoundingBoxCoordinateIndex.MIN_X], box[BoundingBoxCoordinateIndex.MAX_X],
            ray.getOrigin().x(), ray.getDirection().x(),
            toleranceScale,
            nearDistance, farDistance)) {
            return false;
        }
        if (!clipAxisSlab(
            box[BoundingBoxCoordinateIndex.MIN_Y], box[BoundingBoxCoordinateIndex.MAX_Y],
            ray.getOrigin().y(), ray.getDirection().y(),
            toleranceScale,
            nearDistance, farDistance)) {
            return false;
        }
        if (!clipAxisSlab(
            box[BoundingBoxCoordinateIndex.MIN_Z], box[BoundingBoxCoordinateIndex.MAX_Z],
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

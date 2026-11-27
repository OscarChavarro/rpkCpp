package vsdk.toolkit.scene;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;

public class Camera {
    public static final int NUMBER_OF_VIEW_PLANES = 4;

    public Vector3Dd eyePosition; // Virtual camera position in 3D space
    public Vector3Dd lookPosition; // Focus point of camera
    public Vector3Dd upDirection; // Direction pointing upward
    public double viewDistance; // Distance from eye point to focus point
    public double fieldOfVision; // Field of view, horizontal and vertical, in degrees
    public double horizontalFov;
    public double verticalFov;
    public double near; // Far clipping plane distance
    public double far; // Far clipping plane distance
    public int xSize; // Horizontal resolution
    public int ySize; // Vertical resolution
    public Vector3Dd X; // Eye coordinate system: X = right
    public Vector3Dd Y; // Eye coordinate system: Y = down
    public Vector3Dd Z; // Eye coordinate system: Z = viewing direction
    public ColorRgb background; // Window background color
    public int changed; // True when camera position has been updated
    public double pixelWidth;
    public double pixelHeight;
    public double pixelWidthTangent;
    public double pixelHeightTangent;
    public Plane[] viewPlanes; // Clipping planes

    public Camera() {
        eyePosition = new Vector3Dd();
        lookPosition = new Vector3Dd();
        upDirection = new Vector3Dd();
        viewDistance = 0.0;
        fieldOfVision = 0.0;
        horizontalFov = 0.0;
        verticalFov = 0.0;
        near = 0.0;
        far = 0.0;
        xSize = 0;
        ySize = 0;
        X = new Vector3Dd();
        Y = new Vector3Dd();
        Z = new Vector3Dd();
        background = new ColorRgb();
        changed = 0;
        pixelWidth = 0.0;
        pixelHeight = 0.0;
        pixelWidthTangent = 0.0;
        pixelHeightTangent = 0.0;
        viewPlanes = new Plane[NUMBER_OF_VIEW_PLANES];
        for (int i = 0; i < NUMBER_OF_VIEW_PLANES; i++) {
            viewPlanes[i] = new Plane();
        }
    }

    void computeClippingPlanes() {
        double x = pixelWidthTangent * viewDistance; // Half the width of the virtual screen in 3D space
        double y = pixelHeightTangent * viewDistance; // Half the height of the virtual screen
        Vector3Dd[] vScreen = new Vector3Dd[4];
        for (int i = 0; i < 4; i++) {
            vScreen[i] = new Vector3Dd();
        }

        vScreen[0] = lookPosition.add(X.multiply(x)).add(Y.multiply(-y)); // Upper right corner: Y axis positions down!
        vScreen[1] = lookPosition.add(X.multiply(x)).add(Y.multiply(y)); // Lower right
        vScreen[2] = lookPosition.add(X.multiply(-x)).add(Y.multiply(y)); // Lower left
        vScreen[3] = lookPosition.add(X.multiply(-x)).add(Y.multiply(-y)); // Upper left

        for (int i = 0; i < 4; i++) {
            viewPlanes[i].normal = vScreen[i].subtract(eyePosition).crossProduct(vScreen[(i + 1) % 4].subtract(eyePosition));
            viewPlanes[i].normal = viewPlanes[i].normal.normalized(Numeric.EPSILON);
            viewPlanes[i].d = -viewPlanes[i].normal.dotProduct(eyePosition);
        }
    }

    /**
    Computes camera coordinate system and horizontal and vertical fov depending
    on filled in fov value and aspect ratio of the view window. Returns
    nullptr if this fails, and a pointer to the camera arg if success
    */
    Camera complete() {
        // Compute viewing direction ==> Z axis of eye coordinate system
        Z = lookPosition.subtract(eyePosition);

        // Distance from virtual camera position to focus point
        viewDistance = Z.length();
        if (viewDistance < VSDK.EPSILON) {
            Logger.reportMessage(this, Logger.ERROR, "SetCamera", "eye point and look-point coincide");
            return null;
        }
        Z = Z.multiplyByInverse(viewDistance, Numeric.EPSILON);

        // camera->X is a direction pointing to the right in the window
        X = Z.crossProduct(upDirection);
        final double n = X.length();
        if (n < VSDK.EPSILON) {
            Logger.reportMessage(this, Logger.ERROR, "SetCamera", "up-direction and viewing direction coincide");
            return null;
        }
        X = X.multiplyByInverse(n, Numeric.EPSILON);

        // camera->Y is a direction pointing down in the window
        Y = Z.crossProduct(X);
        Y = Y.normalized(Numeric.EPSILON);

        // Compute horizontal and vertical field of view angle from the specified one
        if (xSize < ySize) {
            horizontalFov = fieldOfVision;
            verticalFov = (double)Math.atan(
                Math.tan(fieldOfVision * Math.PI / 180.0) *
                    (double)ySize / (double)xSize) * 180.0 / (double)Math.PI;
        }
        else {
            verticalFov = fieldOfVision;
            horizontalFov = (double)Math.atan(
                Math.tan(fieldOfVision * Math.PI / 180.0) *
                    (double)xSize / (double)ySize) * 180.0 / (double)Math.PI;
        }

        // Default near and far clipping plane distance, will be set to a more reasonable
        // value when setting the camera for rendering
        near = Numeric.EPSILON;
        far = 2.0 * viewDistance;

        // Compute some extra frequently used quantities
        pixelWidthTangent = (double)Math.tan(horizontalFov * Math.PI / 180.0);
        pixelHeightTangent = (double)Math.tan(verticalFov * Math.PI / 180.0);

        pixelWidth = 2.0 * pixelWidthTangent / (double)xSize;
        pixelHeight = 2.0 * pixelHeightTangent / (double)ySize;

        computeClippingPlanes();

        return this;
    }

    /**
    Sets virtual camera position, focus point, up-direction, field of view
    (in degrees), horizontal and vertical window resolution and window
    inBackground. Returns (CAMERA *)nullptr if eye point and focus point coincide or
    viewing direction is equal to the up-direction
    */
    public void set(
        Vector3Dd inEyePosition,
        Vector3Dd inLoopPosition,
        Vector3Dd inUpDirection,
        double inFieldOfVision,
        int inXSize,
        int inYSize,
        ColorRgb inBackground) {
        eyePosition = new Vector3Dd(inEyePosition.x(), inEyePosition.y(), inEyePosition.z());
        lookPosition = new Vector3Dd(inLoopPosition.x(), inLoopPosition.y(), inLoopPosition.z());
        upDirection = new Vector3Dd(inUpDirection.x(), inUpDirection.y(), inUpDirection.z());
        fieldOfVision = inFieldOfVision;
        xSize = inXSize;
        ySize = inYSize;
        background = new ColorRgb(inBackground.getR(), inBackground.getG(), inBackground.getB());
        changed = 1;
        complete();
    }

    /**
    Sets virtual camera position, focus point, up-direction, field of view
    (in degrees), horizontal and vertical window resolution and window
    background. Returns (CAMERA *)nullptr if eye point and focus point coincide or
    viewing direction is equal to the up-direction
    */
    public void setEyePosition(double x, double y, double z) {
        Vector3Dd newEyePosition = new Vector3Dd();
        newEyePosition = new Vector3Dd(x, y, z);
        set(newEyePosition, lookPosition, upDirection, fieldOfVision, xSize, ySize, background);
    }

    public void setLookPosition(double x, double y, double z) {
        Vector3Dd newLookPosition = new Vector3Dd();
        newLookPosition = new Vector3Dd(x, y, z);
        set(eyePosition, newLookPosition, upDirection, fieldOfVision, xSize, ySize, background);
    }

    public void setUpDirection(double x, double y, double z) {
        Vector3Dd newUpDirection = new Vector3Dd();
        newUpDirection = new Vector3Dd(x, y, z);
        set(eyePosition, lookPosition, newUpDirection, fieldOfVision, xSize, ySize, background);
    }

    public void setFieldOfView(double fieldOfView) {
        set(eyePosition, lookPosition, upDirection, fieldOfView, xSize, ySize, background);
    }

    public static void transformBoundingBox(
        RpkAxisAlignedBoundingBox sourceBoundingBox,
        Matrix4x4d transform,
        RpkAxisAlignedBoundingBox transformedBoundingBox) {
        if (transformedBoundingBox == null) {
            return;
        }

        Vector3Dd[] corners = new Vector3Dd[8];
        for (int i = 0; i < 8; i++) {
            corners[i] = new Vector3Dd();
        }
        sourceBoundingBox.corners(corners);

        transformedBoundingBox.copyFrom(new RpkAxisAlignedBoundingBox());
        for (int i = 0; i < 8; i++) {
            // Rotation part only (the 3x3 block), as RenderPark always did
            corners[i] = transform.withoutTranslation().multiply(corners[i]);
            transformedBoundingBox.enlargeToIncludePoint(corners[i]);
        }

        final double xDelta = transformedBoundingBox.dx() * Numeric.EPSILON;
        final double yDelta = transformedBoundingBox.dy() * Numeric.EPSILON;
        final double zDelta = transformedBoundingBox.dz() * Numeric.EPSILON;
        Vector3Dd minPoint = transformedBoundingBox.minPoint();
        Vector3Dd maxPoint = transformedBoundingBox.maxPoint();
        minPoint = minPoint.withX(minPoint.x() - (xDelta));
        minPoint = minPoint.withY(minPoint.y() - (yDelta));
        minPoint = minPoint.withZ(minPoint.z() - (zDelta));
        maxPoint = maxPoint.withX(maxPoint.x() + (xDelta));
        maxPoint = maxPoint.withY(maxPoint.y() + (yDelta));
        maxPoint = maxPoint.withZ(maxPoint.z() + (zDelta));

        RpkAxisAlignedBoundingBox expandedBoundingBox = new RpkAxisAlignedBoundingBox();
        expandedBoundingBox.enlargeToIncludePoint(minPoint);
        expandedBoundingBox.enlargeToIncludePoint(maxPoint);
        transformedBoundingBox.copyFrom(expandedBoundingBox);
    }

    /**
    Transforms the eye point to the origin and rotates so that the centre lies
    on the negative Z axis and the up direction on the positive Y axis (Y up,
    X right, Z towards the viewer)
    */
    public static Matrix4x4d lookAtMatrix(Vector3Dd eye, Vector3Dd centre, Vector3Dd up) {
        final Vector3Dd Z = eye.subtract(centre).normalized(Numeric.EPSILON); // Z positions towards viewer
        final Vector3Dd X = up.crossProduct(Z).normalized(Numeric.EPSILON); // X positions right
        final Vector3Dd Y = Z.crossProduct(X); // Y positions up
        final double[][] orientation = {
            {X.x(), X.y(), X.z(), 0.0},
            {Y.x(), Y.y(), Y.z(), 0.0},
            {Z.x(), Z.y(), Z.z(), 0.0},
            {0.0, 0.0, 0.0, 1.0}
        };
        // Translate the eye to the origin, then orient
        return new Matrix4x4d(orientation).multiply(new Matrix4x4d().translation(eye.multiply(-1.0)));
    }

    /**
    Perspective projection like gluPerspective
    */
    public static Matrix4x4d perspectiveMatrix(double fieldOfViewInRadians, double aspect, double near, double far) {
        final double f = 1.0 / (double)Math.tan(fieldOfViewInRadians / 2.0);
        final double[][] projection = {
            {f / aspect, 0.0, 0.0, 0.0},
            {0.0, f, 0.0, 0.0},
            {0.0, 0.0, (near + far) / (near - far), (2 * far * near) / (near - far)},
            {0.0, 0.0, -1.0, 0.0}
        };
        return new Matrix4x4d(projection);
    }

    public static Matrix4x4d projectionMatrixFromBoundingBox(RpkAxisAlignedBoundingBox boundingBox) {
        return new Matrix4x4d().orthogonalProjection(
            boundingBox.minX(),
            boundingBox.maxX(),
            boundingBox.minY(),
            boundingBox.maxY(),
            -boundingBox.maxZ(),
            -boundingBox.minZ()
        );
    }

    /**
    Fits the near and far clipping distances to the given scene bounds,
    with 2% extra distance on each side
    */
    public void fitClippingPlanes(RpkAxisAlignedBoundingBox sceneBounds) {
        Vector3Dd minimum = new Vector3Dd();
        Vector3Dd maximum = new Vector3Dd();
        Vector3Dd d = new Vector3Dd();

        minimum = new Vector3Dd(sceneBounds.minX(), sceneBounds.minY(), sceneBounds.minZ());
        maximum = new Vector3Dd(sceneBounds.maxX(), sceneBounds.maxY(), sceneBounds.maxZ());

        far = -Double.MAX_VALUE;
        near = Double.MAX_VALUE;
        for ( int i = 0; i <= 1; i++ ) {
            for ( int j = 0; j <= 1; j++ ) {
                for ( int k = 0; k <= 1; k++ ) {
                    d = new Vector3Dd(i != 0 ? maximum.x() : minimum.x(), j != 0 ? maximum.y() : minimum.y(), k != 0 ? maximum.z() : minimum.z());

                    d = d.subtract(eyePosition);
                    double z = d.dotProduct(Z);

                    if ( z > far ) {
                        far = z;
                    }
                    if ( z < near ) {
                        near = z;
                    }
                }
            }
        }

        // Take 2% extra distance for near as well as far clipping plane
        far += 0.02 * (far);
        near -= 0.02 * (near);
        if ( far < VSDK.EPSILON ) {
            far = viewDistance;
        }
        if ( near < VSDK.EPSILON ) {
            near = viewDistance / 100.0;
        }
    }
}

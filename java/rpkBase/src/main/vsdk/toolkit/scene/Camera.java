package vsdk.toolkit.scene;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4f;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.skin.BoundingBox;

public class Camera {
    public static final int NUMBER_OF_VIEW_PLANES = 4;

    public Vector3Df eyePosition; // Virtual camera position in 3D space
    public Vector3Df lookPosition; // Focus point of camera
    public Vector3Df upDirection; // Direction pointing upward
    public float viewDistance; // Distance from eye point to focus point
    public float fieldOfVision; // Field of view, horizontal and vertical, in degrees
    public float horizontalFov;
    public float verticalFov;
    public float near; // Far clipping plane distance
    public float far; // Far clipping plane distance
    public int xSize; // Horizontal resolution
    public int ySize; // Vertical resolution
    public Vector3Df X; // Eye coordinate system: X = right
    public Vector3Df Y; // Eye coordinate system: Y = down
    public Vector3Df Z; // Eye coordinate system: Z = viewing direction
    public ColorRgbMutable background; // Window background color
    public int changed; // True when camera position has been updated
    public float pixelWidth;
    public float pixelHeight;
    public float pixelWidthTangent;
    public float pixelHeightTangent;
    public Plane[] viewPlanes; // Clipping planes

    public Camera() {
        eyePosition = new Vector3Df();
        lookPosition = new Vector3Df();
        upDirection = new Vector3Df();
        viewDistance = 0.0f;
        fieldOfVision = 0.0f;
        horizontalFov = 0.0f;
        verticalFov = 0.0f;
        near = 0.0f;
        far = 0.0f;
        xSize = 0;
        ySize = 0;
        X = new Vector3Df();
        Y = new Vector3Df();
        Z = new Vector3Df();
        background = new ColorRgbMutable();
        changed = 0;
        pixelWidth = 0.0f;
        pixelHeight = 0.0f;
        pixelWidthTangent = 0.0f;
        pixelHeightTangent = 0.0f;
        viewPlanes = new Plane[NUMBER_OF_VIEW_PLANES];
        for (int i = 0; i < NUMBER_OF_VIEW_PLANES; i++) {
            viewPlanes[i] = new Plane();
        }
    }

    void computeClippingPlanes() {
        float x = pixelWidthTangent * viewDistance; // Half the width of the virtual screen in 3D space
        float y = pixelHeightTangent * viewDistance; // Half the height of the virtual screen
        Vector3Df[] vScreen = new Vector3Df[4];
        for (int i = 0; i < 4; i++) {
            vScreen[i] = new Vector3Df();
        }

        vScreen[0] = lookPosition.add(X.multiply(x)).add(Y.multiply(-y)); // Upper right corner: Y axis positions down!
        vScreen[1] = lookPosition.add(X.multiply(x)).add(Y.multiply(y)); // Lower right
        vScreen[2] = lookPosition.add(X.multiply(-x)).add(Y.multiply(y)); // Lower left
        vScreen[3] = lookPosition.add(X.multiply(-x)).add(Y.multiply(-y)); // Upper left

        for (int i = 0; i < 4; i++) {
            viewPlanes[i].normal = vScreen[i].subtract(eyePosition).crossProduct(vScreen[(i + 1) % 4].subtract(eyePosition));
            viewPlanes[i].normal = viewPlanes[i].normal.normalized(Numeric.EPSILON_FLOAT);
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
        Z = Z.multiplyByInverse(viewDistance, Numeric.EPSILON_FLOAT);

        // camera->X is a direction pointing to the right in the window
        X = Z.crossProduct(upDirection);
        final float n = X.length();
        if (n < VSDK.EPSILON) {
            Logger.reportMessage(this, Logger.ERROR, "SetCamera", "up-direction and viewing direction coincide");
            return null;
        }
        X = X.multiplyByInverse(n, Numeric.EPSILON_FLOAT);

        // camera->Y is a direction pointing down in the window
        Y = Z.crossProduct(X);
        Y = Y.normalized(Numeric.EPSILON_FLOAT);

        // Compute horizontal and vertical field of view angle from the specified one
        if (xSize < ySize) {
            horizontalFov = fieldOfVision;
            verticalFov = (float)Math.atan(
                Math.tan(fieldOfVision * Math.PI / 180.0) *
                    (float)ySize / (float)xSize) * 180.0f / (float)Math.PI;
        }
        else {
            verticalFov = fieldOfVision;
            horizontalFov = (float)Math.atan(
                Math.tan(fieldOfVision * Math.PI / 180.0) *
                    (float)xSize / (float)ySize) * 180.0f / (float)Math.PI;
        }

        // Default near and far clipping plane distance, will be set to a more reasonable
        // value when setting the camera for rendering
        near = Numeric.EPSILON_FLOAT;
        far = 2.0f * viewDistance;

        // Compute some extra frequently used quantities
        pixelWidthTangent = (float)Math.tan(horizontalFov * Math.PI / 180.0);
        pixelHeightTangent = (float)Math.tan(verticalFov * Math.PI / 180.0);

        pixelWidth = 2.0f * pixelWidthTangent / (float)xSize;
        pixelHeight = 2.0f * pixelHeightTangent / (float)ySize;

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
        Vector3Df inEyePosition,
        Vector3Df inLoopPosition,
        Vector3Df inUpDirection,
        float inFieldOfVision,
        int inXSize,
        int inYSize,
        ColorRgbMutable inBackground) {
        eyePosition = new Vector3Df(inEyePosition.x(), inEyePosition.y(), inEyePosition.z());
        lookPosition = new Vector3Df(inLoopPosition.x(), inLoopPosition.y(), inLoopPosition.z());
        upDirection = new Vector3Df(inUpDirection.x(), inUpDirection.y(), inUpDirection.z());
        fieldOfVision = inFieldOfVision;
        xSize = inXSize;
        ySize = inYSize;
        background = new ColorRgbMutable(inBackground.getR(), inBackground.getG(), inBackground.getB());
        changed = 1;
        complete();
    }

    /**
    Sets virtual camera position, focus point, up-direction, field of view
    (in degrees), horizontal and vertical window resolution and window
    background. Returns (CAMERA *)nullptr if eye point and focus point coincide or
    viewing direction is equal to the up-direction
    */
    public void setEyePosition(float x, float y, float z) {
        Vector3Df newEyePosition = new Vector3Df();
        newEyePosition = new Vector3Df(x, y, z);
        set(newEyePosition, lookPosition, upDirection, fieldOfVision, xSize, ySize, background);
    }

    public void setLookPosition(float x, float y, float z) {
        Vector3Df newLookPosition = new Vector3Df();
        newLookPosition = new Vector3Df(x, y, z);
        set(eyePosition, newLookPosition, upDirection, fieldOfVision, xSize, ySize, background);
    }

    public void setUpDirection(float x, float y, float z) {
        Vector3Df newUpDirection = new Vector3Df();
        newUpDirection = new Vector3Df(x, y, z);
        set(eyePosition, lookPosition, newUpDirection, fieldOfVision, xSize, ySize, background);
    }

    public void setFieldOfView(float fieldOfView) {
        set(eyePosition, lookPosition, upDirection, fieldOfView, xSize, ySize, background);
    }

    public static void transformBoundingBox(
        BoundingBox sourceBoundingBox,
        Matrix4x4f transform,
        BoundingBox transformedBoundingBox) {
        if (transformedBoundingBox == null) {
            return;
        }

        Vector3Df[] corners = new Vector3Df[8];
        for (int i = 0; i < 8; i++) {
            corners[i] = new Vector3Df();
        }
        sourceBoundingBox.corners(corners);

        transformedBoundingBox.copyFrom(new BoundingBox());
        for (int i = 0; i < 8; i++) {
            // Rotation part only (the 3x3 block), as RenderPark always did
            corners[i] = transform.withoutTranslation().multiply(corners[i]);
            transformedBoundingBox.enlargeToIncludePoint(corners[i]);
        }

        final float xDelta = transformedBoundingBox.dx() * Numeric.EPSILON_FLOAT;
        final float yDelta = transformedBoundingBox.dy() * Numeric.EPSILON_FLOAT;
        final float zDelta = transformedBoundingBox.dz() * Numeric.EPSILON_FLOAT;
        Vector3Df minPoint = transformedBoundingBox.minPoint();
        Vector3Df maxPoint = transformedBoundingBox.maxPoint();
        minPoint = minPoint.withX(minPoint.x() - (xDelta));
        minPoint = minPoint.withY(minPoint.y() - (yDelta));
        minPoint = minPoint.withZ(minPoint.z() - (zDelta));
        maxPoint = maxPoint.withX(maxPoint.x() + (xDelta));
        maxPoint = maxPoint.withY(maxPoint.y() + (yDelta));
        maxPoint = maxPoint.withZ(maxPoint.z() + (zDelta));

        BoundingBox expandedBoundingBox = new BoundingBox();
        expandedBoundingBox.enlargeToIncludePoint(minPoint);
        expandedBoundingBox.enlargeToIncludePoint(maxPoint);
        transformedBoundingBox.copyFrom(expandedBoundingBox);
    }

    /**
    Transforms the eye point to the origin and rotates so that the centre lies
    on the negative Z axis and the up direction on the positive Y axis (Y up,
    X right, Z towards the viewer)
    */
    public static Matrix4x4f lookAtMatrix(Vector3Df eye, Vector3Df centre, Vector3Df up) {
        final Vector3Df Z = eye.subtract(centre).normalized(Numeric.EPSILON_FLOAT); // Z positions towards viewer
        final Vector3Df X = up.crossProduct(Z).normalized(Numeric.EPSILON_FLOAT); // X positions right
        final Vector3Df Y = Z.crossProduct(X); // Y positions up
        final float[][] orientation = {
            {X.x(), X.y(), X.z(), 0.0f},
            {Y.x(), Y.y(), Y.z(), 0.0f},
            {Z.x(), Z.y(), Z.z(), 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        };
        // Translate the eye to the origin, then orient
        return new Matrix4x4f(orientation).multiply(new Matrix4x4f().translation(eye.multiply(-1.0f)));
    }

    /**
    Perspective projection like gluPerspective
    */
    public static Matrix4x4f perspectiveMatrix(float fieldOfViewInRadians, float aspect, float near, float far) {
        final float f = 1.0f / (float)Math.tan(fieldOfViewInRadians / 2.0f);
        final float[][] projection = {
            {f / aspect, 0.0f, 0.0f, 0.0f},
            {0.0f, f, 0.0f, 0.0f},
            {0.0f, 0.0f, (near + far) / (near - far), (2 * far * near) / (near - far)},
            {0.0f, 0.0f, -1.0f, 0.0f}
        };
        return new Matrix4x4f(projection);
    }

    public static Matrix4x4f projectionMatrixFromBoundingBox(BoundingBox boundingBox) {
        return new Matrix4x4f().orthogonalProjection(
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
    public void fitClippingPlanes(BoundingBox sceneBounds) {
        Vector3Df minimum = new Vector3Df();
        Vector3Df maximum = new Vector3Df();
        Vector3Df d = new Vector3Df();

        minimum = new Vector3Df(sceneBounds.minX(), sceneBounds.minY(), sceneBounds.minZ());
        maximum = new Vector3Df(sceneBounds.maxX(), sceneBounds.maxY(), sceneBounds.maxZ());

        far = -Float.MAX_VALUE;
        near = Float.MAX_VALUE;
        for ( int i = 0; i <= 1; i++ ) {
            for ( int j = 0; j <= 1; j++ ) {
                for ( int k = 0; k <= 1; k++ ) {
                    d = new Vector3Df(i != 0 ? maximum.x() : minimum.x(), j != 0 ? maximum.y() : minimum.y(), k != 0 ? maximum.z() : minimum.z());

                    d = d.subtract(eyePosition);
                    float z = d.dotProduct(Z);

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
        far += 0.02F * (far);
        near -= 0.02F * (near);
        if ( far < VSDK.EPSILON ) {
            far = viewDistance;
        }
        if ( near < VSDK.EPSILON ) {
            near = viewDistance / 100.0F;
        }
    }
}

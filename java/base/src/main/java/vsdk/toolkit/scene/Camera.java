package vsdk.toolkit.scene;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.skin.BoundingBox;

public class Camera {
    public static final int NUMBER_OF_VIEW_PLANES = 4;

    public Vector3D eyePosition; // Virtual camera position in 3D space
    public Vector3D lookPosition; // Focus point of camera
    public Vector3D upDirection; // Direction pointing upward
    public float viewDistance; // Distance from eye point to focus point
    public float fieldOfVision; // Field of view, horizontal and vertical, in degrees
    public float horizontalFov;
    public float verticalFov;
    public float near; // Far clipping plane distance
    public float far; // Far clipping plane distance
    public int xSize; // Horizontal resolution
    public int ySize; // Vertical resolution
    public Vector3D X; // Eye coordinate system: X = right
    public Vector3D Y; // Eye coordinate system: Y = down
    public Vector3D Z; // Eye coordinate system: Z = viewing direction
    public ColorRgb background; // Window background color
    public int changed; // True when camera position has been updated
    public float pixelWidth;
    public float pixelHeight;
    public float pixelWidthTangent;
    public float pixelHeightTangent;
    public Plane[] viewPlanes; // Clipping planes

    public Camera() {
        eyePosition = new Vector3D();
        lookPosition = new Vector3D();
        upDirection = new Vector3D();
        viewDistance = 0.0f;
        fieldOfVision = 0.0f;
        horizontalFov = 0.0f;
        verticalFov = 0.0f;
        near = 0.0f;
        far = 0.0f;
        xSize = 0;
        ySize = 0;
        X = new Vector3D();
        Y = new Vector3D();
        Z = new Vector3D();
        background = new ColorRgb();
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
        Vector3D[] vScreen = new Vector3D[4];
        for (int i = 0; i < 4; i++) {
            vScreen[i] = new Vector3D();
        }

        vScreen[0].combine3(lookPosition, x, X, -y, Y); // Upper right corner: Y axis positions down!
        vScreen[1].combine3(lookPosition, x, X, y, Y); // Lower right
        vScreen[2].combine3(lookPosition, -x, X, y, Y); // Lower left
        vScreen[3].combine3(lookPosition, -x, X, -y, Y); // Upper left

        for (int i = 0; i < 4; i++) {
            viewPlanes[i].normal.tripleCrossProduct(vScreen[(i + 1) % 4], eyePosition, vScreen[i]);
            viewPlanes[i].normal.normalize(Numeric.EPSILON_FLOAT);
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
        Z.subtraction(lookPosition, eyePosition);

        // Distance from virtual camera position to focus point
        viewDistance = Z.norm();
        if (viewDistance < Numeric.EPSILON) {
            Logger.error("SetCamera", "eye point and look-point coincide");
            return null;
        }
        Z.inverseScaledCopy(viewDistance, Z, Numeric.EPSILON_FLOAT);

        // camera->X is a direction pointing to the right in the window
        X.crossProduct(Z, upDirection);
        final float n = X.norm();
        if (n < Numeric.EPSILON) {
            Logger.error("SetCamera", "up-direction and viewing direction coincide");
            return null;
        }
        X.inverseScaledCopy(n, X, Numeric.EPSILON_FLOAT);

        // camera->Y is a direction pointing down in the window
        Y.crossProduct(Z, X);
        Y.normalize(Numeric.EPSILON_FLOAT);

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
        Vector3D inEyePosition,
        Vector3D inLoopPosition,
        Vector3D inUpDirection,
        float inFieldOfVision,
        int inXSize,
        int inYSize,
        ColorRgb inBackground) {
        eyePosition = new Vector3D(inEyePosition.x, inEyePosition.y, inEyePosition.z);
        lookPosition = new Vector3D(inLoopPosition.x, inLoopPosition.y, inLoopPosition.z);
        upDirection = new Vector3D(inUpDirection.x, inUpDirection.y, inUpDirection.z);
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
    public void setEyePosition(float x, float y, float z) {
        Vector3D newEyePosition = new Vector3D();
        newEyePosition.set(x, y, z);
        set(newEyePosition, lookPosition, upDirection, fieldOfVision, xSize, ySize, background);
    }

    public void setLookPosition(float x, float y, float z) {
        Vector3D newLookPosition = new Vector3D();
        newLookPosition.set(x, y, z);
        set(eyePosition, newLookPosition, upDirection, fieldOfVision, xSize, ySize, background);
    }

    public void setUpDirection(float x, float y, float z) {
        Vector3D newUpDirection = new Vector3D();
        newUpDirection.set(x, y, z);
        set(eyePosition, lookPosition, newUpDirection, fieldOfVision, xSize, ySize, background);
    }

    public void setFieldOfView(float fieldOfView) {
        set(eyePosition, lookPosition, upDirection, fieldOfView, xSize, ySize, background);
    }

    public static void transformBoundingBox(
        BoundingBox sourceBoundingBox,
        Matrix4x4 transform,
        BoundingBox transformedBoundingBox) {
        if (transformedBoundingBox == null) {
            return;
        }

        Vector3D[] corners = new Vector3D[8];
        for (int i = 0; i < 8; i++) {
            corners[i] = new Vector3D();
        }
        sourceBoundingBox.corners(corners);

        transformedBoundingBox.copyFrom(new BoundingBox());
        for (int i = 0; i < 8; i++) {
            transform.transformPoint3D(corners[i], corners[i]);
            transformedBoundingBox.enlargeToIncludePoint(corners[i]);
        }

        final float xDelta = transformedBoundingBox.dx() * Numeric.EPSILON_FLOAT;
        final float yDelta = transformedBoundingBox.dy() * Numeric.EPSILON_FLOAT;
        final float zDelta = transformedBoundingBox.dz() * Numeric.EPSILON_FLOAT;
        Vector3D minPoint = transformedBoundingBox.minPoint();
        Vector3D maxPoint = transformedBoundingBox.maxPoint();
        minPoint.x -= xDelta;
        minPoint.y -= yDelta;
        minPoint.z -= zDelta;
        maxPoint.x += xDelta;
        maxPoint.y += yDelta;
        maxPoint.z += zDelta;

        BoundingBox expandedBoundingBox = new BoundingBox();
        expandedBoundingBox.enlargeToIncludePoint(minPoint);
        expandedBoundingBox.enlargeToIncludePoint(maxPoint);
        transformedBoundingBox.copyFrom(expandedBoundingBox);
    }

    public static Matrix4x4 projectionMatrixFromBoundingBox(BoundingBox boundingBox) {
        return Matrix4x4.createOrthogonalViewMatrix(
            boundingBox.minX(),
            boundingBox.maxX(),
            boundingBox.minY(),
            boundingBox.maxY(),
            -boundingBox.maxZ(),
            -boundingBox.minZ()
        );
    }
}

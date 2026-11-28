/**
Saves the result of a radiosity computation as a VRML file
*/

package vsdk.toolkit.io.wrl;

import java.io.OutputStream;
import java.util.Locale;
import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.skin.Numeric;
import vsdk.toolkit.material.RendererConfiguration;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.io.PersistenceElement;
import vsdk.toolkit.scene.Camera;

public class VrmlWriter {
    private static final int MAXIMUM_CAMERA_STACK = 20;
    private static final String RPK_HOME = "http://www.cs.kuleuven.ac.be/cwis/research/graphics/RENDERPARK/";

    private static final Camera[] cameraStack = new Camera[MAXIMUM_CAMERA_STACK];

    static {
        for (int i = 0; i < cameraStack.length; i++) {
            cameraStack[i] = new Camera();
        }
    }

    private static void writeFormatted(OutputStream outputStream, String format, Object... arguments) {
        if (outputStream == null || format == null) {
            return;
        }

        String text;
        try {
            text = String.format(Locale.US, format, arguments);
        }
        catch (Exception ignored) {
            text = "";
        }

        if (text.isEmpty()) {
            return;
        }

        byte[] bytes = text.getBytes(java.nio.charset.StandardCharsets.UTF_8);
        PersistenceElement.writeBytes(outputStream, bytes, bytes.length);
    }

    /**
Returns pointer to the next saved camera. If previous==nullptr, the first saved
camera is returned. In subsequent calls, the previous camera returned
by this function should be passed as the parameter. If all saved cameras
have been iterated over, nullptr is returned
*/
    private static Camera nextSavedCamera(Camera previous) {
        if (previous == null) {
            return null;
        }

        int index = -1;
        for (int i = 0; i < cameraStack.length; i++) {
            if (cameraStack[i] == previous) {
                index = i;
                break;
            }
        }

        if (index <= 0) {
            return null;
        }
        return cameraStack[index - 1];
    }

    /**
Compute a rotation that will rotate the current "up"-direction to the Y axis.
Y-axis positions up in VRML2.0
*/
    private static Matrix4x4d transformModel(Camera camera, Vector3Dd[] modelRotationAxis, double[] modelRotationAngle) {
        Vector3Dd upAxis = new Vector3Dd(0.0, 1.0, 0.0);
        double cosA = camera.upDirection.dotProduct(upAxis);
        if (cosA < 1.0 - VSDK.EPSILON) {
            modelRotationAngle[0] = (double)Math.acos(cosA);
            modelRotationAxis[0] = camera.upDirection.crossProduct(upAxis);
            modelRotationAxis[0] = modelRotationAxis[0].normalized(Numeric.EPSILON);
            return new Matrix4x4d().axisRotation(modelRotationAngle[0], modelRotationAxis[0]);
        }

        modelRotationAxis[0] = new Vector3Dd(0.0, 1.0, 0.0);
        modelRotationAngle[0] = 0.0;
        Matrix4x4d identity = new Matrix4x4d();
        return identity;
    }

    /**
    Recovers the rotation axis and angle from the given rotation matrix.
    There is no check whether the transform really is a rotation.
    angle and axis are one-element output holders.
    */
    private static void recoverRotationParameters(Matrix4x4d rotation, double[] angle, Vector3Dd[] axis) {
        final double c = (rotation.get(0, 0) + rotation.get(1, 1) + rotation.get(2, 2) - 1.0) * 0.5;
        if ( c > 1.0 - VSDK.EPSILON ) {
            angle[0] = 0.0;
            axis[0] = new Vector3Dd(0.0, 0.0, 1.0);
        }
        else if ( c < -1.0 + VSDK.EPSILON ) {
            angle[0] = (double)Math.PI;
            double x = (double)Math.sqrt((rotation.get(0, 0) + 1.0) * 0.5);
            double y = (double)Math.sqrt((rotation.get(1, 1) + 1.0) * 0.5);
            double z = (double)Math.sqrt((rotation.get(2, 2) + 1.0) * 0.5);

            // Assume x positive, determine sign of y and z
            if ( rotation.get(1, 0) < 0.0 ) {
                y = -y;
            }
            if ( rotation.get(2, 0) < 0.0 ) {
                z = -z;
            }
            axis[0] = new Vector3Dd(x, y, z);
        }
        else {
            angle[0] = (double)Math.acos(c);
            final double s = (double)Math.sqrt(1.0 - c * c);
            final double r = 1.0 / (2.0 * s);
            axis[0] = new Vector3Dd(
                (rotation.get(2, 1) - rotation.get(1, 2)) * r,
                (rotation.get(0, 2) - rotation.get(2, 0)) * r,
                (rotation.get(1, 0) - rotation.get(0, 1)) * r);
        }
    }

    /**
Write VRML ViewPoint node for the given camera position
*/
    private static void writeViewPoint(
        OutputStream outputStream,
        Matrix4x4d modelTransform,
        Camera camera,
        String viewPointName) {
        Vector3Dd X;
        Vector3Dd Y;
        Vector3Dd Z;
        Vector3Dd[] viewRotationAxis = {new Vector3Dd()};
        Vector3Dd eyePosition;
        double[] viewRotationAngle = new double[] {0.0};

        X = camera.X.multiply(1.0); // camera->X positions right in window
        Y = camera.Y.multiply(-1.0); // camera->Y positions down in window, VRML wants y up
        Z = camera.Z.multiply(-1.0); // camera->Z positions away, VRML wants Z to point towards viewer

        // Apply model transform (its rotation part)
        final Matrix4x4d modelRotation = modelTransform.withoutTranslation();
        X = modelRotation.multiply(X);
        Y = modelRotation.multiply(Y);
        Z = modelRotation.multiply(Z);

        // Construct view orientation transform and recover axis and angle
        final double[][] orientation = {
            {X.x(), Y.x(), Z.x(), 0.0},
            {X.y(), Y.y(), Z.y(), 0.0},
            {X.z(), Y.z(), Z.z(), 0.0},
            {0.0, 0.0, 0.0, 1.0}
        };
        recoverRotationParameters(new Matrix4x4d(orientation), viewRotationAngle, viewRotationAxis);

        // Apply model transform to eye point
        eyePosition = modelRotation.multiply(camera.eyePosition);

        writeFormatted(
            outputStream,
            "Viewpoint {\n  position %g %g %g\n  orientation %g %g %g %g\n  fieldOfView %g\n  description \"%s\"\n}\n\n",
            eyePosition.x(),
            eyePosition.y(),
            eyePosition.z(),
            viewRotationAxis[0].x(),
            viewRotationAxis[0].y(),
            viewRotationAxis[0].z(),
            viewRotationAngle[0],
            2.0 * camera.fieldOfVision * Math.PI / 180.0,
            viewPointName);
    }

    private static void writeViewPoints(
        Camera camera,
        OutputStream outputStream,
        Matrix4x4d modelTransform) {
        Camera localCamera = null;
        int count = 1;
        writeViewPoint(outputStream, modelTransform, camera, "ViewPoint 1");
        while ( (localCamera = nextSavedCamera(localCamera)) != null ) {
            count++;
            StringBuilder viewPointNameBuilder = new StringBuilder();
            viewPointNameBuilder.append("ViewPoint ").append(count);
            writeViewPoint(outputStream, modelTransform, localCamera, viewPointNameBuilder.toString());
        }
    }

    /**
Can also be used by radiance-method specific VRML savers.
*/
    public static void writeHeader(
        Camera camera,
        OutputStream outputStream,
        RendererConfiguration renderOptions) {
        Vector3Dd[] modelRotationAxis = {new Vector3Dd()};
        double[] modelRotationAngle = new double[] {0.0};

        writeFormatted(outputStream, "#VRML V2.0 utf8\n\n");

        writeFormatted(
            outputStream,
            "WorldInfo {\n  title \"%s\"\n  info [ \"Created using RenderPark (%s)\" ]\n}\n\n",
            "Some nice model",
            RPK_HOME);

        writeFormatted(outputStream, "NavigationInfo {\n type \"WALK\"\n headlight FALSE\n}\n\n");

        Matrix4x4d modelTransform = transformModel(camera, modelRotationAxis, modelRotationAngle);
        writeViewPoints(camera, outputStream, modelTransform);

        writeFormatted(
            outputStream,
            "Transform {\n  rotation %g %g %g %g\n  children [\n    Shape {\n      geometry IndexedFaceSet {\n",
            modelRotationAxis[0].x(),
            modelRotationAxis[0].y(),
            modelRotationAxis[0].z(),
            modelRotationAngle[0]);

        writeFormatted(outputStream, "\tsolid %s\n", renderOptions.backfaceCulling ? "TRUE" : "FALSE");
    }

    public static void writeTrailer(OutputStream outputStream) {
        writeFormatted(outputStream, "      }\n    }\n  ]\n}\n\n");
    }
}

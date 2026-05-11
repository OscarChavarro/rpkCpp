/**
Saves the result of a radiosity computation as a VRML file
*/

package vsdk.toolkit.io.wrl;

import java.io.OutputStream;
import java.util.Locale;
import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
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
    private static Matrix4x4 transformModel(Camera camera, Vector3D modelRotationAxis, float[] modelRotationAngle) {
        Vector3D upAxis = new Vector3D();

        upAxis.set(0.0f, 1.0f, 0.0f);
        double cosA = camera.upDirection.dotProduct(upAxis);
        if (cosA < 1.0 - Numeric.EPSILON) {
            modelRotationAngle[0] = (float)Math.acos(cosA);
            modelRotationAxis.crossProduct(camera.upDirection, upAxis);
            modelRotationAxis.normalize(Numeric.EPSILON_FLOAT);
            return Matrix4x4.createRotationMatrix(modelRotationAngle[0], modelRotationAxis);
        }

        modelRotationAxis.set(0.0f, 1.0f, 0.0f);
        modelRotationAngle[0] = 0.0f;
        Matrix4x4 identity = new Matrix4x4();
        return identity;
    }

    /**
Write VRML ViewPoint node for the given camera position
*/
    private static void writeViewPoint(
        OutputStream outputStream,
        Matrix4x4 modelTransform,
        Camera camera,
        String viewPointName) {
        Vector3D X = new Vector3D();
        Vector3D Y = new Vector3D();
        Vector3D Z = new Vector3D();
        Vector3D viewRotationAxis = new Vector3D();
        Vector3D eyePosition = new Vector3D();
        float[] viewRotationAngle = new float[] {0.0f};

        X.scaledCopy(1.0f, camera.X); // camera->X positions right in window
        Y.scaledCopy(-1.0f, camera.Y); // camera->Y positions down in window, VRML wants y up
        Z.scaledCopy(-1.0f, camera.Z); // camera->Z positions away, VRML wants Z to point towards viewer

        // Apply model transform
        modelTransform.transformPoint3D(X, X);
        modelTransform.transformPoint3D(Y, Y);
        modelTransform.transformPoint3D(Z, Z);

        // Construct view orientation transform and recover axis and angle
        Matrix4x4 identity = new Matrix4x4();
        Matrix4x4 viewTransform = identity;
        viewTransform.set3X3Matrix(
            X.x, Y.x, Z.x,
            X.y, Y.y, Z.y,
            X.z, Y.z, Z.z);
        viewTransform.recoverRotationParameters(viewRotationAngle, viewRotationAxis);

        // Apply model transform to eye point
        modelTransform.transformPoint3D(camera.eyePosition, eyePosition);

        writeFormatted(
            outputStream,
            "Viewpoint {\n  position %g %g %g\n  orientation %g %g %g %g\n  fieldOfView %g\n  description \"%s\"\n}\n\n",
            eyePosition.x,
            eyePosition.y,
            eyePosition.z,
            viewRotationAxis.x,
            viewRotationAxis.y,
            viewRotationAxis.z,
            viewRotationAngle[0],
            2.0 * camera.fieldOfVision * Math.PI / 180.0,
            viewPointName);
    }

    private static void writeViewPoints(
        Camera camera,
        OutputStream outputStream,
        Matrix4x4 modelTransform) {
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
        RenderOptions renderOptions) {
        Vector3D modelRotationAxis = new Vector3D();
        float[] modelRotationAngle = new float[] {0.0f};

        writeFormatted(outputStream, "#VRML V2.0 utf8\n\n");

        writeFormatted(
            outputStream,
            "WorldInfo {\n  title \"%s\"\n  info [ \"Created using RenderPark (%s)\" ]\n}\n\n",
            "Some nice model",
            RPK_HOME);

        writeFormatted(outputStream, "NavigationInfo {\n type \"WALK\"\n headlight FALSE\n}\n\n");

        Matrix4x4 modelTransform = transformModel(camera, modelRotationAxis, modelRotationAngle);
        writeViewPoints(camera, outputStream, modelTransform);

        writeFormatted(
            outputStream,
            "Transform {\n  rotation %g %g %g %g\n  children [\n    Shape {\n      geometry IndexedFaceSet {\n",
            modelRotationAxis.x,
            modelRotationAxis.y,
            modelRotationAxis.z,
            modelRotationAngle[0]);

        writeFormatted(outputStream, "\tsolid %s\n", renderOptions.backfaceCulling ? "TRUE" : "FALSE");
    }

    public static void writeTrailer(OutputStream outputStream) {
        writeFormatted(outputStream, "      }\n    }\n  ]\n}\n\n");
    }
}

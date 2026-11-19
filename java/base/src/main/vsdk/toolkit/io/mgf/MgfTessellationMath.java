package vsdk.toolkit.io.mgf;

import java.util.Locale;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

public class MgfTessellationMath {
    public static final int MGF_PV_SIZE = 24;

    public static void formatFloat(StringBuilder target, int targetLength, double value) {
        target.setLength(0);
        target.append(String.format(Locale.US, "%.12g", value));
        if (target.length() > targetLength - 1) {
            target.setLength(targetLength - 1);
        }
    }

    /**
    Compute u and v given w (normalized). u and v are out-parameters: their
    first element receives the computed axis.
    */
    public static void mgfMakeAxes(Vector3Dd[] u, Vector3Dd[] v, Vector3Dd w, double epsilon) {
        double[] vArr = new double[] {0.0, 0.0, 0.0};
        double[] wArr = new double[] {w.x(), w.y(), w.z()};

        int i;
        for (i = 0; i < 3; i++) {
            if (wArr[i] > -0.6 && wArr[i] < 0.6) {
                break;
            }
        }

        if (i < 3) {
            vArr[i] = 1.0;
        }

        v[0] = new Vector3Dd(vArr[0], vArr[1], vArr[2]);

        u[0] = v[0].crossProduct(w);
        normalizeAndGivePreviousNorm(u, epsilon);
        v[0] = w.crossProduct(u[0]);
    }

    /**
    Normalize a vector, return old magnitude. The vector is an in/out-parameter:
    its first element is replaced by the normalized vector.
    */
    public static double normalizeAndGivePreviousNorm(Vector3Dd[] vector, double epsilon) {
        double len = vector[0].dotProduct(vector[0]); // Note: this starts being length ^ 2

        if (len <= 0.0) {
            return 0.0;
        }

        if (len <= 1.0 + epsilon && len >= 1.0 - epsilon) {
            // First order approximation
            len = 0.5 + 0.5 * len;
        } else {
            len = Math.sqrt(len);
        }

        vector[0] = new Vector3Dd(vector[0].x() / len, vector[0].y() / len, vector[0].z() / len);

        return len;
    }

    public static boolean isNull(Vector3Dd vector, double epsilon) {
        return vector.dotProduct(vector) <= epsilon * epsilon;
    }

    /**
    Returns squared distance between the two vectors
    */
    public static double distanceSquared(Vector3Dd a, Vector3Dd b) {
        double dx = b.x() - a.x();
        double dy = b.y() - a.y();
        double dz = b.z() - a.z();
        return dx * dx + dy * dy + dz * dz;
    }
}

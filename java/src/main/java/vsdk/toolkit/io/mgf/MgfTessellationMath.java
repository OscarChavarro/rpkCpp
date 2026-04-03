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
    Compute u and v given w (normalized)
    */
    public static void mgfMakeAxes(Vector3Dd u, Vector3Dd v, Vector3Dd w, double epsilon) {
        v.x = 0.0;
        v.y = 0.0;
        v.z = 0.0;
        double[] vArr = new double[] {v.x, v.y, v.z};
        double[] wArr = new double[] {w.x, w.y, w.z};

        int i;
        for (i = 0; i < 3; i++) {
            if (wArr[i] > -0.6 && wArr[i] < 0.6) {
                break;
            }
        }

        if (i < 3) {
            vArr[i] = 1.0;
        }

        v.x = vArr[0];
        v.y = vArr[1];
        v.z = vArr[2];

        u.crossProduct(v, w);
        u.normalizeAndGivePreviousNorm(epsilon);
        v.crossProduct(w, u);
    }
}

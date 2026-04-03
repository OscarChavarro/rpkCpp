package vsdk.toolkit.material;

import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.skin.RayHit;

/**
Emittance Distribution Functions: the self-emitted radiance distribution of light sources.
*/
public class PhongEmittanceDistributionFunction {
    private ColorRgb Kd;
    private ColorRgb kd;
    private ColorRgb Ks;
    private float Ns;

    private boolean isSpecular() {
        return Ns >= Xxdf.PHONG_LOWEST_SPECULAR_EXP;
    }

    public PhongEmittanceDistributionFunction(ColorRgb KdParameter, ColorRgb KsParameter, double NsParameter) {
        Kd = new ColorRgb(KdParameter.r, KdParameter.g, KdParameter.b);
        kd = new ColorRgb();
        kd.scaledCopy((float)(1.0 / Math.PI), Kd);
        Ks = new ColorRgb(KsParameter.r, KsParameter.g, KsParameter.b);
        if (!Ks.isBlack()) {
            Error.warning("phongEdfCreate", "Non-diffuse light sources not yet implemented");
        }
        Ns = (float)NsParameter;
    }

    public ColorRgb getKd() {
        return Kd;
    }

    public ColorRgb getKs() {
        return Ks;
    }

    public float getNs() {
        return Ns;
    }

    public static boolean edfIsTextured() {
        return false;
    }

    public static boolean edfShadingFrame(RayHit hit, Vector3D X, Vector3D Y, Vector3D Z) {
        return false;
    }

    public ColorRgb phongEmittance(RayHit hit, int flags) {
        ColorRgb result = new ColorRgb();
        result.clear();

        if ((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) != 0) {
            result.add(result, Kd);
        }

        if (isSpecular()) {
            if ((flags & XxdfComponentFlag.SPECULAR_COMPONENT) != 0) {
                result.add(result, Ks);
            }
        }
        else if ((flags & XxdfComponentFlag.GLOSSY_COMPONENT) != 0) {
            result.add(result, Ks);
        }

        return result;
    }

    public ColorRgb phongEdfEval(RayHit hit, Vector3D out, int flags, double[] probabilityDensityFunction) {
        Vector3D normal = new Vector3D();
        ColorRgb result = new ColorRgb();
        result.clear();
        setOut(probabilityDensityFunction, 0.0);

        if (!hit.shadingNormal(normal)) {
            Error.warning("phongEdfEval", "Couldn't determine shading normal");
            return result;
        }

        double cosL = out.dotProduct(normal);
        if (cosL < 0.0) {
            return result;
        }

        if ((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) != 0) {
            result.add(result, kd);
            setOut(probabilityDensityFunction, cosL / Math.PI);
        }

        if ((flags & XxdfComponentFlag.SPECULAR_COMPONENT) != 0) {
            // Not implemented in original C++ code.
        }

        return result;
    }

    public Vector3D phongEdfSample(
        RayHit hit,
        int flags,
        double xi1,
        double xi2,
        ColorRgb selfEmittedRadiance,
        double[] probabilityDensityFunction) {
        if (selfEmittedRadiance != null) {
            selfEmittedRadiance.clear();
        }
        setOut(probabilityDensityFunction, 0.0);

        Vector3D dir = new Vector3D(0.0f, 0.0f, 1.0f);
        if ((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) != 0) {
            CoordinateSystem coord = new CoordinateSystem();
            Vector3D normal = new Vector3D();
            if (!hit.shadingNormal(normal)) {
                Error.warning("phongEdfEval", "Couldn't determine shading normal");
                return dir;
            }

            coord.setFromZAxis(normal);
            double[] sampledPdf = new double[] {0.0};
            dir = coord.sampleHemisphereCosTheta(xi1, xi2, sampledPdf);
            setOut(probabilityDensityFunction, sampledPdf[0]);

            if (selfEmittedRadiance != null) {
                selfEmittedRadiance.scaledCopy((float)(1.0 / Math.PI), Kd);
            }
        }

        return dir;
    }

    private static void setOut(double[] out, double value) {
        if (out != null && out.length > 0) {
            out[0] = value;
        }
    }
}

package vsdk.toolkit.material;

/**
Emittance Distribution Functions: the self-emitted radiance
distribution of light sources
*/

/*hit*/

/**
Creates Phong type EDF, BRDF, BTDF data structs:
Kd = diffuse emittance [W/m^2], reflectance or transmittance (number between 0 and 1)
Ks = specular emittance, reflectance or transmittance (same dimensions as Kd)
Ns = Phong exponent.
note: Emittance is total power emitted by the light source per unit of area
*/

// Because we use it often

/**
Returns emittance, reflectance, transmittance
*/

/**
Returns the emittance (self-emitted radiant exitance) [W / m ^ 2] of the EDF
*/

/**
Evaluates the edf: return exitant radiance [W/m^2 sr] into the direction
out. If probabilityDensityFunction is not null, the stochasticJacobiProbability density of the direction is
computed and returned in probabilityDensityFunction
*/

// Back face of a light does not radiate

// kd + ks (idealReflected * out) ^ n

// Divide by PI to turn radiant exitance [W / m ^ 2] into exitant radiance [W / m ^ 2 sr]

// ???

/**
Samples a direction according to the specified edf and emission range determined
by flags. If emitted_radiance is not a null pointer, the emitted radiance along
the generated direction is returned. If probabilityDensityFunction is not null, the stochasticJacobiProbability density
of the generated direction is computed and returned in probabilityDensityFunction
*/

// Section [ARVO1995b].2: two independent samples (xi1, xi2) in [0,1]^2

// are mapped to a direction on the hemisphere after building a local frame.

/**
Computes a shading frame at the given hit point. The Z axis of this frame is
the shading normal, The X axis is in the tangent plane on the surface at the
hit point ("brush" direction relevant for anisotropic shaders e.g.). Y
is perpendicular to X and Z. X and Y may be null pointers. In this case,
only the shading normal is returned, avoiding computation of the X and
Y axis if possible).
Note: also edf's can have a routine for computing the shading frame. If a
material has both an edf and a bsdf, the shading frame shall of course
be the same.
This routine returns TRUE if a shading frame could be constructed and FALSE if
not. In the latter case, a default frame needs to be used (not computed by this
routine - pointShadingFrame() in material.[ch] constructs such a frame if
needed)
*/

/*X*/

/*Y*/

/*Z*/

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.skin.RayHitFlag;
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
            Logger.warning("phongEdfCreate", "Non-diffuse light sources not yet implemented");
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

    public static boolean edfShadingFrame(ShadingContext context, Vector3D X, Vector3D Y, Vector3D Z) {
        return false;
    }

    public ColorRgb phongEmittance(ShadingContext context, int flags) {
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

    public ColorRgb phongEmittance(RayHit hit, int flags) {
        ShadingContext context = new ShadingContext(
            new Vector3D(),
            new Vector3D(),
            new Vector3D(),
            new Vector3D(),
            new Vector2Dd(),
            new CoordinateSystem(),
            null,
            0);
        return phongEmittance(context, flags);
    }

    public ColorRgb phongEdfEval(ShadingContext context, Vector3D out, int flags, double[] probabilityDensityFunction) {
        Vector3D normal = new Vector3D();
        ColorRgb result = new ColorRgb();
        result.clear();
        setOut(probabilityDensityFunction, 0.0);

        if (context == null || !context.hasFlag(RayHitFlag.NORMAL)) {
            Logger.warning("phongEdfEval", "Couldn't determine shading normal");
            return result;
        }
        normal.copy(context.getShadingNormal());

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

    public ColorRgb phongEdfEval(RayHit hit, Vector3D out, int flags, double[] probabilityDensityFunction) {
        if (hit == null) {
            setOut(probabilityDensityFunction, 0.0);
            ColorRgb result = new ColorRgb();
            result.clear();
            return result;
        }
        Vector3D normal = new Vector3D();
        if (!hit.shadingNormal(normal)) {
            setOut(probabilityDensityFunction, 0.0);
            ColorRgb result = new ColorRgb();
            result.clear();
            return result;
        }
        Vector3D texCoord = new Vector3D();
        int localFlags = RayHitFlag.NORMAL;
        if (hit.getTexCoord(texCoord)) {
            localFlags |= RayHitFlag.TEXTURE_COORDINATE;
        }
        else {
            texCoord.set(0.0f, 0.0f, 0.0f);
        }
        ShadingContext context = new ShadingContext(
            hit.getPoint(),
            hit.getGeometricNormal(),
            normal,
            texCoord,
            hit.getUv(),
            hit.getShadingFrame(),
            hit.getMaterial(),
            localFlags);
        return phongEdfEval(context, out, flags, probabilityDensityFunction);
    }

    public Vector3D phongEdfSample(
        ShadingContext context,
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
            if (context == null || !context.hasFlag(RayHitFlag.NORMAL)) {
                Logger.warning("phongEdfEval", "Couldn't determine shading normal");
                return dir;
            }
            Vector3D normal = context.getShadingNormal();

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
        if (hit == null) {
            setOut(probabilityDensityFunction, 0.0);
            return new Vector3D(0.0, 0.0, 1.0);
        }
        Vector3D normal = new Vector3D();
        if (!hit.shadingNormal(normal)) {
            setOut(probabilityDensityFunction, 0.0);
            return new Vector3D(0.0, 0.0, 1.0);
        }
        Vector3D texCoord = new Vector3D();
        int localFlags = RayHitFlag.NORMAL;
        if (hit.getTexCoord(texCoord)) {
            localFlags |= RayHitFlag.TEXTURE_COORDINATE;
        }
        else {
            texCoord.set(0.0f, 0.0f, 0.0f);
        }
        ShadingContext context = new ShadingContext(
            hit.getPoint(),
            hit.getGeometricNormal(),
            normal,
            texCoord,
            hit.getUv(),
            hit.getShadingFrame(),
            hit.getMaterial(),
            localFlags);
        return phongEdfSample(context, flags, xi1, xi2, selfEmittedRadiance, probabilityDensityFunction);
    }

    private static void setOut(double[] out, double value) {
        if (out != null && out.length > 0) {
            out[0] = value;
        }
    }
}

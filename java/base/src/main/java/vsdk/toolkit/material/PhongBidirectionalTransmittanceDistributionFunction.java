package vsdk.toolkit.material;

/**
Index of refraction data type. Normally when using BSDF's
this should not be needed. In C++ this would of course
be a plain complex number
*/

/**
All components of the Btdf

Vector directions :

in : towards patch
out : from patch
normal : leaving from patch, on the incoming side.
         So in.normal < 0 !!!
*/

/**
Returns the transmittance of the BTDF
*/

/**
Btdf evaluations
*/

// Specular-like refraction can turn into reflection.

// So for refraction a complete sphere should be

// sampled ! Importance sampling is advisable.

// Diffuse transmission is considered to always pass

// the material boundary

// Diffuse part

// Normal is pointing away from refracted direction

// Specular part

// cos(a) ^ n

// Ks -> ks

// Choose sampling mode

// Store in phong data ?

// Determine diffuse or glossy/specular sampling

// Absorption

// Rescaling of x_1

// Sample diffuse

// Section [ARVO1995b].2: square-to-sphere mapping in the frame of the transmitted hemisphere.

// Sample specular

// Section [ARVO1995b].2: same 2D mapping with a lobe centered on the ideal transmitted direction.

// Assume totalIR (maybe we should test the refractionIndices

// Combine Probability Density Functions

// Ensure 'in' on the same side as 'normal'!

// Transmitted ray

// Diffuse sampling probabilityDensityFunction

// Glossy or specular

// Normal was inverted, so materialSides switch also

/**
Refraction index
*/

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;

public class PhongBidirectionalTransmittanceDistributionFunction {
    private ColorRgb Kd;
    private ColorRgb Ks;
    private float avgKd;
    private float avgKs;
    private float Ns;
    private RefractionIndex refractionIndex;

    private boolean isSpecular() {
        return Ns >= Xxdf.PHONG_LOWEST_SPECULAR_EXP;
    }

    public PhongBidirectionalTransmittanceDistributionFunction(
        ColorRgb inKd,
        ColorRgb inKs,
        float inNs,
        float inNr,
        float inNi) {
        Kd = new ColorRgb(inKd.r, inKd.g, inKd.b);
        avgKd = Kd.average();
        Ks = new ColorRgb(inKs.r, inKs.g, inKs.b);
        avgKs = Ks.average();
        Ns = inNs;
        refractionIndex = new RefractionIndex();
        refractionIndex.set(inNr, inNi);
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

    public RefractionIndex getRefractionIndex() {
        return refractionIndex;
    }

    public ColorRgb transmittance(int flags) {
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

        if (!Float.isFinite(result.average())) {
            Logger.fatal(-1, "transmittance", "Oops - result is not finite!");
        }

        return result;
    }

    public ColorRgb evaluate(
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        Vector3D in,
        Vector3D out,
        Vector3D normal,
        int flags) {
        Vector3D inRev = new Vector3D();
        inRev.scaledCopy(-1.0f, in);

        ColorRgb result = new ColorRgb();
        result.clear();

        if (((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) != 0) && (avgKd > 0.0f)) {
            boolean isReflection = normal.dotProduct(out) >= 0.0f;
            if (!isReflection) {
                result = new ColorRgb(Kd.r, Kd.g, Kd.b);
                result.scale((float)(1.0 / Math.PI));
            }
        }

        int nonDiffuseFlag = isSpecular() ? XxdfComponentFlag.SPECULAR_COMPONENT : XxdfComponentFlag.GLOSSY_COMPONENT;
        if (((flags & nonDiffuseFlag) != 0) && (avgKs > 0.0f)) {
            boolean[] totalIR = new boolean[] {false};
            Vector3D idealRefracted = Xxdf.idealRefractedDirection(inRev, normal, inIndex, outIndex, totalIR);
            float localDotProduct = idealRefracted.dotProduct(out);

            if (localDotProduct > 0.0f) {
                float tmpFloat = (float)Math.pow(localDotProduct, Ns);
                tmpFloat *= (Ns + 2.0f) / (2.0f * (float)Math.PI);
                result.addScaled(result, tmpFloat, Ks);
            }
        }

        return result;
    }

    public Vector3D sample(
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        Vector3D in,
        Vector3D normal,
        int doRussianRoulette,
        int flags,
        double x1,
        double x2,
        double[] probabilityDensityFunction) {
        setOut(probabilityDensityFunction, 0.0);

        Vector3D newDir = new Vector3D(0.0f, 0.0f, 0.0f);
        Vector3D inRev = new Vector3D();
        inRev.scaledCopy(-1.0f, in);

        double localAverageKd = ((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) != 0) ? avgKd : 0.0;
        int nonDiffuseFlag = isSpecular() ? XxdfComponentFlag.SPECULAR_COMPONENT : XxdfComponentFlag.GLOSSY_COMPONENT;
        double localAverageKs = ((flags & nonDiffuseFlag) != 0) ? avgKs : 0.0;

        double scatteredPower = localAverageKd + localAverageKs;
        if (scatteredPower < Numeric.EPSILON) {
            return newDir;
        }

        if (doRussianRoulette != 0) {
            if (x1 > scatteredPower) {
                return newDir;
            }
            x1 /= scatteredPower;
        }

        boolean[] totalIR = new boolean[] {false};
        Vector3D idealDir = Xxdf.idealRefractedDirection(inRev, normal, inIndex, outIndex, totalIR);
        Vector3D invNormal = new Vector3D();
        invNormal.scaledCopy(-1.0f, normal);
        CoordinateSystem coord = new CoordinateSystem();

        double diffPdf;
        double nonDiffPdf;

        if (x1 < (localAverageKd / scatteredPower)) {
            x1 = x1 / (localAverageKd / scatteredPower);
            coord.setFromZAxis(invNormal);
            double[] outDiffPdf = new double[] {0.0};
            newDir = coord.sampleHemisphereCosTheta(x1, x2, outDiffPdf);
            diffPdf = outDiffPdf[0];

            float tmpFloat = idealDir.dotProduct(newDir);
            if (tmpFloat > 0.0f) {
                nonDiffPdf = (Ns + 1.0) * Math.pow(tmpFloat, Ns) / (2.0 * Math.PI);
            }
            else {
                nonDiffPdf = 0.0;
            }
        }
        else {
            x1 = (x1 - (localAverageKd / scatteredPower)) / (localAverageKs / scatteredPower);
            coord.setFromZAxis(idealDir);
            double[] outNonDiffPdf = new double[] {0.0};
            newDir = coord.sampleHemisphereCosNTheta(Ns, x1, x2, outNonDiffPdf);
            nonDiffPdf = outNonDiffPdf[0];

            double cosTheta = normal.dotProduct(newDir);
            if (cosTheta > 0.0) {
                diffPdf = cosTheta / Math.PI;
            }
            else {
                diffPdf = 0.0;
            }
        }

        double pdf = localAverageKd * diffPdf + localAverageKs * nonDiffPdf;
        if (doRussianRoulette == 0) {
            pdf /= scatteredPower;
        }

        setOut(probabilityDensityFunction, pdf);
        return newDir;
    }

    public void evaluateProbabilityDensityFunction(
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        Vector3D in,
        Vector3D out,
        Vector3D normal,
        int flags,
        double[] probabilityDensityFunction,
        double[] probabilityDensityFunctionRR) {
        setOut(probabilityDensityFunction, 0.0);
        setOut(probabilityDensityFunctionRR, 0.0);

        Vector3D inRev = new Vector3D();
        inRev.scaledCopy(-1.0f, in);

        Vector3D goodNormal = new Vector3D();
        double cosIn = in.dotProduct(normal);
        if (cosIn >= 0.0) {
            goodNormal.copy(normal);
        }
        else {
            goodNormal.scaledCopy(-1.0f, normal);
        }

        double cosTheta = goodNormal.dotProduct(out);

        double localAverageKd;
        if (((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) != 0) && (cosTheta < 0.0)) {
            localAverageKd = avgKd;
        }
        else {
            localAverageKd = 0.0;
        }

        int nonDiffuseFlag = isSpecular() ? XxdfComponentFlag.SPECULAR_COMPONENT : XxdfComponentFlag.GLOSSY_COMPONENT;
        double localAverageKs = ((flags & nonDiffuseFlag) != 0) ? avgKs : 0.0;

        double scatteredPower = localAverageKd + localAverageKs;
        if (scatteredPower < Numeric.EPSILON) {
            return;
        }

        double diffPdf = (localAverageKd > 0.0) ? (cosTheta / Math.PI) : 0.0;

        double nonDiffPdf = 0.0;
        if (localAverageKs > 0.0) {
            boolean[] totalIR = new boolean[] {false};
            Vector3D idealDir;

            if (cosIn >= 0.0) {
                idealDir = Xxdf.idealRefractedDirection(inRev, goodNormal, inIndex, outIndex, totalIR);
            }
            else {
                idealDir = Xxdf.idealRefractedDirection(inRev, goodNormal, outIndex, inIndex, totalIR);
            }

            double cosAlpha = idealDir.dotProduct(out);
            if (cosAlpha > 0.0) {
                nonDiffPdf = (Ns + 1.0) * Math.pow(cosAlpha, Ns) / (2.0 * Math.PI);
            }
        }

        setOut(probabilityDensityFunction, (localAverageKd * diffPdf + localAverageKs * nonDiffPdf) / scatteredPower);
        setOut(probabilityDensityFunctionRR, scatteredPower);
    }

    public void setIndexOfRefraction(RefractionIndex index) {
        index.set(refractionIndex.getNr(), refractionIndex.getNi());
    }

    private static void setOut(double[] out, double value) {
        if (out != null && out.length > 0) {
            out[0] = value;
        }
    }
}

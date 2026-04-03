package vsdk.toolkit.material;

import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;

public class PhongBidirectionalReflectanceDistributionFunction {
    private ColorRgb Kd;
    private ColorRgb Ks;
    private float avgKd;
    private float avgKs;
    private float Ns;

    private boolean isSpecular() {
        return Ns >= Xxdf.PHONG_LOWEST_SPECULAR_EXP;
    }

    public PhongBidirectionalReflectanceDistributionFunction(ColorRgb inKd, ColorRgb inKs, double inNs) {
        Kd = new ColorRgb(inKd.r, inKd.g, inKd.b);
        avgKd = Kd.average();
        Ks = new ColorRgb(inKs.r, inKs.g, inKs.b);
        avgKs = Ks.average();
        Ns = (float)inNs;
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

    public ColorRgb reflectance(int flags) {
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

    public ColorRgb evaluate(Vector3D in, Vector3D out, Vector3D normal, int flags) {
        ColorRgb result = new ColorRgb();
        result.clear();

        if (out.dotProduct(normal) < 0.0f) {
            return result;
        }

        if (((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) != 0) && (avgKd > 0.0f)) {
            result.addScaled(result, (float)(1.0 / Math.PI), Kd);
        }

        int nonDiffuseFlag = isSpecular() ? XxdfComponentFlag.SPECULAR_COMPONENT : XxdfComponentFlag.GLOSSY_COMPONENT;

        if (((flags & nonDiffuseFlag) != 0) && (avgKs > 0.0f)) {
            Vector3D inRev = new Vector3D();
            inRev.scaledCopy(-1.0f, in);
            Vector3D idealReflected = Xxdf.idealReflectedDirection(inRev, normal);
            float localDotProduct = idealReflected.dotProduct(out);

            if (localDotProduct > 0.0f) {
                float tmpFloat = (float)Math.pow(localDotProduct, Ns);
                tmpFloat *= (Ns + 2.0f) / (2.0f * (float)Math.PI);
                result.addScaled(result, tmpFloat, Ks);
            }
        }

        return result;
    }

    public Vector3D sample(
        Vector3D in,
        Vector3D normal,
        int doRussianRoulette,
        int flags,
        double x1,
        double x2,
        double[] probabilityDensityFunction) {
        setOut(probabilityDensityFunction, 0.0);

        Vector3D inRev = new Vector3D();
        inRev.scaledCopy(-1.0f, in);

        double localAverageKd = ((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) != 0) ? avgKd : 0.0;
        int nonDiffuseFlag = isSpecular() ? XxdfComponentFlag.SPECULAR_COMPONENT : XxdfComponentFlag.GLOSSY_COMPONENT;
        double localAverageKs = ((flags & nonDiffuseFlag) != 0) ? avgKs : 0.0;

        double scatteredPower = localAverageKd + localAverageKs;
        Vector3D newDir = new Vector3D(0.0f, 0.0f, 0.0f);
        if (scatteredPower < Numeric.EPSILON) {
            return newDir;
        }

        if (doRussianRoulette != 0) {
            if (x1 > scatteredPower) {
                return newDir;
            }
            x1 /= scatteredPower;
        }

        Vector3D idealDir = Xxdf.idealReflectedDirection(inRev, normal);
        CoordinateSystem coord = new CoordinateSystem();
        double diffPdf;
        double nonDiffPdf;

        if (x1 < (localAverageKd / scatteredPower)) {
            x1 = x1 / (localAverageKd / scatteredPower);
            coord.setFromZAxis(normal);
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
            if (cosTheta <= 0.0) {
                return newDir;
            }
            diffPdf = cosTheta / Math.PI;
        }

        double pdf = localAverageKd * diffPdf + localAverageKs * nonDiffPdf;
        if (doRussianRoulette == 0) {
            pdf /= scatteredPower;
        }

        setOut(probabilityDensityFunction, pdf);
        return newDir;
    }

    public void evaluateProbabilityDensityFunction(
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
        if (cosTheta < 0.0) {
            return;
        }

        double localAverageKd = ((flags & XxdfComponentFlag.DIFFUSE_COMPONENT) != 0) ? avgKd : 0.0;
        int nonDiffuseFlag = isSpecular() ? XxdfComponentFlag.SPECULAR_COMPONENT : XxdfComponentFlag.GLOSSY_COMPONENT;
        double localAverageKs = ((flags & nonDiffuseFlag) != 0) ? avgKs : 0.0;

        double scatteredPower = localAverageKd + localAverageKs;
        if (scatteredPower < Numeric.EPSILON) {
            return;
        }

        double diffPdf = 0.0;
        if (avgKd > 0.0) {
            diffPdf = cosTheta / Math.PI;
        }

        double nonDiffPdf = 0.0;
        if (avgKs > 0.0) {
            Vector3D idealDir = Xxdf.idealReflectedDirection(inRev, goodNormal);
            double cosAlpha = idealDir.dotProduct(out);
            if (cosAlpha > 0.0) {
                nonDiffPdf = (Ns + 1.0) * Math.pow(cosAlpha, Ns) / (2.0 * Math.PI);
            }
        }

        setOut(probabilityDensityFunction, (avgKd * diffPdf + avgKs * nonDiffPdf) / scatteredPower);
        setOut(probabilityDensityFunctionRR, scatteredPower);
    }

    private static void setOut(double[] out, double value) {
        if (out != null && out.length > 0) {
            out[0] = value;
        }
    }
}

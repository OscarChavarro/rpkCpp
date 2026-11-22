package vsdk.toolkit.environment.material;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;

import static vsdk.toolkit.environment.material.ScatteringArithmetic.dot;
import static vsdk.toolkit.environment.material.ScatteringArithmetic.scale;

/**
Immutable Phong lobe as MGF materials define one: a diffuse color Kd, a
non-diffuse color Ks and a Phong exponent Ns. The non-diffuse part counts as
the SPECULAR component when Ns reaches LOWEST_SPECULAR_EXPONENT and as the
GLOSSY component below it. The same lobe describes reflection, transmission
or emission; PhongMaterial composes them.

The exponent is stored in single precision, as RenderPark stores it, so that
the glossy/specular threshold classifies every material the same way.

Directional functions (RenderPark's modified Phong model, which is symmetric
and energy conserving): with kd = Kd / pi and ks = Ks (Ns + 2) / (2 pi), the
lobe evaluates to kd + ks cos(a)^Ns, a being the angle between the outgoing
direction and the ideal (mirror or refracted) one. Kd and Ks are the energy
scattered for perpendicular incidence, so Kd + Ks <= 1 conserves energy.
Sampling picks the diffuse part with probability avg(Kd) / (avg(Kd) +
avg(Ks)), cosine distributed around the normal, or the non-diffuse part,
distributed as cos^Ns around the ideal direction (density (Ns + 1) cos^Ns /
(2 pi): it differs from the (Ns + 2) of the evaluation); the density of a
direction is the mixture of both. With Russian roulette the first variate
also decides absorption: the direction survives with probability avg(Kd) +
avg(Ks), and the density returned is then not divided by it.

Conventions: `in` and `out` both leave the surface: `in` points back to
where the incoming light (or importance) comes from, `out` to where it
goes; `normal` is the shading normal, on either side (the densities use
the normal turned to the side of `in`; the transmission density swaps the
indices when in . normal < 0). Densities are per unit solid angle; the
diffuse transmission density keeps RenderPark's sign, cos(theta) / pi with
theta measured from the normal on the side of `in`, so it is negative; the caller supplies the variates x1 and x2. A density of 0 means no
direction was produced (absorption or a direction below the surface).
The arithmetic, its precision and its order are RenderPark's, so results are
bit-identical to RenderPark's BRDF and BTDF.

Output parameters (`density`, `survival`) are one-element holders. The
single precision `pow` of cpp (`powf`) is the double precision one rounded
to float.
*/
public final class PhongLobe {
    public static final float LOWEST_SPECULAR_EXPONENT = 250.0F;

    // Scattered power below which nothing is sampled (RenderPark's EPSILON)
    private static final double NO_SCATTERING = 1e-6;

    // 1 / pi, as the C constant M_1_PI
    private static final double M_1_PI = 0.318309886183790671538;

    private final ColorRgb diffuse;
    private final ColorRgb nonDiffuse;
    private final float exponent;
    private final float averageDiffuse;
    private final float averageNonDiffuse;

    private static float averageOf(ColorRgb color)
    {
        return (float)((color.getR() + color.getG() + color.getB()) / 3.0);
    }

    public PhongLobe(ColorRgb inDiffuse, ColorRgb inNonDiffuse, float inExponent)
    {
        diffuse = new ColorRgb(inDiffuse);
        nonDiffuse = new ColorRgb(inNonDiffuse);
        exponent = inExponent;
        averageDiffuse = averageOf(inDiffuse);
        averageNonDiffuse = averageOf(inNonDiffuse);
    }

    public PhongLobe(PhongLobe other)
    {
        diffuse = new ColorRgb(other.diffuse);
        nonDiffuse = new ColorRgb(other.nonDiffuse);
        exponent = other.exponent;
        averageDiffuse = other.averageDiffuse;
        averageNonDiffuse = other.averageNonDiffuse;
    }

    public ColorRgb getDiffuse()
    {
        return diffuse;
    }

    public ColorRgb getNonDiffuse()
    {
        return nonDiffuse;
    }

    public float getExponent()
    {
        return exponent;
    }

    public boolean isSpecular()
    {
        return exponent >= LOWEST_SPECULAR_EXPONENT;
    }

    /** Average diffuse color when the diffuse component is requested, else 0 */
    private double sampledDiffuse(int components)
    {
        return (components & ScatteringComponent.DIFFUSE) != 0 ? averageDiffuse : 0.0;
    }

    /** Average non-diffuse color when its component (glossy or specular) is requested, else 0 */
    private double sampledNonDiffuse(int components)
    {
        final int nonDiffuseComponent = isSpecular() ? ScatteringComponent.SPECULAR : ScatteringComponent.GLOSSY;
        return (components & nonDiffuseComponent) != 0 ? averageNonDiffuse : 0.0;
    }

    /** cpp `java::Math::pow(float, float)` (`powf`) */
    private static float powf(float a, float e)
    {
        return (float)Math.pow(a, e);
    }

    /**
    Sum of the colors of the requested components (ScatteringComponent
    DIFFUSE, GLOSSY, SPECULAR): the integrated reflectance, transmittance
    or emittance of the lobe. Accumulated in double precision starting from
    zero, diffuse first.
    */
    public ColorRgb integrated(int components)
    {
        double r = 0.0;
        double g = 0.0;
        double b = 0.0;

        if ( (components & ScatteringComponent.DIFFUSE) != 0 ) {
            r += diffuse.getR();
            g += diffuse.getG();
            b += diffuse.getB();
        }

        final int nonDiffuseComponent = isSpecular() ? ScatteringComponent.SPECULAR : ScatteringComponent.GLOSSY;
        if ( (components & nonDiffuseComponent) != 0 ) {
            r += nonDiffuse.getR();
            g += nonDiffuse.getG();
            b += nonDiffuse.getB();
        }

        return new ColorRgb(r, g, b);
    }

    /** Reflected radiance factor (BRDF value) of the requested components */
    public ColorRgb reflectionEvaluate(Vector3Df in, Vector3Df out, Vector3Df normal, int components)
    {
        double r = 0.0;
        double g = 0.0;
        double b = 0.0;
        final Vector3Df inReversed = scale(-1.0F, in);

        if ( dot(out, normal) < 0 ) {
            // Below the surface: transmission, not reflection
            return new ColorRgb(0.0, 0.0, 0.0);
        }

        if ( (components & ScatteringComponent.DIFFUSE) != 0 && (averageDiffuse > 0.0) ) {
            r += M_1_PI * diffuse.getR();
            g += M_1_PI * diffuse.getG();
            b += M_1_PI * diffuse.getB();
        }

        final int nonDiffuseComponent = isSpecular() ? ScatteringComponent.SPECULAR : ScatteringComponent.GLOSSY;
        if ( (components & nonDiffuseComponent) != 0 && (averageNonDiffuse > 0.0) ) {
            final Vector3Df idealReflected = ScatteringFrame.idealReflection(inReversed, normal);
            final float cosAlpha = dot(idealReflected, out);
            if ( cosAlpha > 0 ) {
                float factor = powf(cosAlpha, exponent); // cos(a)^Ns
                factor *= (exponent + 2.0F) / (2.0F * (float)Math.PI); // Ks -> ks
                r += factor * nonDiffuse.getR();
                g += factor * nonDiffuse.getG();
                b += factor * nonDiffuse.getB();
            }
        }
        return new ColorRgb(r, g, b);
    }

    /** Samples a reflected direction; its density goes to density[0] */
    public Vector3Df reflectionSample(
        Vector3Df in,
        Vector3Df normal,
        boolean russianRoulette,
        int components,
        double x1,
        double x2,
        double[] density)
    {
        final Vector3Df inReversed = scale(-1.0F, in);
        density[0] = 0;

        final double localDiffuse = sampledDiffuse(components);
        final double localNonDiffuse = sampledNonDiffuse(components);
        final double scatteredPower = localDiffuse + localNonDiffuse;
        if ( scatteredPower < NO_SCATTERING ) {
            return new Vector3Df(0.0F, 0.0F, 0.0F);
        }

        if ( russianRoulette ) {
            if ( x1 > scatteredPower ) {
                // Absorption
                return new Vector3Df(0.0F, 0.0F, 0.0F);
            }
            x1 /= scatteredPower;
        }

        final Vector3Df idealDirection = ScatteringFrame.idealReflection(inReversed, normal);
        // The first variate chooses the part, then is rescaled into [0, 1) within it
        final boolean diffusePart = x1 < (localDiffuse / scatteredPower);
        final double xPart = diffusePart
            ? x1 / (localDiffuse / scatteredPower)
            : (x1 - (localDiffuse / scatteredPower)) / (localNonDiffuse / scatteredPower);
        final double[] diffuseDensity = {0.0};
        final double[] nonDiffuseDensity = {0.0};
        // Diffuse part cosine distributed around the normal; glossy or specular
        // part cos^Ns distributed around the mirror direction
        final Vector3Df newDirection = diffusePart
            ? new ScatteringFrame(normal).sampleCosine(xPart, x2, diffuseDensity)
            : new ScatteringFrame(idealDirection).samplePhong(exponent, xPart, x2, nonDiffuseDensity);

        if ( diffusePart ) {
            final float cosAlpha = dot(idealDirection, newDirection);
            if ( cosAlpha > 0 ) {
                nonDiffuseDensity[0] = (exponent + 1.0) * powf(cosAlpha, exponent) / (2.0 * Math.PI);
            }
        }
        else {
            final double cosTheta = dot(normal, newDirection);
            if ( cosTheta <= 0 ) {
                return newDirection;
            }
            diffuseDensity[0] = cosTheta / Math.PI;
        }

        density[0] = localDiffuse * diffuseDensity[0] + localNonDiffuse * nonDiffuseDensity[0];
        if ( !russianRoulette ) {
            density[0] /= scatteredPower;
        }
        return newDirection;
    }

    /**
    Density with which reflectionSample (without Russian roulette) produces
    `out`, and the survival probability of the roulette; the normal is first
    turned to the side of `in`
    */
    public void reflectionDensity(
        Vector3Df in,
        Vector3Df out,
        Vector3Df normal,
        int components,
        double[] density,
        double[] survival)
    {
        final Vector3Df inReversed = scale(-1.0F, in);
        density[0] = 0;
        survival[0] = 0;

        // The normal on the side of 'in'
        final double cosIn = dot(in, normal);
        final Vector3Df goodNormal = cosIn >= 0 ? normal : scale(-1.0F, normal);
        final double cosTheta = dot(goodNormal, out);
        if ( cosTheta < 0 ) {
            return;
        }

        final double localDiffuse = sampledDiffuse(components);
        final double localNonDiffuse = sampledNonDiffuse(components);
        final double scatteredPower = localDiffuse + localNonDiffuse;
        if ( scatteredPower < NO_SCATTERING ) {
            return;
        }

        double diffuseDensity = 0.0;
        if ( averageDiffuse > 0 ) {
            diffuseDensity = cosTheta / Math.PI;
        }

        double nonDiffuseDensity = 0.0;
        if ( averageNonDiffuse > 0 ) {
            final Vector3Df idealDirection = ScatteringFrame.idealReflection(inReversed, goodNormal);
            final double cosAlpha = dot(idealDirection, out);
            if ( cosAlpha > 0 ) {
                nonDiffuseDensity = (exponent + 1.0) * Math.pow(cosAlpha, (double)exponent) / (2.0 * Math.PI);
            }
        }

        // RenderPark weighs both parts by their averages here, requested or not
        density[0] = (averageDiffuse * diffuseDensity + averageNonDiffuse * nonDiffuseDensity) / scatteredPower;
        survival[0] = scatteredPower;
    }

    /**
    Transmitted radiance factor (BTDF value) of the requested components,
    with the real refraction indices on the side of `in` and beyond
    */
    public ColorRgb transmissionEvaluate(
        float inIndex,
        float outIndex,
        Vector3Df in,
        Vector3Df out,
        Vector3Df normal,
        int components)
    {
        final Vector3Df inReversed = scale(-1.0F, in);
        double r = 0.0;
        double g = 0.0;
        double b = 0.0;

        if ( (components & ScatteringComponent.DIFFUSE) != 0 && (averageDiffuse > 0) ) {
            // Diffuse transmission always crosses the boundary
            final boolean isReflection = (dot(normal, out) >= 0);
            if ( !isReflection ) {
                r += M_1_PI * diffuse.getR();
                g += M_1_PI * diffuse.getG();
                b += M_1_PI * diffuse.getB();
            }
        }

        final int nonDiffuseComponent = isSpecular() ? ScatteringComponent.SPECULAR : ScatteringComponent.GLOSSY;
        if ( (components & nonDiffuseComponent) != 0 && (averageNonDiffuse > 0) ) {
            final boolean[] totalInternalReflection = {false};
            final Vector3Df idealRefracted = ScatteringFrame.idealRefraction(
                inReversed, normal, inIndex, outIndex, totalInternalReflection);
            final float cosAlpha = dot(idealRefracted, out);
            if ( cosAlpha > 0 ) {
                float factor = powf(cosAlpha, exponent); // cos(a)^Ns
                factor *= (exponent + 2.0F) / (2.0F * (float)Math.PI); // Ks -> ks
                r += factor * nonDiffuse.getR();
                g += factor * nonDiffuse.getG();
                b += factor * nonDiffuse.getB();
            }
        }
        return new ColorRgb(r, g, b);
    }

    /** Samples a transmitted direction; its density goes to density[0] */
    public Vector3Df transmissionSample(
        float inIndex,
        float outIndex,
        Vector3Df in,
        Vector3Df normal,
        boolean russianRoulette,
        int components,
        double x1,
        double x2,
        double[] density)
    {
        final Vector3Df inReversed = scale(-1.0F, in);
        density[0] = 0;

        final double localDiffuse = sampledDiffuse(components);
        final double localNonDiffuse = sampledNonDiffuse(components);
        final double scatteredPower = localDiffuse + localNonDiffuse;
        if ( scatteredPower < NO_SCATTERING ) {
            return new Vector3Df(0.0F, 0.0F, 0.0F);
        }

        if ( russianRoulette ) {
            if ( x1 > scatteredPower ) {
                // Absorption
                return new Vector3Df(0.0F, 0.0F, 0.0F);
            }
            x1 /= scatteredPower;
        }

        final boolean[] totalInternalReflection = {false};
        final Vector3Df idealDirection = ScatteringFrame.idealRefraction(
            inReversed, normal, inIndex, outIndex, totalInternalReflection);
        final Vector3Df inverseNormal = scale(-1.0F, normal);
        // The first variate chooses the part, then is rescaled into [0, 1) within it
        final boolean diffusePart = x1 < (localDiffuse / scatteredPower);
        final double xPart = diffusePart
            ? x1 / (localDiffuse / scatteredPower)
            : (x1 - (localDiffuse / scatteredPower)) / (localNonDiffuse / scatteredPower);
        final double[] diffuseDensity = {0.0};
        final double[] nonDiffuseDensity = {0.0};
        // Diffuse part cosine distributed in the transmitted hemisphere; glossy
        // or specular part cos^Ns distributed around the refracted direction
        final Vector3Df newDirection = diffusePart
            ? new ScatteringFrame(inverseNormal).sampleCosine(xPart, x2, diffuseDensity)
            : new ScatteringFrame(idealDirection).samplePhong(exponent, xPart, x2, nonDiffuseDensity);

        if ( diffusePart ) {
            final float cosAlpha = dot(idealDirection, newDirection);
            if ( cosAlpha > 0 ) {
                nonDiffuseDensity[0] = (exponent + 1.0) * powf(cosAlpha, exponent) / (2.0 * Math.PI);
            }
        }
        else {
            // A direction on the side of the normal is taken as total internal reflection
            final double cosTheta = dot(normal, newDirection);
            if ( cosTheta > 0 ) {
                diffuseDensity[0] = cosTheta / Math.PI;
            }
        }

        density[0] = localDiffuse * diffuseDensity[0] + localNonDiffuse * nonDiffuseDensity[0];
        if ( !russianRoulette ) {
            density[0] /= scatteredPower;
        }
        return newDirection;
    }

    /** Density with which transmissionSample produces `out`, and the roulette survival */
    public void transmissionDensity(
        float inIndex,
        float outIndex,
        Vector3Df in,
        Vector3Df out,
        Vector3Df normal,
        int components,
        double[] density,
        double[] survival)
    {
        final Vector3Df inReversed = scale(-1.0F, in);
        density[0] = 0;
        survival[0] = 0;

        // The normal on the side of 'in'
        final double cosIn = dot(in, normal);
        final Vector3Df goodNormal = cosIn >= 0 ? normal : scale(-1.0F, normal);
        final double cosTheta = dot(goodNormal, out);

        // Diffuse transmission only for a direction that crossed the surface
        final double localDiffuse = ((components & ScatteringComponent.DIFFUSE) != 0 && (cosTheta < 0)) ? averageDiffuse : 0.0;
        final double localNonDiffuse = sampledNonDiffuse(components);
        final double scatteredPower = localDiffuse + localNonDiffuse;
        if ( scatteredPower < NO_SCATTERING ) {
            return;
        }

        double diffuseDensity;
        if ( localDiffuse > 0 ) {
            diffuseDensity = cosTheta / Math.PI;
        }
        else {
            diffuseDensity = 0.0;
        }

        double nonDiffuseDensity = 0.0;
        if ( localNonDiffuse > 0 ) {
            final boolean[] totalInternalReflection = {false};
            // With the normal turned around, the two sides swap
            final Vector3Df idealDirection = cosIn >= 0
                ? ScatteringFrame.idealRefraction(inReversed, goodNormal, inIndex, outIndex, totalInternalReflection)
                : ScatteringFrame.idealRefraction(inReversed, goodNormal, outIndex, inIndex, totalInternalReflection);
            final double cosAlpha = dot(idealDirection, out);
            if ( cosAlpha > 0 ) {
                nonDiffuseDensity = (exponent + 1.0) * Math.pow(cosAlpha, (double)exponent) / (2.0 * Math.PI);
            }
        }

        density[0] = (localDiffuse * diffuseDensity + localNonDiffuse * nonDiffuseDensity) / scatteredPower;
        survival[0] = scatteredPower;
    }
}

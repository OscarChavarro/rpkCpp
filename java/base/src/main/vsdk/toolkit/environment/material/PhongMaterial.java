package vsdk.toolkit.environment.material;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.common.logging.Logger;

/**
Surface material made of Phong lobes, as MGF scenes describe materials: an
optional reflection lobe, an optional transmission lobe, an optional emission
lobe, an optional color texture that replaces the diffuse reflection, a name
and a sidedness (one-sided surfaces scatter and emit on their front only).

It answers the integrated quantities a radiosity solver consumes:
scatteredPower (reflectance plus transmittance by component) and emittance
by component, and the directional ones a raytracer consumes: evaluation,
sampling from supplied variates and the density of a direction, for the
scattering (reflection, transmission and the texture, which scatters as a
diffuse reflector whose albedo is the texture color) and for the emission
(diffuse only). Their conventions are PhongLobe's; the arithmetic is
RenderPark's, so results are bit-identical to its split BSDF and Phong EDF.

Media. Transmission needs the refraction index on both sides of the
surface: the directional scattering functions take the materials of the
medium the ray comes from (inMedium) and of the one beyond (outMedium), and
read their getRefractionIndex(). A null medium in evaluation and sampling
has the index 0 (RenderPark's unset index); in densityOf it has the vacuum
index 1. A material without transmission lobe has the vacuum index.

Ownership and copies. The texture is shared and read-only. The material is
immutable once built, so one instance can be read from any number of
threads. copy() is a copy of the lobes and the name that shares the texture.
The geometric transforms of Material do not change a surface material and
return the material itself. Output parameters are one element arrays.
*/
public final class PhongMaterial implements Material {
    /**
    The texture color replaces this component of the scattered power
    when a texture is present.
    */
    public static final int TEXTURED_COMPONENT = ScatteringComponent.REFLECTION_DIFFUSE;

    // Scattering below which nothing is sampled (RenderPark's EPSILON)
    private static final double NO_SCATTERING = 1e-6;
    // C's M_PI literal
    private static final double M_PI = 3.14159265358979323846;

    /** The part of the scattering a first variate chose */
    private enum SampledPart { TEXTURE_PART, REFLECTION_PART, TRANSMISSION_PART, ABSORPTION }

    private final String name;
    private final boolean sided;
    private final PhongLobe reflection;
    private final PhongLobe transmission;
    private final PhongLobe emission;
    private final Texture texture;
    private final float refractionIndexReal;
    private final float refractionIndexImaginary;

    public PhongMaterial(
        String inName,
        PhongLobe inReflection,
        PhongLobe inTransmission,
        PhongLobe inEmission,
        Texture inTexture,
        boolean inSided)
    {
        this(inName, inReflection, inTransmission, inEmission, inTexture, inSided, 1.0F, 0.0F);
    }

    /** As above, with the complex refraction index of the transmission lobe */
    public PhongMaterial(
        String inName,
        PhongLobe inReflection,
        PhongLobe inTransmission,
        PhongLobe inEmission,
        Texture inTexture,
        boolean inSided,
        float inRefractionIndexReal,
        float inRefractionIndexImaginary)
    {
        name = inName != null ? inName : "";
        sided = inSided;
        reflection = inReflection;
        transmission = inTransmission;
        emission = inEmission;
        texture = inTexture;
        refractionIndexReal = inRefractionIndexReal;
        refractionIndexImaginary = inRefractionIndexImaginary;
    }

    private static boolean isFiniteColor(ColorRgb color) {
        return Float.isFinite((float) ((color.getR() + color.getG() + color.getB()) / 3.0));
    }

    private static PhongLobe copyLobe(PhongLobe lobe) {
        return lobe != null ? new PhongLobe(lobe) : null;
    }

    private static double averageOf(ColorRgb color) {
        return (color.getR() + color.getG() + color.getB()) / 3.0;
    }

    /** Refraction index of a medium in evaluation and sampling: 0 when unknown */
    private static float sampledIndex(PhongMaterial medium) {
        return medium != null ? medium.getRefractionIndex() : 0.0F;
    }

    /** Refraction index of a medium in densityOf: vacuum when unknown */
    private static float densityIndex(PhongMaterial medium) {
        return medium != null ? medium.getRefractionIndex() : 1.0F;
    }

    /** Direction sampled within the chosen part, and its density there (0 when none) */
    private static Vector3Df sampleWithinPart(
        SampledPart part,
        PhongLobe reflection,
        PhongLobe transmission,
        Vector3Df normal,
        Vector3Df in,
        float inIndex,
        float outIndex,
        int reflectionComponents,
        int transmissionComponents,
        double x1,
        double x2,
        double[] density)
    {
        switch ( part ) {
            case TEXTURE_PART:
                return new ScatteringFrame(normal).sampleCosine(x1, x2, density);
            case REFLECTION_PART:
                if ( reflection != null ) {
                    return reflection.reflectionSample(in, normal, false, reflectionComponents, x1, x2, density);
                }
                break;
            case TRANSMISSION_PART:
                if ( transmission != null ) {
                    return transmission.transmissionSample(
                        inIndex, outIndex, in, normal, false, transmissionComponents, x1, x2, density);
                }
                break;
            case ABSORPTION:
                break;
        }
        density[0] = 0.0;
        return new Vector3Df(0.0F, 0.0F, 0.0F);
    }

    /** Density of the texture's diffuse scattering: cos / pi */
    private static double textureDensity(Vector3Df out, Vector3Df normal) {
        return ScatteringArithmetic.dot(normal, out) / M_PI;
    }

    public String getName() {
        return name;
    }

    public boolean isSided() {
        return sided;
    }

    public boolean isTextured() {
        return texture != null;
    }

    public boolean hasEmission() {
        return emission != null;
    }

    public PhongLobe getReflection() {
        return reflection;
    }

    public PhongLobe getTransmission() {
        return transmission;
    }

    public PhongLobe getEmission() {
        return emission;
    }

    public Texture getTexture() {
        return texture;
    }

    /**
    Real part of the refraction index seen by a ray that crosses the
    surface: the transmission lobe's, 1 (vacuum) without one
    */
    public float getRefractionIndex() {
        return transmission != null ? refractionIndexReal : 1.0F;
    }

    /** Imaginary part of the transmission lobe's refraction index, 0 without one */
    public float getRefractionIndexImaginary() {
        return transmission != null ? refractionIndexImaginary : 0.0F;
    }

    /** Stored refraction index, also without transmission lobe (for persistence) */
    public float getStoredRefractionIndexReal() {
        return refractionIndexReal;
    }

    public float getStoredRefractionIndexImaginary() {
        return refractionIndexImaginary;
    }

    /**
    Integrated reflectance plus transmittance of the requested components
    (ScatteringComponent mask covering both lobes). When the material is
    textured and TEXTURED_COMPONENT is requested, the texture color at
    textureCoordinate (its x and y) is added first and that component is
    then left out of the reflection lobe. A null textureCoordinate means
    the coordinate is unknown: a warning is reported and the texture adds
    nothing.
    */
    public ColorRgb scatteredPower(int components, Vector3Df textureCoordinate) {
        double r = 0.0;
        double g = 0.0;
        double b = 0.0;

        if ( texture != null && (components & TEXTURED_COMPONENT) != 0 ) {
            if ( textureCoordinate == null ) {
                Logger.reportMessage("PhongMaterial", Logger.WARNING, "scatteredPower", "Couldn't get texture coordinates");
            } else {
                final ColorRgb textureColor = texture.evaluateColor(textureCoordinate.x(), textureCoordinate.y());
                r += textureColor.getR();
                g += textureColor.getG();
                b += textureColor.getB();
            }
            components &= ~TEXTURED_COMPONENT;
        }

        if ( reflection != null ) {
            final ColorRgb reflectance = reflection.integrated(ScatteringComponent.reflectionComponents(components));
            if ( !isFiniteColor(reflectance) ) {
                Logger.reportMessage("PhongMaterial", Logger.FATAL_ERROR, "scatteredPower", "Reflectance is not finite");
            }
            r += reflectance.getR();
            g += reflectance.getG();
            b += reflectance.getB();
        }

        if ( transmission != null ) {
            final ColorRgb transmittance = transmission.integrated(ScatteringComponent.transmissionComponents(components));
            if ( !isFiniteColor(transmittance) ) {
                Logger.reportMessage("PhongMaterial", Logger.FATAL_ERROR, "scatteredPower", "Transmittance is not finite");
            }
            r += transmittance.getR();
            g += transmittance.getG();
            b += transmittance.getB();
        }

        return new ColorRgb(r, g, b);
    }

    /**
    Integrated emittance of the requested components (ScatteringComponent
    DIFFUSE, GLOSSY, SPECULAR), black when the material does not emit.
    */
    public ColorRgb emittance(int components) {
        if ( emission == null ) {
            return new ColorRgb(0.0, 0.0, 0.0);
        }
        return emission.integrated(components);
    }

    @Override
    public Material copy() {
        return new PhongMaterial(name, copyLobe(reflection), copyLobe(transmission), copyLobe(emission), texture, sided,
            refractionIndexReal, refractionIndexImaginary);
    }

    @Override
    public Material translate(Vector3Dd vector) {
        return this;
    }

    @Override
    public Material rotate(Vector3Dd vector) {
        return this;
    }

    @Override
    public Material scale(Vector3Dd vector) {
        return this;
    }

    /** Texture color at the point; black without texture or texture coordinate (reported) */
    private ColorRgb textureColor(SurfaceShadingPoint point) {
        if ( texture == null ) {
            return new ColorRgb(0.0, 0.0, 0.0);
        }
        if ( !point.hasTextureCoordinate() ) {
            Logger.reportMessage("PhongMaterial", Logger.WARNING, "textureColor", "Couldn't get texture coordinates");
            return new ColorRgb(0.0, 0.0, 0.0);
        }
        return texture.evaluateColor(point.getTextureCoordinate().x(), point.getTextureCoordinate().y());
    }

    /**
    Average albedos of the texture, of the reflection without the textured
    component, and of the transmission (parts[0..2]), and the components
    left to each lobe (lobeComponents[0..1])
    */
    private void samplingProbabilities(
        SurfaceShadingPoint point,
        int components,
        double[] parts,
        int[] lobeComponents)
    {
        parts[0] = 0.0;
        if ( texture != null && (components & TEXTURED_COMPONENT) != 0 ) {
            parts[0] = averageOf(textureColor(point));
            components &= ~TEXTURED_COMPONENT;
        }
        lobeComponents[0] = ScatteringComponent.reflectionComponents(components);
        lobeComponents[1] = ScatteringComponent.transmissionComponents(components);
        parts[1] = reflection != null ? averageOf(reflection.integrated(lobeComponents[0])) : 0.0;
        parts[2] = transmission != null ? averageOf(transmission.integrated(lobeComponents[1])) : 0.0;
    }

    /**
    Scattering (BSDF) value for the requested components (ScatteringComponent
    mask over both lobes). The transmission part is added only when the
    material has a reflection lobe, as RenderPark does.
    */
    public ColorRgb scatteringEvaluate(
        SurfaceShadingPoint point,
        PhongMaterial inMedium,
        PhongMaterial outMedium,
        Vector3Df in,
        Vector3Df out,
        int components)
    {
        double r = 0.0;
        double g = 0.0;
        double b = 0.0;
        if ( !point.hasShadingNormal() ) {
            Logger.reportMessage("PhongMaterial", Logger.WARNING, "scatteringEvaluate", "Couldn't determine shading normal");
            return new ColorRgb(r, g, b);
        }
        final Vector3Df normal = point.getShadingNormal();

        if ( texture != null && (components & TEXTURED_COMPONENT) != 0 ) {
            final double textureValue = 1.0 / M_PI;
            final ColorRgb color = textureColor(point);
            r += textureValue * color.getR();
            g += textureValue * color.getG();
            b += textureValue * color.getB();
            components &= ~TEXTURED_COMPONENT;
        }

        if ( reflection != null ) {
            final ColorRgb reflected = reflection.reflectionEvaluate(
                in, out, normal, ScatteringComponent.reflectionComponents(components));
            r += reflected.getR();
            g += reflected.getG();
            b += reflected.getB();
            if ( transmission != null ) {
                final ColorRgb transmitted = transmission.transmissionEvaluate(
                    sampledIndex(inMedium), sampledIndex(outMedium), in, out, normal,
                    ScatteringComponent.transmissionComponents(components));
                r += transmitted.getR();
                g += transmitted.getG();
                b += transmitted.getB();
            } else {
                r += 0.0;
                g += 0.0;
                b += 0.0;
            }
        }
        return new ColorRgb(r, g, b);
    }

    /**
    Samples a scattered direction: the first variate picks the texture, the
    reflection or the transmission in proportion to their average albedos
    (with Russian roulette it can also pick absorption, and a null vector
    with density 0 is returned). The density of the direction over all the
    parts goes to density[0].
    */
    public Vector3Df scatteringSample(
        SurfaceShadingPoint point,
        PhongMaterial inMedium,
        PhongMaterial outMedium,
        Vector3Df in,
        boolean russianRoulette,
        int components,
        double x1,
        double x2,
        double[] density)
    {
        density[0] = 0;
        if ( !point.hasShadingNormal() ) {
            Logger.reportMessage("PhongMaterial", Logger.WARNING, "scatteringSample", "Couldn't determine shading normal");
            return new Vector3Df(0.0F, 0.0F, 1.0F);
        }
        final Vector3Df normal = point.getShadingNormal();

        final double[] parts = new double[3];
        final int[] lobeComponents = new int[2];
        samplingProbabilities(point, components, parts, lobeComponents);
        double texturePart = parts[0];
        double reflectionPart = parts[1];
        double transmissionPart = parts[2];
        final int reflectionComponents = lobeComponents[0];
        final int transmissionComponents = lobeComponents[1];

        final double scattering = texturePart + reflectionPart + transmissionPart;
        if ( scattering < NO_SCATTERING ) {
            return new Vector3Df(0.0F, 0.0F, 0.0F);
        }

        if ( !russianRoulette ) {
            // No absorption: the parts are normalized
            texturePart /= scattering;
            reflectionPart /= scattering;
            transmissionPart /= scattering;
        }

        // The first variate chooses the part, then is rescaled into [0, 1)
        SampledPart mode = SampledPart.ABSORPTION;
        if ( x1 < texturePart ) {
            mode = SampledPart.TEXTURE_PART;
            x1 /= texturePart;
        } else {
            x1 -= texturePart;
            if ( x1 < reflectionPart ) {
                mode = SampledPart.REFLECTION_PART;
                x1 /= reflectionPart;
            } else {
                x1 -= reflectionPart;
                if ( x1 < transmissionPart ) {
                    mode = SampledPart.TRANSMISSION_PART;
                    x1 /= transmissionPart;
                }
            }
        }

        final float inIndex = sampledIndex(inMedium);
        final float outIndex = sampledIndex(outMedium);
        final double[] p = {0.0};
        final Vector3Df out = sampleWithinPart(
            mode, reflection, transmission, normal, in, inIndex, outIndex,
            reflectionComponents, transmissionComponents, x1, x2, p);
        if ( mode == SampledPart.ABSORPTION ) {
            density[0] = 0;
            return out;
        }
        if ( p[0] < NO_SCATTERING ) {
            return out;
        }

        density[0] = (mode == SampledPart.TEXTURE_PART ? texturePart : mode == SampledPart.REFLECTION_PART ? reflectionPart : transmissionPart) * p[0];

        // Density of the same direction through the parts not chosen
        if ( mode != SampledPart.TEXTURE_PART ) {
            p[0] = textureDensity(out, normal);
            density[0] += texturePart * p[0];
        }

        final double[] survival = {0.0};
        if ( mode != SampledPart.REFLECTION_PART ) {
            if ( reflection == null ) {
                p[0] = 0.0;
            } else {
                reflection.reflectionDensity(in, out, normal, reflectionComponents, p, survival);
            }
            density[0] += reflectionPart * p[0];
        }

        if ( mode != SampledPart.TRANSMISSION_PART ) {
            if ( transmission == null ) {
                p[0] = 0.0;
            } else {
                transmission.transmissionDensity(inIndex, outIndex, in, out, normal, transmissionComponents, p, survival);
            }
            density[0] += transmissionPart * p[0];
        }

        return out;
    }

    /**
    Density with which scatteringSample (without Russian roulette)
    produces `out`, and the survival probability of the roulette
    */
    public void scatteringDensity(
        SurfaceShadingPoint point,
        PhongMaterial inMedium,
        PhongMaterial outMedium,
        Vector3Df in,
        Vector3Df out,
        int components,
        double[] density,
        double[] survival)
    {
        density[0] = survival[0] = 0.0;
        if ( !point.hasShadingNormal() ) {
            Logger.reportMessage("PhongMaterial", Logger.WARNING, "scatteringDensity", "Couldn't determine shading normal");
            return;
        }
        final Vector3Df normal = point.getShadingNormal();

        final double[] parts = new double[3];
        final int[] lobeComponents = new int[2];
        samplingProbabilities(point, components, parts, lobeComponents);
        final double texturePart = parts[0];
        final double reflectionPart = parts[1];
        final double transmissionPart = parts[2];
        final int reflectionComponents = lobeComponents[0];
        final int transmissionComponents = lobeComponents[1];

        final double scattering = texturePart + reflectionPart + transmissionPart;
        if ( scattering < NO_SCATTERING ) {
            return;
        }
        survival[0] = scattering;

        final float inIndex = densityIndex(inMedium);
        final float outIndex = densityIndex(outMedium);

        final double[] p = {textureDensity(out, normal)};
        final double[] pSurvival = {0.0};
        density[0] = texturePart * p[0];

        if ( reflection == null ) {
            p[0] = 0.0;
        } else {
            reflection.reflectionDensity(in, out, normal, reflectionComponents, p, pSurvival);
        }
        density[0] += reflectionPart * p[0];

        if ( transmission == null ) {
            p[0] = 0.0;
        } else {
            transmission.transmissionDensity(inIndex, outIndex, in, out, normal, transmissionComponents, p, pSurvival);
        }
        density[0] += transmissionPart * p[0];

        density[0] /= scattering;
    }

    /**
    scatteringEvaluate of each requested component on its own, into
    componentValues[i] for component 1 << i (black for the others, six
    entries); returns their sum
    */
    public ColorRgb scatteringEvaluateComponents(
        SurfaceShadingPoint point,
        PhongMaterial inMedium,
        PhongMaterial outMedium,
        Vector3Df in,
        Vector3Df out,
        int components,
        ColorRgb[] componentValues)
    {
        double r = 0.0;
        double g = 0.0;
        double b = 0.0;
        for ( int i = 0; i < 2 * ScatteringComponent.COMPONENTS_PER_LOBE; i++ ) {
            final int component = 1 << i;
            if ( (components & component) != 0 ) {
                componentValues[i] = scatteringEvaluate(point, inMedium, outMedium, in, out, component);
                r += componentValues[i].getR();
                g += componentValues[i].getG();
                b += componentValues[i].getB();
            } else {
                componentValues[i] = new ColorRgb(0.0, 0.0, 0.0);
            }
        }
        return new ColorRgb(r, g, b);
    }

    /**
    Emitted radiance into `out` of the requested emission components (only
    the diffuse one emits: Kd / pi), and, when density is not null, the
    density of `out` under emissionSample. Black when the point or its
    normal is unknown (reported), or `out` is below the surface.
    */
    public ColorRgb emissionEvaluate(
        SurfaceShadingPoint point,
        Vector3Df out,
        int components,
        double[] density)
    {
        if ( density != null ) {
            density[0] = 0.0;
        }
        if ( point == null || !point.hasShadingNormal() ) {
            Logger.reportMessage("PhongMaterial", Logger.WARNING, "emissionEvaluate", "Couldn't determine shading normal");
            return new ColorRgb(0.0, 0.0, 0.0);
        }

        final double cosL = ScatteringArithmetic.dot(out, point.getShadingNormal());
        if ( cosL < 0.0 || emission == null ) {
            return new ColorRgb(0.0, 0.0, 0.0);
        }

        if ( (components & ScatteringComponent.DIFFUSE) != 0 ) {
            // kd = Kd / pi, with 1 / pi in single precision as RenderPark stores it
            final double toRadiance = 1.00F / (float) M_PI;
            if ( density != null ) {
                density[0] = cosL / M_PI;
            }
            return new ColorRgb(
                0.0 + toRadiance * emission.getDiffuse().getR(),
                0.0 + toRadiance * emission.getDiffuse().getG(),
                0.0 + toRadiance * emission.getDiffuse().getB());
        }
        return new ColorRgb(0.0, 0.0, 0.0);
    }

    /**
    Samples an emitted direction (cosine distributed around the normal when
    the diffuse component is requested; (0, 0, 1) otherwise or when the
    normal is unknown); the radiance along it and its density go to the
    non-null one element arrays
    */
    public Vector3Df emissionSample(
        SurfaceShadingPoint point,
        int components,
        double xi1,
        double xi2,
        ColorRgb[] radiance,
        double[] density)
    {
        if ( radiance != null ) {
            radiance[0] = new ColorRgb(0.0, 0.0, 0.0);
        }
        if ( density != null ) {
            density[0] = 0.0;
        }
        if ( (components & ScatteringComponent.DIFFUSE) == 0 ) {
            return new Vector3Df(0.0F, 0.0F, 1.0F);
        }
        if ( point == null || !point.hasShadingNormal() ) {
            Logger.reportMessage("PhongMaterial", Logger.WARNING, "emissionSample", "Couldn't determine shading normal");
            return new Vector3Df(0.0F, 0.0F, 1.0F);
        }

        final double[] sampleDensity = {0.0};
        final Vector3Df direction = new ScatteringFrame(point.getShadingNormal()).sampleCosine(xi1, xi2, sampleDensity);
        if ( density != null ) {
            density[0] = sampleDensity[0];
        }
        if ( radiance != null && emission != null ) {
            final double toRadiance = 1.0F / (float) M_PI;
            radiance[0] = new ColorRgb(
                toRadiance * emission.getDiffuse().getR(),
                toRadiance * emission.getDiffuse().getG(),
                toRadiance * emission.getDiffuse().getB());
        }
        return direction;
    }
}

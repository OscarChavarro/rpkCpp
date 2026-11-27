package vsdk.toolkit.environment.material;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
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

Ownership and copies. The material owns its lobes and its name; the texture
is shared and read-only. Construction adopts the lobes passed in (null for
an absent lobe). The material is immutable once built, so one instance can
be read from any number of threads. copy() is a deep copy of the lobes and
the name that shares the texture. The geometric transforms of Material do not
change a surface material and return the material itself.

Output parameters (`density`, `survival`, `radiance`) are one-element
holders; where cpp accepts a null pointer, a null holder is accepted.
*/
public class PhongMaterial implements Material {
    /**
    The texture color replaces this component of the scattered power
    when a texture is present.
    */
    public static final int TEXTURED_COMPONENT = ScatteringComponent.REFLECTION_DIFFUSE;

    // Scattering below which nothing is sampled (RenderPark's EPSILON)
    private static final double NO_SCATTERING = 1e-6;

    /** The part of the scattering a first variate chose */
    private enum SampledPart { TEXTURE_PART, REFLECTION_PART, TRANSMISSION_PART, ABSORPTION }

    private final String name;
    private final boolean sided;
    private final PhongLobe reflection;
    private final PhongLobe transmission;
    private final PhongLobe emission;
    private final Texture texture;
    private final double storedRefractionIndexReal;
    private final double storedRefractionIndexImaginary;

    public PhongMaterial(
        String inName,
        PhongLobe inReflection,
        PhongLobe inTransmission,
        PhongLobe inEmission,
        Texture inTexture,
        boolean inSided)
    {
        this(inName, inReflection, inTransmission, inEmission, inTexture, inSided, 1.0, 0.0);
    }

    /** As above, with the complex refraction index of the transmission lobe */
    public PhongMaterial(
        String inName,
        PhongLobe inReflection,
        PhongLobe inTransmission,
        PhongLobe inEmission,
        Texture inTexture,
        boolean inSided,
        double inRefractionIndexReal,
        double inRefractionIndexImaginary)
    {
        name = inName != null ? inName : "";
        sided = inSided;
        reflection = inReflection;
        transmission = inTransmission;
        emission = inEmission;
        texture = inTexture;
        storedRefractionIndexReal = inRefractionIndexReal;
        storedRefractionIndexImaginary = inRefractionIndexImaginary;
    }

    private static boolean isFiniteColor(ColorRgb color)
    {
        return Double.isFinite((double)((color.getR() + color.getG() + color.getB()) / 3.0));
    }

    private static PhongLobe copyLobe(PhongLobe lobe)
    {
        return lobe != null ? new PhongLobe(lobe) : null;
    }

    private static double averageOf(ColorRgb color)
    {
        return (color.getR() + color.getG() + color.getB()) / 3.0;
    }

    /** Refraction index of a medium in evaluation and sampling: 0 when unknown */
    private static double sampledIndex(PhongMaterial medium)
    {
        return medium != null ? medium.getRefractionIndex() : 0.0;
    }

    /** Refraction index of a medium in densityOf: vacuum when unknown */
    private static double densityIndex(PhongMaterial medium)
    {
        return medium != null ? medium.getRefractionIndex() : 1.0;
    }

    /** Direction sampled within the chosen part, and its density there (0 when none) */
    private static Vector3Dd sampleWithinPart(
        SampledPart part,
        PhongLobe reflection,
        PhongLobe transmission,
        Vector3Dd normal,
        Vector3Dd in,
        double inIndex,
        double outIndex,
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
        return new Vector3Dd(0.0, 0.0, 0.0);
    }

    /** Density of the texture's diffuse scattering: cos / pi */
    private static double textureDensity(Vector3Dd out, Vector3Dd normal)
    {
        return ScatteringArithmetic.dot(normal, out) / Math.PI;
    }

    public String getName()
    {
        return name;
    }

    public boolean isSided()
    {
        return sided;
    }

    public boolean isTextured()
    {
        return texture != null;
    }

    public boolean hasEmission()
    {
        return emission != null;
    }

    public PhongLobe getReflection()
    {
        return reflection;
    }

    public PhongLobe getTransmission()
    {
        return transmission;
    }

    public PhongLobe getEmission()
    {
        return emission;
    }

    public Texture getTexture()
    {
        return texture;
    }

    /**
    Real part of the refraction index seen by a ray that crosses the
    surface: the transmission lobe's, 1 (vacuum) without one
    */
    public double getRefractionIndex()
    {
        return transmission != null ? storedRefractionIndexReal : 1.0;
    }

    /** Imaginary part of the transmission lobe's refraction index, 0 without one */
    public double getRefractionIndexImaginary()
    {
        return transmission != null ? storedRefractionIndexImaginary : 0.0;
    }

    /** Stored refraction index, also without transmission lobe (for persistence) */
    public double getStoredRefractionIndexReal()
    {
        return storedRefractionIndexReal;
    }

    public double getStoredRefractionIndexImaginary()
    {
        return storedRefractionIndexImaginary;
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
    public ColorRgb scatteredPower(int components, Vector3Dd textureCoordinate)
    {
        double r = 0.0;
        double g = 0.0;
        double b = 0.0;

        if ( texture != null && (components & TEXTURED_COMPONENT) != 0 ) {
            if ( textureCoordinate == null ) {
                Logger.reportMessage(this, Logger.WARNING, "scatteredPower", "Couldn't get texture coordinates");
            }
            else {
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
                Logger.reportMessage(this, Logger.FATAL_ERROR, "scatteredPower", "Reflectance is not finite");
            }
            r += reflectance.getR();
            g += reflectance.getG();
            b += reflectance.getB();
        }

        if ( transmission != null ) {
            final ColorRgb transmittance = transmission.integrated(ScatteringComponent.transmissionComponents(components));
            if ( !isFiniteColor(transmittance) ) {
                Logger.reportMessage(this, Logger.FATAL_ERROR, "scatteredPower", "Transmittance is not finite");
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
    public ColorRgb emittance(int components)
    {
        if ( emission == null ) {
            return new ColorRgb(0.0, 0.0, 0.0);
        }
        return emission.integrated(components);
    }

    /** Texture color at the point; black without texture or texture coordinate (reported) */
    private ColorRgb textureColor(SurfaceShadingPoint point)
    {
        if ( texture == null ) {
            return new ColorRgb(0.0, 0.0, 0.0);
        }
        if ( !point.hasTextureCoordinate() ) {
            Logger.reportMessage(this, Logger.WARNING, "textureColor", "Couldn't get texture coordinates");
            return new ColorRgb(0.0, 0.0, 0.0);
        }
        return texture.evaluateColor(point.getTextureCoordinate().x(), point.getTextureCoordinate().y());
    }

    /**
    Average albedos of the texture, of the reflection without the textured
    component, and of the transmission, and the components left to each lobe
    (one-element output holders)
    */
    private void samplingProbabilities(
        SurfaceShadingPoint point,
        int components,
        double[] texturePart,
        double[] reflectionPart,
        double[] transmissionPart,
        int[] reflectionComponents,
        int[] transmissionComponents)
    {
        texturePart[0] = 0.0;
        if ( texture != null && (components & TEXTURED_COMPONENT) != 0 ) {
            texturePart[0] = averageOf(textureColor(point));
            components &= ~TEXTURED_COMPONENT;
        }
        reflectionComponents[0] = ScatteringComponent.reflectionComponents(components);
        transmissionComponents[0] = ScatteringComponent.transmissionComponents(components);
        reflectionPart[0] = reflection != null ? averageOf(reflection.integrated(reflectionComponents[0])) : 0.0;
        transmissionPart[0] = transmission != null ? averageOf(transmission.integrated(transmissionComponents[0])) : 0.0;
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
        Vector3Dd in,
        Vector3Dd out,
        int components)
    {
        double r = 0.0;
        double g = 0.0;
        double b = 0.0;
        if ( !point.hasShadingNormal() ) {
            Logger.reportMessage(this, Logger.WARNING, "scatteringEvaluate", "Couldn't determine shading normal");
            return new ColorRgb(r, g, b);
        }
        final Vector3Dd normal = point.getShadingNormal();

        if ( texture != null && (components & TEXTURED_COMPONENT) != 0 ) {
            final double textureValue = 1.0 / Math.PI;
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
            }
            else {
                r += 0.0;
                g += 0.0;
                b += 0.0;
            }
        }
        return new ColorRgb(r, g, b);
    }

    /**
    Samples a scattered direction: the first variate chooses the texture,
    the reflection or the transmission in proportion to their average
    albedos (and absorption, with Russian roulette), then the chosen part
    samples the direction; density[0] is the density of the direction summed
    over all parts, 0 when no direction was produced
    */
    public Vector3Dd scatteringSample(
        SurfaceShadingPoint point,
        PhongMaterial inMedium,
        PhongMaterial outMedium,
        Vector3Dd in,
        boolean russianRoulette,
        int components,
        double x1,
        double x2,
        double[] density)
    {
        density[0] = 0;
        if ( !point.hasShadingNormal() ) {
            Logger.reportMessage(this, Logger.WARNING, "scatteringSample", "Couldn't determine shading normal");
            return new Vector3Dd(0.0, 0.0, 1.0);
        }
        final Vector3Dd normal = point.getShadingNormal();

        final double[] texturePartHolder = {0.0};
        final double[] reflectionPartHolder = {0.0};
        final double[] transmissionPartHolder = {0.0};
        final int[] reflectionComponentsHolder = {0};
        final int[] transmissionComponentsHolder = {0};
        samplingProbabilities(point, components, texturePartHolder, reflectionPartHolder, transmissionPartHolder,
            reflectionComponentsHolder, transmissionComponentsHolder);
        double texturePart = texturePartHolder[0];
        double reflectionPart = reflectionPartHolder[0];
        double transmissionPart = transmissionPartHolder[0];
        final int reflectionComponents = reflectionComponentsHolder[0];
        final int transmissionComponents = transmissionComponentsHolder[0];
        final double scattering = texturePart + reflectionPart + transmissionPart;
        if ( scattering < NO_SCATTERING ) {
            return new Vector3Dd(0.0, 0.0, 0.0);
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
        }
        else {
            x1 -= texturePart;
            if ( x1 < reflectionPart ) {
                mode = SampledPart.REFLECTION_PART;
                x1 /= reflectionPart;
            }
            else {
                x1 -= reflectionPart;
                if ( x1 < transmissionPart ) {
                    mode = SampledPart.TRANSMISSION_PART;
                    x1 /= transmissionPart;
                }
            }
        }

        final double inIndex = sampledIndex(inMedium);
        final double outIndex = sampledIndex(outMedium);

        final double[] p = {0.0};
        final Vector3Dd out = sampleWithinPart(
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
            }
            else {
                reflection.reflectionDensity(in, out, normal, reflectionComponents, p, survival);
            }
            density[0] += reflectionPart * p[0];
        }
        if ( mode != SampledPart.TRANSMISSION_PART ) {
            if ( transmission == null ) {
                p[0] = 0.0;
            }
            else {
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
        Vector3Dd in,
        Vector3Dd out,
        int components,
        double[] density,
        double[] survival)
    {
        density[0] = survival[0] = 0.0;
        if ( !point.hasShadingNormal() ) {
            Logger.reportMessage(this, Logger.WARNING, "scatteringDensity", "Couldn't determine shading normal");
            return;
        }
        final Vector3Dd normal = point.getShadingNormal();

        final double[] texturePart = {0.0};
        final double[] reflectionPart = {0.0};
        final double[] transmissionPart = {0.0};
        final int[] reflectionComponents = {0};
        final int[] transmissionComponents = {0};
        samplingProbabilities(point, components, texturePart, reflectionPart, transmissionPart,
            reflectionComponents, transmissionComponents);
        final double scattering = texturePart[0] + reflectionPart[0] + transmissionPart[0];
        if ( scattering < NO_SCATTERING ) {
            return;
        }
        survival[0] = scattering;

        final double inIndex = densityIndex(inMedium);
        final double outIndex = densityIndex(outMedium);
        final double[] p = {textureDensity(out, normal)};
        final double[] pSurvival = {0.0};
        density[0] = texturePart[0] * p[0];
        if ( reflection == null ) {
            p[0] = 0.0;
        }
        else {
            reflection.reflectionDensity(in, out, normal, reflectionComponents[0], p, pSurvival);
        }
        density[0] += reflectionPart[0] * p[0];
        if ( transmission == null ) {
            p[0] = 0.0;
        }
        else {
            transmission.transmissionDensity(inIndex, outIndex, in, out, normal, transmissionComponents[0], p, pSurvival);
        }
        density[0] += transmissionPart[0] * p[0];
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
        Vector3Dd in,
        Vector3Dd out,
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
            }
            else {
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
        Vector3Dd out,
        int components,
        double[] density)
    {
        if ( density != null ) {
            density[0] = 0.0;
        }
        if ( point == null || !point.hasShadingNormal() ) {
            Logger.reportMessage(this, Logger.WARNING, "emissionEvaluate", "Couldn't determine shading normal");
            return new ColorRgb(0.0, 0.0, 0.0);
        }
        final double cosL = ScatteringArithmetic.dot(out, point.getShadingNormal());
        if ( cosL < 0.0 || emission == null ) {
            return new ColorRgb(0.0, 0.0, 0.0);
        }
        if ( (components & ScatteringComponent.DIFFUSE) != 0 ) {
            // kd = Kd / pi, with 1 / pi in single precision as RenderPark stores it
            final double toRadiance = 1.00 / Math.PI;
            if ( density != null ) {
                density[0] = cosL / Math.PI;
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
    non-null holders
    */
    public Vector3Dd emissionSample(
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
            return new Vector3Dd(0.0, 0.0, 1.0);
        }
        if ( point == null || !point.hasShadingNormal() ) {
            Logger.reportMessage(this, Logger.WARNING, "emissionSample", "Couldn't determine shading normal");
            return new Vector3Dd(0.0, 0.0, 1.0);
        }
        final double[] sampleDensity = {0.0};
        final Vector3Dd direction = new ScatteringFrame(point.getShadingNormal()).sampleCosine(xi1, xi2, sampleDensity);
        if ( density != null ) {
            density[0] = sampleDensity[0];
        }
        if ( radiance != null && emission != null ) {
            final double toRadiance = 1.0 / Math.PI;
            radiance[0] = new ColorRgb(
                toRadiance * emission.getDiffuse().getR(),
                toRadiance * emission.getDiffuse().getG(),
                toRadiance * emission.getDiffuse().getB());
        }
        return direction;
    }

    @Override
    public Material copy()
    {
        return new PhongMaterial(name, copyLobe(reflection), copyLobe(transmission), copyLobe(emission), texture, sided,
            storedRefractionIndexReal, storedRefractionIndexImaginary);
    }

    @Override
    public Material translate(Vector3Dd vector)
    {
        return this;
    }

    @Override
    public Material rotate(Vector3Dd vector)
    {
        return this;
    }

    @Override
    public Material scale(Vector3Dd vector)
    {
        return this;
    }
}

package vsdk.toolkit.environment.material;

import vsdk.toolkit.common.Entity;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

public class SimpleMaterial extends Entity implements Material
{
    private final ColorRgb ambient;
    private final ColorRgb diffuse;
    private final ColorRgb specular;
    private final ColorRgb emission;
    private final ColorRgb transmittance;
    private final boolean doubleSided;
    private final double reflectionCoefficient;
    private final double refractionCoefficient; // Also known as "transmission"
    private final double indexOfRefraction;
    private final String name;
    private final double opacity;
    private final double phongExponent;
    
    /** Creates a new instance of MaterialGL */
    public SimpleMaterial()
    {
        this(
            "VSDK_default_material",
            new ColorRgb(0.1, 0.1, 0.1),
            new ColorRgb(0.9, 0.5, 0.5),
            new ColorRgb(1, 1, 1),
            new ColorRgb(0, 0, 0),
            new ColorRgb(0, 0, 0),
            true,
            0.0,
            0.0,
            1.0,
            1.0,
            128.0);
    }

    public SimpleMaterial(SimpleMaterial m)
    {
        this(
            m.name,
            m.ambient,
            m.diffuse,
            m.specular,
            m.emission,
            m.transmittance,
            m.doubleSided,
            m.reflectionCoefficient,
            m.refractionCoefficient,
            m.indexOfRefraction,
            m.opacity,
            m.phongExponent);
    }

    public SimpleMaterial(
        String name,
        ColorRgb ambient,
        ColorRgb diffuse,
        ColorRgb specular,
        ColorRgb emission,
        ColorRgb transmittance,
        boolean doubleSided,
        double reflectionCoefficient,
        double refractionCoefficient,
        double indexOfRefraction,
        double opacity,
        double phongExponent)
    {
        this.name = name;
        this.ambient = new ColorRgb(ambient);
        this.diffuse = new ColorRgb(diffuse);
        this.specular = new ColorRgb(specular);
        this.emission = new ColorRgb(emission);
        this.transmittance = new ColorRgb(transmittance);
        this.doubleSided = doubleSided;
        this.reflectionCoefficient = reflectionCoefficient;
        this.refractionCoefficient = refractionCoefficient;
        this.indexOfRefraction = indexOfRefraction;
        this.opacity = opacity;
        this.phongExponent = phongExponent;
    }

    public SimpleMaterial(
        String name,
        ColorRgb ambient,
        ColorRgb diffuse,
        ColorRgb specular,
        boolean doubleSided,
        double reflectionCoefficient,
        double refractionCoefficient,
        double opacity,
        double phongExponent)
    {
        this(
            name,
            ambient,
            diffuse,
            specular,
            new ColorRgb(0, 0, 0),
            new ColorRgb(0, 0, 0),
            doubleSided,
            reflectionCoefficient,
            refractionCoefficient,
            1.0,
            opacity,
            phongExponent);
    }

    public String getName()
    {
        return name;
    }

    public SimpleMaterial withName(String n)
    {
        return new SimpleMaterial(
            n, ambient, diffuse, specular, emission, transmittance, doubleSided,
            reflectionCoefficient, refractionCoefficient, indexOfRefraction, opacity, phongExponent);
    }

    public SimpleMaterial withAmbient(ColorRgb a)
    {
        return new SimpleMaterial(
            name, a, diffuse, specular, emission, transmittance, doubleSided,
            reflectionCoefficient, refractionCoefficient, indexOfRefraction, opacity, phongExponent);
    }

    public SimpleMaterial withDiffuse(ColorRgb d)
    {
        return new SimpleMaterial(
            name, ambient, d, specular, emission, transmittance, doubleSided,
            reflectionCoefficient, refractionCoefficient, indexOfRefraction, opacity, phongExponent);
    }

    public SimpleMaterial withSpecular(ColorRgb s)
    {
        return new SimpleMaterial(
            name, ambient, diffuse, s, emission, transmittance, doubleSided,
            reflectionCoefficient, refractionCoefficient, indexOfRefraction, opacity, phongExponent);
    }

    public SimpleMaterial withEmission(ColorRgb e)
    {
        return new SimpleMaterial(
            name, ambient, diffuse, specular, e, transmittance, doubleSided,
            reflectionCoefficient, refractionCoefficient, indexOfRefraction, opacity, phongExponent);
    }

    public SimpleMaterial withTransmittance(ColorRgb t)
    {
        return new SimpleMaterial(
            name, ambient, diffuse, specular, emission, t, doubleSided,
            reflectionCoefficient, refractionCoefficient, indexOfRefraction, opacity, phongExponent);
    }

    public SimpleMaterial withPhongExponent(double p)
    {
        return new SimpleMaterial(
            name, ambient, diffuse, specular, emission, transmittance, doubleSided,
            reflectionCoefficient, refractionCoefficient, indexOfRefraction, opacity, p);
    }

    public SimpleMaterial withReflectionCoefficient(double kr)
    {
        return new SimpleMaterial(
            name, ambient, diffuse, specular, emission, transmittance, doubleSided,
            kr, refractionCoefficient, indexOfRefraction, opacity, phongExponent);
    }

    public SimpleMaterial withRefractionCoefficient(double kr)
    {
        return new SimpleMaterial(
            name, ambient, diffuse, specular, emission, transmittance, doubleSided,
            reflectionCoefficient, kr, indexOfRefraction, opacity, phongExponent);
    }

    public SimpleMaterial withIndexOfRefraction(double ior)
    {
        return new SimpleMaterial(
            name, ambient, diffuse, specular, emission, transmittance, doubleSided,
            reflectionCoefficient, refractionCoefficient, ior, opacity, phongExponent);
    }

    public SimpleMaterial withOpacity(double a)
    {
        return new SimpleMaterial(
            name, ambient, diffuse, specular, emission, transmittance, doubleSided,
            reflectionCoefficient, refractionCoefficient, indexOfRefraction, a, phongExponent);
    }

    public boolean isDoubleSided()
    {
        return doubleSided;
    }

    @Override
    public Material copy()
    {
        return new SimpleMaterial(this);
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

    public SimpleMaterial withDoubleSided(boolean doubleSided)
    {
        return new SimpleMaterial(
            name, ambient, diffuse, specular, emission, transmittance, doubleSided,
            reflectionCoefficient, refractionCoefficient, indexOfRefraction, opacity, phongExponent);
    }

    public ColorRgb getAmbient()
    {
        return new ColorRgb(ambient);
    }

    public ColorRgb getAmbientReference()
    {
        return ambient;
    }

    public ColorRgb getDiffuse()
    {
        return new ColorRgb(diffuse);
    }

    public ColorRgb getDiffuseReference()
    {
        return diffuse;
    }

    public ColorRgb getSpecular()
    {
        return new ColorRgb(specular);
    }

    public ColorRgb getSpecularReference()
    {
        return specular;
    }

    public ColorRgb getEmission()
    {
        return new ColorRgb(emission);
    }

    public ColorRgb getEmissionReference()
    {
        return emission;
    }

    public ColorRgb getTransmittance()
    {
        return new ColorRgb(transmittance);
    }

    public ColorRgb getTransmittanceReference()
    {
        return transmittance;
    }

    public double getPhongExponent()
    {
        return phongExponent;
    }

    public double getReflectionCoefficient()
    {
        return reflectionCoefficient;
    }

    public double getRefractionCoefficient()
    {
        return refractionCoefficient;
    }

    public double getIndexOfRefraction()
    {
        return indexOfRefraction;
    }

    public double getOpacity()
    {
        return opacity;
    }

    /**
    Provides an object to text report conversion, optimized for human
    readability and debugging. Do not use for serialization or persistence
    purposes.
    @return human-readable representation of current SimpleMaterial information
    */
    @Override
    public String toString()
    {
        return "SimpleMaterial [" + name + "]:\n" +
               "  - Specular " + specular + "\n" +
               "  - Diffuse " + diffuse + "\n" +
               "  - Ambient " + ambient + "\n" +
               "  - Phong exponent: " + phongExponent + "\n" +
               (isDoubleSided()?"  - Double sided\n":"  - Single sided\n") +
               "\n";
    }
}

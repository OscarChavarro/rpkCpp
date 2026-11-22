package vsdk.toolkit.environment.material;

/**
Bit masks naming the components of a scattering or emission model, as the
integrated quantities of PhongMaterial select them.

A single lobe (reflection, transmission or emission) has three components:
DIFFUSE, GLOSSY and SPECULAR. A scattering request covers both lobes in one
mask: the reflection components in the three low bits and the transmission
components in the next three, so that a request for the diffuse reflection
and the diffuse transmission is REFLECTION_DIFFUSE | TRANSMISSION_DIFFUSE.
The values match RenderPark's BRDF/BTDF component flags bit for bit.
*/
public final class ScatteringComponent {
    public static final int NONE = 0;
    public static final int DIFFUSE = 1;
    public static final int GLOSSY = 2;
    public static final int SPECULAR = 4;
    public static final int ALL = DIFFUSE | GLOSSY | SPECULAR;
    public static final int COMPONENTS_PER_LOBE = 3;

    public static final int REFLECTION_DIFFUSE = DIFFUSE;
    public static final int REFLECTION_GLOSSY = GLOSSY;
    public static final int REFLECTION_SPECULAR = SPECULAR;
    public static final int TRANSMISSION_DIFFUSE = DIFFUSE << COMPONENTS_PER_LOBE;
    public static final int TRANSMISSION_GLOSSY = GLOSSY << COMPONENTS_PER_LOBE;
    public static final int TRANSMISSION_SPECULAR = SPECULAR << COMPONENTS_PER_LOBE;

    private ScatteringComponent()
    {
    }

    public static int reflectionComponents(int scatteringComponents)
    {
        return scatteringComponents & ALL;
    }

    public static int transmissionComponents(int scatteringComponents)
    {
        return (scatteringComponents >> COMPONENTS_PER_LOBE) & ALL;
    }
}

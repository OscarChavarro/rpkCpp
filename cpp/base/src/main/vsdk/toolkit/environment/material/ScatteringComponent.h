#ifndef __SCATTERING_COMPONENT__
#define __SCATTERING_COMPONENT__

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
class ScatteringComponent {
  public:
    enum {
        NONE = 0,
        DIFFUSE = 1,
        GLOSSY = 2,
        SPECULAR = 4,
        ALL = DIFFUSE | GLOSSY | SPECULAR,
        COMPONENTS_PER_LOBE = 3,

        REFLECTION_DIFFUSE = DIFFUSE,
        REFLECTION_GLOSSY = GLOSSY,
        REFLECTION_SPECULAR = SPECULAR,
        TRANSMISSION_DIFFUSE = DIFFUSE << COMPONENTS_PER_LOBE,
        TRANSMISSION_GLOSSY = GLOSSY << COMPONENTS_PER_LOBE,
        TRANSMISSION_SPECULAR = SPECULAR << COMPONENTS_PER_LOBE
    };

    static int reflectionComponents(int scatteringComponents);
    static int transmissionComponents(int scatteringComponents);
};

inline int
ScatteringComponent::reflectionComponents(const int scatteringComponents) {
    return scatteringComponents & ALL;
}

inline int
ScatteringComponent::transmissionComponents(const int scatteringComponents) {
    return (scatteringComponents >> COMPONENTS_PER_LOBE) & ALL;
}

#endif

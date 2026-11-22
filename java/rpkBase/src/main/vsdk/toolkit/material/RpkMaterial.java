package vsdk.toolkit.material;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.environment.material.PhongLobe;
import vsdk.toolkit.environment.material.PhongMaterial;
import vsdk.toolkit.environment.material.Texture;

/**
RenderPark material: the VITRAL PhongMaterial that holds its lobes and
answers every integrated and directional query (plan steps 2a and 2b), and
the two RenderPark views of it the raytracers call, with RenderPark's
argument types: the scattering view (null when the material has no BSDF)
and the emission view (null when it does not emit). Each material has its
own views, so a view identifies its material (the raytracers use them as
media).
*/
public class RpkMaterial {
    private final String name; // RpkMaterial name
    private final boolean sided; // True for 1-sided surface, false for 2-sided, see mgf docs
    private final Texture texture; // Shared read-only by the PhongMaterial
    private final PhongMaterial surfaceMaterial;
    private PhongEmittanceDistributionFunction edf; // View, null when not emitting
    private PhongBidirectionalScatteringDistributionFunction bsdf; // View, null without BSDF

    private static boolean isBlack(ColorRgb c) {
        return Math.abs(c.getR()) < VSDK.EPSILON
            && Math.abs(c.getG()) < VSDK.EPSILON
            && Math.abs(c.getB()) < VSDK.EPSILON;
    }

    /**
    Adopts the lobes (null for an absent one) and the texture. hasBsdf
    tells whether the material scatters at all (RenderPark's BSDF, possibly
    without reflection and transmission lobes); the refraction index is the
    transmission lobe's.
    */
    public RpkMaterial(
        String inName,
        PhongLobe emission,
        boolean hasBsdf,
        PhongLobe reflection,
        PhongLobe transmission,
        Texture inTexture,
        float refractionIndexReal,
        float refractionIndexImaginary,
        boolean inSided)
    {
        name = inName;
        sided = inSided;
        texture = inTexture;
        surfaceMaterial = new PhongMaterial(
            inName, reflection, transmission, emission, texture, inSided, refractionIndexReal, refractionIndexImaginary);
        edf = null;
        bsdf = null;
        if ( emission != null ) {
            if ( !isBlack(emission.getNonDiffuse()) ) {
                Logger.reportMessage("PhongEmittanceDistributionFunction", Logger.WARNING, "phongEdfCreate", "Non-diffuse light sources not yet implemented");
            }
            edf = new PhongEmittanceDistributionFunction(surfaceMaterial);
        }
        if ( hasBsdf ) {
            bsdf = new PhongBidirectionalScatteringDistributionFunction(surfaceMaterial);
        }
    }

    public PhongEmittanceDistributionFunction getEdf() {
        return edf;
    }

    public PhongBidirectionalScatteringDistributionFunction getBsdf() {
        return bsdf;
    }

    public boolean isSided() {
        return sided;
    }

    public String getName() {
        return name;
    }

    public PhongMaterial getSurfaceMaterial() {
        return surfaceMaterial;
    }
}

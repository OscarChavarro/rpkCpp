package vsdk.toolkit.numericalAnalysis;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RpkRayHit;
import vsdk.toolkit.environment.material.PhongMaterial;
import vsdk.toolkit.numericalAnalysis.quasiMonteCarlo.Niederreiter31;
/**
Use next function (with PatchListIterate) to close any open files of the patch use for recording
Computes average scattered power and emittance of the Patch
*/


public class PatchVisitor {
    private static int getNumberOfSamples(Patch patch) {
        int numberOfSamples = 1;
        if ( patch.getMaterial().getSurfaceMaterial().isTextured() ) {
            if (patch.getVertices()[0].textureCoordinates == patch.getVertices()[1].textureCoordinates
                && patch.getVertices()[0].textureCoordinates == patch.getVertices()[2].textureCoordinates
                && (patch.getNumberOfVertices() == 3 || patch.getVertices()[0].textureCoordinates == patch.getVertices()[3].textureCoordinates)
                && patch.getVertices()[0].textureCoordinates != null) {
                // All vertices have same texture coordinates (important special case)
                numberOfSamples = 1;
            }
            else {
                numberOfSamples = 100;
            }
        }
        return numberOfSamples;
    }

    /**
    Use next function (with PatchListIterate) to close any open files of the patch use for recording
    Computes average scattered power and emittance of the Patch
    */
    public static ColorRgb averageNormalAlbedo(Patch patch, int components) {
        ColorRgb albedo = new ColorRgb(0.0, 0.0, 0.0);
        final PhongMaterial material = patch.getMaterial().getSurfaceMaterial();
        RpkRayHit hit = new RpkRayHit();

        hit.init(patch, patch.midPoint(), patch.getNormal(), patch.getMaterial());

        final int numberOfSamples = getNumberOfSamples(patch);
        albedo = new ColorRgb(0.0, 0.0, 0.0);
        for ( int i = 0; i < numberOfSamples; i++ ) {
            ColorRgb sample;
            final long[] xi = Niederreiter31.niederreiter31(i);
            hit.setUv(xi[0] * Niederreiter31.RECIP, xi[1] * Niederreiter31.RECIP);
            if ( material.isTextured() ) {
                Vector3Dd[] texCoord = {new Vector3Dd()};
                if ( hit.getTexCoord(texCoord) ) {
                    final Vector3Dd textureCoordinate = texCoord[0];
                    sample = new ColorRgb(material.scatteredPower(components, textureCoordinate));
                } else {
                    sample = new ColorRgb(material.scatteredPower(components, null));
                }
            } else {
                sample = new ColorRgb(material.scatteredPower(components, null));
            }
            albedo = albedo.add(sample);
        }
        albedo = albedo.scaleInverse((double)numberOfSamples);

        return albedo;
    }

    public static ColorRgb averageEmittance(Patch patch, int components) {
        ColorRgb emittance = new ColorRgb(0.0, 0.0, 0.0);
        final PhongMaterial material = patch.getMaterial().getSurfaceMaterial();

        final int numberOfSamples = getNumberOfSamples(patch);
        emittance = new ColorRgb(0.0, 0.0, 0.0);
        for ( int i = 0; i < numberOfSamples; i++ ) {
            ColorRgb sample = new ColorRgb(material.emittance(components));
            emittance = emittance.add(sample);
        }
        emittance = emittance.scaleInverse((double)numberOfSamples);

        return emittance;
    }
}

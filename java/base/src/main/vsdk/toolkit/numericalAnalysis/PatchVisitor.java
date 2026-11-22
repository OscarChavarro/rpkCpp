package vsdk.toolkit.numericalAnalysis;

import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RayHit;
import vsdk.toolkit.environment.material.PhongMaterial;
import vsdk.toolkit.numericalAnalysis.quasiMonteCarlo.Niederreiter31;
/**
Use next function (with PatchListIterate) to close any open files of the patch use for recording
Computes average scattered power and emittance of the Patch
*/


public class PatchVisitor {
    private static int getNumberOfSamples(Patch patch) {
        int numberOfSamples = 1;
        if ( patch.material.getSurfaceMaterial().isTextured() ) {
            if (patch.vertex[0].textureCoordinates == patch.vertex[1].textureCoordinates
                && patch.vertex[0].textureCoordinates == patch.vertex[2].textureCoordinates
                && (patch.numberOfVertices == 3 || patch.vertex[0].textureCoordinates == patch.vertex[3].textureCoordinates)
                && patch.vertex[0].textureCoordinates != null) {
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
    public static ColorRgbMutable averageNormalAlbedo(Patch patch, int components) {
        ColorRgbMutable albedo = new ColorRgbMutable(0.0, 0.0, 0.0);
        final PhongMaterial material = patch.material.getSurfaceMaterial();
        RayHit hit = new RayHit();

        hit.init(patch, patch.midPoint, patch.normal, patch.material);

        final int numberOfSamples = getNumberOfSamples(patch);
        albedo.clear();
        for ( int i = 0; i < numberOfSamples; i++ ) {
            ColorRgbMutable sample;
            final long[] xi = Niederreiter31.niederreiter31(i);
            hit.setUv(xi[0] * Niederreiter31.RECIP, xi[1] * Niederreiter31.RECIP);
            if ( material.isTextured() ) {
                Vector3D texCoord = new Vector3D();
                if ( hit.getTexCoord(texCoord) ) {
                    final Vector3Df textureCoordinate = new Vector3Df(texCoord.x, texCoord.y, texCoord.z);
                    sample = new ColorRgbMutable(material.scatteredPower(components, textureCoordinate));
                } else {
                    sample = new ColorRgbMutable(material.scatteredPower(components, null));
                }
            } else {
                sample = new ColorRgbMutable(material.scatteredPower(components, null));
            }
            albedo.add(albedo, sample);
        }
        albedo.scaleInverse((float) numberOfSamples, albedo);

        return albedo;
    }

    public static ColorRgbMutable averageEmittance(Patch patch, int components) {
        ColorRgbMutable emittance = new ColorRgbMutable(0.0, 0.0, 0.0);
        final PhongMaterial material = patch.material.getSurfaceMaterial();

        final int numberOfSamples = getNumberOfSamples(patch);
        emittance.clear();
        for ( int i = 0; i < numberOfSamples; i++ ) {
            ColorRgbMutable sample = new ColorRgbMutable(material.emittance(components));
            emittance.add(emittance, sample);
        }
        emittance.scaleInverse((float) numberOfSamples, emittance);

        return emittance;
    }
}

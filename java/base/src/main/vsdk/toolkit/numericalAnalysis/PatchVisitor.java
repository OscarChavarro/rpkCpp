package vsdk.toolkit.numericalAnalysis;

import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RayHit;
import vsdk.toolkit.numericalAnalysis.quasiMonteCarlo.Niederreiter31;
/**
Use next function (with PatchListIterate) to close any open files of the patch use for recording
Computes average scattered power and emittance of the Patch
*/


public class PatchVisitor {
    private static int getNumberOfSamples(Patch patch) {
        int numberOfSamples = 1;
        if (patch.material.getBsdf() != null && patch.material.getBsdf().splitBsdfIsTextured()) {
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
        ColorRgbMutable albedo = new ColorRgbMutable();
        RayHit hit = new RayHit();

        hit.init(patch, patch.midPoint, patch.normal, patch.material);

        final int numberOfSamples = getNumberOfSamples(patch);
        albedo.clear();
        for (int i = 0; i < numberOfSamples; i++) {
            ColorRgbMutable sample;
            long[] xi = Niederreiter31.niederreiter31(i);
            hit.setUv(xi[0] * Niederreiter31.RECIP, xi[1] * Niederreiter31.RECIP);
            Vector3D position = hit.getPoint();
            patch.pointBarycentricMapping(hit.getUv().x, hit.getUv().y, position);
            sample = new ColorRgbMutable();
            if (patch.material.getBsdf() != null) {
                sample = new ColorRgbMutable(patch.material.getBsdf().splitBsdfScatteredPower(hit.shadingContext(), components));
            }
            albedo.add(albedo, new ColorRgbMutable(sample));
        }
        albedo.scaleInverse(numberOfSamples, albedo);

        return new ColorRgbMutable(albedo);
    }

    public static ColorRgbMutable averageEmittance(Patch patch, int components) {
        ColorRgbMutable emittance = new ColorRgbMutable();
        RayHit hit = new RayHit();
        hit.init(patch, patch.midPoint, patch.normal, patch.material);

        final int numberOfSamples = getNumberOfSamples(patch);
        emittance.clear();
        for (int i = 0; i < numberOfSamples; i++) {
            ColorRgbMutable sample = new ColorRgbMutable();
            long[] xi = Niederreiter31.niederreiter31(i);
            hit.setUv(xi[0] * Niederreiter31.RECIP, xi[1] * Niederreiter31.RECIP);
            Vector3D position = hit.getPoint();
            patch.pointBarycentricMapping(hit.getUv().x, hit.getUv().y, position);

            if (patch.material.getEdf() == null) {
                sample = new ColorRgbMutable();
            }
            else {
                sample = new ColorRgbMutable(patch.material.getEdf().phongEmittance(hit.shadingContext(), components));
            }
            emittance.add(emittance, new ColorRgbMutable(sample));
        }
        emittance.scaleInverse(numberOfSamples, emittance);

        return new ColorRgbMutable(emittance);
    }
}

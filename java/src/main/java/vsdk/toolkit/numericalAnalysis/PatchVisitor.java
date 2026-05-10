package vsdk.toolkit.numericalAnalysis;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.skin.RayHitFlag;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.RayHit;
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
    public static ColorRgb averageNormalAlbedo(Patch patch, int components) {
        ColorRgb albedo = new ColorRgb();
        RayHit hit = new RayHit();

        hit.init(patch, patch.midPoint, patch.normal, patch.material);

        final int numberOfSamples = getNumberOfSamples(patch);
        albedo.clear();
        for (int i = 0; i < numberOfSamples; i++) {
            ColorRgb sample;
            long[] xi = Niederreiter31.niederreiter31(i);
            hit.setUv(xi[0] * Niederreiter31.RECIP, xi[1] * Niederreiter31.RECIP);
            int newFlags = hit.getFlags() | RayHitFlag.UV;
            hit.setFlags(newFlags);
            Vector3D position = hit.getPoint();
            patch.pointBarycentricMapping(hit.getUv().u, hit.getUv().v, position);
            sample = new ColorRgb();
            sample.clear();
            if (patch.material.getBsdf() != null) {
                sample = patch.material.getBsdf().splitBsdfScatteredPower(hit, components);
            }
            albedo.add(albedo, sample);
        }
        albedo.scaleInverse(numberOfSamples, albedo);

        return albedo;
    }

    public static ColorRgb averageEmittance(Patch patch, int components) {
        ColorRgb emittance = new ColorRgb();
        RayHit hit = new RayHit();
        hit.init(patch, patch.midPoint, patch.normal, patch.material);

        final int numberOfSamples = getNumberOfSamples(patch);
        emittance.clear();
        for (int i = 0; i < numberOfSamples; i++) {
            ColorRgb sample = new ColorRgb();
            long[] xi = Niederreiter31.niederreiter31(i);
            hit.setUv(xi[0] * Niederreiter31.RECIP, xi[1] * Niederreiter31.RECIP);
            int newFlags = hit.getFlags() | RayHitFlag.UV;
            hit.setFlags(newFlags);
            Vector3D position = hit.getPoint();
            patch.pointBarycentricMapping(hit.getUv().u, hit.getUv().v, position);

            if (patch.material.getEdf() == null) {
                sample.clear();
            }
            else {
                sample = patch.material.getEdf().phongEmittance(hit, components);
            }
            emittance.add(emittance, sample);
        }
        emittance.scaleInverse(numberOfSamples, emittance);

        return emittance;
    }
}

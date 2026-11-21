#include "vsdk/toolkit/numericalAnalysis/quasiMonteCarlo/Niederreiter31.h"
#include "vsdk/toolkit/numericalAnalysis/PatchVisitor.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkRayHit.h"
#include "vsdk/toolkit/environment/material/PhongMaterial.h"

int
PatchVisitor::getNumberOfSamples(const Patch *patch) {
    int numberOfSamples = 1;
    if ( patch->getMaterial()->getSurfaceMaterial()->isTextured() ) {
        if ( patch->getVertices()[0]->textureCoordinates == patch->getVertices()[1]->textureCoordinates &&
             patch->getVertices()[0]->textureCoordinates == patch->getVertices()[2]->textureCoordinates &&
             (patch->getNumberOfVertices() == 3 || patch->getVertices()[0]->textureCoordinates == patch->getVertices()[3]->textureCoordinates) &&
             patch->getVertices()[0]->textureCoordinates != nullptr ) {
            // All vertices have same texture coordinates (important special case)
            numberOfSamples = 1;
        } else {
            numberOfSamples = 100;
        }
    }
    return numberOfSamples;
}

/**
Average scattered power of the patch's material over the patch, for the
requested BSDF components: the VITRAL PhongMaterial's integrated reflectance
and transmittance, sampled at quasi-random points when a texture makes it
vary over the patch.

Each sample sets its own (u,v) on the hit record, which invalidates the
texture coordinate cached for the previous sample, so every sample reads the
texel at its own position.
*/
ColorRgbMutable
PatchVisitor::averageNormalAlbedo(const Patch *patch, char components) {
    ColorRgbMutable albedo(0.0, 0.0, 0.0);
    const PhongMaterial *material = patch->getMaterial()->getSurfaceMaterial();
    RpkRayHit hit;

    hit.init(patch, &patch->midPoint(), &patch->getNormal(), patch->getMaterial());

    const int numberOfSamples = getNumberOfSamples(patch);
    albedo.clear();
    for ( int i = 0; i < numberOfSamples; i++ ) {
        ColorRgbMutable sample(0.0, 0.0, 0.0);
        const unsigned *xi = Niederreiter31::niederreiter31(i);
        hit.setUv(xi[0] * Niederreiter31::RECIP, xi[1] * Niederreiter31::RECIP);
        if ( material->isTextured() ) {
            Vector3D texCoord;
            if ( hit.getTexCoord(&texCoord) ) {
                const Vector3Df textureCoordinate(texCoord.x, texCoord.y, texCoord.z);
                sample = material->scatteredPower(components, &textureCoordinate);
            } else {
                sample = material->scatteredPower(components, nullptr);
            }
        } else {
            sample = material->scatteredPower(components, nullptr);
        }
        albedo.add(albedo, sample);
    }
    albedo.scaleInverse(static_cast<float>(numberOfSamples), albedo);

    return albedo;
}

/**
Average emittance of the patch's material for the requested components. The
emittance does not vary over the patch, but it is still accumulated over the
same number of samples as the albedo so that the rounding of the result is
the established one.
*/
ColorRgbMutable
PatchVisitor::averageEmittance(const Patch *patch, char components) {
    ColorRgbMutable emittance(0.0, 0.0, 0.0);
    const PhongMaterial *material = patch->getMaterial()->getSurfaceMaterial();

    const int numberOfSamples = getNumberOfSamples(patch);
    emittance.clear();
    for ( int i = 0; i < numberOfSamples; i++ ) {
        ColorRgbMutable sample(0.0, 0.0, 0.0);
        sample = material->emittance(components);
        emittance.add(emittance, sample);
    }
    emittance.scaleInverse(static_cast<float>(numberOfSamples), emittance);

    return emittance;
}

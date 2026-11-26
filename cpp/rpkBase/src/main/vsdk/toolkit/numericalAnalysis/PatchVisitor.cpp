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
ColorRgb
PatchVisitor::averageNormalAlbedo(const Patch *patch, char components) {
    double albedo[3] = {0.0, 0.0, 0.0};
    const PhongMaterial *material = patch->getMaterial()->getSurfaceMaterial();
    RpkRayHit hit;

    const Vector3Dd patchMidPoint = patch->midPoint();
    const Vector3Dd patchNormal = patch->getNormal();
    hit.init(patch, &patchMidPoint, &patchNormal, patch->getMaterial());

    const int numberOfSamples = getNumberOfSamples(patch);
    for ( int i = 0; i < numberOfSamples; i++ ) {
        ColorRgb sample(0.0, 0.0, 0.0);
        const unsigned *xi = Niederreiter31::niederreiter31(i);
        hit.setUv(xi[0] * Niederreiter31::RECIP, xi[1] * Niederreiter31::RECIP);
        if ( material->isTextured() ) {
            Vector3Dd texCoord;
            if ( hit.getTexCoord(&texCoord) ) {
                const Vector3Dd textureCoordinate(texCoord.x(), texCoord.y(), texCoord.z());
                sample = material->scatteredPower(components, &textureCoordinate);
            } else {
                sample = material->scatteredPower(components, nullptr);
            }
        } else {
            sample = material->scatteredPower(components, nullptr);
        }
        albedo[0] += sample.getR();
        albedo[1] += sample.getG();
        albedo[2] += sample.getB();
    }
    const double inverse = 1.0 / static_cast<double>(numberOfSamples);
    return ColorRgb(inverse * albedo[0], inverse * albedo[1], inverse * albedo[2]);
}

/**
Average emittance of the patch's material for the requested components. The
emittance does not vary over the patch, but it is still accumulated over the
same number of samples as the albedo so that the rounding of the result is
the established one.
*/
ColorRgb
PatchVisitor::averageEmittance(const Patch *patch, char components) {
    double emittance[3] = {0.0, 0.0, 0.0};
    const PhongMaterial *material = patch->getMaterial()->getSurfaceMaterial();

    const int numberOfSamples = getNumberOfSamples(patch);
    for ( int i = 0; i < numberOfSamples; i++ ) {
        const ColorRgb sample = material->emittance(components);
        emittance[0] += sample.getR();
        emittance[1] += sample.getG();
        emittance[2] += sample.getB();
    }
    const double inverse = 1.0 / static_cast<double>(numberOfSamples);
    return ColorRgb(inverse * emittance[0], inverse * emittance[1], inverse * emittance[2]);
}

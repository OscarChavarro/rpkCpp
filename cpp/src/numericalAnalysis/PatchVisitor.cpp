#include "numericalAnalysis/quasiMonteCarlo/Niederreiter31.h"
#include "numericalAnalysis/PatchVisitor.h"

int
PatchVisitor::getNumberOfSamples(Patch *patch) {
    int numberOfSamples = 1;
    if ( patch->material->getBsdf() != nullptr && patch->material->getBsdf()->splitBsdfIsTextured() ) {
        if ( patch->vertex[0]->textureCoordinates == patch->vertex[1]->textureCoordinates &&
             patch->vertex[0]->textureCoordinates == patch->vertex[2]->textureCoordinates &&
             (patch->numberOfVertices == 3 || patch->vertex[0]->textureCoordinates == patch->vertex[3]->textureCoordinates) &&
             patch->vertex[0]->textureCoordinates != nullptr ) {
            // All vertices have same texture coordinates (important special case)
            numberOfSamples = 1;
        } else {
            numberOfSamples = 100;
        }
    }
    return numberOfSamples;
}

/**
Use next function (with PatchListIterate) to close any open files of the patch use for recording
Computes average scattered power and emittance of the Patch
*/
ColorRgb
PatchVisitor::averageNormalAlbedo(Patch *patch, char components) {
    ColorRgb albedo;
    RayHit hit;

    hit.init(patch, &patch->midPoint(), &patch->normal, patch->material);

    const int numberOfSamples = getNumberOfSamples(patch);
    albedo.clear();
    for ( int i = 0; i < numberOfSamples; i++ ) {
        ColorRgb sample;
        const unsigned *xi = Niederreiter31::niederreiter31(i);
        hit.setUv(xi[0] * Niederreiter31::RECIP, xi[1] * Niederreiter31::RECIP);
        unsigned int newFlags = hit.getFlags() | RayHitFlag::UV;
        hit.setFlags(newFlags);
        Vector3D position = hit.getPoint();
        patch->pointBarycentricMapping(hit.getUv().u, hit.getUv().v, &position);
        sample.clear();
        if ( patch->material->getBsdf() != nullptr ) {
            sample = patch->material->getBsdf()->splitBsdfScatteredPower(&hit, components);
        }
        albedo.add(albedo, sample);
    }
    albedo.scaleInverse(static_cast<float>(numberOfSamples), albedo);

    return albedo;
}

ColorRgb
PatchVisitor::averageEmittance(Patch *patch, char components) {
    ColorRgb emittance;
    RayHit hit;
    hit.init(patch, &patch->midPoint(), &patch->normal, patch->material);

    const int numberOfSamples = getNumberOfSamples(patch);
    emittance.clear();
    for ( int i = 0; i < numberOfSamples; i++ ) {
        ColorRgb sample;
        const unsigned *xi = Niederreiter31::niederreiter31(i);
        hit.setUv(xi[0] * Niederreiter31::RECIP, xi[1] * Niederreiter31::RECIP);
        unsigned int newFlags = hit.getFlags() | RayHitFlag::UV;
        hit.setFlags(newFlags);
        Vector3D position = hit.getPoint();
        patch->pointBarycentricMapping(hit.getUv().u, hit.getUv().v, &position);

        if ( patch->material->getEdf() == nullptr ) {
            sample.clear();
        } else {
            sample = patch->material->getEdf()->phongEmittance(&hit, components);
        }
        emittance.add(emittance, sample);
    }
    emittance.scaleInverse(static_cast<float>(numberOfSamples), emittance);

    return emittance;
}

#include "numericalAnalysis/quasiMonteCarlo/Niederreiter31.h"
#include "numericalAnalysis/PatchVisitor.h"

int
PatchVisitor::getNumberOfSamples(Patch *patch) {
    int numberOfSamples = 1;
    if ( patch->material->getBsdf() != NULL && patch->material->getBsdf()->splitBsdfIsTextured() ) {
        if ( patch->vertex[0]->textureCoordinates == patch->vertex[1]->textureCoordinates &&
             patch->vertex[0]->textureCoordinates == patch->vertex[2]->textureCoordinates &&
             (patch->numberOfVertices == 3 || patch->vertex[0]->textureCoordinates == patch->vertex[3]->textureCoordinates) &&
             patch->vertex[0]->textureCoordinates != NULL ) {
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
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    RayHit hit;

    hit.init(patch, &patch->midPoint, &patch->normal, patch->material);

    const int numberOfSamples = getNumberOfSamples(patch);
    for ( int i = 0; i < numberOfSamples; i++ ) {
        ColorRgb sample(0.0f, 0.0f, 0.0f);
        const unsigned *xi = Niederreiter31::niederreiter31(i);
        hit.setUv(xi[0] * Niederreiter31::RECIP, xi[1] * Niederreiter31::RECIP);
        unsigned int newFlags = hit.getFlags() | UV;
        hit.setFlags(newFlags);
        Vector3D position = hit.getPoint();
        patch->pointBarycentricMapping(hit.getUv().u, hit.getUv().v, &position);
        if ( patch->material->getBsdf() != NULL ) {
            sample = patch->material->getBsdf()->splitBsdfScatteredPower(&hit, components);
        }
        r += sample.getR();
        g += sample.getG();
        b += sample.getB();
    }
    const float inv = 1.0f / ((float)(numberOfSamples));
    return ColorRgb(r * inv, g * inv, b * inv);
}

ColorRgb
PatchVisitor::averageEmittance(Patch *patch, char components) {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    RayHit hit;
    hit.init(patch, &patch->midPoint, &patch->normal, patch->material);

    const int numberOfSamples = getNumberOfSamples(patch);
    for ( int i = 0; i < numberOfSamples; i++ ) {
        ColorRgb sample(0.0f, 0.0f, 0.0f);
        const unsigned *xi = Niederreiter31::niederreiter31(i);
        hit.setUv(xi[0] * Niederreiter31::RECIP, xi[1] * Niederreiter31::RECIP);
        unsigned int newFlags = hit.getFlags() | UV;
        hit.setFlags(newFlags);
        Vector3D position = hit.getPoint();
        patch->pointBarycentricMapping(hit.getUv().u, hit.getUv().v, &position);

        if ( patch->material->getEdf() != NULL ) {
            sample = patch->material->getEdf()->phongEmittance(&hit, components);
        }
        r += sample.getR();
        g += sample.getG();
        b += sample.getB();
    }
    const float inv = 1.0f / ((float)(numberOfSamples));
    return ColorRgb(r * inv, g * inv, b * inv);
}

#include "numericalAnalysis/quasiMonteCarlo/Niederreiter31.h"
#include "numericalAnalysis/PatchVisitor.h"
#include "material/ShadingContext.h"
#include "environment/geometry/elements/RayHit.h"
#include "environment/geometry/elements/RayHitFlag.h"

int
PatchVisitor::getNumberOfSamples(Patch *patch) {
    int numberOfSamples = 1;
    if ( patch->getMaterial()->getBsdf() != nullptr && patch->getMaterial()->getBsdf()->splitBsdfIsTextured() ) {
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
Use next function (with PatchListIterate) to close any open files of the patch use for recording
Computes average scattered power and emittance of the Patch
*/
ColorRgb
PatchVisitor::averageNormalAlbedo(Patch *patch, char components) {
    ColorRgb albedo;
    RayHit hit;

    hit.init(patch, &patch->midPoint(), &patch->getNormal(), patch->getMaterial());

    const int numberOfSamples = getNumberOfSamples(patch);
    albedo.clear();
    for ( int i = 0; i < numberOfSamples; i++ ) {
        ColorRgb sample;
        const unsigned *xi = Niederreiter31::niederreiter31(i);
        hit.setUv(xi[0] * Niederreiter31::RECIP, xi[1] * Niederreiter31::RECIP);
        const unsigned int newFlags = hit.getFlags() | RayHitFlag::UV;
        hit.setFlags(newFlags);
        Vector3D position = hit.getPoint();
        patch->pointBarycentricMapping(hit.getUv().u, hit.getUv().v, &position);
        sample.clear();
        if ( patch->getMaterial()->getBsdf() != nullptr ) {
            Vector3D normal;
            hit.shadingNormal(&normal);
            Vector3D texCoord;
            unsigned int shFlags = SHCTX_NORMAL;
            if ( hit.getTexCoord(&texCoord) ) {
                shFlags |= SHCTX_TEXTURE_COORDINATE;
            } else {
                texCoord.set(0.0, 0.0, 0.0);
            }
            ShadingContext context(
                hit.getPoint(),
                hit.getGeometricNormal(),
                normal,
                texCoord,
                hit.getUv(),
                hit.getShadingFrame(),
                hit.getMaterial(),
                shFlags);
            sample = patch->getMaterial()->getBsdf()->splitBsdfScatteredPower(context, components);
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
    hit.init(patch, &patch->midPoint(), &patch->getNormal(), patch->getMaterial());

    const int numberOfSamples = getNumberOfSamples(patch);
    emittance.clear();
    for ( int i = 0; i < numberOfSamples; i++ ) {
        ColorRgb sample;
        const unsigned *xi = Niederreiter31::niederreiter31(i);
        hit.setUv(xi[0] * Niederreiter31::RECIP, xi[1] * Niederreiter31::RECIP);
        const unsigned int newFlags = hit.getFlags() | RayHitFlag::UV;
        hit.setFlags(newFlags);
        Vector3D position = hit.getPoint();
        patch->pointBarycentricMapping(hit.getUv().u, hit.getUv().v, &position);

        if ( patch->getMaterial()->getEdf() == nullptr ) {
            sample.clear();
        } else {
            Vector3D normal;
            hit.shadingNormal(&normal);
            Vector3D texCoord;
            unsigned int shFlags = SHCTX_NORMAL;
            if ( hit.getTexCoord(&texCoord) ) {
                shFlags |= SHCTX_TEXTURE_COORDINATE;
            } else {
                texCoord.set(0.0, 0.0, 0.0);
            }
            ShadingContext context(
                hit.getPoint(),
                hit.getGeometricNormal(),
                normal,
                texCoord,
                hit.getUv(),
                hit.getShadingFrame(),
                hit.getMaterial(),
                shFlags);
            sample = patch->getMaterial()->getEdf()->phongEmittance(&context, components);
        }
        emittance.add(emittance, sample);
    }
    emittance.scaleInverse(static_cast<float>(numberOfSamples), emittance);

    return emittance;
}

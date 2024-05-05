/**
Implementation of a BSDF consisting of one brdf and one bsdf. Either of the components may be nullptr
*/

#include "common/error.h"
#include "common/linealAlgebra/CoordinateSystem.h"
#include "material/SplitBidirectionalScatteringDistributionFunction.h"

// Texture modulates diffuse reflection
#define TEXTURED_COMPONENT BRDF_DIFFUSE_COMPONENT

ColorRgb
SplitBidirectionalScatteringDistributionFunction::splitBsdfEvalTexture(const Texture *texture,  RayHit *hit) {
    Vector3D texCoord;
    ColorRgb col;
    col.clear();

    if ( texture == nullptr ) {
        return col;
    }

    if ( hit == nullptr || !hit->getTexCoord(&texCoord) ) {
        logWarning("splitBsdfEvalTexture", "Couldn't get texture coordinates");
        return col;
    }

    return texture->evaluateColor(texCoord.x, texCoord.y);
}

double
SplitBidirectionalScatteringDistributionFunction::texturedScattererEval(
    const Vector3D * /*in*/,
    const Vector3D * /*out*/,
    const Vector3D * /*normal*/)
{
    return (1.0 / M_PI);
}

/**
Albedo is assumed to be 1
*/
Vector3D
SplitBidirectionalScatteringDistributionFunction::texturedScattererSample(
    const Vector3D * /*in*/,
    const Vector3D *normal,
    double x1,
    double x2,
    double *probabilityDensityFunction)
{
    CoordinateSystem coord;
    coord.setFromZAxis(normal);
    return coord.sampleHemisphereCosTheta(x1, x2, probabilityDensityFunction);
}

void
SplitBidirectionalScatteringDistributionFunction::texturedScattererEvalPdf(
    const Vector3D * /*in*/,
    const Vector3D *out,
    const Vector3D *normal,
    double *probabilityDensityFunction)
{
    *probabilityDensityFunction = vectorDotProduct(*normal, *out) / M_PI;
}

/**
sample a split bsdf. If no sample was taken (RR/absorption)
the pdf will be 0 upon return  Computes probabilities for sampling the texture, reflection minus texture,
or transmission. Also determines b[r|t]dfFlags taking into
account potential texturing
*/
void
SplitBidirectionalScatteringDistributionFunction::splitBsdfProbabilities(
    const BidirectionalScatteringDistributionFunction *bsdf,
    RayHit *hit,
    BSDF_FLAGS flags,
    double *texture,
    double *reflection,
    double *transmission,
    char *brdfFlags,
    char *btdfFlags)
{
    *texture = 0.0;
    if ( bsdf->texture && (flags & TEXTURED_COMPONENT) ) {
        // bsdf has a texture for diffuse reflection and diffuse reflection needs to be sampled
        ColorRgb textureColor;
        textureColor = SplitBidirectionalScatteringDistributionFunction::splitBsdfEvalTexture(bsdf->texture, hit);
        *texture = textureColor.average();
        flags &= ~TEXTURED_COMPONENT;
    }

    *brdfFlags = GET_BRDF_FLAGS(flags);
    *btdfFlags = GET_BTDF_FLAGS(flags);

    ColorRgb reflectance;
    if ( bsdf->brdf == nullptr ) {
        reflectance.clear();
    } else {
        reflectance = bsdf->brdf->reflectance(*brdfFlags);
    }
    *reflection = reflectance.average();

    ColorRgb transmittance;
    if ( bsdf->btdf == nullptr ) {
        transmittance.clear();
    } else {
        transmittance = bsdf->btdf->transmittance(*btdfFlags);
    }
    *transmission = transmittance.average();
}

SplitBSDFSamplingMode
SplitBidirectionalScatteringDistributionFunction::splitBsdfSamplingMode(double texture, double reflection, double transmission, double *x1) {
    SplitBSDFSamplingMode mode = SplitBSDFSamplingMode::SAMPLE_ABSORPTION;
    if ( *x1 < texture ) {
        mode = SplitBSDFSamplingMode::SAMPLE_TEXTURE;
        *x1 /= texture; // Rescale into [0,1) interval again
    } else {
        *x1 -= texture;
        if ( *x1 < reflection ) {
            mode = SplitBSDFSamplingMode::SAMPLE_REFLECTION;
            *x1 /= reflection;
        } else {
            *x1 -= reflection;
            if ( *x1 < transmission ) {
                mode = SplitBSDFSamplingMode::SAMPLE_TRANSMISSION;
                *x1 /= transmission;
            }
        }
    }
    return mode;
}

/**
Returns the scattered power (diffuse/glossy/specular
reflectance and/or transmittance) according to flags
*/
ColorRgb
SplitBidirectionalScatteringDistributionFunction::splitBsdfScatteredPower(
    const BidirectionalScatteringDistributionFunction *bsdf, RayHit *hit, char flags) {
    ColorRgb albedo;
    albedo.clear();

    if ( bsdf->texture && (flags & TEXTURED_COMPONENT) ) {
        ColorRgb textureColor = SplitBidirectionalScatteringDistributionFunction::splitBsdfEvalTexture(bsdf->texture, hit);
        albedo.add(albedo, textureColor);
        flags &= ~TEXTURED_COMPONENT; // Avoid taking it into account again
    }

    if ( bsdf->brdf ) {
        ColorRgb reflectance = bsdf->brdf->reflectance(flags);
        if ( !std::isfinite(reflectance.average()) ) {
            logFatal(-1, "brdfReflectance", "Oops - test Rd is not finite!");
        }

        albedo.add(albedo, reflectance);
    }

    if ( bsdf->btdf != nullptr ) {
        ColorRgb transmitted = bsdf->btdf->transmittance(GET_BTDF_FLAGS(flags));
        albedo.add(albedo, transmitted);
    }

    return albedo;
}

void
SplitBidirectionalScatteringDistributionFunction::indexOfRefraction(const BidirectionalScatteringDistributionFunction *bsdf, RefractionIndex *index) {
    if ( bsdf->btdf == nullptr ) {
        index->nr = 1.0; // Vacuum
        index->ni = 0.0;
    } else {
        bsdf->btdf->indexOfRefraction(index);
    }
}

/**
Bsdf evaluations
All components of the Bsdf

Vector directions :

in: from patch
out: from patch
hit->normal : leaving from patch, on the incoming side.
         So in . hit->normal > 0!
*/
ColorRgb
SplitBidirectionalScatteringDistributionFunction::evaluate(
    const BidirectionalScatteringDistributionFunction *bsdf,
    RayHit *hit,
    const BidirectionalScatteringDistributionFunction *inBsdf,
    const BidirectionalScatteringDistributionFunction *outBsdf,
    const Vector3D *in,
    const Vector3D *out,
    char flags)
{
    ColorRgb result;
    Vector3D normal;

    result.clear();
    if ( !hit->shadingNormal(&normal) ) {
        logWarning("evaluate", "Couldn't determine shading normal");
        return result;
    }

    if ( bsdf->texture && (flags & TEXTURED_COMPONENT) ) {
        double textureBsdf = SplitBidirectionalScatteringDistributionFunction::texturedScattererEval(
            in, out, &normal);
        ColorRgb textureCol = SplitBidirectionalScatteringDistributionFunction::splitBsdfEvalTexture(bsdf->texture, hit);
        result.addScaled(result, (float) textureBsdf, textureCol);
        flags &= ~TEXTURED_COMPONENT;
    }

    // Just add brdf and btdf contributions, the eval routines handle the direction of out.
    // Note that out * normal is computed more than once :-(
    if ( bsdf->brdf != nullptr ) {
        ColorRgb reflectionCol = bsdf->brdf->evaluate(in, out, &normal, GET_BRDF_FLAGS(flags));
        result.add(result, reflectionCol);

        RefractionIndex inIndex{};
        RefractionIndex outIndex{};
        ColorRgb refractionCol;
        bsdfIndexOfRefraction(inBsdf, &inIndex);
        bsdfIndexOfRefraction(outBsdf, &outIndex);

        if ( bsdf->btdf == nullptr ) {
            refractionCol.clear();
        } else {
            refractionCol = bsdf->btdf->evaluate(
                inIndex, outIndex, in, out, &normal, GET_BTDF_FLAGS(flags));
        }

        result.add(result, refractionCol);
    }

    return result;
}

Vector3D
SplitBidirectionalScatteringDistributionFunction::sample(
    const BidirectionalScatteringDistributionFunction *bsdf,
    RayHit *hit,
    const BidirectionalScatteringDistributionFunction *inBsdf,
    const BidirectionalScatteringDistributionFunction *outBsdf,
    const Vector3D *in,
    int doRussianRoulette,
    char flags,
    double x1,
    double x2,
    double *probabilityDensityFunction)
{
    Vector3D normal;
    Vector3D out;

    *probabilityDensityFunction = 0; // So we can return safely
    if ( !hit->shadingNormal(&normal) ) {
        logWarning("sample", "Couldn't determine shading normal");
        out.set(0.0, 0.0, 1.0);
        return out;
    }

    // Calculate probabilities for sampling the texture, reflection minus texture,
    // and transmission. Also fills in correct b[r|t]dfFlags
    double texture;
    double reflection;
    double transmission;
    char brdfFlags;
    char btdfFlags;
    SplitBidirectionalScatteringDistributionFunction::splitBsdfProbabilities(
        bsdf,
        hit,
        flags,
        &texture,
        &reflection,
        &transmission,
        &brdfFlags,
        &btdfFlags);

    double scattering = texture + reflection + transmission;
    if ( scattering < EPSILON ) {
        return out;
    }

    // Decide whether to sample the texture reflectance, the reflectance
    // modes not in the texture, transmission or absorption
    if ( !doRussianRoulette ) {
        // Normalize: no absorption sampling
        texture /= scattering;
        reflection /= scattering;
        transmission /= scattering;
    }

    SplitBSDFSamplingMode mode = SplitBidirectionalScatteringDistributionFunction::splitBsdfSamplingMode(
        texture, reflection, transmission, &x1);
    RefractionIndex inIndex{};
    RefractionIndex outIndex{};

    bsdfIndexOfRefraction(inBsdf, &inIndex);
    bsdfIndexOfRefraction(outBsdf, &outIndex);

    // Sample according to the selected mode
    double p;
    switch ( mode ) {
        case SplitBSDFSamplingMode::SAMPLE_TEXTURE:
            out = SplitBidirectionalScatteringDistributionFunction::texturedScattererSample(in, &normal, x1, x2, &p);
            if ( p < EPSILON ) {
                // Don´t care
                return out;
            }
            *probabilityDensityFunction = texture * p; /* other components will be added later */
            break;
        case SplitBSDFSamplingMode::SAMPLE_REFLECTION:
            if ( bsdf->brdf == nullptr ) {
                p = 0.0;
            } else {
                out = bsdf->brdf->sample(in, &normal, false, brdfFlags, x1, x2, &p);
            }
            if ( p < EPSILON )
                return out;
            *probabilityDensityFunction = reflection * p;
            break;
        case SplitBSDFSamplingMode::SAMPLE_TRANSMISSION:
            if ( bsdf->btdf == nullptr ) {
                p = 0.0;
                out.x = 0.0f;
                out.y = 0.0f;
                out.z = 0.0f;
            } else {
                out = bsdf->btdf->sample(inIndex, outIndex, in, &normal, false, btdfFlags, x1, x2, &p);
            }
            if ( p < EPSILON ) {
                return out;
            }
            *probabilityDensityFunction = transmission * p;
            break;
        case SplitBSDFSamplingMode::SAMPLE_ABSORPTION:
            *probabilityDensityFunction = 0;
            return out;
    }

    // Add probability of sampling the same direction in other than the
    // selected scattering mode (e.g. internal reflection) */
    if ( mode != SplitBSDFSamplingMode::SAMPLE_TEXTURE ) {
        SplitBidirectionalScatteringDistributionFunction::texturedScattererEvalPdf(in, &out, &normal, &p);
        *probabilityDensityFunction += texture * p;
    }

    double pRR;
    if ( mode != SplitBSDFSamplingMode::SAMPLE_REFLECTION ) {
        if ( bsdf->brdf == nullptr ) {
            p = 0.0;
        } else {
            bsdf->brdf->evaluateProbabilityDensityFunction(in, &out, &normal, brdfFlags, &p, &pRR);
        }
        *probabilityDensityFunction += reflection * p;
    }
    if ( mode != SplitBSDFSamplingMode::SAMPLE_TRANSMISSION ) {
        if ( bsdf->btdf == nullptr ) {
            p = 0.0;
        } else {
            bsdf->btdf->evaluateProbabilityDensityFunction(inIndex, outIndex, in, &out, &normal, btdfFlags, &p, &pRR);
        }
        *probabilityDensityFunction += transmission * p;
    }

    return out;
}

/**
Constructs shading frame at hit point. Returns TRUE if successful and
FALSE if not. X and Y may be null pointers

Sample a split bsdf. If no sample was taken (RR/absorption)
the pdf will be 0 upon return
*/
void
SplitBidirectionalScatteringDistributionFunction::evaluateProbabilityDensityFunction(
    const BidirectionalScatteringDistributionFunction *bsdf,
    RayHit *hit,
    const BidirectionalScatteringDistributionFunction *inBsdf,
    const BidirectionalScatteringDistributionFunction *outBsdf,
    const Vector3D *in,
    const Vector3D *out,
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR)
{
    double pTexture;
    double pReflection;
    double pTransmission;
    double pScattering;
    double p;
    double pRR;
    RefractionIndex inIndex{};
    RefractionIndex outIndex{};
    char brdfFlags;
    char btdfFlags;
    Vector3D normal;

    *probabilityDensityFunction = *probabilityDensityFunctionRR = 0.0; // So we can return safely
    if ( !hit->shadingNormal(&normal) ) {
        logWarning("evaluateProbabilityDensityFunction", "Couldn't determine shading normal");
        return;
    }

    // Calculate probabilities for sampling the texture, reflection minus texture,
    // and transmission. Also fills in correct b[r|t]dfFlags
    SplitBidirectionalScatteringDistributionFunction::splitBsdfProbabilities(
        bsdf,
        hit,
        flags,
        &pTexture,
        &pReflection,
        &pTransmission,
        &brdfFlags,
        &btdfFlags);
    pScattering = pTexture + pReflection + pTransmission;
    if ( pScattering < EPSILON ) {
        return;
    }

    // Survival probability
    *probabilityDensityFunctionRR = pScattering;

    // Probability of sampling the outgoing direction, after survival decision
    bsdfIndexOfRefraction(inBsdf, &inIndex);
    bsdfIndexOfRefraction(outBsdf, &outIndex);

    SplitBidirectionalScatteringDistributionFunction::texturedScattererEvalPdf(in, out, &normal, &p);
    *probabilityDensityFunction = pTexture * p;

    if ( bsdf->brdf == nullptr ) {
        p = 0.0;
    } else {
        bsdf->brdf->evaluateProbabilityDensityFunction(in, out, &normal, brdfFlags, &p, &pRR);
    }
    *probabilityDensityFunction += pReflection * p;

    if ( bsdf->btdf == nullptr ) {
        p = 0.0;
    } else {
        bsdf->btdf->evaluateProbabilityDensityFunction(inIndex, outIndex, in, out, &normal, btdfFlags, &p, &pRR);
    }
    *probabilityDensityFunction += pTransmission * p;

    *probabilityDensityFunction /= pScattering;
}

int
SplitBidirectionalScatteringDistributionFunction::splitBsdfIsTextured(const BidirectionalScatteringDistributionFunction *bsdf) {
    return bsdf->texture != nullptr;
}

/**
Implementation of a BSDF consisting of one brdf and one bsdf. Either of the components may
be nullptr
*/

#include "common/error.h"
#include "material/spherical.h"
#include "material/splitbsdf.h"

// Texture modulates diffuse reflection
#define TEXTURED_COMPONENT BRDF_DIFFUSE_COMPONENT

// Selects sampling mode according to given probabilities and random number x_1.
// If the selected mode is not absorption, x_1 is rescaled to the interval [0,1)
// again
enum SAMPLING_MODE {
    SAMPLE_TEXTURE,
    SAMPLE_REFLECTION,
    SAMPLE_TRANSMISSION,
    SAMPLE_ABSORPTION
};

static ColorRgb
splitBsdfEvalTexture(TEXTURE *texture, RayHit *hit) {
    Vector3D texCoord;
    ColorRgb col;
    colorClear(col);

    if ( !texture ) {
        return col;
    }

    if ( !hit || !hitTexCoord(hit, &texCoord) ) {
        logWarning("splitBsdfEvalTexture", "Couldn't get texture coordinates");
        return col;
    }

    return evalTextureColor(texture, texCoord.x, texCoord.y);
}

static double
texturedScattererEval(Vector3D * /*in*/, Vector3D * /*out*/, Vector3D * /*normal*/) {
    return (1.0 / M_PI);
}

/**
Albedo is assumed to be 1
*/
static Vector3D
texturedScattererSample(Vector3D * /*in*/, Vector3D *normal, double x1, double x2, double *probabilityDensityFunction) {
    CoordSys coord;
    vectorCoordSys(normal, &coord);
    return sampleHemisphereCosTheta(&coord, x1, x2, probabilityDensityFunction);
}

static void
texturedScattererEvalPdf(Vector3D * /*in*/, Vector3D *out, Vector3D *normal, double *probabilityDensityFunction) {
    *probabilityDensityFunction = vectorDotProduct(*normal, *out) / M_PI;
}

/**
Returns the scattered power (diffuse/glossy/specular
reflectance and/or transmittance) according to flags
*/
ColorRgb
splitBsdfScatteredPower(BSDF *bsdf, RayHit *hit, char flags) {
    ColorRgb albedo;
    colorClear(albedo);

    if ( bsdf->texture && (flags & TEXTURED_COMPONENT) ) {
        ColorRgb textureColor = splitBsdfEvalTexture(bsdf->texture, hit);
        colorAdd(albedo, textureColor, albedo);
        flags &= ~TEXTURED_COMPONENT;  /* avoid taking it into account again */
    }

    if ( bsdf->brdf ) {
        ColorRgb reflectance = brdfReflectance(bsdf->brdf, GET_BRDF_FLAGS(flags));
        colorAdd(albedo, reflectance, albedo);
    }

    if ( bsdf->btdf ) {
        ColorRgb trans = btdfTransmittance(bsdf->btdf, GET_BTDF_FLAGS(flags));
        colorAdd(albedo, trans, albedo);
    }

    return albedo;
}

void
splitBsdfIndexOfRefraction(BSDF *bsdf, RefractionIndex *index) {
    btdfIndexOfRefraction(bsdf->btdf, index);
}

ColorRgb
splitBsdfEval(
    BSDF *bsdf,
    RayHit *hit,
    BSDF *inBsdf,
    BSDF *outBsdf,
    Vector3D *in,
    Vector3D *out,
    BSDF_FLAGS flags)
{
    ColorRgb result;
    Vector3D normal;

    colorClear(result);
    if ( !hitShadingNormal(hit, &normal) ) {
        logWarning("splitBsdfEval", "Couldn't determine shading normal");
        return result;
    }

    if ( bsdf->texture && (flags & TEXTURED_COMPONENT) ) {
        double textureBsdf = texturedScattererEval(in, out, &normal);
        ColorRgb textureCol = splitBsdfEvalTexture(bsdf->texture, hit);
        colorAddScaled(result, (float)textureBsdf, textureCol, result);
        flags &= ~TEXTURED_COMPONENT;
    }

    // Just add brdf and btdf contributions, the eval routines
    // handle the direction of out. Note that out * normal is
    // computed more than once :-(
    if ( bsdf->brdf ) {
        ColorRgb reflectionCol = brdfEval(bsdf->brdf, in, out, &normal, GET_BRDF_FLAGS(flags));
        colorAdd(result, reflectionCol, result);
    }

    if ( bsdf->btdf ) {
        RefractionIndex inIndex{};
        RefractionIndex outIndex{};
        ColorRgb refractionCol;
        bsdfIndexOfRefraction(inBsdf, &inIndex);
        bsdfIndexOfRefraction(outBsdf, &outIndex);
        refractionCol = btdfEval(bsdf->btdf, inIndex, outIndex,
                                 in, out, &normal, GET_BTDF_FLAGS(flags));
        colorAdd(result, refractionCol, result);
    }

    return result;
}

/**
sample a split bsdf. If no sample was taken (RR/absorption)
the pdf will be 0 upon return  Computes probabilities for sampling the texture, reflection minus texture,
or transmission. Also determines b[r|t]dfFlags taking into
account potential texturing
*/
static void
splitBsdfProbabilities(
    BSDF *bsdf,
    RayHit *hit,
    BSDF_FLAGS flags,
    double *texture,
    double *reflection,
    double *transmission,
    char *brdfFlags,
    char *btdfFlags)
{
    ColorRgb textureColor;
    ColorRgb reflectance;
    ColorRgb transmittance;

    *texture = 0.0;
    if ( bsdf->texture && (flags & TEXTURED_COMPONENT) ) {
        // bsdf has a texture for diffuse reflection and diffuse reflection
        // needs to be sampled
        textureColor = splitBsdfEvalTexture(bsdf->texture, hit);
        *texture = colorAverage(textureColor);
        flags &= ~TEXTURED_COMPONENT;
    }

    *brdfFlags = GET_BRDF_FLAGS(flags);
    *btdfFlags = GET_BTDF_FLAGS(flags);

    reflectance = brdfReflectance(bsdf->brdf, *brdfFlags);
    *reflection = colorAverage(reflectance);

    transmittance = btdfTransmittance(bsdf->btdf, *btdfFlags);
    *transmission = colorAverage(transmittance);
}

static SAMPLING_MODE
splitBsdfSamplingMode(double texture, double reflection, double transmission, double *x1) {
    SAMPLING_MODE mode = SAMPLE_ABSORPTION;
    if ( *x1 < texture ) {
        mode = SAMPLE_TEXTURE;
        *x1 /= texture; // Rescale into [0,1) interval again
    } else {
        *x1 -= texture;
        if ( *x1 < reflection ) {
            mode = SAMPLE_REFLECTION;
            *x1 /= reflection;
        } else {
            *x1 -= reflection;
            if ( *x1 < transmission ) {
                mode = SAMPLE_TRANSMISSION;
                *x1 /= transmission;
            }
        }
    }
    return mode;
}

Vector3D
splitBsdfSample(
    BSDF *bsdf,
    RayHit *hit,
    BSDF *inBsdf,
    BSDF *outBsdf,
    Vector3D *in,
    int doRussianRoulette,
    BSDF_FLAGS flags,
    double x1,
    double x2,
    double *probabilityDensityFunction)
{
    double texture;
    double reflection;
    double transmission;
    double scattering;
    double p;
    double pRR;
    RefractionIndex inIndex{};
    RefractionIndex outIndex{};
    char brdfFlags;
    char btdfFlags;
    Vector3D normal;
    SAMPLING_MODE mode;
    Vector3D out;

    *probabilityDensityFunction = 0; // So we can return safely
    if ( !hitShadingNormal(hit, &normal) ) {
        logWarning("splitBsdfSample", "Couldn't determine shading normal");
        out.set(0.0, 0.0, 1.0);
        return out;
    }

    // Calculate probabilities for sampling the texture, reflection minus texture,
    // and transmission. Also fills in correct b[r|t]dfFlags. */
    splitBsdfProbabilities(bsdf, hit, flags,
                           &texture, &reflection, &transmission,
                           &brdfFlags, &btdfFlags);
    scattering = texture + reflection + transmission;
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
    mode = splitBsdfSamplingMode(texture, reflection, transmission, &x1);

    bsdfIndexOfRefraction(inBsdf, &inIndex);
    bsdfIndexOfRefraction(outBsdf, &outIndex);

    // Sample according to the selected mode
    switch ( mode ) {
        case SAMPLE_TEXTURE:
            out = texturedScattererSample(in, &normal, x1, x2, &p);
            if ( p < EPSILON ) {
                return out;
            } /* don't care */
            *probabilityDensityFunction = texture * p; /* other components will be added later */
            break;
        case SAMPLE_REFLECTION:
            out = brdfSample(bsdf->brdf, in, &normal, false, brdfFlags, x1, x2, &p);
            if ( p < EPSILON )
                return out;
            *probabilityDensityFunction = reflection * p;
            break;
        case SAMPLE_TRANSMISSION:
            out = btdfSample(bsdf->btdf, inIndex, outIndex, in, &normal,
                             false, btdfFlags, x1, x2, &p);
            if ( p < EPSILON )
                return out;
            *probabilityDensityFunction = transmission * p;
            break;
        case SAMPLE_ABSORPTION:
            *probabilityDensityFunction = 0;
            return out;
        default:
            logFatal(-1, "splitBsdfSample", "Impossible sampling mode %d", mode);
    }

    // Add probability of sampling the same direction in other than the
    // selected scattering mode (e.g. internal reflection) */
    if ( mode != SAMPLE_TEXTURE ) {
        texturedScattererEvalPdf(in, &out, &normal, &p);
        *probabilityDensityFunction += texture * p;
    }
    if ( mode != SAMPLE_REFLECTION ) {
        brdfEvalPdf(bsdf->brdf, in, &out, &normal,
                    brdfFlags, &p, &pRR);
        *probabilityDensityFunction += reflection * p;
    }
    if ( mode != SAMPLE_TRANSMISSION ) {
        btdfEvalPdf(bsdf->btdf, inIndex, outIndex, in, &out, &normal,
                    btdfFlags, &p, &pRR);
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
splitBsdfEvalPdf(
    BSDF *bsdf,
    RayHit *hit,
    BSDF *inBsdf,
    BSDF *outBsdf,
    Vector3D *in,
    Vector3D *out,
    BSDF_FLAGS flags,
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
    if ( !hitShadingNormal(hit, &normal) ) {
        logWarning("splitBsdfEvalPdf", "Couldn't determine shading normal");
        return;
    }

    // Calculate probabilities for sampling the texture, reflection minus texture,
    // and transmission. Also fills in correct b[r|t]dfFlags
    splitBsdfProbabilities(bsdf, hit, flags,
                           &pTexture, &pReflection, &pTransmission,
                           &brdfFlags, &btdfFlags);
    pScattering = pTexture + pReflection + pTransmission;
    if ( pScattering < EPSILON ) {
        return;
    }

    // Survival probability
    *probabilityDensityFunctionRR = pScattering;

    // Probability of sampling the outgoing direction, after survival decision
    bsdfIndexOfRefraction(inBsdf, &inIndex);
    bsdfIndexOfRefraction(outBsdf, &outIndex);

    texturedScattererEvalPdf(in, out, &normal, &p);
    *probabilityDensityFunction = pTexture * p;

    brdfEvalPdf(bsdf->brdf, in, out, &normal,
                brdfFlags, &p, &pRR);
    *probabilityDensityFunction += pReflection * p;

    btdfEvalPdf(bsdf->btdf, inIndex, outIndex, in, out, &normal,
                btdfFlags, &p, &pRR);
    *probabilityDensityFunction += pTransmission * p;

    *probabilityDensityFunction /= pScattering;
}

int
splitBsdfIsTextured(BSDF *bsdf) {
    return bsdf->texture != nullptr;
}

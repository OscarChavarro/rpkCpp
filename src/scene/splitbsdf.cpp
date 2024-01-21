/* splitbsdf.c : implementation of a bsdf consisting of
     one brdf and one bsdf. Either of the components may
     be nullptr */

#include <cstdlib>

#include "common/linealAlgebra/vectorMacros.h"
#include "common/error.h"
#include "material/bsdf.h"
#include "scene/splitbsdf.h"
#include "scene/spherical.h"

/* texture modulates diffuse reflection */
#define TEXTURED_COMPONENT BRDF_DIFFUSE_COMPONENT

/* Selects sampling mode according to given probabilities and random number x_1.
 * If the selected mode is not absorption, x_1 is rescaled to the interval [0,1)
 * again. */
enum SAMPLING_MODE {
    SAMPLE_TEXTURE, SAMPLE_REFLECTION, SAMPLE_TRANSMISSION, SAMPLE_ABSORPTION
};

void
SplitBsdfPrint(FILE *out, SPLIT_BSDF *bsdf) {
    fprintf(out, "Split Bsdf :\n  ");
    brdfPrint(out, bsdf->brdf);
    fprintf(out, "  ");
    btdfPrint(out, bsdf->btdf);
    fprintf(out, "  ");
    PrintTexture(out, bsdf->texture);
}

SPLIT_BSDF *
SplitBSDFCreate(BRDF *brdf, BTDF *btdf, TEXTURE *texture) {
    SPLIT_BSDF *bsdf = (SPLIT_BSDF *)malloc(sizeof(SPLIT_BSDF));

    bsdf->brdf = brdf;
    bsdf->btdf = btdf;
    bsdf->texture = texture;

    return bsdf;
}

static SPLIT_BSDF *
SplitBsdfDuplicate(SPLIT_BSDF *bsdf) {
    SPLIT_BSDF *newBsdf = (SPLIT_BSDF *)malloc(sizeof(SPLIT_BSDF));
    *newBsdf = *bsdf;
    return newBsdf;
}

static void
SplitBsdfDestroy(SPLIT_BSDF *bsdf) {
    free(bsdf);
}

static COLOR
SplitBsdfEvalTexture(TEXTURE *texture, RayHit *hit) {
    Vector3D texCoord;
    COLOR col;
    colorClear(col);

    if ( !texture ) {
        return col;
    }

    if ( !hit || !hitTexCoord(hit, &texCoord)) {
        logWarning("SplitBsdfEvalTexture", "Couldn't get texture coordinates");
        return col;
    }

    return EvalTextureColor(texture, texCoord.x, texCoord.y);
}

static double
TexturedScattererEval(Vector3D *in, Vector3D *out, Vector3D *normal) {
    return (1. / M_PI);
}

/* albedo is assumed to be 1 */
static Vector3D
TexturedScattererSample(Vector3D *in, Vector3D *normal, double x_1, double x_2, double *pdf) {
    COORDSYS coord;
    vectorCoordSys(normal, &coord);
    return sampleHemisphereCosTheta(&coord, x_1, x_2, pdf);
}

static void
TexturedScattererEvalPdf(Vector3D *in, Vector3D *out, Vector3D *normal, double *pdf) {
    *pdf = VECTORDOTPRODUCT(*normal, *out) / M_PI;
}

static COLOR
SplitBsdfScatteredPower(SPLIT_BSDF *bsdf, RayHit *hit, Vector3D *in, BSDFFLAGS flags) {
    COLOR albedo;
    colorClear(albedo);

    if ( bsdf->texture && (flags & TEXTURED_COMPONENT)) {
        COLOR textureColor = SplitBsdfEvalTexture(bsdf->texture, hit);
        colorAdd(albedo, textureColor, albedo);
        flags &= ~TEXTURED_COMPONENT;  /* avoid taking it into account again */
    }

    if ( bsdf->brdf ) {
        COLOR refl = brdfReflectance(bsdf->brdf, GETBRDFFLAGS(flags));
        colorAdd(albedo, refl, albedo);
    }

    if ( bsdf->btdf ) {
        COLOR trans = btdfTransmittance(bsdf->btdf, GETBTDFFLAGS(flags));
        colorAdd(albedo, trans, albedo);
    }

    return albedo;
}

static void
SplitBsdfIndexOfRefraction(SPLIT_BSDF *bsdf, REFRACTIONINDEX *index) {
    btdfIndexOfRefraction(bsdf->btdf, index);
}

static COLOR
SplitBsdfEval(
        SPLIT_BSDF *bsdf,
        RayHit *hit,
        BSDF *inBsdf,
        BSDF *outBsdf,
        Vector3D *in,
        Vector3D *out,
        BSDFFLAGS flags)
{
    COLOR result;
    Vector3D normal;

    colorClear(result);
    if ( !hitShadingNormal(hit, &normal)) {
        logWarning("SplitBsdfEval", "Couldn't determine shading normal");
        return result;
    }

    if ( bsdf->texture && (flags & TEXTURED_COMPONENT)) {
        double textureBsdf = TexturedScattererEval(in, out, &normal);
        COLOR textureCol = SplitBsdfEvalTexture(bsdf->texture, hit);
        colorAddScaled(result, textureBsdf, textureCol, result);
        flags &= ~TEXTURED_COMPONENT;
    }

    /* Just add brdf and btdf contributions, the eval routines
       handle the direction of out. Note that out * normal is
       computed more than once :-( */
    if ( bsdf->brdf ) {
        COLOR reflectionCol = brdfEval(bsdf->brdf, in, out, &normal,
                                       GETBRDFFLAGS(flags));
        colorAdd(result, reflectionCol, result);
    }

    if ( bsdf->btdf ) {
        REFRACTIONINDEX inIndex, outIndex;
        COLOR refractionCol;
        bsdfIndexOfRefraction(inBsdf, &inIndex);
        bsdfIndexOfRefraction(outBsdf, &outIndex);
        refractionCol = btdfEval(bsdf->btdf, inIndex, outIndex,
                                 in, out, &normal, GETBTDFFLAGS(flags));
        colorAdd(result, refractionCol, result);
    }

    return result;
}

/* Sample a split bsdf. If no sample was taken (RR/absorption)
   the pdf will be 0 upon return */

/* Computes probabilities for sampling the texture, reflection minus texture,
 * or transmission. Also determines b[r|t]dfFlags taking into
 * account potential texturing. */
static void
SplitBsdfProbabilities(SPLIT_BSDF *bsdf, RayHit *hit, BSDFFLAGS flags,
                       double *Ptexture,
                       double *Preflection,
                       double *Ptransmission,
                       XXDFFLAGS *brdfFlags, XXDFFLAGS *btdfFlags) {
    COLOR textureColor, reflectance, transmittance;

    *Ptexture = 0.;
    if ( bsdf->texture && (flags & TEXTURED_COMPONENT)) {
        /* bsdf has a texture for diffuse reflection and diffuse reflection
         * needs to be sampled */
        textureColor = SplitBsdfEvalTexture(bsdf->texture, hit);
        *Ptexture = colorAverage(textureColor);
        flags &= ~TEXTURED_COMPONENT;
    }

    *brdfFlags = GETBRDFFLAGS(flags);
    *btdfFlags = GETBTDFFLAGS(flags);

    reflectance = brdfReflectance(bsdf->brdf, *brdfFlags);
    *Preflection = colorAverage(reflectance);

    transmittance = btdfTransmittance(bsdf->btdf, *btdfFlags);
    *Ptransmission = colorAverage(transmittance);
}

static SAMPLING_MODE
SplitBsdfSamplingMode(double Ptexture, double Preflection, double Ptransmission, double *x_1) {
    SAMPLING_MODE mode = SAMPLE_ABSORPTION;
    if ( *x_1 < Ptexture ) {
        mode = SAMPLE_TEXTURE;
        *x_1 /= Ptexture;     /* rescale into [0,1) interval again */
    } else {
        *x_1 -= Ptexture;
        if ( *x_1 < Preflection ) {
            mode = SAMPLE_REFLECTION;
            *x_1 /= Preflection;
        } else {
            *x_1 -= Preflection;
            if ( *x_1 < Ptransmission ) {
                mode = SAMPLE_TRANSMISSION;
                *x_1 /= Ptransmission;
            }
        }
    }
    return mode;
}

static Vector3D
SplitBsdfSample(SPLIT_BSDF *bsdf, RayHit *hit,
                BSDF *inBsdf, BSDF *outBsdf,
                Vector3D *in,
                int doRussianRoulette,
                BSDFFLAGS flags,
                double x_1, double x_2,
                double *pdf) {
    double Ptexture, Preflection, Ptransmission, Pscattering, p, pRR;
    REFRACTIONINDEX inIndex, outIndex;
    XXDFFLAGS brdfFlags, btdfFlags;
    Vector3D normal;
    SAMPLING_MODE mode;
    Vector3D out;

    *pdf = 0; /* so we can return safely */
    if ( !hitShadingNormal(hit, &normal)) {
        logWarning("SplitBsdfSample", "Couldn't determine shading normal");
        VECTORSET(out, 0., 0., 1.);
        return out;
    }

    /* Calculate probabilities for sampling the texture, reflection minus texture,
     * and transmission. Also fills in correct b[r|t]dfFlags. */
    SplitBsdfProbabilities(bsdf, hit, flags,
                           &Ptexture, &Preflection, &Ptransmission,
                           &brdfFlags, &btdfFlags);
    Pscattering = Ptexture + Preflection + Ptransmission;
    if ( Pscattering < EPSILON ) {
        return out;
    }

    /* Decide whether to sample the texture reflectance, the reflectance
     * modes not in the texture, transmission or absorption */
    if ( !doRussianRoulette ) {
        /* normalize: no absorption sampling */
        Ptexture /= Pscattering;
        Preflection /= Pscattering;
        Ptransmission /= Pscattering;
    }
    mode = SplitBsdfSamplingMode(Ptexture, Preflection, Ptransmission, &x_1);

    bsdfIndexOfRefraction(inBsdf, &inIndex);
    bsdfIndexOfRefraction(outBsdf, &outIndex);

    /* sample according to the selected mode */
    switch ( mode ) {
        case SAMPLE_TEXTURE:
            out = TexturedScattererSample(in, &normal, x_1, x_2, &p);
            if ( p < EPSILON ) {
                return out;
            } /* don't care */
            *pdf = Ptexture * p; /* other components will be added later */
            break;
        case SAMPLE_REFLECTION:
            out = brdfSample(bsdf->brdf, in, &normal, false, brdfFlags, x_1, x_2, &p);
            if ( p < EPSILON )
                return out;
            *pdf = Preflection * p;
            break;
        case SAMPLE_TRANSMISSION:
            out = btdfSample(bsdf->btdf, inIndex, outIndex, in, &normal,
                             false, btdfFlags, x_1, x_2, &p);
            if ( p < EPSILON )
                return out;
            *pdf = Ptransmission * p;
            break;
        case SAMPLE_ABSORPTION:
            *pdf = 0;
            return out;
            break;
        default:
            logFatal(-1, "SplitBsdfSample", "Impossible sampling mode %d", mode);
    }

    /* add probability of sampling the same direction in other than the
     * selected scattering mode (e.g. internal reflection) */
    if ( mode != SAMPLE_TEXTURE ) {
        TexturedScattererEvalPdf(in, &out, &normal, &p);
        *pdf += Ptexture * p;
    }
    if ( mode != SAMPLE_REFLECTION ) {
        brdfEvalPdf(bsdf->brdf, in, &out, &normal,
                    brdfFlags, &p, &pRR);
        *pdf += Preflection * p;
    }
    if ( mode != SAMPLE_TRANSMISSION ) {
        btdfEvalPdf(bsdf->btdf, inIndex, outIndex, in, &out, &normal,
                    btdfFlags, &p, &pRR);
        *pdf += Ptransmission * p;
    }

    return out;
}


/* Sample a split bsdf. If no sample was taken (RR/absorption)
   the pdf will be 0 upon return */
static void
SplitBsdfEvalPdf(SPLIT_BSDF *bsdf, RayHit *hit,
                 BSDF *inBsdf, BSDF *outBsdf,
                 Vector3D *in, Vector3D *out,
                 BSDFFLAGS flags,
                 double *pdf, double *pdfRR) {
    double Ptexture, Preflection, Ptransmission, Pscattering, p, pRR;
    REFRACTIONINDEX inIndex, outIndex;
    XXDFFLAGS brdfFlags, btdfFlags;
    Vector3D normal;

    *pdf = *pdfRR = 0.; /* so we can return safely */
    if ( !hitShadingNormal(hit, &normal)) {
        logWarning("SplitBsdfEvalPdf", "Couldn't determine shading normal");
        return;
    }

    /* Calculate probabilities for sampling the texture, reflection minus texture,
     * and transmission. Also fills in correct b[r|t]dfFlags. */
    SplitBsdfProbabilities(bsdf, hit, flags,
                           &Ptexture, &Preflection, &Ptransmission,
                           &brdfFlags, &btdfFlags);
    Pscattering = Ptexture + Preflection + Ptransmission;
    if ( Pscattering < EPSILON ) {
        return;
    }

    /* survival probability */
    *pdfRR = Pscattering;

    /* probability of sampling the outgoing direction, after survival decision */
    bsdfIndexOfRefraction(inBsdf, &inIndex);
    bsdfIndexOfRefraction(outBsdf, &outIndex);

    TexturedScattererEvalPdf(in, out, &normal, &p);
    *pdf = Ptexture * p;

    brdfEvalPdf(bsdf->brdf, in, out, &normal,
                brdfFlags, &p, &pRR);
    *pdf += Preflection * p;

    btdfEvalPdf(bsdf->btdf, inIndex, outIndex, in, out, &normal,
                btdfFlags, &p, &pRR);
    *pdf += Ptransmission * p;

    *pdf /= Pscattering;
}

static int
SplitBsdfIsTextured(SPLIT_BSDF *bsdf) {
    return bsdf->texture != nullptr;
}

BSDF_METHODS GLOBAL_scene_splitBsdfMethods = {
        (COLOR (*)(void *data, RayHit *hit, Vector3D *in, BSDFFLAGS flags)) SplitBsdfScatteredPower,
        (int (*)(void *)) SplitBsdfIsTextured,
        (void (*)(void *data, REFRACTIONINDEX *index)) SplitBsdfIndexOfRefraction,
        (COLOR (*)(void *data, RayHit *hit, void *inBsdf, void *outBsdf, Vector3D *in, Vector3D *out,
                   BSDFFLAGS flags)) SplitBsdfEval,
        (Vector3D (*)(void *data, RayHit *hit, void *inBsdf, void *outBsdf, Vector3D *in,
                      int doRussianRoulette,
                      BSDFFLAGS flags, double x_1, double x_2,
                      double *pdf)) SplitBsdfSample,
        (void (*)(void *data, RayHit *hit, void *inBsdf, void *outBsdf,
                  Vector3D *in, Vector3D *out,
                  BSDFFLAGS flags,
                  double *pdf, double *pdfRR)) SplitBsdfEvalPdf,
        (int (*)(void *data, RayHit *hit, Vector3D *X, Vector3D *Y, Vector3D *Z)) nullptr,
        (void (*)(FILE *out, void *data)) SplitBsdfPrint,
        (void *(*)(void *data)) SplitBsdfDuplicate,
        (void *(*)(void *parent, void *data)) nullptr,
        (void (*)(void *data)) SplitBsdfDestroy
};

/**
Bidirectional Reflectance Distribution Functions (BSDF)

Implementation of a BSDF consisting of one brdf and one bsdf. Either of the components may be NULL
*/
#include "java/lang/Float.h"
#include "common/Error.h"
#include "common/RenderOptions.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"

namespace {
bool extractHitData(
    RayHit *hit,
    Vector3D *normal,
    Vector3D *texCoord,
    unsigned int *flags)
{
    if ( hit == NULL || normal == NULL || texCoord == NULL || flags == NULL ) {
        return false;
    }
    if ( !hit->shadingNormal(normal) ) {
        return false;
    }
    *flags = NORMAL;
    if ( hit->getTexCoord(texCoord) ) {
        *flags |= TEXTURE_COORDINATE;
    } else {
        texCoord->set(0.0, 0.0, 0.0);
    }
    return true;
}
}

/**
Creates a BSDF instance with given data and methods
*/
PhongBidirScattDistFunc::PhongBidirScattDistFunc(
    PhongBidirReflDistFunc *brdf,
    PhongBidirTransDistFunc *btdf,
    Texture *texture)
{
    this->brdf = brdf;
    this->btdf = btdf;
    this->texture = texture;
}

PhongBidirScattDistFunc::~PhongBidirScattDistFunc() {
    if ( brdf != NULL ) {
        delete brdf;
        brdf = NULL;
    }

    if ( btdf != NULL ) {
        delete btdf;
        btdf = NULL;
    }

    if ( texture != NULL ) {
        delete texture;
    }
}

/**
Computes a shading frame at the given hit point. The Z axis of this frame is
the shading normal, The X axis is in the tangent plane on the surface at the
hit point ("brush" direction relevant for anisotropic shaders e.g.). Y
is perpendicular to X and Z. X and Y may be null pointers. In this case,
only the shading normal is returned, avoiding computation of the X and
Y axis if possible).
Note: edf can have also a routine for computing the shading frame. If a
material has both an edf and a bsdf, the shading frame shall of course
be the same.
This routine returns TRUE if a shading frame could be constructed and FALSE if
not. In the latter case, a default frame needs to be used (not computed by this
routine - pointShadingFrame() in material.[ch] constructs such a frame if
needed)
*/
bool
PhongBidirScattDistFunc::bsdfShadingFrame(
    const ShadingContext & /*context*/,
    const Vector3D * /*X*/,
    const Vector3D * /*Y*/,
    const Vector3D * /*Z*/)
{
    return false;
}

bool
PhongBidirScattDistFunc::bsdfShadingFrame(
    const RayHit * /*hit*/,
    const Vector3D * /*X*/,
    const Vector3D * /*Y*/,
    const Vector3D * /*Z*/)
{
    // Not implemented, should call to bsdf->methods->setShadingFrame or something like that
    return false;
}

ColorRgb
PhongBidirScattDistFunc::splitBsdfEvalTexture(const Texture *texture, const ShadingContext &context) {
    ColorRgb col;
    col.clear();

    if ( texture == NULL ) {
        return col;
    }
    if ( !context.hasFlag(TEXTURE_COORDINATE) ) {
        Error::warning("splitBsdfEvalTexture", "Couldn't get texture coordinates");
        return col;
    }

    const Vector3D texCoord = context.getTexCoord();
    return texture->evaluateColor(texCoord.x, texCoord.y);
}

ColorRgb
PhongBidirScattDistFunc::splitBsdfEvalTexture(const Texture *texture,  RayHit *hit) {
    Vector3D texCoord;
    ColorRgb col;
    col.clear();

    if ( texture == NULL ) {
        return col;
    }

    if ( hit == NULL || !hit->getTexCoord(&texCoord) ) {
        Error::warning("splitBsdfEvalTexture", "Couldn't get texture coordinates");
        return col;
    }

    return texture->evaluateColor(texCoord.x, texCoord.y);
}

/**
Returns the scattered power (diffuse/glossy/specular reflectance and/or transmittance) according to flags
*/
ColorRgb
PhongBidirScattDistFunc::splitBsdfScatteredPower(const ShadingContext &context, char flags) const {
    ColorRgb albedo;
    albedo.clear();

    if ( texture && (flags & TEXTURED_COMPONENT) ) {
        ColorRgb textureColor = PhongBidirScattDistFunc::splitBsdfEvalTexture(texture, context);
        albedo.add(albedo, textureColor);
        flags &= ~TEXTURED_COMPONENT;
    }

    if ( brdf ) {
        ColorRgb reflectance = brdf->reflectance(flags);
        if ( !Float::isFinite(reflectance.average()) ) {
            Error::fatal(-1, "brdfReflectance", "Oops - test Rd is not finite!");
        }

        albedo.add(albedo, reflectance);
    }

    if ( btdf != NULL ) {
        ColorRgb transmitted = btdf->transmittance(BsdfComponentFlag::getBtdfFlags(flags));
        albedo.add(albedo, transmitted);
    }

    return albedo;
}

ColorRgb
PhongBidirScattDistFunc::splitBsdfScatteredPower(RayHit *hit, char flags) const {
    Vector3D normal;
    Vector3D texCoord;
    unsigned int localFlags;
    if ( !extractHitData(hit, &normal, &texCoord, &localFlags) ) {
        ColorRgb out;
        out.clear();
        return out;
    }
    ShadingContext context(
        hit->getPoint(),
        hit->getGeometricNormal(),
        normal,
        texCoord,
        hit->getUv(),
        hit->getShadingFrame(),
        hit->getMaterial(),
        localFlags);
    return splitBsdfScatteredPower(context, flags);
}

bool
PhongBidirScattDistFunc::splitBsdfIsTextured() const {
    return texture != NULL;
}

#ifdef RAYTRACING_ENABLED
/**
Albedo is assumed to be 1
*/
Vector3D
PhongBidirScattDistFunc::texturedScattererSample(
    const Vector3D * /*in*/,
    const Vector3D *normal,
    double x1,
    double x2,
    double *probabilityDensityFunction)
{
    CoordinateSystem coord;
    // Section [ARVO1995b].2: map (x1, x2) from [0,1]^2 into a hemisphere direction.
    coord.setFromZAxis(normal);
    return coord.sampleHemisphereCosTheta(x1, x2, probabilityDensityFunction);
}

void
PhongBidirScattDistFunc::texturedScattererEvalPdf(
    const Vector3D * /*in*/,
    const Vector3D *out,
    const Vector3D *normal,
    double *probabilityDensityFunction)
{
    *probabilityDensityFunction = normal->dotProduct(*out) / M_PI;
}

/**
Sample a split bsdf. If no sample was taken (RR/absorption)
the pdf will be 0 upon return  Computes probabilities for sampling the texture, reflection minus texture,
or transmission. Also determines b[r|t]dfFlags taking into
account potential texturing
*/
void
PhongBidirScattDistFunc::splitBsdfProbabilities(
    const ShadingContext &context,
    char flags,
    double *inTexture,
    double *reflection,
    double *transmission,
    char *brdfFlags,
    char *btdfFlags) const
{
    *inTexture = 0.0;
    if ( texture && (flags & TEXTURED_COMPONENT) ) {
        ColorRgb textureColor;
        textureColor = PhongBidirScattDistFunc::splitBsdfEvalTexture(texture, context);
        *inTexture = textureColor.average();
        flags &= ~TEXTURED_COMPONENT;
    }

    *brdfFlags = BsdfComponentFlag::getBrdfFlags(flags);
    *btdfFlags = BsdfComponentFlag::getBtdfFlags(flags);

    ColorRgb reflectance;
    if ( brdf == NULL ) {
        reflectance.clear();
    } else {
        reflectance = brdf->reflectance(*brdfFlags);
    }
    *reflection = reflectance.average();

    ColorRgb transmittance;
    if ( btdf == NULL ) {
        transmittance.clear();
    } else {
        transmittance = btdf->transmittance(*btdfFlags);
    }
    *transmission = transmittance.average();
}

void
PhongBidirScattDistFunc::splitBsdfProbabilities(
    RayHit *hit,
    char flags,
    double *inTexture,
    double *reflection,
    double *transmission,
    char *brdfFlags,
    char *btdfFlags) const
{
    *inTexture = 0.0;
    if ( texture && (flags & TEXTURED_COMPONENT) ) {
        // bsdf has a texture for diffuse reflection and diffuse reflection needs to be sampled
        ColorRgb textureColor;
        textureColor = PhongBidirScattDistFunc::splitBsdfEvalTexture(texture, hit);
        *inTexture = textureColor.average();
        flags &= ~TEXTURED_COMPONENT;
    }

    *brdfFlags = BsdfComponentFlag::getBrdfFlags(flags);
    *btdfFlags = BsdfComponentFlag::getBtdfFlags(flags);

    ColorRgb reflectance;
    if ( brdf == NULL ) {
        reflectance.clear();
    } else {
        reflectance = brdf->reflectance(*brdfFlags);
    }
    *reflection = reflectance.average();

    ColorRgb transmittance;
    if ( btdf == NULL ) {
        transmittance.clear();
    } else {
        transmittance = btdf->transmittance(*btdfFlags);
    }
    *transmission = transmittance.average();
}

SplitBSDFSamplingMode
PhongBidirScattDistFunc::splitBsdfSamplingMode(double texture, double reflection, double transmission, double *x1) {
    SplitBSDFSamplingMode mode = SAMPLE_ABSORPTION;
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

/**
Returns the index of refraction of the BSDF
*/
void
PhongBidirScattDistFunc::indexOfRefraction(RefractionIndex *index) const {
    if ( btdf == NULL ) {
        index->set(1.0, 0.0); // Vacuum
    } else {
        btdf->setIndexOfRefraction(index);
    }
}

/**
Sampling and pdf evaluation

Sampling routines, parameters as in evaluation, except that two
random numbers x1 and x2 are needed (2D sampling process)
*/
Vector3D
PhongBidirScattDistFunc::sample(
    const ShadingContext &context,
    const PhongBidirScattDistFunc *inBsdf,
    const PhongBidirScattDistFunc *outBsdf,
    const Vector3D *in,
    int doRussianRoulette,
    char flags,
    double x1,
    double x2,
    double *probabilityDensityFunction) const
{
    Vector3D normal;
    Vector3D out;

    *probabilityDensityFunction = 0; // So we can return safely
    if ( !context.hasFlag(NORMAL) ) {
        Error::warning("sample", "Couldn't determine shading normal");
        out.set(0.0, 0.0, 1.0);
        return out;
    }
    normal = context.getShadingNormal();

    // Calculate probabilities for sampling the texture, reflection minus texture,
    // and transmission. Also fills in correct b[r|t]dfFlags
    double localTexture;
    double reflection;
    double transmission;
    char brdfFlags;
    char btdfFlags;
    splitBsdfProbabilities(
        context,
        flags,
        &localTexture,
        &reflection,
        &transmission,
        &brdfFlags,
        &btdfFlags);

    double scattering = localTexture + reflection + transmission;
    if ( scattering < Numeric::EPSILON ) {
        return out;
    }

    // Decide whether to sample the texture reflectance, the reflectance
    // modes not in the texture, transmission or absorption
    if ( !doRussianRoulette ) {
        // Normalize: no absorption sampling
        localTexture /= scattering;
        reflection /= scattering;
        transmission /= scattering;
    }

    SplitBSDFSamplingMode mode = PhongBidirScattDistFunc::splitBsdfSamplingMode(
        localTexture, reflection, transmission, &x1);
    RefractionIndex inIndex = RefractionIndex();
    RefractionIndex outIndex = RefractionIndex();

    if ( inBsdf != NULL ) {
        inBsdf->indexOfRefraction(&inIndex);
    }

    if ( outBsdf != NULL ) {
        outBsdf->indexOfRefraction(&outIndex);
    }

    // Sample according to the selected mode
    double p;
    switch ( mode ) {
        case SAMPLE_TEXTURE:
            out = PhongBidirScattDistFunc::texturedScattererSample(in, &normal, x1, x2, &p);
            if ( p < Numeric::EPSILON ) {
                // Don't care
                return out;
            }
            *probabilityDensityFunction = localTexture * p; /* other components will be added later */
            break;
        case SAMPLE_REFLECTION:
            if ( brdf == NULL ) {
                p = 0.0;
            } else {
                out = brdf->sample(in, &normal, false, brdfFlags, x1, x2, &p);
            }
            if ( p < Numeric::EPSILON )
                return out;
            *probabilityDensityFunction = reflection * p;
            break;
        case SAMPLE_TRANSMISSION:
            if ( btdf == NULL ) {
                p = 0.0;
                out.x = 0.0f;
                out.y = 0.0f;
                out.z = 0.0f;
            } else {
                out = btdf->sample(inIndex, outIndex, in, &normal, false, btdfFlags, x1, x2, &p);
            }
            if ( p < Numeric::EPSILON ) {
                return out;
            }
            *probabilityDensityFunction = transmission * p;
            break;
        case SAMPLE_ABSORPTION:
            *probabilityDensityFunction = 0;
            return out;
    }

    // Add probability of sampling the same direction in other than the
    // selected scattering mode (e.g. internal reflection) */
    if ( mode != SAMPLE_TEXTURE ) {
        PhongBidirScattDistFunc::texturedScattererEvalPdf(in, &out, &normal, &p);
        *probabilityDensityFunction += localTexture * p;
    }

    double pRR;
    if ( mode != SAMPLE_REFLECTION ) {
        if ( brdf == NULL ) {
            p = 0.0;
        } else {
            brdf->evalProbDensFunc(in, &out, &normal, brdfFlags, &p, &pRR);
        }
        *probabilityDensityFunction += reflection * p;
    }
    if ( mode != SAMPLE_TRANSMISSION ) {
        if ( btdf == NULL ) {
            p = 0.0;
        } else {
            btdf->evalProbDensFunc(inIndex, outIndex, in, &out, &normal, btdfFlags, &p, &pRR);
        }
        *probabilityDensityFunction += transmission * p;
    }

    return out;
}

Vector3D
PhongBidirScattDistFunc::sample(
    RayHit *hit,
    const PhongBidirScattDistFunc *inBsdf,
    const PhongBidirScattDistFunc *outBsdf,
    const Vector3D *in,
    int doRussianRoulette,
    char flags,
    double x1,
    double x2,
    double *probabilityDensityFunction) const
{
    Vector3D normal;
    Vector3D texCoord;
    unsigned int localFlags;
    if ( !extractHitData(hit, &normal, &texCoord, &localFlags) ) {
        Vector3D out;
        out.set(0.0, 0.0, 1.0);
        if ( probabilityDensityFunction ) {
            *probabilityDensityFunction = 0.0;
        }
        return out;
    }
    ShadingContext context(
        hit->getPoint(),
        hit->getGeometricNormal(),
        normal,
        texCoord,
        hit->getUv(),
        hit->getShadingFrame(),
        hit->getMaterial(),
        localFlags);
    return sample(context, inBsdf, outBsdf, in, doRussianRoulette, flags, x1, x2, probabilityDensityFunction);
}

double
PhongBidirScattDistFunc::texturedScattererEval(
    const Vector3D * /*in*/,
    const Vector3D * /*out*/,
    const Vector3D * /*normal*/)
{
    return (1.0 / M_PI);
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
PhongBidirScattDistFunc::evaluate(
    const ShadingContext &context,
    const PhongBidirScattDistFunc *inBsdf,
    const PhongBidirScattDistFunc *outBsdf,
    const Vector3D *in,
    const Vector3D *out,
    char flags) const
{
    ColorRgb result;
    Vector3D normal;

    result.clear();
    if ( !context.hasFlag(NORMAL) ) {
        Error::warning("evaluate", "Couldn't determine shading normal");
        return result;
    }
    normal = context.getShadingNormal();

    if ( texture && (flags & TEXTURED_COMPONENT) ) {
        double textureBsdf = PhongBidirScattDistFunc::texturedScattererEval(
                in, out, &normal);
        ColorRgb textureCol = PhongBidirScattDistFunc::splitBsdfEvalTexture(texture, context);
        result.addScaled(result, ((float)(textureBsdf)), textureCol);
        flags &= ~TEXTURED_COMPONENT;
    }

    // Just add brdf and btdf contributions, the eval routines handle the direction of out.
    // Note that out * normal is computed more than once :-(
    if ( brdf != NULL ) {
        ColorRgb reflectionCol = brdf->evaluate(in, out, &normal, BsdfComponentFlag::getBrdfFlags(flags));
        result.add(result, reflectionCol);

        RefractionIndex inIndex = RefractionIndex();
        RefractionIndex outIndex = RefractionIndex();
        ColorRgb refractionCol;

        if ( inBsdf != NULL ) {
            inBsdf->indexOfRefraction(&inIndex);
        }

        if ( outBsdf != NULL ) {
            outBsdf->indexOfRefraction(&outIndex);
        }

        if ( btdf == NULL ) {
            refractionCol.clear();
        } else {
            refractionCol = btdf->evaluate(
                    inIndex, outIndex, in, out, &normal, BsdfComponentFlag::getBtdfFlags(flags));
        }

        result.add(result, refractionCol);
    }

    return result;
}

ColorRgb
PhongBidirScattDistFunc::evaluate(
    RayHit *hit,
    const PhongBidirScattDistFunc *inBsdf,
    const PhongBidirScattDistFunc *outBsdf,
    const Vector3D *in,
    const Vector3D *out,
    char flags) const
{
    Vector3D normal;
    Vector3D texCoord;
    unsigned int localFlags;
    if ( !extractHitData(hit, &normal, &texCoord, &localFlags) ) {
        ColorRgb result;
        result.clear();
        return result;
    }
    ShadingContext context(
        hit->getPoint(),
        hit->getGeometricNormal(),
        normal,
        texCoord,
        hit->getUv(),
        hit->getShadingFrame(),
        hit->getMaterial(),
        localFlags);
    return evaluate(context, inBsdf, outBsdf, in, out, flags);
}

/**
Constructs shading frame at hit point. Returns TRUE if successful and
FALSE if not. X and Y may be null pointers

Sample a split bsdf. If no sample was taken (RR/absorption)
the pdf will be 0 upon return
*/
void
PhongBidirScattDistFunc::evalProbDensFunc(
    const ShadingContext &context,
    const PhongBidirScattDistFunc *inBsdf,
    const PhongBidirScattDistFunc *outBsdf,
    const Vector3D *in,
    const Vector3D *out,
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR) const
{
    double pTexture;
    double pReflection;
    double pTransmission;
    double pScattering;
    double p;
    double pRR;
    RefractionIndex inIndex = RefractionIndex();
    RefractionIndex outIndex = RefractionIndex();
    char brdfFlags;
    char btdfFlags;
    Vector3D normal;

    *probabilityDensityFunction = *probabilityDensityFunctionRR = 0.0; // So we can return safely
    if ( !context.hasFlag(NORMAL) ) {
        Error::warning("evalProbDensFunc", "Couldn't determine shading normal");
        return;
    }
    normal = context.getShadingNormal();

    // Calculate probabilities for sampling the texture, reflection minus texture,
    // and transmission. Also fills in correct b[r|t]dfFlags
    splitBsdfProbabilities(
        context,
        flags,
        &pTexture,
        &pReflection,
        &pTransmission,
        &brdfFlags,
        &btdfFlags);
    pScattering = pTexture + pReflection + pTransmission;
    if ( pScattering < Numeric::EPSILON ) {
        return;
    }

    // Survival probability
    *probabilityDensityFunctionRR = pScattering;

    // Probability of sampling the outgoing direction, after survival decision
    if ( inBsdf == NULL ) {
        inIndex.set(1.0, 0.0); // Vacuum
    } else {
        inBsdf->indexOfRefraction(&inIndex);
    }

    if ( outBsdf == NULL ) {
        outIndex.set(1.0, 0.0); // Vacuum
    } else {
        outBsdf->indexOfRefraction(&outIndex);
    }

    PhongBidirScattDistFunc::texturedScattererEvalPdf(in, out, &normal, &p);
    *probabilityDensityFunction = pTexture * p;

    if ( brdf == NULL ) {
        p = 0.0;
    } else {
        brdf->evalProbDensFunc(in, out, &normal, brdfFlags, &p, &pRR);
    }
    *probabilityDensityFunction += pReflection * p;

    if ( btdf == NULL ) {
        p = 0.0;
    } else {
        btdf->evalProbDensFunc(inIndex, outIndex, in, out, &normal, btdfFlags, &p, &pRR);
    }
    *probabilityDensityFunction += pTransmission * p;

    *probabilityDensityFunction /= pScattering;
}

void
PhongBidirScattDistFunc::evalProbDensFunc(
    RayHit *hit,
    const PhongBidirScattDistFunc *inBsdf,
    const PhongBidirScattDistFunc *outBsdf,
    const Vector3D *in,
    const Vector3D *out,
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR) const
{
    Vector3D normal;
    Vector3D texCoord;
    unsigned int localFlags;
    if ( !extractHitData(hit, &normal, &texCoord, &localFlags) ) {
        *probabilityDensityFunction = 0.0;
        *probabilityDensityFunctionRR = 0.0;
        return;
    }
    ShadingContext context(
        hit->getPoint(),
        hit->getGeometricNormal(),
        normal,
        texCoord,
        hit->getUv(),
        hit->getShadingFrame(),
        hit->getMaterial(),
        localFlags);
    evalProbDensFunc(context, inBsdf, outBsdf, in, out, flags, probabilityDensityFunction, probabilityDensityFunctionRR);
}

/**
Evaluates all requested components of the BSDF separately and
stores the result in 'colArray'.
Total evaluation is returned.
*/
ColorRgb
PhongBidirScattDistFunc::bsdfEvalComponents(
        const ShadingContext &context,
        const PhongBidirScattDistFunc *inBsdf,
        const PhongBidirScattDistFunc *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        const char flags,
        ColorRgb *colArray) const
{
    // Some caching optimisation could be used here
    ColorRgb result;
    ColorRgb empty;
    char thisFlag;

    empty.clear();
    result.clear();

    for ( int i = 0; i < BsdfComponentInfo::BSDF_COMPONENTS; i++ ) {
        thisFlag = ((char)(BsdfComponentFlag::bsdfIndexToComp(i)));

        if ( flags & thisFlag ) {
            colArray[i] = PhongBidirScattDistFunc::evaluate(
                    context, inBsdf, outBsdf, in, out, thisFlag);
            result.add(result, colArray[i]);
        } else {
            colArray[i] = empty;  // Set to 0 for safety
        }
    }

    return result;
}

ColorRgb
PhongBidirScattDistFunc::bsdfEvalComponents(
        RayHit *hit,
        const PhongBidirScattDistFunc *inBsdf,
        const PhongBidirScattDistFunc *outBsdf,
        const Vector3D *in,
        const Vector3D *out,
        const char flags,
        ColorRgb *colArray) const
{
    Vector3D normal;
    Vector3D texCoord;
    unsigned int localFlags;
    if ( !extractHitData(hit, &normal, &texCoord, &localFlags) ) {
        ColorRgb result;
        result.clear();
        return result;
    }
    ShadingContext context(
        hit->getPoint(),
        hit->getGeometricNormal(),
        normal,
        texCoord,
        hit->getUv(),
        hit->getShadingFrame(),
        hit->getMaterial(),
        localFlags);
    return bsdfEvalComponents(context, inBsdf, outBsdf, in, out, flags, colArray);
}
#endif

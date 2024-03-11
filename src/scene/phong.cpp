/**
Phong-type EDFs, BRDFs, BTDFs
*/

#include "common/error.h"
#include "scene/spherical.h"
#include "scene/phong.h"

/**
The BRDF described in this file (phong.c) is a modified phong-brdf.
It satisfies the requirements of symmetry and energy conservation.

The BRDF is expressed as:

brdf(in, out) = kd + ks*pow(cos(a),n)

  where:
kd: diffuse coefficient of the BRDF
ks: specular coefficient of the BRDF
n: specular power
  n small : glossy reflectance
  n large : specular reflectance (>= PHONG_LOWEST_SPECULAR_EXP)
a: angle between the out direction and the perfect mirror direction for in

The variables Kd and Ks which are stored in in the PHONG_BRDF type are
not the above coefficients, but represent the total energy reflected
for light incident perpendicular on the surface.

Thus:
Kd = kd*pi		or	kd = Kd/pi
Ks = ks*2*pi/(n+2)	or	ks = Ks*(n+2)/(2*pi)

For this BRDF to be energy conserving, the following condition must be met:

Kd + Ks <= 1

Some functions sample a direction on the hemisphere, given a specific
incoming direction, proportional to the value of the Modified Phong BRDF.
There are several sampling strategies to achieve this:
rejection sampling PhongBrdfSampleRejection()
inverse cumulative PDF sampling	PhongBrdfSampleCumPdf()

The different sampling functions are commented separately.
*/

/**
Creates Phong type EDF, BRDF, BTDF data structs:
Kd = diffuse emittance [W/m^2], reflectance or transmittance (number between 0 and 1)
Ks = specular emittance, reflectance or transmittance (same dimensions as Kd)
Ns = Phong exponent.
note: Emittance is total power emitted by the light source per unit of area
*/
PHONG_EDF *
phongEdfCreate(COLOR *Kd, COLOR *Ks, double Ns) {
    PHONG_EDF *edf = (PHONG_EDF *)malloc(sizeof(PHONG_EDF));
    edf->Kd = *Kd;
    colorScale((1.00f / (float)M_PI), edf->Kd, edf->kd); // Because we use it often
    edf->Ks = *Ks;
    if ( !colorNull(edf->Ks) ) {
        logWarning("phongEdfCreate", "Non-diffuse light sources not yet implemented");
    }
    edf->Ns = (float)Ns;
    return edf;
}

PHONG_BRDF *
phongBrdfCreate(COLOR *Kd, COLOR *Ks, double Ns) {
    PHONG_BRDF *brdf = (PHONG_BRDF *)malloc(sizeof(PHONG_BRDF));
    brdf->Kd = *Kd;
    brdf->avgKd = colorAverage(brdf->Kd);
    brdf->Ks = *Ks;
    brdf->avgKs = colorAverage(brdf->Ks);
    brdf->Ns = (float)Ns;
    return brdf;
}

PHONG_BTDF *
phongBtdfCreate(COLOR *Kd, COLOR *Ks, float Ns, float nr, float ni) {
    PHONG_BTDF *btdf = (PHONG_BTDF *)malloc(sizeof(PHONG_BTDF));
    btdf->Kd = *Kd;
    btdf->avgKd = colorAverage(btdf->Kd);
    btdf->Ks = *Ks;
    btdf->avgKs = colorAverage(btdf->Ks);
    btdf->Ns = Ns;
    btdf->refractionIndex.nr = nr;
    btdf->refractionIndex.ni = ni;
    return btdf;
}

/**
Returns emittance, reflectance, transmittance
*/
static COLOR
phongEmittance(PHONG_EDF *edf, RayHit * /*hit*/, XXDFFLAGS flags) {
    COLOR result;

    colorClear(result);

    if ( flags & DIFFUSE_COMPONENT ) {
        colorAdd(result, edf->Kd, result);
    }

    if ( PHONG_IS_SPECULAR(*edf)) {
        if ( flags & SPECULAR_COMPONENT ) {
            colorAdd(result, edf->Ks, result);
        }
    } else {
        if ( flags & GLOSSY_COMPONENT ) {
            colorAdd(result, edf->Ks, result);
        }
    }

    return result;
}

static COLOR
phongReflectance(PHONG_BRDF *brdf, XXDFFLAGS flags) {
    COLOR result;

    colorClear(result);

    if ( flags & DIFFUSE_COMPONENT ) {
        colorAdd(result, brdf->Kd, result);
    }

    if ( PHONG_IS_SPECULAR(*brdf)) {
        if ( flags & SPECULAR_COMPONENT ) {
            colorAdd(result, brdf->Ks, result);
        }
    } else {
        if ( flags & GLOSSY_COMPONENT ) {
            colorAdd(result, brdf->Ks, result);
        }
    }

    return result;
}

static COLOR
phongTransmittance(PHONG_BTDF *btdf, XXDFFLAGS flags) {
    COLOR result;

    colorClear(result);

    if ( flags & DIFFUSE_COMPONENT ) {
        colorAdd(result, btdf->Kd, result);
    }

    if ( PHONG_IS_SPECULAR(*btdf)) {
        if ( flags & SPECULAR_COMPONENT ) {
            colorAdd(result, btdf->Ks, result);
        }
    } else {
        if ( flags & GLOSSY_COMPONENT ) {
            colorAdd(result, btdf->Ks, result);
        }
    }

    if ( !std::isfinite(colorAverage(result))) {
        logFatal(-1, "phongTransmittance", "Oops - result is not finite!");
    }

    return result;
}

/**
Refraction index
*/
static void
phongIndexOfRefraction(PHONG_BTDF *btdf, RefractionIndex *index) {
    index->nr = btdf->refractionIndex.nr;
    index->ni = btdf->refractionIndex.ni;
}

/**
Edf evaluations
*/
static COLOR
phongEdfEval(PHONG_EDF *edf, RayHit *hit, Vector3D *out, XXDFFLAGS flags, double *pdf) {
    Vector3D normal;
    COLOR result;
    double cosL;

    colorClear(result);
    if ( pdf ) {
        *pdf = 0.0;
    }

    if ( !hitShadingNormal(hit, &normal) ) {
        logWarning("phongEdfEval", "Couldn't determine shading normal");
        return result;
    }

    cosL = vectorDotProduct(*out, normal);

    if ( cosL < 0.0 ) {
        return result;
    } // Back face of a light does not radiate

    // kd + ks (idealReflected * out)^n

    if ( flags & DIFFUSE_COMPONENT ) {
        // Divide by PI to turn radiant exitance [W/m^2] into exitant radiance [W/m^2 sr]
        colorAdd(result, edf->kd, result);
        if ( pdf ) {
            *pdf = cosL / M_PI;
        }
    }

    if ( flags & SPECULAR_COMPONENT ) {
        // ???
    }

    return result;
}

/**
Edf sampling
*/
static Vector3D
phongEdfSample(
    PHONG_EDF *edf,
    RayHit *hit,
    XXDFFLAGS flags,
    double xi1,
    double xi2,
    COLOR *selfEmittedRadiance,
    double *pdf)
{
    Vector3D dir = {0.0, 0.0, 1.0};
    if ( selfEmittedRadiance ) {
        colorClear(*selfEmittedRadiance);
    }
    if ( pdf ) {
        *pdf = 0.0;
    }

    if ( flags & DIFFUSE_COMPONENT ) {
        double spdf;
        CoordSys coord;

        Vector3D normal;
        if ( !hitShadingNormal(hit, &normal)) {
            logWarning("phongEdfEval", "Couldn't determine shading normal");
            return dir;
        }

        vectorCoordSys(&normal, &coord);
        dir = sampleHemisphereCosTheta(&coord, xi1, xi2, &spdf);
        if ( pdf ) {
            *pdf = spdf;
        }
        if ( selfEmittedRadiance ) {
            colorScale((1.0f / (float)M_PI), edf->Kd, *selfEmittedRadiance);
        }
    }

    return dir;
}

/**
Brdf evaluations
*/
static COLOR
phongBrdfEval(PHONG_BRDF *brdf, Vector3D *in, Vector3D *out, Vector3D *normal, XXDFFLAGS flags) {
    COLOR result;
    float tmpFloat;
    float dotProduct;
    Vector3D idealReflected;
    XXDFFLAGS nonDiffuseFlag;
    Vector3D inRev;
    vectorScale(-1.0, *in, inRev);

    colorClear(result);

    // kd + ks (idealReflected * out)^n
    if ( vectorDotProduct(*out, *normal) < 0 ) {
        /* refracted ray ! */
        return result;
    }

    if ( (flags & DIFFUSE_COMPONENT) && (brdf->avgKd > 0.0) ) {
        colorAddScaled(result, M_1_PI, brdf->Kd, result);
    }

    if ( PHONG_IS_SPECULAR(*brdf)) {
        nonDiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nonDiffuseFlag = GLOSSY_COMPONENT;
    }

    if ( (flags & nonDiffuseFlag) && (brdf->avgKs > 0.0) ) {
        idealReflected = idealReflectedDirection(&inRev, normal);
        dotProduct = vectorDotProduct(idealReflected, *out);

        if ( dotProduct > 0 ) {
            tmpFloat = (float)std::pow(dotProduct, brdf->Ns); // cos(a) ^ n
            tmpFloat *= (brdf->Ns + 2.0f) / (2.0f * (float)M_PI); // Ks -> ks
            colorAddScaled(result, tmpFloat, brdf->Ks, result);
        }
    }

    return result;
}

/**
Brdf sampling
*/
static Vector3D
phongBrdfSample(
    PHONG_BRDF *brdf,
    Vector3D *in,
    Vector3D *normal,
    int doRussianRoulette,
    XXDFFLAGS flags,
    double x_1,
    double x_2,
    double *pdf)
{
    Vector3D newDir = {0.0, 0.0, 0.0}, idealDir;
    double cosTheta;
    double diffPdf;
    double nonDiffPdf;
    double scatteredPower;
    double avgKd;
    double avgKs;
    float tmpFloat;
    CoordSys coord;
    XXDFFLAGS nonDiffuseFlag;
    Vector3D inRev;
    vectorScale(-1.0, *in, inRev);

    *pdf = 0;

    if ( flags & DIFFUSE_COMPONENT ) {
        avgKd = brdf->avgKd;
    } else {
        avgKd = 0.0;
    }

    if ( PHONG_IS_SPECULAR(*brdf) ) {
        nonDiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nonDiffuseFlag = GLOSSY_COMPONENT;
    }

    if ( flags & nonDiffuseFlag ) {
        avgKs = brdf->avgKs;
    } else {
        avgKs = 0.0;
    }

    scatteredPower = avgKd + avgKs;

    if ( scatteredPower < EPSILON ) {
        return newDir;
    }

    // Determine diffuse or glossy/specular sampling
    if ( doRussianRoulette ) {
        if ( x_1 > scatteredPower ) {
            /* Absorption */
            return newDir;
        }

        // Rescaling of x_1
        x_1 /= scatteredPower;
    }

    idealDir = idealReflectedDirection(&inRev, normal);

    if ( x_1 < (avgKd / scatteredPower) ) {
        // Sample diffuse
        x_1 = x_1 / (avgKd / scatteredPower);

        vectorCoordSys(normal, &coord);
        newDir = sampleHemisphereCosTheta(&coord, x_1, x_2, &diffPdf);

        tmpFloat = vectorDotProduct(idealDir, newDir);

        if ( tmpFloat > 0 ) {
            nonDiffPdf = (brdf->Ns + 1.0) * std::pow(tmpFloat,
                                                brdf->Ns) / (2.0 * M_PI);
        } else {
            nonDiffPdf = 0;
        }
    } else {
        // Sample specular
        x_1 = (x_1 - (avgKd / scatteredPower)) / (avgKs / scatteredPower);

        vectorCoordSys(&idealDir, &coord);
        newDir = sampleHemisphereCosNTheta(&coord, brdf->Ns, x_1, x_2,
                                           &nonDiffPdf);

        cosTheta = vectorDotProduct(*normal, newDir);
        if ( cosTheta <= 0 ) {
            return newDir;
        }

        diffPdf = cosTheta / M_PI;
    }

    // Combine pdf's
    *pdf = avgKd * diffPdf + avgKs * nonDiffPdf;

    if ( !doRussianRoulette ) {
        *pdf /= scatteredPower;
    }

    return newDir;
}

static void
phongBrdfEvalPdf(
    PHONG_BRDF *brdf,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    XXDFFLAGS flags,
    double *pdf,
    double *pdfRR)
{
    double cos_theta;
    double cos_alpha;
    double cos_in;
    double diffPdf;
    double nonDiffPdf;
    double scatteredPower;
    double avgKs;
    double avgKd;
    XXDFFLAGS nonDiffuseFlag;
    Vector3D idealDir;
    Vector3D inrev;
    Vector3D goodNormal;

    vectorScale(-1., *in, inrev);

    *pdf = 0;
    *pdfRR = 0;

    // Ensure 'in' on the same side as 'normal'!
    cos_in = vectorDotProduct(*in, *normal);
    if ( cos_in >= 0 ) {
        vectorCopy(*normal, goodNormal);
    } else {
        vectorScale(-1, *normal, goodNormal);
    }

    cos_theta = vectorDotProduct(goodNormal, *out);

    if ( cos_theta < 0 ) {
        return;
    }

    // 'out' is a reflected direction
    if ( flags & DIFFUSE_COMPONENT ) {
        avgKd = brdf->avgKd; // Store in phong data ?
    } else {
        avgKd = 0.0;
    }

    if ( PHONG_IS_SPECULAR(*brdf)) {
        nonDiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nonDiffuseFlag = GLOSSY_COMPONENT;
    }

    if ( flags & nonDiffuseFlag ) {
        avgKs = brdf->avgKs;
    } else {
        avgKs = 0.0;
    }

    scatteredPower = avgKd + avgKs;

    if ( scatteredPower < EPSILON ) {
        return;
    }

    // Diffuse sampling pdf
    diffPdf = 0.0;

    if ( avgKd > 0 ) {
        diffPdf = cos_theta / M_PI;
    }

    // Glossy or specular
    nonDiffPdf = 0.0;
    if ( avgKs > 0 ) {
        idealDir = idealReflectedDirection(&inrev, &goodNormal);

        cos_alpha = vectorDotProduct(idealDir, *out);

        if ( cos_alpha > 0 ) {
            nonDiffPdf = (brdf->Ns + 1.0) * std::pow(cos_alpha,
                                                brdf->Ns) / (2.0 * M_PI);
        }
    }

    *pdf = (avgKd * diffPdf + avgKs * nonDiffPdf) / scatteredPower;
    *pdfRR = scatteredPower;
}

/**
Btdf evaluations
*/
static COLOR
phongBtdfEval(
        PHONG_BTDF *btdf,
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        Vector3D *in,
        Vector3D *out,
        Vector3D *normal,
        XXDFFLAGS flags)
{
    COLOR result;
    float tmpFloat;
    float dotProduct;
    Vector3D idealRefracted;
    int totalIR;
    int IsReflection;
    XXDFFLAGS nonDiffuseFlag;
    Vector3D inrev;
    vectorScale(-1.0, *in, inrev);

    /* Specular-like refraction can turn into reflection.
       So for refraction a complete sphere should be
       sampled ! Importance sampling is advisable.
       Diffuse transmission is considered to always pass
       the material boundary
       */

    colorClear(result);

    if ( (flags & DIFFUSE_COMPONENT) && (btdf->avgKd > 0) ) {
        // Diffuse part

        // Normal is pointing away from refracted direction

        IsReflection = (vectorDotProduct(*normal, *out) >= 0);

        if ( !IsReflection ) {
            result = btdf->Kd;
            colorScale(M_1_PI, result, result);
        }
    }

    if ( PHONG_IS_SPECULAR(*btdf)) {
        nonDiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nonDiffuseFlag = GLOSSY_COMPONENT;
    }

    if ( (flags & nonDiffuseFlag) && (btdf->avgKs > 0) ) {
        // Specular part

        idealRefracted = idealRefractedDirection(&inrev, normal, inIndex,
                                                 outIndex, &totalIR);

        dotProduct = vectorDotProduct(idealRefracted, *out);

        if ( dotProduct > 0 ) {
            tmpFloat = (float)std::pow(dotProduct, btdf->Ns); // cos(a) ^ n
            tmpFloat *= (btdf->Ns + 2.0f) / (2.0f * (float)M_PI); // Ks -> ks
            colorAddScaled(result, tmpFloat, btdf->Ks, result);
        }
    }

    return result;
}

static Vector3D
phongBtdfSample(
        PHONG_BTDF *btdf,
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        Vector3D *in,
        Vector3D *normal,
        int doRussianRoulette,
        XXDFFLAGS flags,
        double x_1,
        double x_2,
        double *pdf)
{
    Vector3D newDir = {0.0, 0.0, 0.0};
    int totalIR;
    Vector3D idealDir, invNormal;
    CoordSys coord;
    double cos_theta;
    double avgKd;
    double avgKs;
    double scatteredPower;
    double diffPdf;
    double nonDiffPdf;
    float tmpFloat;
    XXDFFLAGS nonDiffuseFlag;
    Vector3D inrev;
    vectorScale(-1.0, *in, inrev);

    *pdf = 0;

    // Choose sampling mode
    if ( flags & DIFFUSE_COMPONENT ) {
        avgKd = btdf->avgKd; // Store in phong data ?
    } else {
        avgKd = 0.0;
    }

    if ( PHONG_IS_SPECULAR(*btdf)) {
        nonDiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nonDiffuseFlag = GLOSSY_COMPONENT;
    }

    if ( flags & nonDiffuseFlag ) {
        avgKs = btdf->avgKs;
    } else {
        avgKs = 0.0;
    }

    scatteredPower = avgKd + avgKs;

    if ( scatteredPower < EPSILON ) {
        return newDir;
    }

    // Determine diffuse or glossy/specular sampling
    if ( doRussianRoulette ) {
        if ( x_1 > scatteredPower ) {
            // Absorption
            return newDir;
        }

        // Rescaling of x_1
        x_1 /= scatteredPower;
    }

    idealDir = idealRefractedDirection(&inrev, normal, inIndex, outIndex,
                                       &totalIR);
    vectorScale(-1, *normal, invNormal);

    if ( x_1 < (avgKd / scatteredPower) ) {
        // Sample diffuse
        x_1 = x_1 / (avgKd / scatteredPower);

        vectorCoordSys(&invNormal, &coord);

        newDir = sampleHemisphereCosTheta(&coord, x_1, x_2, &diffPdf);

        tmpFloat = vectorDotProduct(idealDir, newDir);

        if ( tmpFloat > 0 ) {
            nonDiffPdf = (btdf->Ns + 1.0) * std::pow(tmpFloat,
                                                btdf->Ns) / (2.0 * M_PI);
        } else {
            nonDiffPdf = 0;
        }
    } else {
        // Sample specular
        x_1 = (x_1 - (avgKd / scatteredPower)) / (avgKs / scatteredPower);

        vectorCoordSys(&idealDir, &coord);
        newDir = sampleHemisphereCosNTheta(&coord, btdf->Ns, x_1, x_2,
                                           &nonDiffPdf);

        cos_theta = vectorDotProduct(*normal, newDir);
        if ( cos_theta > 0 ) {
            diffPdf = cos_theta / M_PI;
        } else {
            // Assume totalIR (maybe we should test the refractionIndices
            diffPdf = 0.0;
        }
    }

    // Combine pdf's
    *pdf = avgKd * diffPdf + avgKs * nonDiffPdf;

    if ( !doRussianRoulette ) {
        *pdf /= scatteredPower;
    }

    return newDir;
}

static void
phongBtdfEvalPdf(
        PHONG_BTDF *btdf,
        RefractionIndex inIndex,
        RefractionIndex outIndex,
        Vector3D *in,
        Vector3D *out,
        Vector3D *normal,
        XXDFFLAGS flags,
        double *pdf,
        double *pdfRR)
{
    double cos_theta;
    double cos_alpha;
    double cos_in;
    double diffPdf;
    double nonDiffPdf = 0.0;
    double scatteredPower;
    double avgKs;
    double avgKd;
    XXDFFLAGS nonDiffuseFlag;
    Vector3D idealDir;
    int totalIR;
    Vector3D goodNormal;
    Vector3D inrev;
    vectorScale(-1.0, *in, inrev);

    *pdf = 0;
    *pdfRR = 0;

    // Ensure 'in' on the same side as 'normal'!

    cos_in = vectorDotProduct(*in, *normal);
    if ( cos_in >= 0 ) {
        vectorCopy(*normal, goodNormal);
    } else {
        vectorScale(-1, *normal, goodNormal);
    }

    cos_theta = vectorDotProduct(goodNormal, *out);

    if ( flags & DIFFUSE_COMPONENT && (cos_theta < 0))  /* transmitted ray */
    {
        avgKd = btdf->avgKd;
    } else {
        avgKd = 0.0;
    }

    if ( PHONG_IS_SPECULAR(*btdf)) {
        nonDiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nonDiffuseFlag = GLOSSY_COMPONENT;
    }

    if ( flags & nonDiffuseFlag ) {
        avgKs = btdf->avgKs;
    } else {
        avgKs = 0.0;
    }

    scatteredPower = avgKd + avgKs;

    if ( scatteredPower < EPSILON ) {
        return;
    }

    // Diffuse sampling pdf
    if ( avgKd > 0 ) {
        diffPdf = cos_theta / M_PI;
    } else {
        diffPdf = 0.0;
    }

    // Glossy or specular
    if ( avgKs > 0 ) {
        if ( cos_in >= 0 ) {
            idealDir = idealRefractedDirection(&inrev, &goodNormal, inIndex,
                                               outIndex, &totalIR);
        } else {
            // Normal was inverted, so materialSides switch also
            idealDir = idealRefractedDirection(&inrev, &goodNormal, outIndex,
                                               inIndex, &totalIR);
        }

        cos_alpha = vectorDotProduct(idealDir, *out);

        nonDiffPdf = 0.0;
        if ( cos_alpha > 0 ) {
            nonDiffPdf = (btdf->Ns + 1.0) * std::pow(cos_alpha, btdf->Ns) / (2.0 * M_PI);
        }
    }

    *pdf = (avgKd * diffPdf + avgKs * nonDiffPdf) / scatteredPower;
    *pdfRR = scatteredPower;
}

// Phong-type edf method structs
EDF_METHODS GLOBAL_scene_phongEdfMethods = {
    (COLOR (*)(void *, RayHit *, XXDFFLAGS)) phongEmittance,
    nullptr, // Not textured
    (COLOR (*)(void *, RayHit *, Vector3D *, XXDFFLAGS, double *)) phongEdfEval,
    (Vector3D (*)(void *, RayHit *, XXDFFLAGS, double, double, COLOR *, double *)) phongEdfSample,
    nullptr
};

BRDF_METHODS GLOBAL_scene_phongBrdfMethods = {
    (COLOR (*)(void *, XXDFFLAGS)) phongReflectance,
    (COLOR (*)(void *, Vector3D *, Vector3D *, Vector3D *, XXDFFLAGS)) phongBrdfEval,
    (Vector3D (*)(void *, Vector3D *, Vector3D *, int, XXDFFLAGS, double, double,
    double *)) phongBrdfSample,
    (void (*)(void *, Vector3D *, Vector3D *, Vector3D *,
    XXDFFLAGS, double *, double *)) phongBrdfEvalPdf
};

BTDF_METHODS GLOBAL_scene_phongBtdfMethods = {
    (COLOR (*)(void *, XXDFFLAGS)) phongTransmittance,
    reinterpret_cast<void (*)(void *, RefractionIndex *)>((void (*)()) phongIndexOfRefraction),
    (COLOR (*)(void *, RefractionIndex, RefractionIndex, Vector3D *, Vector3D *, Vector3D *,
               XXDFFLAGS)) phongBtdfEval,
    (Vector3D (*)(void *, RefractionIndex, RefractionIndex, Vector3D *,
                  Vector3D *, int, XXDFFLAGS, double, double,
                  double *)) phongBtdfSample,
    (void (*)(void *, RefractionIndex, RefractionIndex, Vector3D *,
              Vector3D *, Vector3D *, XXDFFLAGS, double *, double *)) phongBtdfEvalPdf
};

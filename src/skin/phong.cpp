/**
Phong-type EDFs, BRDFs, BTDFs
*/

#include "common/error.h"
#include "skin/spherical.h"
#include "skin/phong.h"

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

PhongBiDirectionalReflectanceDistributionFunction *
phongBrdfCreate(COLOR *Kd, COLOR *Ks, double Ns) {
    PhongBiDirectionalReflectanceDistributionFunction *brdf = new PhongBiDirectionalReflectanceDistributionFunction();
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

COLOR
phongReflectance(PhongBiDirectionalReflectanceDistributionFunction *brdf, char flags) {
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

COLOR
phongTransmittance(PHONG_BTDF *btdf, char flags) {
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
void
phongIndexOfRefraction(PHONG_BTDF *btdf, RefractionIndex *index) {
    index->nr = btdf->refractionIndex.nr;
    index->ni = btdf->refractionIndex.ni;
}

/**
Brdf evaluations
*/
COLOR
phongBrdfEval(
    PhongBiDirectionalReflectanceDistributionFunction *brdf,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    char flags)
{
    COLOR result;
    float tmpFloat;
    float dotProduct;
    Vector3D idealReflected;
    char nonDiffuseFlag;
    Vector3D inRev;
    vectorScale(-1.0, *in, inRev);

    colorClear(result);

    // kd + ks (idealReflected * out)^n
    if ( vectorDotProduct(*out, *normal) < 0 ) {
        // Refracted ray
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
Vector3D
phongBrdfSample(
    PhongBiDirectionalReflectanceDistributionFunction *brdf,
    Vector3D *in,
    Vector3D *normal,
    int doRussianRoulette,
    char flags,
    double x1,
    double x2,
    double *probabilityDensityFunction)
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
    char nonDiffuseFlag;
    Vector3D inRev;
    vectorScale(-1.0, *in, inRev);

    *probabilityDensityFunction = 0;

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
        if ( x1 > scatteredPower ) {
            // Absorption
            return newDir;
        }

        // Rescaling of x_1
        x1 /= scatteredPower;
    }

    idealDir = idealReflectedDirection(&inRev, normal);

    if ( x1 < (avgKd / scatteredPower) ) {
        // Sample diffuse
        x1 = x1 / (avgKd / scatteredPower);

        vectorCoordSys(normal, &coord);
        newDir = sampleHemisphereCosTheta(&coord, x1, x2, &diffPdf);

        tmpFloat = vectorDotProduct(idealDir, newDir);

        if ( tmpFloat > 0 ) {
            nonDiffPdf = (brdf->Ns + 1.0) * std::pow(tmpFloat,
                                                brdf->Ns) / (2.0 * M_PI);
        } else {
            nonDiffPdf = 0;
        }
    } else {
        // Sample specular
        x1 = (x1 - (avgKd / scatteredPower)) / (avgKs / scatteredPower);

        vectorCoordSys(&idealDir, &coord);
        newDir = sampleHemisphereCosNTheta(&coord, brdf->Ns, x1, x2,
                                           &nonDiffPdf);

        cosTheta = vectorDotProduct(*normal, newDir);
        if ( cosTheta <= 0 ) {
            return newDir;
        }

        diffPdf = cosTheta / M_PI;
    }

    // Combine probabilityDensityFunctions
    *probabilityDensityFunction = avgKd * diffPdf + avgKs * nonDiffPdf;

    if ( !doRussianRoulette ) {
        *probabilityDensityFunction /= scatteredPower;
    }

    return newDir;
}

void
phongBrdfEvalPdf(
    PhongBiDirectionalReflectanceDistributionFunction *brdf,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR)
{
    double cos_theta;
    double cos_alpha;
    double cos_in;
    double diffPdf;
    double nonDiffPdf;
    double scatteredPower;
    double avgKs;
    double avgKd;
    char nonDiffuseFlag;
    Vector3D idealDir;
    Vector3D inRev;
    Vector3D goodNormal;

    vectorScale(-1.0, *in, inRev);

    *probabilityDensityFunction = 0;
    *probabilityDensityFunctionRR = 0;

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

    // Diffuse sampling probabilityDensityFunction
    diffPdf = 0.0;

    if ( avgKd > 0 ) {
        diffPdf = cos_theta / M_PI;
    }

    // Glossy or specular
    nonDiffPdf = 0.0;
    if ( avgKs > 0 ) {
        idealDir = idealReflectedDirection(&inRev, &goodNormal);

        cos_alpha = vectorDotProduct(idealDir, *out);

        if ( cos_alpha > 0 ) {
            nonDiffPdf = (brdf->Ns + 1.0) * std::pow(cos_alpha,
                                                brdf->Ns) / (2.0 * M_PI);
        }
    }

    *probabilityDensityFunction = (avgKd * diffPdf + avgKs * nonDiffPdf) / scatteredPower;
    *probabilityDensityFunctionRR = scatteredPower;
}

/**
Btdf evaluations
*/
COLOR
phongBtdfEval(
    PHONG_BTDF *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    char flags)
{
    COLOR result;
    float tmpFloat;
    float dotProduct;
    Vector3D idealRefracted;
    int totalIR;
    int isReflection;
    char nonDiffuseFlag;
    Vector3D inRev;
    vectorScale(-1.0, *in, inRev);

    // Specular-like refraction can turn into reflection.
    // So for refraction a complete sphere should be
    // sampled ! Importance sampling is advisable.
    // Diffuse transmission is considered to always pass
    // the material boundary
    colorClear(result);

    if ( (flags & DIFFUSE_COMPONENT) && (btdf->avgKd > 0) ) {
        // Diffuse part

        // Normal is pointing away from refracted direction
        isReflection = (vectorDotProduct(*normal, *out) >= 0);

        if ( !isReflection ) {
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

        idealRefracted = idealRefractedDirection(&inRev, normal, inIndex,
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

Vector3D
phongBtdfSample(
    PHONG_BTDF *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    Vector3D *in,
    Vector3D *normal,
    int doRussianRoulette,
    char flags,
    double x1,
    double x2,
    double *probabilityDensityFunction)
{
    Vector3D newDir = {0.0, 0.0, 0.0};
    int totalIR;
    Vector3D idealDir;
    Vector3D invNormal;
    CoordSys coord;
    double cosTheta;
    double avgKd;
    double avgKs;
    double scatteredPower;
    double diffPdf;
    double nonDiffPdf;
    float tmpFloat;
    char nonDiffuseFlag;
    Vector3D inRev;
    vectorScale(-1.0, *in, inRev);

    *probabilityDensityFunction = 0;

    // Choose sampling mode
    if ( flags & DIFFUSE_COMPONENT ) {
        avgKd = btdf->avgKd; // Store in phong data ?
    } else {
        avgKd = 0.0;
    }

    if ( PHONG_IS_SPECULAR(*btdf) ) {
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
        if ( x1 > scatteredPower ) {
            // Absorption
            return newDir;
        }

        // Rescaling of x_1
        x1 /= scatteredPower;
    }

    idealDir = idealRefractedDirection(&inRev, normal, inIndex, outIndex, &totalIR);
    vectorScale(-1, *normal, invNormal);

    if ( x1 < (avgKd / scatteredPower) ) {
        // Sample diffuse
        x1 = x1 / (avgKd / scatteredPower);

        vectorCoordSys(&invNormal, &coord);

        newDir = sampleHemisphereCosTheta(&coord, x1, x2, &diffPdf);

        tmpFloat = vectorDotProduct(idealDir, newDir);

        if ( tmpFloat > 0 ) {
            nonDiffPdf = (btdf->Ns + 1.0) * std::pow(tmpFloat,
                                                btdf->Ns) / (2.0 * M_PI);
        } else {
            nonDiffPdf = 0;
        }
    } else {
        // Sample specular
        x1 = (x1 - (avgKd / scatteredPower)) / (avgKs / scatteredPower);

        vectorCoordSys(&idealDir, &coord);
        newDir = sampleHemisphereCosNTheta(&coord, btdf->Ns, x1, x2,
                                           &nonDiffPdf);

        cosTheta = vectorDotProduct(*normal, newDir);
        if ( cosTheta > 0 ) {
            diffPdf = cosTheta / M_PI;
        } else {
            // Assume totalIR (maybe we should test the refractionIndices
            diffPdf = 0.0;
        }
    }

    // Combine Probability Density Functions
    *probabilityDensityFunction = avgKd * diffPdf + avgKs * nonDiffPdf;

    if ( !doRussianRoulette ) {
        *probabilityDensityFunction /= scatteredPower;
    }

    return newDir;
}

void
phongBtdfEvalPdf(
    PHONG_BTDF *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    Vector3D *in,
    Vector3D *out,
    Vector3D *normal,
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR)
{
    double cosTheta;
    double cosAlpha;
    double cosIn;
    double diffPdf;
    double nonDiffPdf = 0.0;
    double scatteredPower;
    double avgKs;
    double avgKd;
    char nonDiffuseFlag;
    Vector3D idealDir;
    int totalIR;
    Vector3D goodNormal;
    Vector3D inRev;
    vectorScale(-1.0, *in, inRev);

    *probabilityDensityFunction = 0;
    *probabilityDensityFunctionRR = 0;

    // Ensure 'in' on the same side as 'normal'!

    cosIn = vectorDotProduct(*in, *normal);
    if ( cosIn >= 0 ) {
        vectorCopy(*normal, goodNormal);
    } else {
        vectorScale(-1, *normal, goodNormal);
    }

    cosTheta = vectorDotProduct(goodNormal, *out);

    if ( flags & DIFFUSE_COMPONENT && (cosTheta < 0) ) {
        // Transmitted ray
        avgKd = btdf->avgKd;
    } else {
        avgKd = 0.0;
    }

    if ( PHONG_IS_SPECULAR(*btdf) ) {
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

    // Diffuse sampling probabilityDensityFunction
    if ( avgKd > 0 ) {
        diffPdf = cosTheta / M_PI;
    } else {
        diffPdf = 0.0;
    }

    // Glossy or specular
    if ( avgKs > 0 ) {
        if ( cosIn >= 0 ) {
            idealDir = idealRefractedDirection(&inRev, &goodNormal, inIndex,
                                               outIndex, &totalIR);
        } else {
            // Normal was inverted, so materialSides switch also
            idealDir = idealRefractedDirection(&inRev, &goodNormal, outIndex,
                                               inIndex, &totalIR);
        }

        cosAlpha = vectorDotProduct(idealDir, *out);

        nonDiffPdf = 0.0;
        if ( cosAlpha > 0 ) {
            nonDiffPdf = (btdf->Ns + 1.0) * std::pow(cosAlpha, btdf->Ns) / (2.0 * M_PI);
        }
    }

    *probabilityDensityFunction = (avgKd * diffPdf + avgKs * nonDiffPdf) / scatteredPower;
    *probabilityDensityFunctionRR = scatteredPower;
}

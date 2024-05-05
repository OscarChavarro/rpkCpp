#include "common/error.h"
#include "common/linealAlgebra/CoordinateSystem.h"
#include "material/xxdf.h"
#include "material/PhongBidirectionalReflectanceDistributionFunction.h"
#include "material/PhongBidirectionalTransmittanceDistributionFunction.h"

PhongBidirectionalTransmittanceDistributionFunction::PhongBidirectionalTransmittanceDistributionFunction(
    const ColorRgb *inKd, const ColorRgb *inKs, const float inNs, const float inNr, const float inNi):
    refractionIndex()
{
    Kd = *inKd;
    avgKd = Kd.average();
    Ks = *inKs;
    avgKs = Ks.average();
    Ns = inNs;
    refractionIndex.nr = inNr;
    refractionIndex.ni = inNi;
}

ColorRgb
phongTransmittance(const PhongBidirectionalTransmittanceDistributionFunction *btdf, char flags) {
    ColorRgb result;

    result.clear();

    if ( flags & DIFFUSE_COMPONENT ) {
        result.add(result, btdf->Kd);
    }

    if ( PHONG_IS_SPECULAR(*btdf) ) {
        if ( flags & SPECULAR_COMPONENT ) {
            result.add(result, btdf->Ks);
        }
    } else {
        if ( flags & GLOSSY_COMPONENT ) {
            result.add(result, btdf->Ks);
        }
    }

    if ( !std::isfinite(result.average()) ) {
        logFatal(-1, "phongTransmittance", "Oops - result is not finite!");
    }

    return result;
}

/**
Refraction index
*/
void
phongIndexOfRefraction(const PhongBidirectionalTransmittanceDistributionFunction *btdf, RefractionIndex *index) {
    index->nr = btdf->refractionIndex.nr;
    index->ni = btdf->refractionIndex.ni;
}

/**
Btdf evaluations
*/
ColorRgb
phongBtdfEval(
    const PhongBidirectionalTransmittanceDistributionFunction *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    char flags)
{
    ColorRgb result;
    float tmpFloat;
    float dotProduct;
    Vector3D idealRefracted;
    bool totalIR;
    int isReflection;
    char nonDiffuseFlag;
    Vector3D inRev;
    vectorScale(-1.0, *in, inRev);

    // Specular-like refraction can turn into reflection.
    // So for refraction a complete sphere should be
    // sampled ! Importance sampling is advisable.
    // Diffuse transmission is considered to always pass
    // the material boundary
    result.clear();

    if ( (flags & DIFFUSE_COMPONENT) && (btdf->avgKd > 0) ) {
        // Diffuse part

        // Normal is pointing away from refracted direction
        isReflection = (vectorDotProduct(*normal, *out) >= 0);

        if ( !isReflection ) {
            result = btdf->Kd;
            result.scale((float)M_1_PI);
        }
    }

    if ( PHONG_IS_SPECULAR(*btdf) ) {
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
            tmpFloat = std::pow(dotProduct, btdf->Ns); // cos(a) ^ n
            tmpFloat *= (btdf->Ns + 2.0f) / (2.0f * (float)M_PI); // Ks -> ks
            result.addScaled(result, tmpFloat, btdf->Ks);
        }
    }

    return result;
}

Vector3D
phongBtdfSample(
    const PhongBidirectionalTransmittanceDistributionFunction *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    const Vector3D *in,
    const Vector3D *normal,
    int doRussianRoulette,
    char flags,
    double x1,
    double x2,
    double *probabilityDensityFunction)
{
    Vector3D newDir = {0.0, 0.0, 0.0};
    bool totalIR;
    Vector3D idealDir;
    Vector3D invNormal;
    CoordinateSystem coord;
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

        coord.setFromZAxis(&invNormal);

        newDir = coord.sampleHemisphereCosTheta(x1, x2, &diffPdf);

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

        coord.setFromZAxis(&idealDir);
        newDir = coord.sampleHemisphereCosNTheta(btdf->Ns, x1, x2, &nonDiffPdf);

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
    const PhongBidirectionalTransmittanceDistributionFunction *btdf,
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    const char flags,
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
    bool totalIR;
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

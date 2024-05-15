#include "java/lang/Float.h"
#include "common/error.h"
#include "common/linealAlgebra/CoordinateSystem.h"
#include "material/xxdf.h"
#include "material/PhongBidirectionalReflectanceDistributionFunction.h"
#include "material/PhongBidirectionalTransmittanceDistributionFunction.h"

/**
All components of the Btdf

Vector directions :

in : towards patch
out : from patch
normal : leaving from patch, on the incoming side.
         So in.normal < 0 !!!
*/

bool
PhongBidirectionalTransmittanceDistributionFunction::isSpecular() const {
    return Ns >= PHONG_LOWEST_SPECULAR_EXP;
}

PhongBidirectionalTransmittanceDistributionFunction::PhongBidirectionalTransmittanceDistributionFunction(
    const ColorRgb *inKd, const ColorRgb *inKs, const float inNs, const float inNr, const float inNi):
    refractionIndex()
{
    Kd = *inKd;
    avgKd = Kd.average();
    Ks = *inKs;
    avgKs = Ks.average();
    Ns = inNs;
    refractionIndex.set(inNr, inNi);
}

PhongBidirectionalTransmittanceDistributionFunction::~PhongBidirectionalTransmittanceDistributionFunction() {
}

/**
Returns the transmittance of the BTDF
*/
ColorRgb
PhongBidirectionalTransmittanceDistributionFunction::transmittance(char flags) const {
    ColorRgb result;

    result.clear();

    if ( flags & DIFFUSE_COMPONENT ) {
        result.add(result, Kd);
    }

    if ( isSpecular() ) {
        if ( flags & SPECULAR_COMPONENT ) {
            result.add(result, Ks);
        }
    } else {
        if ( flags & GLOSSY_COMPONENT ) {
            result.add(result, Ks);
        }
    }

    if ( !java::Float::isFinite(result.average()) ) {
        logFatal(-1, "transmittance", "Oops - result is not finite!");
    }

    return result;
}

/**
Refraction index
*/
void
PhongBidirectionalTransmittanceDistributionFunction::setIndexOfRefraction(RefractionIndex *index) const {
    index->set(refractionIndex.getNr(), refractionIndex.getNi());
}

/**
Btdf evaluations
*/
ColorRgb
PhongBidirectionalTransmittanceDistributionFunction::evaluate(
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    char flags) const
{
    Vector3D inRev;
    inRev.scaledCopy(-1.0, *in);

    // Specular-like refraction can turn into reflection.
    // So for refraction a complete sphere should be
    // sampled ! Importance sampling is advisable.
    // Diffuse transmission is considered to always pass
    // the material boundary
    ColorRgb result;
    result.clear();

    if ( (flags & DIFFUSE_COMPONENT) && (avgKd > 0) ) {
        // Diffuse part

        // Normal is pointing away from refracted direction
        bool isReflection = (normal->dotProduct(*out) >= 0);

        if ( !isReflection ) {
            result = Kd;
            result.scale((float)M_1_PI);
        }
    }

    char nonDiffuseFlag;

    if ( isSpecular() ) {
        nonDiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nonDiffuseFlag = GLOSSY_COMPONENT;
    }

    if ( (flags & nonDiffuseFlag) && (avgKs > 0) ) {
        // Specular part
        bool totalIR;
        Vector3D idealRefracted = idealRefractedDirection(&inRev, normal, inIndex, outIndex, &totalIR);
        float localDotProduct = idealRefracted.dotProduct(*out);

        if ( localDotProduct > 0 ) {
            float tmpFloat = java::Math::pow(localDotProduct, Ns); // cos(a) ^ n
            tmpFloat *= (Ns + 2.0f) / (2.0f * (float)M_PI); // Ks -> ks
            result.addScaled(result, tmpFloat, Ks);
        }
    }

    return result;
}

Vector3D
PhongBidirectionalTransmittanceDistributionFunction::sample(
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    const Vector3D *in,
    const Vector3D *normal,
    int doRussianRoulette,
    char flags,
    double x1,
    double x2,
    double *probabilityDensityFunction) const
{
    Vector3D newDir = {0.0, 0.0, 0.0};
    bool totalIR;
    Vector3D idealDir;
    Vector3D invNormal;
    CoordinateSystem coord;
    double cosTheta;
    double localAverageKd;
    double localAverageKs;
    double scatteredPower;
    double diffPdf;
    double nonDiffPdf;
    float tmpFloat;
    char nonDiffuseFlag;
    Vector3D inRev;
    inRev.scaledCopy(-1.0, *in);

    *probabilityDensityFunction = 0;

    // Choose sampling mode
    if ( flags & DIFFUSE_COMPONENT ) {
        localAverageKd = avgKd; // Store in phong data ?
    } else {
        localAverageKd = 0.0;
    }

    if ( isSpecular() ) {
        nonDiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nonDiffuseFlag = GLOSSY_COMPONENT;
    }

    if ( flags & nonDiffuseFlag ) {
        localAverageKs = avgKs;
    } else {
        localAverageKs = 0.0;
    }

    scatteredPower = localAverageKd + localAverageKs;

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
    invNormal.scaledCopy(-1, *normal);

    if ( x1 < (localAverageKd / scatteredPower) ) {
        // Sample diffuse
        x1 = x1 / (localAverageKd / scatteredPower);

        coord.setFromZAxis(&invNormal);

        newDir = coord.sampleHemisphereCosTheta(x1, x2, &diffPdf);

        tmpFloat = idealDir.dotProduct(newDir);

        if ( tmpFloat > 0 ) {
            nonDiffPdf = (Ns + 1.0) * java::Math::pow(tmpFloat, Ns) / (2.0 * M_PI);
        } else {
            nonDiffPdf = 0;
        }
    } else {
        // Sample specular
        x1 = (x1 - (localAverageKd / scatteredPower)) / (localAverageKs / scatteredPower);

        coord.setFromZAxis(&idealDir);
        newDir = coord.sampleHemisphereCosNTheta(Ns, x1, x2, &nonDiffPdf);

        cosTheta = normal->dotProduct(newDir);
        if ( cosTheta > 0 ) {
            diffPdf = cosTheta / M_PI;
        } else {
            // Assume totalIR (maybe we should test the refractionIndices
            diffPdf = 0.0;
        }
    }

    // Combine Probability Density Functions
    *probabilityDensityFunction = localAverageKd * diffPdf + localAverageKs * nonDiffPdf;

    if ( !doRussianRoulette ) {
        *probabilityDensityFunction /= scatteredPower;
    }

    return newDir;
}

void
PhongBidirectionalTransmittanceDistributionFunction::evaluateProbabilityDensityFunction(
    RefractionIndex inIndex,
    RefractionIndex outIndex,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR) const
{
    Vector3D inRev;
    inRev.scaledCopy(-1.0, *in);

    *probabilityDensityFunction = 0;
    *probabilityDensityFunctionRR = 0;

    // Ensure 'in' on the same side as 'normal'!
    Vector3D goodNormal;
    double cosIn = in->dotProduct(*normal);

    if ( cosIn >= 0 ) {
        goodNormal.copy(*normal);
    } else {
        goodNormal.scaledCopy(-1, *normal);
    }

    double cosTheta = goodNormal.dotProduct(*out);

    double localAverageKd;
    if ( flags & DIFFUSE_COMPONENT && (cosTheta < 0) ) {
        // Transmitted ray
        localAverageKd = avgKd;
    } else {
        localAverageKd = 0.0;
    }

    char nonDiffuseFlag;
    if ( isSpecular() ) {
        nonDiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nonDiffuseFlag = GLOSSY_COMPONENT;
    }

    double localAverageKs;
    if ( flags & nonDiffuseFlag ) {
        localAverageKs = avgKs;
    } else {
        localAverageKs = 0.0;
    }

    double scatteredPower = localAverageKd + localAverageKs;

    if ( scatteredPower < EPSILON ) {
        return;
    }

    // Diffuse sampling probabilityDensityFunction
    double diffPdf;
    if ( localAverageKd > 0 ) {
        diffPdf = cosTheta / M_PI;
    } else {
        diffPdf = 0.0;
    }

    // Glossy or specular
    double nonDiffPdf = 0.0;
    if ( localAverageKs > 0 ) {
        double cosAlpha;
        Vector3D idealDir;
        bool totalIR;

        if ( cosIn >= 0 ) {
            idealDir = idealRefractedDirection(&inRev, &goodNormal, inIndex,
                                               outIndex, &totalIR);
        } else {
            // Normal was inverted, so materialSides switch also
            idealDir = idealRefractedDirection(&inRev, &goodNormal, outIndex,
                                               inIndex, &totalIR);
        }

        cosAlpha = idealDir.dotProduct(*out);

        nonDiffPdf = 0.0;
        if ( cosAlpha > 0 ) {
            nonDiffPdf = (Ns + 1.0) * java::Math::pow(cosAlpha, (double)Ns) / (2.0 * M_PI);
        }
    }

    *probabilityDensityFunction = (localAverageKd * diffPdf + localAverageKs * nonDiffPdf) / scatteredPower;
    *probabilityDensityFunctionRR = scatteredPower;
}

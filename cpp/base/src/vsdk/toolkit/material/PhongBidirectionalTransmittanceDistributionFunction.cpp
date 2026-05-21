#include "vsdk/toolkit/java/lang/Float.h"
#include "vsdk/toolkit/common/linealAlgebra/CoordinateSystem.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/material/PhongBidirectionalReflectanceDistributionFunction.h"
#include "vsdk/toolkit/material/PhongBidirectionalTransmittanceDistributionFunction.h"
#include "vsdk/toolkit/material/Xxdf.h"

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
    return Ns >= Xxdf::PHONG_LOWEST_SPECULAR_EXP;
}

PhongBidirectionalTransmittanceDistributionFunction::PhongBidirectionalTransmittanceDistributionFunction(
    const ColorRgb *inKd, const ColorRgb *inKs, const float inNs, const float inNr, const float inNi):
    Kd(*inKd),
    Ks(*inKs),
    avgKd(Kd.average()),
    avgKs(Ks.average()),
    Ns(inNs),
    refractionIndex()
{
    refractionIndex.set(inNr, inNi);
}

PhongBidirectionalTransmittanceDistributionFunction::~PhongBidirectionalTransmittanceDistributionFunction() {
}

/**
Returns the transmittance of the BTDF
*/
ColorRgb
PhongBidirectionalTransmittanceDistributionFunction::transmittance(char flags) const {
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;

    if ( flags & DIFFUSE_COMPONENT ) {
        r += Kd.getR();
        g += Kd.getG();
        b += Kd.getB();
    }

    if ( isSpecular() ) {
        if ( flags & SPECULAR_COMPONENT ) {
            r += Ks.getR();
            g += Ks.getG();
            b += Ks.getB();
        }
    } else {
        if ( flags & GLOSSY_COMPONENT ) {
            r += Ks.getR();
            g += Ks.getG();
            b += Ks.getB();
        }
    }

    const ColorRgb result(r, g, b);
    if ( !java::Float::isFinite(static_cast<float>(result.average())) ) {
        Logger::fatal(-1, "transmittance", "Oops - result is not finite!");
    }

    return result;
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
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;

    if ( (flags & DIFFUSE_COMPONENT) && (avgKd > 0) ) {
        // Diffuse part

        // Normal is pointing away from refracted direction
        bool isReflection = (normal->dotProduct(*out) >= 0);

        if ( !isReflection ) {
            r += M_1_PI * Kd.getR();
            g += M_1_PI * Kd.getG();
            b += M_1_PI * Kd.getB();
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
        Vector3D idealRefracted = Xxdf::idealRefractedDirection(&inRev, normal, inIndex, outIndex, &totalIR);
        float localDotProduct = idealRefracted.dotProduct(*out);

        if ( localDotProduct > 0 ) {
            float tmpFloat = java::Math::pow(localDotProduct, Ns); // cos(a) ^ n
            tmpFloat *= (Ns + 2.0F) / (2.0F * static_cast<float>(M_PI)); // Ks -> ks
            r += tmpFloat * Ks.getR();
            g += tmpFloat * Ks.getG();
            b += tmpFloat * Ks.getB();
        }
    }

    return {r, g, b};
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

    if ( scatteredPower < Numeric::EPSILON ) {
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

    idealDir = Xxdf::idealRefractedDirection(&inRev, normal, inIndex, outIndex, &totalIR);
    invNormal.scaledCopy(-1, *normal);

    if ( x1 < (localAverageKd / scatteredPower) ) {
        // Sample diffuse
        x1 = x1 / (localAverageKd / scatteredPower);

        // Section [ARVO1995b].2: square-to-sphere mapping in the frame of the transmitted hemisphere.
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

        // Section [ARVO1995b].2: same 2D mapping with a lobe centered on the ideal transmitted direction.
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

    if ( scatteredPower < Numeric::EPSILON ) {
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
            idealDir = Xxdf::idealRefractedDirection(&inRev, &goodNormal, inIndex,
                                               outIndex, &totalIR);
        } else {
            // Normal was inverted, so materialSides switch also
            idealDir = Xxdf::idealRefractedDirection(&inRev, &goodNormal, outIndex,
                                               inIndex, &totalIR);
        }

        cosAlpha = idealDir.dotProduct(*out);

        nonDiffPdf = 0.0;
        if ( cosAlpha > 0 ) {
            nonDiffPdf = (Ns + 1.0) * java::Math::pow(cosAlpha, static_cast<double>(Ns)) / (2.0 * M_PI);
        }
    }

    *probabilityDensityFunction = (localAverageKd * diffPdf + localAverageKs * nonDiffPdf) / scatteredPower;
    *probabilityDensityFunctionRR = scatteredPower;
}

#ifdef RAYTRACING_ENABLED
/**
Refraction index
*/
void
PhongBidirectionalTransmittanceDistributionFunction::setIndexOfRefraction(RefractionIndex *index) const {
    index->set(refractionIndex.getNr(), refractionIndex.getNi());
}
#endif

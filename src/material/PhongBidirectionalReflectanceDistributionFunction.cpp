#include "common/linealAlgebra/CoordinateSystem.h"
#include "material/PhongBidirectionalReflectanceDistributionFunction.h"
#include "material/xxdf.h"

/**
The BRDF here is a modified phong-brdf. It satisfies the requirements of symmetry and energy conservation.

The BRDF is expressed as:

brdf(in, out) = kd + ks * pow(cos(a), n)

where:
- kd: diffuse coefficient of the BRDF
- ks: specular coefficient of the BRDF
- n: specular power
    n small : glossy reflectance
    n large : specular reflectance (>= PHONG_LOWEST_SPECULAR_EXP)
- a: angle between the out direction and the perfect mirror direction for in

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

PhongBidirectionalReflectanceDistributionFunction::PhongBidirectionalReflectanceDistributionFunction(
    const ColorRgb *inKd, const ColorRgb *inKs, double inNs) {
    Kd = *inKd;
    avgKd = Kd.average();
    Ks = *inKs;
    avgKs = Ks.average();
    Ns = (float)inNs;
}

PhongBidirectionalReflectanceDistributionFunction::~PhongBidirectionalReflectanceDistributionFunction() {
}

/**
Returns the diffuse reflectance of the BRDF according to the flags
*/
ColorRgb
PhongBidirectionalReflectanceDistributionFunction::reflectance(const char flags) const {
    ColorRgb result;

    result.clear();

    if ( flags & DIFFUSE_COMPONENT ) {
        result.add(result, Kd);
    }

    if ( PHONG_IS_SPECULAR(*this) ) {
        if ( flags & SPECULAR_COMPONENT ) {
            result.add(result, Ks);
        }
    } else {
        if ( flags & GLOSSY_COMPONENT ) {
            result.add(result, Ks);
        }
    }

    return result;
}

/**
Brdf evaluations
*/
ColorRgb
PhongBidirectionalReflectanceDistributionFunction::evaluate(
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    char flags) const
{
    ColorRgb result;
    float tmpFloat;
    float localDotProduct;
    Vector3D idealReflected;
    char nonDiffuseFlag;
    Vector3D inRev;
    vectorScale(-1.0, *in, inRev);

    result.clear();

    // kd + ks (idealReflected * out)^n
    if ( out->dotProduct(*normal) < 0 ) {
        // Refracted ray
        return result;
    }

    if ( (flags & DIFFUSE_COMPONENT) && (avgKd > 0.0) ) {
        result.addScaled(result, (float)M_1_PI, Kd);
    }

    if ( PHONG_IS_SPECULAR(*this) ) {
        nonDiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nonDiffuseFlag = GLOSSY_COMPONENT;
    }

    if ( (flags & nonDiffuseFlag) && (avgKs > 0.0) ) {
        idealReflected = idealReflectedDirection(&inRev, normal);
        localDotProduct = idealReflected.dotProduct(*out);

        if ( localDotProduct > 0 ) {
            tmpFloat = std::pow(localDotProduct, Ns); // cos(a) ^ n
            tmpFloat *= (Ns + 2.0f) / (2.0f * (float)M_PI); // Ks -> ks
            result.addScaled(result, tmpFloat, Ks);
        }
    }

    return result;
}

/**
Brdf sampling
*/
Vector3D
PhongBidirectionalReflectanceDistributionFunction::sample(
    const Vector3D *in,
    const Vector3D *normal,
    const int doRussianRoulette,
    const char flags,
    double x1,
    const double x2,
    double *probabilityDensityFunction) const
{
    Vector3D inRev;
    vectorScale(-1.0, *in, inRev);

    *probabilityDensityFunction = 0;

    double localAverageKd;

    if ( flags & DIFFUSE_COMPONENT ) {
        localAverageKd = avgKd;
    } else {
        localAverageKd = 0.0;
    }

    char nonDiffuseFlag;

    if ( PHONG_IS_SPECULAR(*this) ) {
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
    Vector3D newDir = {0.0, 0.0, 0.0};

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

    Vector3D idealDir = idealReflectedDirection(&inRev, normal);
    CoordinateSystem coord;
    double diffPdf;
    float tmpFloat;
    double nonDiffPdf;

    if ( x1 < (localAverageKd / scatteredPower) ) {
        // Sample diffuse
        x1 = x1 / (localAverageKd / scatteredPower);

        coord.setFromZAxis(normal);
        newDir = coord.sampleHemisphereCosTheta(x1, x2, &diffPdf);

        tmpFloat = idealDir.dotProduct(newDir);

        if ( tmpFloat > 0 ) {
            nonDiffPdf = (Ns + 1.0) * std::pow(tmpFloat, Ns) / (2.0 * M_PI);
        } else {
            nonDiffPdf = 0;
        }
    } else {
        // Sample specular
        x1 = (x1 - (localAverageKd / scatteredPower)) / (localAverageKs / scatteredPower);

        coord.setFromZAxis(&idealDir);
        newDir = coord.sampleHemisphereCosNTheta(Ns, x1, x2, &nonDiffPdf);

        double cosTheta = normal->dotProduct(newDir);
        if ( cosTheta <= 0 ) {
            return newDir;
        }

        diffPdf = cosTheta / M_PI;
    }

    // Combine probabilityDensityFunctions
    *probabilityDensityFunction = localAverageKd * diffPdf + localAverageKs * nonDiffPdf;

    if ( !doRussianRoulette ) {
        *probabilityDensityFunction /= scatteredPower;
    }

    return newDir;
}

void
PhongBidirectionalReflectanceDistributionFunction::evaluateProbabilityDensityFunction(
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR) const
{
    double cosAlpha;
    double diffPdf;
    double nonDiffPdf;
    double scatteredPower;
    double localAverageKs;
    double localAverageKd;
    char nonDiffuseFlag;
    Vector3D idealDir;
    Vector3D inRev;
    Vector3D goodNormal;

    vectorScale(-1.0, *in, inRev);

    *probabilityDensityFunction = 0;
    *probabilityDensityFunctionRR = 0;

    // Ensure 'in' on the same side as 'normal'!
    double cosIn = in->dotProduct(*normal);
    if ( cosIn >= 0 ) {
        goodNormal.copy(*normal);
    } else {
        vectorScale(-1, *normal, goodNormal);
    }

    double cosTheta = goodNormal.dotProduct(*out);
    if ( cosTheta < 0 ) {
        return;
    }

    // 'out' is a reflected direction
    if ( flags & DIFFUSE_COMPONENT ) {
        localAverageKd = avgKd; // Store in phong data ?
    } else {
        localAverageKd = 0.0;
    }

    if ( PHONG_IS_SPECULAR(*this) ) {
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
        return;
    }

    // Diffuse sampling probabilityDensityFunction
    diffPdf = 0.0;

    if ( avgKd > 0 ) {
        diffPdf = cosTheta / M_PI;
    }

    // Glossy or specular
    nonDiffPdf = 0.0;
    if ( avgKs > 0 ) {
        idealDir = idealReflectedDirection(&inRev, &goodNormal);

        cosAlpha = idealDir.dotProduct(*out);

        if ( cosAlpha > 0 ) {
            nonDiffPdf = (Ns + 1.0) * std::pow(cosAlpha, Ns) / (2.0 * M_PI);
        }
    }

    *probabilityDensityFunction = (avgKd * diffPdf + avgKs * nonDiffPdf) / scatteredPower;
    *probabilityDensityFunctionRR = scatteredPower;
}

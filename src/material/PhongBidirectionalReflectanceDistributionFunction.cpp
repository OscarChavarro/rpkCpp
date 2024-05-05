#include "common/linealAlgebra/CoordinateSystem.h"
#include "material/PhongBidirectionalReflectanceDistributionFunction.h"
#include "material/PhongBidirectionalTransmittanceDistributionFunction.h"
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
phongBrdfEval(
    const PhongBidirectionalReflectanceDistributionFunction *brdf,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    const char flags)
{
    ColorRgb result;
    float tmpFloat;
    float dotProduct;
    Vector3D idealReflected;
    char nonDiffuseFlag;
    Vector3D inRev;
    vectorScale(-1.0, *in, inRev);

    result.clear();

    // kd + ks (idealReflected * out)^n
    if ( vectorDotProduct(*out, *normal) < 0 ) {
        // Refracted ray
        return result;
    }

    if ( (flags & DIFFUSE_COMPONENT) && (brdf->avgKd > 0.0) ) {
        result.addScaled(result, (float)M_1_PI, brdf->Kd);
    }

    if ( PHONG_IS_SPECULAR(*brdf) ) {
        nonDiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nonDiffuseFlag = GLOSSY_COMPONENT;
    }

    if ( (flags & nonDiffuseFlag) && (brdf->avgKs > 0.0) ) {
        idealReflected = idealReflectedDirection(&inRev, normal);
        dotProduct = vectorDotProduct(idealReflected, *out);

        if ( dotProduct > 0 ) {
            tmpFloat = std::pow(dotProduct, brdf->Ns); // cos(a) ^ n
            tmpFloat *= (brdf->Ns + 2.0f) / (2.0f * (float)M_PI); // Ks -> ks
            result.addScaled(result, tmpFloat, brdf->Ks);
        }
    }

    return result;
}

/**
Brdf sampling
*/
Vector3D
phongBrdfSample(
    const PhongBidirectionalReflectanceDistributionFunction *brdf,
    const Vector3D *in,
    const Vector3D *normal,
    int doRussianRoulette,
    char flags,
    double x1,
    double x2,
    double *probabilityDensityFunction)
{
    Vector3D newDir = {0.0, 0.0, 0.0};
    Vector3D idealDir;
    double cosTheta;
    double diffPdf;
    double nonDiffPdf;
    double scatteredPower;
    double avgKd;
    double avgKs;
    float tmpFloat;
    CoordinateSystem coord;
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

        coord.setFromZAxis(normal);
        newDir = coord.sampleHemisphereCosTheta(x1, x2, &diffPdf);

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

        coord.setFromZAxis(&idealDir);
        newDir = coord.sampleHemisphereCosNTheta(brdf->Ns, x1, x2, &nonDiffPdf);

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
evaluateProbabilityDensityFunction(
    const PhongBidirectionalReflectanceDistributionFunction *brdf,
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
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
            nonDiffPdf = (brdf->Ns + 1.0) * std::pow(cos_alpha, brdf->Ns) / (2.0 * M_PI);
        }
    }

    *probabilityDensityFunction = (avgKd * diffPdf + avgKs * nonDiffPdf) / scatteredPower;
    *probabilityDensityFunctionRR = scatteredPower;
}

#include "common/linealAlgebra/CoordinateSystem.h"
#include "material/PhongBidirectionalReflectanceDistributionFunction.h"
#include "material/Xxdf.h"

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

bool
PhongBidirReflDistFunc::isSpecular() const {
    return Ns >= PHONG_LOWEST_SPECULAR_EXP;
}

PhongBidirReflDistFunc::PhongBidirReflDistFunc(
    const ColorRgb *inKd, const ColorRgb *inKs, double inNs) {
    Kd = *inKd;
    avgKd = Kd.average();
    Ks = *inKs;
    avgKs = Ks.average();
    Ns = ((float)(inNs));
}

PhongBidirReflDistFunc::~PhongBidirReflDistFunc() {
}

/**
Returns the diffuse reflectance of the BRDF according to the flags
*/
ColorRgb
PhongBidirReflDistFunc::reflectance(const char flags) const {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;

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

    return ColorRgb(r, g, b);
}

/**
Brdf evaluations
*/
ColorRgb
PhongBidirReflDistFunc::evaluate(
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    char flags) const
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    char nonDiffuseFlag;
    Vector3D inRev;
    inRev.scaledCopy(-1.0, *in);

    // kd + ks (idealReflected * out)^n
    if ( out->dotProduct(*normal) < 0 ) {
        // Refracted ray
        return ColorRgb(0.0f, 0.0f, 0.0f);
    }

    if ( (flags & DIFFUSE_COMPONENT) && (avgKd > 0.0) ) {
        r += M_1_PI * Kd.getR();
        g += M_1_PI * Kd.getG();
        b += M_1_PI * Kd.getB();
    }

    if ( isSpecular() ) {
        nonDiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nonDiffuseFlag = GLOSSY_COMPONENT;
    }

    if ( (flags & nonDiffuseFlag) && (avgKs > 0.0) ) {
        Vector3D idealReflected = Xxdf::idealReflectedDirection(&inRev, normal);
        float localDotProduct = idealReflected.dotProduct(*out);

        if ( localDotProduct > 0 ) {
            float tmpFloat = Math::pow(localDotProduct, Ns); // cos(a) ^ n
            tmpFloat *= (Ns + 2.0f) / (2.0f * ((float)(M_PI))); // Ks -> ks
            r += tmpFloat * Ks.getR();
            g += tmpFloat * Ks.getG();
            b += tmpFloat * Ks.getB();
        }
    }

    return ColorRgb(r, g, b);
}

/**
Brdf sampling
*/
Vector3D
PhongBidirReflDistFunc::sample(
    const Vector3D *in,
    const Vector3D *normal,
    const int doRussianRoulette,
    const char flags,
    double x1,
    const double x2,
    double *probabilityDensityFunction) const
{
    Vector3D inRev;
    inRev.scaledCopy(-1.0, *in);

    *probabilityDensityFunction = 0;

    double localAverageKd;

    if ( flags & DIFFUSE_COMPONENT ) {
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
    Vector3D newDir(0.0, 0.0, 0.0);

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

    Vector3D idealDir = Xxdf::idealReflectedDirection(&inRev, normal);
    CoordinateSystem coord;
    double diffPdf;
    double nonDiffPdf;

    if ( x1 < (localAverageKd / scatteredPower) ) {
        // Sample diffuse
        x1 = x1 / (localAverageKd / scatteredPower);

        // Section [ARVO1995b].2: square-to-sphere mapping in a frame aligned with the surface normal.
        coord.setFromZAxis(normal);
        newDir = coord.sampleHemisphereCosTheta(x1, x2, &diffPdf);

        float tmpFloat = idealDir.dotProduct(newDir);

        if ( tmpFloat > 0 ) {
            nonDiffPdf = (Ns + 1.0) * Math::pow(tmpFloat, Ns) / (2.0 * M_PI);
        } else {
            nonDiffPdf = 0;
        }
    } else {
        // Sample specular
        x1 = (x1 - (localAverageKd / scatteredPower)) / (localAverageKs / scatteredPower);

        // Section [ARVO1995b].2: same 2D random-parameter mapping, but around the ideal reflection axis.
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
PhongBidirReflDistFunc::evalProbDensFunc(
    const Vector3D *in,
    const Vector3D *out,
    const Vector3D *normal,
    char flags,
    double *probabilityDensityFunction,
    double *probabilityDensityFunctionRR) const
{
    double localAverageKs;
    double localAverageKd;
    char nonDiffuseFlag;
    Vector3D inRev;
    Vector3D goodNormal;

    inRev.scaledCopy(-1.0, *in);

    *probabilityDensityFunction = 0;
    *probabilityDensityFunctionRR = 0;

    // Ensure 'in' on the same side as 'normal'!
    double cosIn = in->dotProduct(*normal);
    if ( cosIn >= 0 ) {
        goodNormal.copy(*normal);
    } else {
        goodNormal.scaledCopy(-1, *normal);
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

    double scatteredPower = localAverageKd + localAverageKs;

    if ( scatteredPower < Numeric::EPSILON ) {
        return;
    }

    // Diffuse sampling probabilityDensityFunction
    double diffPdf = 0.0;

    if ( avgKd > 0 ) {
        diffPdf = cosTheta / M_PI;
    }

    // Glossy or specular
    double nonDiffPdf = 0.0;
    if ( avgKs > 0 ) {
        const Vector3D idealDir = Xxdf::idealReflectedDirection(&inRev, &goodNormal);

        const double cosAlpha = idealDir.dotProduct(*out);

        if ( cosAlpha > 0 ) {
            nonDiffPdf = (Ns + 1.0) * Math::pow(cosAlpha, ((double)(Ns))) / (2.0 * M_PI);
        }
    }

    *probabilityDensityFunction = (avgKd * diffPdf + avgKs * nonDiffPdf) / scatteredPower;
    *probabilityDensityFunctionRR = scatteredPower;
}

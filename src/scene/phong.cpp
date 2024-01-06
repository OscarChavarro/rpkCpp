/* phong.c: Phong-type EDFs, BRDFs, BTDFs */

#include <cstdlib>

#include "common/mymath.h"
#include "phong.h"
#include "scene/spherical.h"
#include "common/error.h"

/*
 * The BRDF described in this file (phong.c) is a modified phong-brdf.
 * It satisfies the requirements of symmetry and energy conservation.
 *
 * The BRDF is expressed as:
 *
 * brdf(in, out) = kd + ks*pow(cos(a),n)
 *
 *   where:
 * kd: diffuse coefficient of the BRDF
 * ks: specular coefficient of the BRDF
 * n: specular power
 *   n small : glossy reflectance
 *   n large : specular reflectance (>= PHONG_LOWEST_SPECULAR_EXP)
 * a: angle between the out direction and the perfect mirror direction for in
 *
 * The variables Kd and Ks which are stored in in the PHONG_BRDF type are
 * not the above coefficients, but represent the total energy reflected
 * for light incident perpendicular on the surface.
 *
 * Thus:
 * Kd = kd*pi		or	kd = Kd/pi
 * Ks = ks*2*pi/(n+2)	or	ks = Ks*(n+2)/(2*pi)
 *
 * For this BRDF to be energy conserving, the following condition must be met:
 *
 * Kd + Ks <= 1
 *
 * Some fuctions sample a direction on the hemisphere, given a specific
 * incoming firection, proportional to the value of the Modified Phong BRDF.
 * There are several sampling strategies to achieve this:
 *	rejection sampling		PhongBrdfSampleRejection()
 *	inverse cumulative PDF sampling	PhongBrdfSampleCumPdf()
 *
 * The different sampling functions are commented seperately.
 */

#define NEWPHONGEDF() (PHONG_EDF *)malloc(sizeof(PHONG_EDF))
#define NEWPHONGBRDF() (PHONG_BRDF *)malloc(sizeof(PHONG_BRDF))
#define NEWPHONGBTDF() (PHONG_BTDF *)malloc(sizeof(PHONG_BTDF))

static void
PhongBrdfPrint(FILE *out, PHONG_BRDF *brdf);

/* creates Phong type EDF, BRDF, BTDF data structs */
PHONG_EDF *PhongEdfCreate(COLOR *Kd, COLOR *Ks, double Ns) {
    PHONG_EDF *edf = NEWPHONGEDF();
    edf->Kd = *Kd;
    COLORSCALE((1. / M_PI), edf->Kd, edf->kd);    /* because we use it often */
    edf->Ks = *Ks;
    if ( !COLORNULL(edf->Ks)) {
        Warning("PhongEdfCreate", "Non-diffuse light sources not yet inplemented");
    }
    edf->Ns = Ns;
    return edf;
}

PHONG_BRDF *PhongBrdfCreate(COLOR *Kd, COLOR *Ks, double Ns) {
    PHONG_BRDF *brdf = NEWPHONGBRDF();
    brdf->Kd = *Kd;
    brdf->avgKd = COLORAVERAGE(brdf->Kd);
    brdf->Ks = *Ks;
    brdf->avgKs = COLORAVERAGE(brdf->Ks);
    brdf->Ns = Ns;
    return brdf;
}

PHONG_BTDF *PhongBtdfCreate(COLOR *Kd, COLOR *Ks, double Ns, double nr, double ni) {
    PHONG_BTDF *btdf = NEWPHONGBTDF();
    btdf->Kd = *Kd;
    btdf->avgKd = COLORAVERAGE(btdf->Kd);
    btdf->Ks = *Ks;
    btdf->avgKs = COLORAVERAGE(btdf->Ks);
    btdf->Ns = Ns;
    btdf->refrIndex.nr = nr;
    btdf->refrIndex.ni = ni;
    return btdf;
}

/* prints data for Phong-type edf, ... to the file pointed to by 'out' */
static void PhongEdfPrint(FILE *out, PHONG_EDF *edf) {
    fprintf(out, "Phong Edf: Kd = ");
    edf->Kd.print(out);
    fprintf(out, ", Ks = ");
    edf->Ks.print(out);
    fprintf(out, ", Ns = %g\n", edf->Ns);
}

static void PhongBrdfPrint(FILE *out, PHONG_BRDF *brdf) {
    fprintf(out, "Phong Brdf: Kd = ");
    brdf->Kd.print(out);
    fprintf(out, ", Ks = ");
    brdf->Ks.print(out);
    fprintf(out, ", Ns = %g\n", brdf->Ns);
}

static void PhongBtdfPrint(FILE *out, PHONG_BTDF *btdf) {
    fprintf(out, "Phong Btdf: Kd = ");
    btdf->Kd.print(out);
    fprintf(out, ", Ks = ");
    btdf->Ks.print(out);
    fprintf(out, ", Ns = %g, nr=%g, ni=%g\n", btdf->Ns, btdf->refrIndex.nr, btdf->refrIndex.ni);
}

/* Returns emittance, reflectance, transmittance */
static COLOR PhongEmittance(PHONG_EDF *edf, HITREC *hit, XXDFFLAGS flags) {
    COLOR result;

    colorClear(result);

    if ( flags & DIFFUSE_COMPONENT ) {
        COLORADD(result, edf->Kd, result);
    }

    if ( PHONG_IS_SPECULAR(*edf)) {
        if ( flags & SPECULAR_COMPONENT ) {
            COLORADD(result, edf->Ks, result);
        }
    } else {
        if ( flags & GLOSSY_COMPONENT ) {
            COLORADD(result, edf->Ks, result);
        }
    }

    return result;
}

static COLOR PhongReflectance(PHONG_BRDF *brdf, XXDFFLAGS flags) {
    COLOR result;

    colorClear(result);

    if ( flags & DIFFUSE_COMPONENT ) {
        COLORADD(result, brdf->Kd, result);
    }

    if ( PHONG_IS_SPECULAR(*brdf)) {
        if ( flags & SPECULAR_COMPONENT ) {
            COLORADD(result, brdf->Ks, result);
        }
    } else {
        if ( flags & GLOSSY_COMPONENT ) {
            COLORADD(result, brdf->Ks, result);
        }
    }

    return result;
}

static COLOR PhongTransmittance(PHONG_BTDF *btdf, XXDFFLAGS flags) {
    COLOR result;

    colorClear(result);

    if ( flags & DIFFUSE_COMPONENT ) {
        COLORADD(result, btdf->Kd, result);
    }

    if ( PHONG_IS_SPECULAR(*btdf)) {
        if ( flags & SPECULAR_COMPONENT ) {
            COLORADD(result, btdf->Ks, result);
        }
    } else {
        if ( flags & GLOSSY_COMPONENT ) {
            COLORADD(result, btdf->Ks, result);
        }
    }

    if ( !std::isfinite(COLORAVERAGE(result))) {
        Fatal(-1, "PhongTransmittance", "Oops - result is not finite!");
    }

    return result;
}


/* Refraction index */
void PhongIndexOfRefraction(PHONG_BTDF *btdf, REFRACTIONINDEX *index) {
    index->nr = btdf->refrIndex.nr;
    index->ni = btdf->refrIndex.ni;
}


/* Edf evaluations */
static COLOR PhongEdfEval(PHONG_EDF *edf, HITREC *hit, Vector3D *out, XXDFFLAGS flags, double *pdf) {
    Vector3D normal;
    COLOR result;
    double cosl;

    colorClear(result);
    if ( pdf ) {
        *pdf = 0.;
    }

    if ( !HitShadingNormal(hit, &normal)) {
        Warning("PhongEdfEval", "Couldn't determine shading normal");
        return result;
    }

    cosl = VECTORDOTPRODUCT(*out, normal);

    if ( cosl < 0.0 ) {
        return result;
    } /* Back face of a light does not radiate */

    /* kd + ks (idealreflected * out)^n */

    if ( flags & DIFFUSE_COMPONENT ) {
        /* divide by PI to turn radiant exitance [W/m^2] into exitant radiance [W/m^2 sr] */
        COLORADD(result, edf->kd, result);
        if ( pdf )
            *pdf = cosl / M_PI;
    }

    if ( flags & SPECULAR_COMPONENT ) {
        /* ??? */
    }

    return result;
}

/* Edf sampling */
static Vector3D PhongEdfSample(PHONG_EDF *edf, HITREC *hit, XXDFFLAGS flags,
                               double xi1, double xi2,
                               COLOR *selfemitted_radiance, double *pdf) {
    Vector3D dir = {0., 0., 1.};
    if ( selfemitted_radiance ) {
        colorClear(*selfemitted_radiance);
    }
    if ( pdf ) {
        *pdf = 0.;
    }

    if ( flags & DIFFUSE_COMPONENT ) {
        double spdf;
        COORDSYS coord;

        Vector3D normal;
        if ( !HitShadingNormal(hit, &normal)) {
            Warning("PhongEdfEval", "Couldn't determine shading normal");
            return dir;
        }

        VectorCoordSys(&normal, &coord);
        dir = SampleHemisphereCosTheta(&coord, xi1, xi2, &spdf);
        if ( pdf ) {
            *pdf = spdf;
        }
        if ( selfemitted_radiance ) {
            COLORSCALE((1. / M_PI), edf->Kd, *selfemitted_radiance);
        }
    }

    /* Other components not yet implemented, and they probably never will: The
     * VRML parser with extensions for physically based rendering contains all this
     * stuff already and much cleaner.*/

    return dir;
}

/* Brdf evaluations */
static COLOR PhongBrdfEval(PHONG_BRDF *brdf, Vector3D *in, Vector3D *out, Vector3D *normal, XXDFFLAGS flags) {
    COLOR result;
    float tmpFloat, dotProduct;
    Vector3D idealReflected;
    XXDFFLAGS nondiffuseFlag;
    Vector3D inrev;
    VECTORSCALE(-1., *in, inrev);

    colorClear(result);

    /* kd + ks (idealreflected * out)^n */

    if ( VECTORDOTPRODUCT(*out, *normal) < 0 ) {
        /* refracted ray ! */
        return result;
    }

    if ((flags & DIFFUSE_COMPONENT) && (brdf->avgKd > 0.0)) {
        COLORADDSCALED(result, M_1_PI, brdf->Kd, result);
    }

    if ( PHONG_IS_SPECULAR(*brdf)) {
        nondiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nondiffuseFlag = GLOSSY_COMPONENT;
    }


    if ((flags & nondiffuseFlag) && (brdf->avgKs > 0.0)) {
        idealReflected = IdealReflectedDirection(&inrev, normal);
        dotProduct = VECTORDOTPRODUCT(idealReflected, *out);

        if ( dotProduct > 0 ) {
            tmpFloat = (float) pow(dotProduct, brdf->Ns); /* cos(a) ^ n */
            tmpFloat *= ((int) brdf->Ns + 2.0) / (2 * M_PI); /* Ks -> ks */
            COLORADDSCALED(result, tmpFloat, brdf->Ks, result);
        }
    }

    return result;
}


/* Brdf sampling */
static Vector3D PhongBrdfSample(PHONG_BRDF *brdf, Vector3D *in,
                                Vector3D *normal, int doRussianRoulette,
                                XXDFFLAGS flags, double x_1, double x_2,
                                double *pdf) {
    Vector3D newDir = {0., 0., 0.}, idealDir;
    double cos_theta, diffPdf, nonDiffPdf;
    double scatteredPower, avgKd, avgKs;
    float tmpFloat;
    COORDSYS coord;
    XXDFFLAGS nondiffuseFlag;
    Vector3D inrev;
    VECTORSCALE(-1., *in, inrev);

    *pdf = 0;

    if ( flags & DIFFUSE_COMPONENT ) {
        avgKd = brdf->avgKd;
    } else {
        avgKd = 0.0;
    }

    if ( PHONG_IS_SPECULAR(*brdf)) {
        nondiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nondiffuseFlag = GLOSSY_COMPONENT;
    }


    if ( flags & nondiffuseFlag ) {
        avgKs = brdf->avgKs;
    } else {
        avgKs = 0.0;
    }

    scatteredPower = avgKd + avgKs;

    if ( scatteredPower < EPSILON ) {
        return newDir;
    }

    /* determine diffuse or glossy/specular sampling */

    if ( doRussianRoulette ) {
        if ( x_1 > scatteredPower ) {
            /* Absorption */
            return newDir;
        }

        /* Rescaling of x_1 */
        x_1 /= scatteredPower;
    }

    idealDir = IdealReflectedDirection(&inrev, normal);

    if ( x_1 < (avgKd / scatteredPower)) {
        /* Sample diffuse */
        x_1 = x_1 / (avgKd / scatteredPower);

        VectorCoordSys(normal, &coord);
        newDir = SampleHemisphereCosTheta(&coord, x_1, x_2, &diffPdf);
        /* newDir = SampleHemisphereUniform(&coord, x_1, x_2, &diffPdf); */

        tmpFloat = VECTORDOTPRODUCT(idealDir, newDir);

        if ( tmpFloat > 0 ) {
            nonDiffPdf = (brdf->Ns + 1.0) * pow(tmpFloat,
                                                brdf->Ns) / (2.0 * M_PI);
        } else {
            nonDiffPdf = 0;
        }
    } else {
        /* Sample specular */
        x_1 = (x_1 - (avgKd / scatteredPower)) / (avgKs / scatteredPower);

        VectorCoordSys(&idealDir, &coord);
        newDir = SampleHemisphereCosNTheta(&coord, brdf->Ns, x_1, x_2,
                                           &nonDiffPdf);

        cos_theta = VECTORDOTPRODUCT(*normal, newDir);
        if ( cos_theta <= 0 ) {
            return newDir;
        }

        /* diffPdf = 1 / (2.0 * M_PI); */
        diffPdf = cos_theta / M_PI;
    }

    /* Combine pdf's */

    *pdf = avgKd * diffPdf + avgKs * nonDiffPdf;

    if ( !doRussianRoulette ) {
        *pdf /= scatteredPower;
    }

    return newDir;
}

static void PhongBrdfEvalPdf(PHONG_BRDF *brdf, Vector3D *in,
                             Vector3D *out, Vector3D *normal,
                             XXDFFLAGS flags, double *pdf, double *pdfRR) {
    double cos_theta, cos_alpha, cos_in;
    double diffPdf, nonDiffPdf, scatteredPower;
    double avgKs, avgKd;
    XXDFFLAGS nondiffuseFlag;
    Vector3D idealDir;
    Vector3D inrev;
    Vector3D goodNormal;

    VECTORSCALE(-1., *in, inrev);

    *pdf = 0;
    *pdfRR = 0;

    /* ensure 'in' on the same side as 'normal' ! */

    cos_in = VECTORDOTPRODUCT(*in, *normal);
    if ( cos_in >= 0 ) {
        VECTORCOPY(*normal, goodNormal);
    } else {
        VECTORSCALE(-1, *normal, goodNormal);
    }

    cos_theta = VECTORDOTPRODUCT(goodNormal, *out);

    if ( cos_theta < 0 ) {
        return;
    }

    /* 'out' is a reflected direction */
    if ( flags & DIFFUSE_COMPONENT ) {
        avgKd = brdf->avgKd;  /* -- Store in phong data ? -- */
    } else {
        avgKd = 0.0;
    }

    if ( PHONG_IS_SPECULAR(*brdf)) {
        nondiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nondiffuseFlag = GLOSSY_COMPONENT;
    }


    if ( flags & nondiffuseFlag ) {
        avgKs = brdf->avgKs;
    } else {
        avgKs = 0.0;
    }

    scatteredPower = avgKd + avgKs;

    if ( scatteredPower < EPSILON ) {
        return;
    }

    /* Diffuse sampling pdf */
    diffPdf = 0.0;

    if ( avgKd > 0 ) {
        diffPdf = cos_theta / M_PI;
    }

    /* Glossy or specular */
    nonDiffPdf = 0.0;
    if ( avgKs > 0 ) {
        idealDir = IdealReflectedDirection(&inrev, &goodNormal);

        cos_alpha = VECTORDOTPRODUCT(idealDir, *out);

        if ( cos_alpha > 0 ) {
            nonDiffPdf = (brdf->Ns + 1.0) * pow(cos_alpha,
                                                brdf->Ns) / (2.0 * M_PI);
        }
    }

    *pdf = (avgKd * diffPdf + avgKs * nonDiffPdf) / scatteredPower;
    *pdfRR = scatteredPower;

    return;
}


/* Btdf evaluations */
static COLOR PhongBtdfEval(PHONG_BTDF *btdf, REFRACTIONINDEX inIndex, REFRACTIONINDEX outIndex, Vector3D *in, Vector3D *out,
                           Vector3D *normal, XXDFFLAGS flags) {
    COLOR result;
    float tmpFloat, dotProduct;
    Vector3D idealrefracted;
    int totalIR;
    int IsReflection;
    XXDFFLAGS nondiffuseFlag;
    Vector3D inrev;
    VECTORSCALE(-1., *in, inrev);

    /* Specular-like refraction can turn into reflection.
       So for refraction a complete sphere should be
       sampled ! Importance sampling is advisable.
       Diffuse transmission is considered to always pass
       the material boundary
       */

    colorClear(result);

    if ( (flags & DIFFUSE_COMPONENT) && (btdf->avgKd > 0) ) {
        /* Diffuse part */

        /* Normal is pointing away from refracted direction */

        IsReflection = (VECTORDOTPRODUCT(*normal, *out) >= 0);

        if ( !IsReflection ) {
            result = btdf->Kd;
            COLORSCALE(M_1_PI, result, result);
        }
    }

    if ( PHONG_IS_SPECULAR(*btdf)) {
        nondiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nondiffuseFlag = GLOSSY_COMPONENT;
    }


    if ((flags & nondiffuseFlag) && (btdf->avgKs > 0)) {
        /* Specular part */

        idealrefracted = IdealRefractedDirection(&inrev, normal, inIndex,
                                                 outIndex, &totalIR);

        dotProduct = VECTORDOTPRODUCT(idealrefracted, *out);

        if ( dotProduct > 0 ) {
            tmpFloat = (float) pow(dotProduct, btdf->Ns); /* cos(a) ^ n */
            tmpFloat *= (btdf->Ns + 2.0) / (2 * M_PI); /* Ks -> ks */
            COLORADDSCALED(result, tmpFloat, btdf->Ks, result);
        }
    }

    return result;
}

static Vector3D PhongBtdfSample(PHONG_BTDF *btdf, REFRACTIONINDEX inIndex,
                                REFRACTIONINDEX outIndex, Vector3D *in,
                                Vector3D *normal, int doRussianRoulette,
                                XXDFFLAGS flags, double x_1, double x_2,
                                double *pdf) {
    Vector3D newDir = {0., 0., 0.};
    int totalIR;
    Vector3D idealDir, invNormal;
    COORDSYS coord;
    double cos_theta;
    double avgKd, avgKs, scatteredPower;
    double diffPdf, nonDiffPdf;
    float tmpFloat;
    XXDFFLAGS nondiffuseFlag;
    Vector3D inrev;
    VECTORSCALE(-1., *in, inrev);

    *pdf = 0;

    /* Chosse sampling mode */
    if ( flags & DIFFUSE_COMPONENT ) {
        avgKd = btdf->avgKd;  /* -- Store in phong data ? -- */
    } else {
        avgKd = 0.0;
    }

    if ( PHONG_IS_SPECULAR(*btdf)) {
        nondiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nondiffuseFlag = GLOSSY_COMPONENT;
    }


    if ( flags & nondiffuseFlag ) {
        avgKs = btdf->avgKs;
    } else {
        avgKs = 0.0;
    }

    scatteredPower = avgKd + avgKs;

    if ( scatteredPower < EPSILON ) {
        return newDir;
    }

    /* determine diffuse or glossy/specular sampling */
    if ( doRussianRoulette ) {
        if ( x_1 > scatteredPower ) {
            /* Absorption */
            return newDir;
        }

        /* Rescaling of x_1 */
        x_1 /= scatteredPower;
    }

    idealDir = IdealRefractedDirection(&inrev, normal, inIndex, outIndex,
                                       &totalIR);
    VECTORSCALE(-1, *normal, invNormal);

    if ( x_1 < (avgKd / scatteredPower)) {
        /* Sample diffuse */
        x_1 = x_1 / (avgKd / scatteredPower);

        VectorCoordSys(&invNormal, &coord);

        newDir = SampleHemisphereCosTheta(&coord, x_1, x_2, &diffPdf);
        /* newDir = SampleHemisphereUniform(&coord, x_1, x_2, &diffPdf); */

        tmpFloat = VECTORDOTPRODUCT(idealDir, newDir);

        if ( tmpFloat > 0 ) {
            nonDiffPdf = (btdf->Ns + 1.0) * pow(tmpFloat,
                                                btdf->Ns) / (2.0 * M_PI);
        } else {
            nonDiffPdf = 0;
        }
    } else {
        /* Sample specular */
        x_1 = (x_1 - (avgKd / scatteredPower)) / (avgKs / scatteredPower);

        VectorCoordSys(&idealDir, &coord);
        newDir = SampleHemisphereCosNTheta(&coord, btdf->Ns, x_1, x_2,
                                           &nonDiffPdf);

        cos_theta = VECTORDOTPRODUCT(*normal, newDir);
        if ( cos_theta > 0 ) {
            /* diffPdf = 1 / (2.0 * M_PI); */
            diffPdf = cos_theta / M_PI;
        } else {
            /* assume totalIR (maybe we should test the refractionindices */

            diffPdf = 0.0;
        }
    }

    /* Combine pdf's */
    *pdf = avgKd * diffPdf + avgKs * nonDiffPdf;

    if ( !doRussianRoulette ) {
        *pdf /= scatteredPower;
    }


    return newDir;
}

static void PhongBtdfEvalPdf(PHONG_BTDF *btdf, REFRACTIONINDEX inIndex,
                             REFRACTIONINDEX outIndex, Vector3D *in,
                             Vector3D *out, Vector3D *normal,
                             XXDFFLAGS flags,
                             double *pdf, double *pdfRR) {
    double cos_theta, cos_alpha, cos_in;
    double diffPdf, nonDiffPdf = 0., scatteredPower;
    double avgKs, avgKd;
    XXDFFLAGS nondiffuseFlag;
    Vector3D idealDir;
    int totalIR;
    Vector3D goodNormal;
    Vector3D inrev;
    VECTORSCALE(-1., *in, inrev);

    *pdf = 0;
    *pdfRR = 0;

    /* ensure 'in' on the same side as 'normal' ! */

    cos_in = VECTORDOTPRODUCT(*in, *normal);
    if ( cos_in >= 0 ) {
        VECTORCOPY(*normal, goodNormal);
    } else {
        VECTORSCALE(-1, *normal, goodNormal);
    }

    cos_theta = VECTORDOTPRODUCT(goodNormal, *out);

    if ( flags & DIFFUSE_COMPONENT && (cos_theta < 0))  /* transmitted ray */
    {
        avgKd = btdf->avgKd;
    } else {
        avgKd = 0.0;
    }

    if ( PHONG_IS_SPECULAR(*btdf)) {
        nondiffuseFlag = SPECULAR_COMPONENT;
    } else {
        nondiffuseFlag = GLOSSY_COMPONENT;
    }


    if ( flags & nondiffuseFlag ) {
        avgKs = btdf->avgKs;
    } else {
        avgKs = 0.0;
    }

    scatteredPower = avgKd + avgKs;

    if ( scatteredPower < EPSILON ) {
        return;
    }

    /* Diffuse sampling pdf */
    if ( avgKd > 0 ) {
        /* diffPdf = 1.0 / (2.0 * M_PI); */
        diffPdf = cos_theta / M_PI;
    } else {
        diffPdf = 0.0;
    }

    /* Glossy or specular */
    if ( avgKs > 0 ) {
        if ( cos_in >= 0 ) {
            idealDir = IdealRefractedDirection(&inrev, &goodNormal, inIndex,
                                               outIndex, &totalIR);
        } else {
            /* Normal was inverted, so materialsides switch also */
            idealDir = IdealRefractedDirection(&inrev, &goodNormal, outIndex,
                                               inIndex, &totalIR);
        }

        cos_alpha = VECTORDOTPRODUCT(idealDir, *out);

        nonDiffPdf = 0.0;
        if ( cos_alpha > 0 ) {
            nonDiffPdf = (btdf->Ns + 1.0) * pow(cos_alpha,
                                                btdf->Ns) / (2.0 * M_PI);
        }
    }

    *pdf = (avgKd * diffPdf + avgKs * nonDiffPdf) / scatteredPower;
    *pdfRR = scatteredPower;

    return;
}


/* Phong-type edf... method structs */
EDF_METHODS PhongEdfMethods = {
        (COLOR (*)(void *, HITREC *, XXDFFLAGS)) PhongEmittance,
        (int (*)(void *)) nullptr,    /* not textured */
        (COLOR (*)(void *, HITREC *, Vector3D *, XXDFFLAGS, double *)) PhongEdfEval,
        (Vector3D (*)(void *, HITREC *, XXDFFLAGS, double, double, COLOR *, double *)) PhongEdfSample,
        (int (*)(void *, HITREC *, Vector3D *, Vector3D *, Vector3D *)) nullptr,
        (void (*)(FILE *, void *)) PhongEdfPrint
};

BRDF_METHODS PhongBrdfMethods = {
        (COLOR (*)(void *, XXDFFLAGS)) PhongReflectance,
        (COLOR (*)(void *, Vector3D *, Vector3D *, Vector3D *, XXDFFLAGS)) PhongBrdfEval,
        (Vector3D (*)(void *, Vector3D *, Vector3D *, int, XXDFFLAGS, double, double,
                      double *)) PhongBrdfSample,
        (void (*)(void *, Vector3D *, Vector3D *, Vector3D *,
                  XXDFFLAGS, double *, double *)) PhongBrdfEvalPdf,
        (void (*)(FILE *, void *)) PhongBrdfPrint
};

BTDF_METHODS PhongBtdfMethods = {
        (COLOR (*)(void *, XXDFFLAGS)) PhongTransmittance,
        reinterpret_cast<void (*)(void *, REFRACTIONINDEX *)>((void (*)()) PhongIndexOfRefraction),
        (COLOR (*)(void *, REFRACTIONINDEX, REFRACTIONINDEX, Vector3D *, Vector3D *, Vector3D *, XXDFFLAGS)) PhongBtdfEval,
        (Vector3D (*)(void *, REFRACTIONINDEX, REFRACTIONINDEX, Vector3D *,
                      Vector3D *, int, XXDFFLAGS, double, double,
                      double *)) PhongBtdfSample,
        (void (*)(void *, REFRACTIONINDEX, REFRACTIONINDEX, Vector3D *,
                  Vector3D *, Vector3D *, XXDFFLAGS, double *, double *)) PhongBtdfEvalPdf,
        (void (*)(FILE *, void *)) PhongBtdfPrint
};

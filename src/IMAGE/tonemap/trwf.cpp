/**
Tumblin/Rushmeier/Ward/Ferwerda tone maps (Jan Prikryl)
*/

#include "common/error.h"
#include "material/rgb.h"
#include "material/color.h"
#include "material/cie.h"
#include "common/mymath.h"
#include "IMAGE/tonemap/trwf.h"

/**
REFERENCES:

J. Tumblin, H.E. Rushmeier. Tone Reproduction for Realistic Images,
IEEE Computer Graphics and Applications, 13:6, 1993, pp. 42-48.

G. Ward. A Contrast-Based Scalefactor for Luminance Display, Graphics
Gems IV, Academic Press, 1994, pp. 415-421.

J. Tumblin, J.K. Hodgins, B.K. Guenter. Two Methods for Display of High
Contrast Images, ACM Transactions on Graphics, 18:1, 1999, pp. 56-94.

J.A. Ferwerda, S.N. Pattanaik, P. Shirley, D. Greenberg. A Model of
Visual Adaptation for Realistic Image Synthesis, SIGGRAPH 1996,
pp. 249-258.
*/

// Precomputed parameters for different tone mapping methods
static RGB f_sf = {0.062f, 0.608f, 0.330f};
static float f_msf;
static float f_pm_comp;
static float f_pm_disp;
static float f_sm_comp;
static float f_sm_disp;
static float invcmax;
static float lrwm_comp;
static float lrwm_disp;
static float lrwexponent;
static float m_comp;
static float m_disp;
static float g;
static float r_comp;
static float r_disp;

// Environment parameters
static float _lwa;
static float _ldaTumb;
static float _ldaWard;

static float
stevensGamma(float lum) {
    if ( lum > 100.0 ) {
        return 2.655;
    } else {
        return 1.855 + 0.4 * log10(lum + 2.3e-5);
    }
}

static float
photopicOperator(float logLa) {
    float r;
    if ( logLa <= -2.6 ) {
        r = -0.72;
    } else if ( logLa >= 1.9 ) {
        r = logLa - 1.255;
    } else {
        r = pow(0.249 * logLa + 0.65, 2.7) - 0.72;
    }

    return std::pow(10.0, r);
}

static float
scotopicOperator(float logLa) {
    float r;
    if ( logLa <= -3.94 ) {
        r = -2.86;
    } else if ( logLa >= -1.44 ) {
        r = logLa - 0.395;
    } else {
        r = pow(0.405 * logLa + 1.6, 2.18) - 2.86;
    }

    return pow(10.0, r);
}

static float
mesopicScaleFactor(float logLwa) {
    if ( logLwa < -2.5 ) {
        return 1.0;
    } else if ( logLwa > 0.8 ) {
        return 0.0;
    } else {
        return (0.8 - logLwa) / 3.3;
    }
}

static void
trwfDefaults() {
}

static void
trwfInit() {
    float lwa = _lwa = GLOBAL_toneMap_options.lwa;
    float ldmax = GLOBAL_toneMap_options.ldm;
    float cmax = GLOBAL_toneMap_options.cmax;

    float alpharw;
    float betarw;
    float alphad;
    float betad;
    float gwd;

    // Tumblin & Rushmeier
    _ldaTumb = ldmax / sqrt(cmax);

    {
        float l10 = log10(TMO_CANDELA_LAMBERT(lwa));
        alpharw = 0.4 * l10 + 2.92;
        betarw = -0.4 * (l10 * l10) - 2.584 * l10 + 2.0208;
    }

    {
        float l10 = log10(TMO_CANDELA_LAMBERT(_ldaTumb));
        alphad = 0.4 * l10 + 2.92;
        betad = -0.4 * (l10 * l10) - 2.584 * l10 + 2.0208;
    }

    lrwexponent = alpharw / alphad;
    lrwm_comp = pow(10.0, (betarw - betad) / alphad);
    lrwm_disp = lrwm_comp / (TMO_CANDELA_LAMBERT(ldmax));
    invcmax = 1.0 / cmax;

    // Ward
    _ldaWard = ldmax / 2.0;
    {
        double p1 = pow(_ldaWard, 0.4);
        double p2 = pow(lwa, 0.4);
        double p3 = (1.219 + p1) / (1.219 + p2);

        m_comp = pow(p3, 2.5);
    }

    m_disp = m_comp / ldmax;

    // Ferwerda
    f_msf = mesopicScaleFactor(log10(lwa));
    f_sm_comp = scotopicOperator(log10(_ldaWard)) /
                scotopicOperator(log10(lwa));
    f_pm_comp = photopicOperator(log10(_ldaWard)) /
                photopicOperator(log10(lwa));
    f_sm_disp = f_sm_comp / ldmax;
    f_pm_disp = f_pm_comp / ldmax;

    // Revised Tumblin & Rushmeier
    g = stevensGamma(lwa) / stevensGamma(_ldaTumb);
    gwd = stevensGamma(lwa) / (1.855 + 0.4 * log(_ldaTumb));
    r_comp = pow(sqrt(cmax), gwd - 1) * _ldaTumb;
    r_disp = r_comp / ldmax;
}

static void
trwfTerminate() {
}

static COLOR
trwfScaleForComputations(COLOR radiance) {
    float rwl;
    float scale;

    rwl = colorLuminance(radiance);

    if ( rwl > 0.0 ) {
        float m = TMO_LAMBERT_CANDELA(
                (pow(TMO_CANDELA_LAMBERT(rwl), lrwexponent) * lrwm_comp));
        scale = m > 0.0 ? m / rwl : 0.0;
    } else {
        scale = 0.0;
    }

    colorScale(scale, radiance, radiance);
    return radiance;
}

static COLOR
trwfScaleForDisplay(COLOR radiance) {
    float rwl;
    float scale;
    float eff;

    rwl = M_PI * colorLuminance(radiance);

    getLuminousEfficacy(&eff);
    colorScale(eff * M_PI, radiance, radiance);

    if ( rwl > 0.0 ) {
        float m = (pow(TMO_CANDELA_LAMBERT(rwl), lrwexponent) * lrwm_disp - invcmax);
        scale = m > 0.0 ? m / rwl : 0.0;
    } else {
        scale = 0.0;
    }

    colorScale(scale, radiance, radiance);
    return radiance;
}

static float
trfwReverseScaleForComputations(float dl) {
    if ( dl > 0.0 ) {
        return (float)pow(dl * 3.14e-4 / lrwm_comp, 1.0 / lrwexponent) /
                (float)(3.14e-4 * dl);
    } else {
        return 0.0f;
    }
}

TONEMAP GLOBAL_toneMap_tumblinRushmeier = {
    "Tumblin/Rushmeier's Mapping",
    "TumblinRushmeier",
    "tmoTmblRushButton",
    3,
    trwfDefaults,
    (void (*)(int *, char **)) nullptr,
    (void (*)(FILE *)) nullptr,
    trwfInit,
    trwfTerminate,
    trwfScaleForComputations,
    trwfScaleForDisplay,
    trfwReverseScaleForComputations,
    (void (*)(void *)) nullptr,
    (void (*)(void *)) nullptr,
    (void (*)()) nullptr,
    (void (*)()) nullptr
};

static COLOR
wardScaleForComputations(COLOR radiance) {
    colorScale(m_comp, radiance, radiance);
    return radiance;
}

static COLOR
wardScaleForDisplay(COLOR radiance) {
    float eff;

    getLuminousEfficacy(&eff);

    colorScale(eff * m_disp, radiance, radiance);
    return radiance;
}

static float
wardReverseScaleForComputations(float dl) {
    return 1.0 / m_comp;
}

TONEMAP GLOBAL_toneMap_ward = {
    "Ward's Mapping",
    "Ward",
    "tmoWardButton",
    3,
    trwfDefaults,
    (void (*)(int *, char **)) nullptr,
    (void (*)(FILE *)) nullptr,
    trwfInit,
    trwfTerminate,
    wardScaleForComputations,
    wardScaleForDisplay,
    wardReverseScaleForComputations,
    (void (*)(void *)) nullptr,
    (void (*)(void *)) nullptr,
    (void (*)()) nullptr,
    (void (*)()) nullptr
};

static COLOR
revisedTRScaleForComputations(COLOR radiance) {
    float rwl;
    float scale;

    rwl = colorLuminance(radiance);

    if ( rwl > 0.0 ) {
        scale = r_comp * pow(rwl / _lwa, g) / rwl;
    } else {
        scale = 0.0;
    }

    colorScale(scale, radiance, radiance);
    return radiance;
}

static COLOR
revisedTRScaleForDisplay(COLOR radiance) {
    float rwl;
    float scale;
    float eff;

    rwl = M_PI * colorLuminance(radiance);

    getLuminousEfficacy(&eff);
    colorScale(eff * M_PI, radiance, radiance);

    if ( rwl > 0.0 ) {
        scale = r_disp * (float)pow(rwl / _lwa, g) / rwl;
    } else {
        scale = 0.0f;
    }

    colorScale(scale, radiance, radiance);
    return radiance;
}

static float
revisedTRReverseScaleForComputations(float dl) {
    if ( dl > 0.0 ) {
        return _lwa * std::pow(dl / r_comp, 1.0 / g) / dl;
    } else {
        return 0.0;
    }
}

TONEMAP GLOBAL_toneMap_revisedTumblinRushmeier = {
    "Revised Tumblin/Rushmeier's Mapping",
    "RevisedTR",
    "tmoRevisedTmblRushButton",
    3,
    trwfDefaults,
    (void (*)(int *, char **)) nullptr,
    (void (*)(FILE *)) nullptr,
    trwfInit,
    trwfTerminate,
    revisedTRScaleForComputations,
    revisedTRScaleForDisplay,
    revisedTRReverseScaleForComputations,
    (void (*)(void *)) nullptr,
    (void (*)(void *)) nullptr,
    (void (*)()) nullptr,
    (void (*)()) nullptr
};

static COLOR
ferwerdaScaleForComputations(COLOR radiance) {
    RGB p;
    float sl;
    float eff;

    // Convert to photometric values
    getLuminousEfficacy(&eff);
    colorScale(eff, radiance, radiance);

    // Compute the scotopic grayscale shift
    convertColorToRGB(radiance, &p);
    sl = f_sm_comp * f_msf * (p.r * f_sf.r + p.g * f_sf.g + p.b * f_sf.b);

    // Scale the photopic luminance
    colorScale(f_pm_comp, radiance, radiance);

    // Eventually, offset by the scotopic luminance
    if ( sl > 0.0 ) {
        colorAddConstant(radiance, sl, radiance);
    }

    return radiance;
}

static COLOR
ferwerdaScaleForDisplay(COLOR radiance) {
    RGB p;
    float sl;
    float eff;

    // Convert to photometric values
    getLuminousEfficacy(&eff);
    colorScale(eff, radiance, radiance);

    // Compute the scotopic grayscale shift
    convertColorToRGB(radiance, &p);
    sl = f_sm_disp * f_msf * (p.r * f_sf.r + p.g * f_sf.g + p.b * f_sf.b);

    // Scale the photopic luminance
    colorScale(f_pm_disp, radiance, radiance);

    // Eventually, offset by the scotopic luminance
    if ( sl > 0.0 ) {
        colorAddConstant(radiance, sl, radiance);
    }

    return radiance;
}

static float
ferwerdaReverseScaleForComputations(float dl) {
    logFatal(-1, "ferwerdaReverseScaleForComputations", "Not yet implemented");
    return -1.0;
}

TONEMAP GLOBAL_toneMap_ferwerda = {
    "Partial Ferwerda's Mapping", "Ferwerda", "tmoFerwerdaButton", 3,
    trwfDefaults,
    (void (*)(int *, char **)) nullptr,
    (void (*)(FILE *)) nullptr,
    trwfInit,
    trwfTerminate,
    ferwerdaScaleForComputations,
    ferwerdaScaleForDisplay,
    ferwerdaReverseScaleForComputations,
    (void (*)(void *)) nullptr,
    (void (*)(void *)) nullptr,
    (void (*)()) nullptr,
    (void (*)()) nullptr
};

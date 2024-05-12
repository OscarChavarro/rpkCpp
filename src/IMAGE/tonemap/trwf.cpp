/**
Tumblin/Rushmeier/Ward/Ferwerda tone maps (Jan Prikryl)
*/

#include "java/lang/Math.h"
#include "common/ColorRgb.h"
#include "common/cie.h"
#include "IMAGE/tonemap/trwf.h"

/**
REFERENCES:

[TUMB1993] J. Tumblin, H.E. Rushmeier. Tone Reproduction for Realistic Images,
IEEE Computer Graphics and Applications, 13:6, 1993, pp. 42-48.

G. Ward. A Contrast-Based Scale factor for Luminance Display, Graphics
Gems IV, Academic Press, 1994, pp. 415-421.

[TUMB1999b] J. Tumblin, J.K. Hodgins, B.K. Guenter. Two Methods for Display of High
Contrast Images, ACM Transactions on Graphics, 18:1, 1999, pp. 56-94.

J.A. Ferwerda, S.N. Pattanaik, P. Shirley, D. Greenberg. A Model of
Visual Adaptation for Realistic Image Synthesis, SIGGRAPH 1996,
pp. 249-258.
*/

// Precomputed parameters for different tone mapping methods
static ColorRgb f_sf(0.062f, 0.608f, 0.330f);
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
        return 2.655f;
    } else {
        return 1.855f + 0.4f * std::log10(lum + 2.3e-5f);
    }
}

static float
photopicOperator(float logLa) {
    float r;
    if ( logLa <= -2.6 ) {
        r = -0.72f;
    } else if ( logLa >= 1.9 ) {
        r = logLa - 1.255f;
    } else {
        r = java::Math::pow(0.249f * logLa + 0.65f, 2.7f) - 0.72f;
    }

    return java::Math::pow(10.0f, r);
}

static float
scotopicOperator(float logLa) {
    float r;
    if ( logLa <= -3.94 ) {
        r = -2.86f;
    } else if ( logLa >= -1.44 ) {
        r = logLa - 0.395f;
    } else {
        r = java::Math::pow(0.405f * logLa + 1.6f, 2.18f) - 2.86f;
    }

    return java::Math::pow(10.0f, r);
}

static float
mesopicScaleFactor(float logLwa) {
    if ( logLwa < -2.5 ) {
        return 1.0;
    } else if ( logLwa > 0.8 ) {
        return 0.0;
    } else {
        return (0.8f - logLwa) / 3.3f;
    }
}

static void
trwfDefaults() {
}

static void
trwfInit() {
    float lwa = _lwa = GLOBAL_toneMap_options.realWorldAdaptionLuminance;
    float ldmax = GLOBAL_toneMap_options.maximumDisplayLuminance;
    float cmax = GLOBAL_toneMap_options.maximumDisplayContrast;

    float alpharw;
    float betarw;
    float alphad;
    float betad;
    float gwd;

    // Tumblin & Rushmeier
    _ldaTumb = ldmax / java::Math::sqrt(cmax);

    {
        float l10 = std::log10(tmoCandelaLambert(lwa));
        alpharw = 0.4f * l10 + 2.92f;
        betarw = -0.4f * (l10 * l10) - 2.584f * l10 + 2.0208f;
    }

    {
        float l10 = std::log10(tmoCandelaLambert(_ldaTumb));
        alphad = 0.4f * l10 + 2.92f;
        betad = -0.4f * (l10 * l10) - 2.584f * l10 + 2.0208f;
    }

    lrwexponent = alpharw / alphad;
    lrwm_comp = java::Math::pow(10.0f, (betarw - betad) / alphad);
    lrwm_disp = lrwm_comp / (tmoCandelaLambert(ldmax));
    invcmax = 1.0f / cmax;

    // Ward
    _ldaWard = ldmax / 2.0f;
    {
        float p1 = java::Math::pow(_ldaWard, 0.4f);
        float p2 = java::Math::pow(lwa, 0.4f);
        float p3 = (1.219f + p1) / (1.219f + p2);
        m_comp = java::Math::pow(p3, 2.5f);
    }

    m_disp = m_comp / ldmax;

    // Ferwerda
    f_msf = mesopicScaleFactor(std::log10(lwa));
    f_sm_comp = scotopicOperator(std::log10(_ldaWard)) /
                scotopicOperator(std::log10(lwa));
    f_pm_comp = photopicOperator(std::log10(_ldaWard)) /
                photopicOperator(std::log10(lwa));
    f_sm_disp = f_sm_comp / ldmax;
    f_pm_disp = f_pm_comp / ldmax;

    // Revised Tumblin & Rushmeier
    g = stevensGamma(lwa) / stevensGamma(_ldaTumb);
    gwd = stevensGamma(lwa) / (1.855f + 0.4f * std::log(_ldaTumb));
    r_comp = java::Math::pow(java::Math::sqrt(cmax), gwd - 1) * _ldaTumb;
    r_disp = r_comp / ldmax;
}

static void
trwfTerminate() {
}

static ColorRgb
trwfScaleForComputations(ColorRgb radiance) {
    float rwl;
    float scale;

    rwl = radiance.luminance();

    if ( rwl > 0.0 ) {
        float m = tmoLambertCandela(
                (java::Math::pow(tmoCandelaLambert(rwl), lrwexponent) * lrwm_comp));
        scale = m > 0.0f ? m / rwl : 0.0f;
    } else {
        scale = 0.0f;
    }

    radiance.scale(scale);
    return radiance;
}

static ColorRgb
trwfScaleForDisplay(ColorRgb radiance) {
    float rwl;
    float scale;
    float eff;

    rwl = (float)M_PI * radiance.luminance();

    eff = getLuminousEfficacy();
    radiance.scale(eff * (float) M_PI);

    if ( rwl > 0.0 ) {
        float m = (java::Math::pow(tmoCandelaLambert(rwl), lrwexponent) * lrwm_disp - invcmax);
        scale = m > 0.0f ? m / rwl : 0.0f;
    } else {
        scale = 0.0f;
    }

    radiance.scale(scale);
    return radiance;
}

ToneMap GLOBAL_toneMap_tumblinRushmeier = {
    "Tumblin/Rushmeier's Mapping",
    "TumblinRushmeier",
    3,
    trwfDefaults,
    nullptr,
    trwfInit,
    trwfTerminate,
    trwfScaleForComputations,
    trwfScaleForDisplay
};

static ColorRgb
wardScaleForComputations(ColorRgb radiance) {
    radiance.scale(m_comp);
    return radiance;
}

static ColorRgb
wardScaleForDisplay(ColorRgb radiance) {
    float eff = getLuminousEfficacy();

    radiance.scale(eff * m_disp);
    return radiance;
}

ToneMap GLOBAL_toneMap_ward = {
    "Ward's Mapping",
    "Ward",
    3,
    trwfDefaults,
    nullptr,
    trwfInit,
    trwfTerminate,
    wardScaleForComputations,
    wardScaleForDisplay
};

static ColorRgb
revisedTRScaleForComputations(ColorRgb radiance) {
    float rwl;
    float scale;

    rwl = radiance.luminance();

    if ( rwl > 0.0 ) {
        scale = r_comp * java::Math::pow(rwl / _lwa, g) / rwl;
    } else {
        scale = 0.0;
    }

    radiance.scale(scale);
    return radiance;
}

static ColorRgb
revisedTRScaleForDisplay(ColorRgb radiance) {
    float rwl;
    float scale;

    rwl = (float)M_PI * radiance.luminance();

    float eff = getLuminousEfficacy();
    radiance.scale(eff * (float)M_PI);

    if ( rwl > 0.0 ) {
        scale = r_disp * java::Math::pow(rwl / _lwa, g) / rwl;
    } else {
        scale = 0.0f;
    }

    radiance.scale(scale);
    return radiance;
}

ToneMap GLOBAL_toneMap_revisedTumblinRushmeier = {
    "Revised Tumblin/Rushmeier's Mapping",
    "RevisedTR",
    3,
    trwfDefaults,
    nullptr,
    trwfInit,
    trwfTerminate,
    revisedTRScaleForComputations,
    revisedTRScaleForDisplay
};

static ColorRgb
ferwerdaScaleForComputations(ColorRgb radiance) {
    ColorRgb p{};
    float sl;

    // Convert to photometric values
    float eff = getLuminousEfficacy();
    radiance.scale(eff);

    // Compute the scotopic grayscale shift
    p.set(radiance.r, radiance.g, radiance.b);
    sl = f_sm_comp * f_msf * (p.r * f_sf.r + p.g * f_sf.g + p.b * f_sf.b);

    // Scale the photopic luminance
    radiance.scale(f_pm_comp);

    // Eventually, offset by the scotopic luminance
    if ( sl > 0.0 ) {
        radiance.addConstant(radiance, sl);
    }

    return radiance;
}

static ColorRgb
ferwerdaScaleForDisplay(ColorRgb radiance) {
    ColorRgb p{};
    float sl;

    // Convert to photometric values
    float eff = getLuminousEfficacy();
    radiance.scale(eff);

    // Compute the scotopic grayscale shift
    radiance.set(p.r, p.g, p.b);
    sl = f_sm_disp * f_msf * (p.r * f_sf.r + p.g * f_sf.g + p.b * f_sf.b);

    // Scale the photopic luminance
    radiance.scale(f_pm_disp);

    // Eventually, offset by the scotopic luminance
    if ( sl > 0.0 ) {
        radiance.addConstant(radiance, sl);
    }

    return radiance;
}

ToneMap GLOBAL_toneMap_ferwerda = {
    "Partial Ferwerda's Mapping",
    "Ferwerda",
    3,
    trwfDefaults,
    nullptr,
    trwfInit,
    trwfTerminate,
    ferwerdaScaleForComputations,
    ferwerdaScaleForDisplay
};

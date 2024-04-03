/**
Tumblin/Rushmeier/Ward/Ferwerda tone maps (Jan Prikryl)
*/

#include "common/rgb.h"
#include "common/color.h"
#include "common/cie.h"
#include "common/mymath.h"
#include "IMAGE/tonemap/trwf.h"

/**
REFERENCES:

J. Tumblin, H.E. Rushmeier. Tone Reproduction for Realistic Images,
IEEE Computer Graphics and Applications, 13:6, 1993, pp. 42-48.

G. Ward. A Contrast-Based Scale factor for Luminance Display, Graphics
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
        return 1.855f + 0.4f * std::log10(lum + 2.3e-5f);
    }
}

static float
photopicOperator(float logLa) {
    float r;
    if ( logLa <= -2.6 ) {
        r = -0.72;
    } else if ( logLa >= 1.9 ) {
        r = logLa - 1.255f;
    } else {
        r = std::pow(0.249f * logLa + 0.65f, 2.7f) - 0.72f;
    }

    return std::pow(10.0f, r);
}

static float
scotopicOperator(float logLa) {
    float r;
    if ( logLa <= -3.94 ) {
        r = -2.86;
    } else if ( logLa >= -1.44 ) {
        r = logLa - 0.395f;
    } else {
        r = std::pow(0.405f * logLa + 1.6f, 2.18f) - 2.86f;
    }

    return std::pow(10.0f, r);
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
    _ldaTumb = ldmax / std::sqrt(cmax);

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
    lrwm_comp = std::pow(10.0f, (betarw - betad) / alphad);
    lrwm_disp = lrwm_comp / (tmoCandelaLambert(ldmax));
    invcmax = 1.0f / cmax;

    // Ward
    _ldaWard = ldmax / 2.0f;
    {
        float p1 = std::pow(_ldaWard, 0.4f);
        float p2 = std::pow(lwa, 0.4f);
        float p3 = (1.219f + p1) / (1.219f + p2);
        m_comp = std::pow(p3, 2.5f);
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
    r_comp = std::pow(std::sqrt(cmax), gwd - 1) * _ldaTumb;
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
        float m = tmoLambertCandela(
                (std::pow(tmoCandelaLambert(rwl), lrwexponent) * lrwm_comp));
        scale = m > 0.0f ? m / rwl : 0.0f;
    } else {
        scale = 0.0f;
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
    colorScale(eff * (float)M_PI, radiance, radiance);

    if ( rwl > 0.0 ) {
        float m = (std::pow(tmoCandelaLambert(rwl), lrwexponent) * lrwm_disp - invcmax);
        scale = m > 0.0f ? m / rwl : 0.0f;
    } else {
        scale = 0.0f;
    }

    colorScale(scale, radiance, radiance);
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

static COLOR
revisedTRScaleForComputations(COLOR radiance) {
    float rwl;
    float scale;

    rwl = colorLuminance(radiance);

    if ( rwl > 0.0 ) {
        scale = r_comp * std::pow(rwl / _lwa, g) / rwl;
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
    colorScale(eff * (float)M_PI, radiance, radiance);

    if ( rwl > 0.0 ) {
        scale = r_disp * std::pow(rwl / _lwa, g) / rwl;
    } else {
        scale = 0.0f;
    }

    colorScale(scale, radiance, radiance);
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

static COLOR
ferwerdaScaleForComputations(COLOR radiance) {
    RGB p{};
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
    RGB p{};
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

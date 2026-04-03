#include <cstdlib>

#include "common/linealAlgebra/Numeric.h"
#include "io/context/ParseErrorContext.h"
#include "io/context/ColorContext.h"
#include "io/context/TokenValidationContext.h"

const ColorContext ColorContext::DEFAULT_COLOR_CONTEXT = {
    1,
    COLOR_DEFINED_WITH_XY_FLAG | COLOR_XY_IS_SET_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_EFFICACY_FLAG,
    {
        ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE,
        ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE,
        ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE,
        ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE,
        ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE,
        ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE,
        ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE, ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE
    },
    static_cast<long>(ColorContext::NUMBER_OF_SPECTRAL_SAMPLES) * ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE,
    1.0 / 3.0,
    1.0 / 3.0,
    178.006f
};

// Derived CIE 1931 Primaries (imaginary)
ColorContext ColorContext::cie_xp = {
    1, COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_XY_IS_SET_FLAG,
    {-174, -198, -195, -197, -202, -213, -235, -272, -333,
     -444, -688, -1232, -2393, -4497, -6876, -6758, -5256,
     -3100, -815, 1320, 3200, 4782, 5998, 6861, 7408, 7754,
     7980, 8120, 8199, 8240, 8271, 8292, 8309, 8283, 8469,
     8336, 8336, 8336, 8336, 8336, 8336},
    127424L, 1.0f, 0.0f, 0.0f
};

ColorContext ColorContext::cie_yp = {
    1, COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_XY_IS_SET_FLAG,
    {-451, -431, -431, -430, -427, -417, -399, -366, -312,
     -204, 57, 691, 2142, 4990, 8810, 9871, 9122, 7321, 5145,
     3023, 1123, -473, -1704, -2572, -3127, -3474, -3704,
     -3846, -3927, -3968, -3999, -4021, -4038, -4012, -4201,
     -4066, -4066, -4066, -4066, -4066, -4066},
    -23035L, 0.0f, 1.0f, 0.0f
};

ColorContext ColorContext::cie_zp = {
    1, COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_XY_IS_SET_FLAG,
    {4051, 4054, 4052, 4053, 4054, 4056, 4059, 4064, 4071,
     4074, 4056, 3967, 3677, 2933, 1492, 313, -440, -795,
     -904, -918, -898, -884, -869, -863, -855, -855, -851,
     -848, -847, -846, -846, -846, -845, -846, -843, -845,
     -845, -845, -845, -845, -845},
    36057L, 0.0f, 0.0f, 0.0f,
};

// CIE 1931 Standard Observer curves
ColorContext ColorContext::cie_xf = {
    1, COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_XY_IS_SET_FLAG | COLOR_EFFICACY_FLAG,
    {14, 42, 143, 435, 1344, 2839, 3483, 3362, 2908, 1954, 956,
     320, 49, 93, 633, 1655, 2904, 4334, 5945, 7621, 9163, 10263,
     10622, 10026, 8544, 6424, 4479, 2835, 1649, 874, 468, 227,
     114, 58, 29, 14, 7, 3, 2, 1, 0}, 106836L, 0.467f, 0.368f, 362.230f
};

ColorContext ColorContext::cie_yf = {
    1, COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_XY_IS_SET_FLAG | COLOR_EFFICACY_FLAG,
    {0, 1, 4, 12, 40, 116, 230, 380, 600, 910, 1390, 2080, 3230,
     5030, 7100, 8620, 9540, 9950, 9950, 9520, 8700, 7570, 6310,
     5030, 3810, 2650, 1750, 1070, 610, 320, 170, 82, 41, 21, 10,
     5, 2, 1, 1, 0, 0}, 106856L, 0.398f, 0.542f, 493.525f
};

ColorContext ColorContext::cie_zf = {
    1, COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_XY_IS_SET_FLAG | COLOR_EFFICACY_FLAG,
    {65, 201, 679, 2074, 6456, 13856, 17471, 17721, 16692,
     12876, 8130, 4652, 2720, 1582, 782, 422, 203, 87, 39, 21, 17,
     11, 8, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    106770L, 0.147f, 0.077f, 54.363f
};

/**
Convert a spectrum
*/
int
ColorContext::setSpectrum(double wlMinimum, double wlMaximum, int ac, const char **av) {
    double scale;
    float va[ColorContext::NUMBER_OF_SPECTRAL_SAMPLES];
    int i;
    int pos;
    int n;
    int imax;
    int wl;
    double wl0;
    double wlStep;
    double boxPos;
    double boxStep;
    int argumentStartIndex = 0;

    // Check getBoundingBox
    if ( wlMaximum <= COLOR_MINIMUM_WAVE_LENGTH || wlMaximum <= wlMinimum || wlMinimum >= COLOR_MAXIMUM_WAVE_LENGTH ) {
        return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    wlStep = (wlMaximum - wlMinimum) / (ac - 1);
    while ( wlMinimum < COLOR_MINIMUM_WAVE_LENGTH ) {
        wlMinimum += wlStep;
        ac--;
        argumentStartIndex++;
    }
    while ( wlMaximum > COLOR_MAXIMUM_WAVE_LENGTH ) {
        wlMaximum -= wlStep;
        ac--;
    }
    imax = ac; // Box filter if necessary
    boxPos = 0;
    boxStep = 1;
    if ( wlStep < static_cast<double>(colorWaveLengthDeltaI()) ) {
        imax = static_cast<int>(java::Math::round((wlMaximum - wlMinimum) / colorWaveLengthDeltaI() + (1 - Numeric::EPSILON)));
        boxPos = (wlMinimum - COLOR_MINIMUM_WAVE_LENGTH) / colorWaveLengthDeltaI();
        boxStep = wlStep / colorWaveLengthDeltaI();
        wlStep = colorWaveLengthDeltaI();
    }
    scale = 0.0; // Get values and maximum
    pos = 0;
    for ( i = 0; i < imax; i++ ) {
        va[i] = 0.0;
        n = 0;
        while ( boxPos < i + 0.5 && pos < ac ) {
            const char *value = av[argumentStartIndex + pos];
            if ( !TokenValidationContext::isFloat(value) ) {
                return ParseErrorContext::MGF_ERROR_ARGUMENT_TYPE;
            }
            va[i] += strtof(value, nullptr);
            pos++;
            n++;
            boxPos += boxStep;
        }
        if ( n > 1 ) {
            va[i] /= static_cast<float>(n);
        }
        if ( va[i] > scale ) {
            scale = va[i];
        } else if ( va[i] < -scale ) {
                scale = -va[i];
            }
    }
    if ( scale <= Numeric::EPSILON ) {
        return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    scale = ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE / scale;
    spectralStraightSum = 0; // Convert to our spacing
    wl0 = wlMinimum;
    pos = 0;
    for ( i = 0, wl = COLOR_MINIMUM_WAVE_LENGTH; i < ColorContext::NUMBER_OF_SPECTRAL_SAMPLES; i++, wl += static_cast<int>(colorWaveLengthDeltaI()) ) {
        if ( wl < wlMinimum || wl > wlMaximum ) {
            straightSamples[i] = 0;
        } else {
            while ( wl0 + wlStep < wl + Numeric::EPSILON ) {
                wl0 += wlStep;
                pos++;
            }
            if ( wl + Numeric::EPSILON >= wl0 && wl - Numeric::EPSILON <= wl0 ) {
                straightSamples[i] = static_cast<short>(java::Math::round(scale * va[pos] + 0.5));
            } else {
                // Interpolate if necessary
                straightSamples[i] = static_cast<short>(java::Math::round(0.5 + scale / wlStep *
                                                                          (va[pos] * (wl0 + wlStep - wl) +
                                                                           va[pos + 1] * (wl - wl0))));
            }
            spectralStraightSum += straightSamples[i];
        }
    }
    flags = COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG;
    clock++;
    return ParseErrorContext::MGF_OK;
}

/**
Set black body spectrum
*/
int
ColorContext::setBlackBodyTemperature(double tk) {
    double sf;
    double wl;

    if ( tk < 1000 ) {
        return ParseErrorContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    wl = bBlm(tk);
    // Scale factor based on peak
    if ( wl < COLOR_MINIMUM_WAVE_LENGTH * 1e-9 ) {
        wl = COLOR_MINIMUM_WAVE_LENGTH * 1e-9;
    } else if ( wl > COLOR_MAXIMUM_WAVE_LENGTH * 1e-9 ) {
            wl = COLOR_MAXIMUM_WAVE_LENGTH * 1e-9;
        }
    sf = ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE / bBsp(wl, tk);
    spectralStraightSum = 0;
    for ( int i = 0; i < ColorContext::NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
        wl = (COLOR_MINIMUM_WAVE_LENGTH + static_cast<float>(i) * colorWaveLengthDeltaI()) * 1e-9;
        spectralStraightSum += straightSamples[i] = static_cast<short>(java::Math::round(sf * bBsp(wl, tk) + 0.5));
    }
    flags = COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG;
    clock++;
    return ParseErrorContext::MGF_OK;
}

/**
Convert color representations
*/
void
ColorContext::fixColorRepresentation(int fl) {
    double x;
    double y;
    double z;
    int i;

    fl &= ~flags; // Ignore what's done
    if ( !fl ) {
        // Everything's done!
        return;
    }
    if ( !(flags & (COLOR_XY_IS_SET_FLAG | COLOR_SPECTRUM_IS_SET_FLAG)) ) {
        // Nothing set!
        *this = ColorContext::DEFAULT_COLOR_CONTEXT;
    }
    if ( fl & COLOR_XY_IS_SET_FLAG ) {
        // spec -> cxy *
        x = 0.0;
        y = 0.0;
        z = 0.0;
        for ( i = 0; i < ColorContext::NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
            x += cie_xf.straightSamples[i] * straightSamples[i];
            y += cie_yf.straightSamples[i] * straightSamples[i];
            z += cie_zf.straightSamples[i] * straightSamples[i];
        }
        x /= static_cast<double>(cie_xf.spectralStraightSum);
        y /= static_cast<double>(cie_yf.spectralStraightSum);
        z /= static_cast<double>(cie_zf.spectralStraightSum);
        z += x + y;
        cx = static_cast<float>(x / z);
        cy = static_cast<float>(y / z);
        flags |= COLOR_XY_IS_SET_FLAG;
    } else if ( fl & COLOR_SPECTRUM_IS_SET_FLAG ) {
            // cxy -> spec
            x = cx;
            y = cy;
            z = 1.0 - x - y;
            spectralStraightSum = 0;
            for ( i = 0; i < ColorContext::NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
                straightSamples[i] = static_cast<short>(java::Math::round(x * cie_xp.straightSamples[i] + y * cie_yp.straightSamples[i]
                                                                          + z * cie_zp.straightSamples[i] + 0.5));
                if ( straightSamples[i] < 0 ) {
                    // Out of gamut!
                    straightSamples[i] = 0;
                } else {
                    spectralStraightSum += straightSamples[i];
                }
            }
            flags |= COLOR_SPECTRUM_IS_SET_FLAG;
        }
    if ( fl & COLOR_EFFICACY_FLAG ) {
        // Compute efficacy
        if ( flags & COLOR_SPECTRUM_IS_SET_FLAG ) {
            // From spectrum
            y = 0.0;
            for ( i = 0; i < ColorContext::NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
                y += cie_yf.straightSamples[i] * straightSamples[i];
            }
            eff = static_cast<float>((colorPeakLumensPerWatt() * y / static_cast<double>(spectralStraightSum)));
        } else {
            // flags & C_CS_XY from (x,y)
            eff = static_cast<float>(cx * cie_xf.eff + cy * cie_yf.eff +
                                     (1.0 - cx - cy) * cie_zf.eff);
        }
        flags |= COLOR_EFFICACY_FLAG;
    }
}

/**
Mix two colors according to weights given
*/
void
ColorContext::mixColors(
    double w1,
    ColorContext *c1,
    double w2,
    ColorContext *c2)
{
    double scale;
    float cMix[ColorContext::NUMBER_OF_SPECTRAL_SAMPLES];

    if ( (c1->flags | c2->flags) & COLOR_DEFINED_WITH_SPECTRUM_FLAG ) {
        int i;
        // Spectral mixing
        c1->fixColorRepresentation(COLOR_SPECTRUM_IS_SET_FLAG | COLOR_EFFICACY_FLAG);
        c2->fixColorRepresentation(COLOR_SPECTRUM_IS_SET_FLAG | COLOR_EFFICACY_FLAG);
        w1 /= c1->eff * static_cast<float>(c1->spectralStraightSum);
        w2 /= c2->eff * static_cast<float>(c2->spectralStraightSum);
        scale = 0.0;
        for ( i = 0; i < ColorContext::NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
            cMix[i] = static_cast<float>(w1 * c1->straightSamples[i] + w2 * c2->straightSamples[i]);
            if ( cMix[i] > scale ) {
                scale = cMix[i];
            }
        }
        scale = ColorContext::COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE / scale;
        spectralStraightSum = 0;
        for ( i = 0; i < ColorContext::NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
            spectralStraightSum += straightSamples[i] = static_cast<short>(java::Math::round(scale * cMix[i] + 0.5));
        }
        flags = COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG;
    } else {
        // CIE xy mixing
        c1->fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
        c2->fixColorRepresentation(COLOR_XY_IS_SET_FLAG);
        scale = w1 / c1->cy + w2 / c2->cy;
        if ( scale == 0.0 ) {
            return;
        }
        scale = 1.0 / scale;
        cx = static_cast<float>((c1->cx * w1 / c1->cy + c2->cx * w2 / c2->cy) * scale);
        cy = static_cast<float>((w1 + w2) * scale);
        flags = COLOR_DEFINED_WITH_XY_FLAG | COLOR_XY_IS_SET_FLAG;
    }
}

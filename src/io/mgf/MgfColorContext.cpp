#include "io/mgf/MgfContext.h"
#include "io/mgf/words.h"
#include "io/mgf/MgfColorContext.h"

// Derived CIE 1931 Primaries (imaginary)
static MgfColorContext cie_xp = {
        1, COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_XY_IS_SET_FLAG,
        {-174, -198, -195, -197, -202, -213, -235, -272, -333,
         -444, -688, -1232, -2393, -4497, -6876, -6758, -5256,
         -3100, -815, 1320, 3200, 4782, 5998, 6861, 7408, 7754,
         7980, 8120, 8199, 8240, 8271, 8292, 8309, 8283, 8469,
         8336, 8336, 8336, 8336, 8336, 8336},
        127424L, 1.0, 0.0
};

static MgfColorContext cie_yp = {
        1, COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_XY_IS_SET_FLAG,
        {-451, -431, -431, -430, -427, -417, -399, -366, -312,
         -204, 57, 691, 2142, 4990, 8810, 9871, 9122, 7321, 5145,
         3023, 1123, -473, -1704, -2572, -3127, -3474, -3704,
         -3846, -3927, -3968, -3999, -4021, -4038, -4012, -4201,
         -4066, -4066, -4066, -4066, -4066, -4066},
        -23035L, 0.0, 1.0
};

static MgfColorContext cie_zp = {
        1, COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_XY_IS_SET_FLAG,
        {4051, 4054, 4052, 4053, 4054, 4056, 4059, 4064, 4071,
         4074, 4056, 3967, 3677, 2933, 1492, 313, -440, -795,
         -904, -918, -898, -884, -869, -863, -855, -855, -851,
         -848, -847, -846, -846, -846, -845, -846, -843, -845,
         -845, -845, -845, -845, -845},
        36057L, 0.0, 0.0,
};

// CIE 1931 Standard Observer curves
static MgfColorContext cie_xf = {
        1, COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_XY_IS_SET_FLAG | COLOR_EFFICACY_FLAG,
        {14, 42, 143, 435, 1344, 2839, 3483, 3362, 2908, 1954, 956,
         320, 49, 93, 633, 1655, 2904, 4334, 5945, 7621, 9163, 10263,
         10622, 10026, 8544, 6424, 4479, 2835, 1649, 874, 468, 227,
         114, 58, 29, 14, 7, 3, 2, 1, 0}, 106836L, 0.467f, 0.368f, 362.230f
};

static MgfColorContext cie_yf = {
        1, COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG | COLOR_XY_IS_SET_FLAG | COLOR_EFFICACY_FLAG,
        {0, 1, 4, 12, 40, 116, 230, 380, 600, 910, 1390, 2080, 3230,
         5030, 7100, 8620, 9540, 9950, 9950, 9520, 8700, 7570, 6310,
         5030, 3810, 2650, 1750, 1070, 610, 320, 170, 82, 41, 21, 10,
         5, 2, 1, 1, 0, 0}, 106856L, 0.398f, 0.542f, 493.525f
};

static MgfColorContext cie_zf = {
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
MgfColorContext::setSpectrum(double wlMinimum, double wlMaximum, int ac, const char **av) {
    double scale;
    float va[NUMBER_OF_SPECTRAL_SAMPLES];
    int i;
    int pos;
    int n;
    int imax;
    int wl;
    double wl0;
    double wlStep;
    double boxPos;
    double boxStep;

    // Check getBoundingBox
    if ( wlMaximum <= COLOR_MINIMUM_WAVE_LENGTH || wlMaximum <= wlMinimum || wlMinimum >= COLOR_MAXIMUM_WAVE_LENGTH ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    wlStep = (wlMaximum - wlMinimum) / (ac - 1);
    while ( wlMinimum < COLOR_MINIMUM_WAVE_LENGTH ) {
        wlMinimum += wlStep;
        ac--;
        av++;
    }
    while ( wlMaximum > COLOR_MAXIMUM_WAVE_LENGTH ) {
        wlMaximum -= wlStep;
        ac--;
    }
    imax = ac; // Box filter if necessary
    boxPos = 0;
    boxStep = 1;
    if ( wlStep < (double)COLOR_WAVE_LENGTH_DELTA_I ) {
        imax = (int)java::Math::round((wlMaximum - wlMinimum) / COLOR_WAVE_LENGTH_DELTA_I + (1 - Numeric::EPSILON));
        boxPos = (wlMinimum - COLOR_MINIMUM_WAVE_LENGTH) / COLOR_WAVE_LENGTH_DELTA_I;
        boxStep = wlStep / COLOR_WAVE_LENGTH_DELTA_I;
        wlStep = COLOR_WAVE_LENGTH_DELTA_I;
    }
    scale = 0.0; // Get values and maximum
    pos = 0;
    for ( i = 0; i < imax; i++ ) {
        va[i] = 0.0;
        n = 0;
        while ( boxPos < i + 0.5 && pos < ac ) {
            if ( !isFloatWords(av[pos]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            va[i] += strtof(av[pos++], nullptr);
            n++;
            boxPos += boxStep;
        }
        if ( n > 1 ) {
            va[i] /= (float)n;
        }
        if ( va[i] > scale ) {
            scale = va[i];
        } else if ( va[i] < -scale ) {
                scale = -va[i];
            }
    }
    if ( scale <= Numeric::EPSILON ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    scale = COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE / scale;
    spectralStraightSum = 0; // Convert to our spacing
    wl0 = wlMinimum;
    pos = 0;
    for ( i = 0, wl = COLOR_MINIMUM_WAVE_LENGTH; i < NUMBER_OF_SPECTRAL_SAMPLES; i++, wl += (int)COLOR_WAVE_LENGTH_DELTA_I) {
        if ( wl < wlMinimum || wl > wlMaximum ) {
            straightSamples[i] = 0;
        } else {
            while ( wl0 + wlStep < wl + Numeric::EPSILON ) {
                wl0 += wlStep;
                pos++;
            }
            if ( wl + Numeric::EPSILON >= wl0 && wl - Numeric::EPSILON <= wl0 ) {
                straightSamples[i] = (short)java::Math::round(scale * va[pos] + 0.5);
            } else {
                // Interpolate if necessary
                straightSamples[i] = (short)java::Math::round(0.5 + scale / wlStep *
                                                              (va[pos] * (wl0 + wlStep - wl) +
                                                               va[pos + 1] * (wl - wl0)));
            }
            spectralStraightSum += straightSamples[i];
        }
    }
    flags = COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG;
    clock++;
    return MGF_OK;
}

/**
Set black body spectrum
*/
int
MgfColorContext::setBlackBodyTemperature(double tk) {
    double sf;
    double wl;

    if ( tk < 1000 ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    wl = bBlm(tk);
    // Scale factor based on peak
    if ( wl < COLOR_MINIMUM_WAVE_LENGTH * 1e-9 ) {
        wl = COLOR_MINIMUM_WAVE_LENGTH * 1e-9;
    } else if ( wl > COLOR_MAXIMUM_WAVE_LENGTH * 1e-9 ) {
            wl = COLOR_MAXIMUM_WAVE_LENGTH * 1e-9;
        }
    sf = COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE / bBsp(wl, tk);
    spectralStraightSum = 0;
    for ( int i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
        wl = (COLOR_MINIMUM_WAVE_LENGTH + (float)i * COLOR_WAVE_LENGTH_DELTA_I) * 1e-9;
        spectralStraightSum += straightSamples[i] = (short)java::Math::round(sf * bBsp(wl, tk) + 0.5);
    }
    flags = COLOR_DEFINED_WITH_SPECTRUM_FLAG | COLOR_SPECTRUM_IS_SET_FLAG;
    clock++;
    return MGF_OK;
}

/**
Convert color representations
*/
void
MgfColorContext::fixColorRepresentation(int fl) {
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
        *this = DEFAULT_COLOR_CONTEXT;
    }
    if ( fl & COLOR_XY_IS_SET_FLAG ) {
        // spec -> cxy *
        x = 0.0;
        y = 0.0;
        z = 0.0;
        for ( i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
            x += cie_xf.straightSamples[i] * straightSamples[i];
            y += cie_yf.straightSamples[i] * straightSamples[i];
            z += cie_zf.straightSamples[i] * straightSamples[i];
        }
        x /= (double) cie_xf.spectralStraightSum;
        y /= (double) cie_yf.spectralStraightSum;
        z /= (double) cie_zf.spectralStraightSum;
        z += x + y;
        cx = (float)(x / z);
        cy = (float)(y / z);
        flags |= COLOR_XY_IS_SET_FLAG;
    } else if ( fl & COLOR_SPECTRUM_IS_SET_FLAG ) {
            // cxy -> spec
            x = cx;
            y = cy;
            z = 1.0 - x - y;
            spectralStraightSum = 0;
            for ( i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
                straightSamples[i] = (short)java::Math::round(x * cie_xp.straightSamples[i] + y * cie_yp.straightSamples[i]
                                                        + z * cie_zp.straightSamples[i] + 0.5);
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
            for ( i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
                y += cie_yf.straightSamples[i] * straightSamples[i];
            }
            eff = (float)(COLOR_PEAK_LUMENS_PER_WATT * y / (double)spectralStraightSum);
        } else {
            // flags & C_CS_XY from (x,y)
            eff = (float)(cx * cie_xf.eff + cy * cie_yf.eff +
                          (1.0 - cx - cy) * cie_zf.eff);
        }
        flags |= COLOR_EFFICACY_FLAG;
    }
}

/**
Mix two colors according to weights given
*/
void
MgfColorContext::mixColors(
    double w1,
    MgfColorContext *c1,
    double w2,
    MgfColorContext *c2)
{
    double scale;
    float cMix[NUMBER_OF_SPECTRAL_SAMPLES];
    int i;

    if ( (c1->flags | c2->flags) & COLOR_DEFINED_WITH_SPECTRUM_FLAG ) {
        // Spectral mixing
        c1->fixColorRepresentation(COLOR_SPECTRUM_IS_SET_FLAG | COLOR_EFFICACY_FLAG);
        c2->fixColorRepresentation(COLOR_SPECTRUM_IS_SET_FLAG | COLOR_EFFICACY_FLAG);
        w1 /= c1->eff * (float) c1->spectralStraightSum;
        w2 /= c2->eff * (float) c2->spectralStraightSum;
        scale = 0.0;
        for ( i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
            cMix[i] = (float)(w1 * c1->straightSamples[i] + w2 * c2->straightSamples[i]);
            if ( cMix[i] > scale ) {
                scale = cMix[i];
            }
        }
        scale = COLOR_NOMINAL_MAXIMUM_SAMPLE_VALUE / scale;
        spectralStraightSum = 0;
        for ( i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
            spectralStraightSum += straightSamples[i] = (short)java::Math::round(scale * cMix[i] + 0.5);
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
        cx = (float)((c1->cx * w1 / c1->cy + c2->cx * w2 / c2->cy) * scale);
        cy = (float)((w1 + w2) * scale);
        flags = COLOR_DEFINED_WITH_XY_FLAG | COLOR_XY_IS_SET_FLAG;
    }
}

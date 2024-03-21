#include "io/mgf/parser.h"
#include "io/mgf/lookup.h"
#include "io/mgf/MgfColorContext.h"

// W-m^2
#define C1 3.741832e-16

// m-K
#define C2 1.4388e-2

// Default context values
static MgfColorContext globalDefaultMgfColorContext = DEFAULT_COLOR_CONTEXT;
MgfVertexContext GLOBAL_mgf_defaultVertexContext = DEFAULT_VERTEX;

// The unnamed contexts
static MgfColorContext globalUnNamedColorContext = DEFAULT_COLOR_CONTEXT;

// Current contexts
MgfVertexContext GLOBAL_mgf_vertexContext = DEFAULT_VERTEX;
MgfColorContext *GLOBAL_mgf_currentColor = &globalUnNamedColorContext;
char *GLOBAL_mgf_currentMaterialName = nullptr;
MgfVertexContext *GLOBAL_mgf_currentVertex = &GLOBAL_mgf_vertexContext;
char *GLOBAL_mgf_currentVertexName = nullptr;

// Color lookup table
static LUTAB clr_tab = LU_SINIT(free, free);

// CIE 1931 Standard Observer curves
static MgfColorContext cie_xf = {
    1, C_CDSPEC | C_CSSPEC | C_CSXY | C_CSEFF,
     {14, 42, 143, 435, 1344, 2839, 3483, 3362, 2908, 1954, 956,
              320, 49, 93, 633, 1655, 2904, 4334, 5945, 7621, 9163, 10263,
              10622, 10026, 8544, 6424, 4479, 2835, 1649, 874, 468, 227,
              114, 58, 29, 14, 7, 3, 2, 1, 0}, 106836L, .467, .368, 362.230
};

static MgfColorContext cie_yf = {
        1, C_CDSPEC | C_CSSPEC | C_CSXY | C_CSEFF,
         {0, 1, 4, 12, 40, 116, 230, 380, 600, 910, 1390, 2080, 3230,
          5030, 7100, 8620, 9540, 9950, 9950, 9520, 8700, 7570, 6310,
          5030, 3810, 2650, 1750, 1070, 610, 320, 170, 82, 41, 21, 10,
          5, 2, 1, 1, 0, 0}, 106856L, .398, .542, 493.525
};

static MgfColorContext cie_zf = {
    1, C_CDSPEC | C_CSSPEC | C_CSXY | C_CSEFF,
       {65, 201, 679, 2074, 6456, 13856, 17471, 17721, 16692,
                  12876, 8130, 4652, 2720, 1582, 782, 422, 203, 87, 39, 21, 17,
                  11, 8, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                     106770L, 0.147, 0.077, 54.363
};

// Derived CIE 1931 Primaries (imaginary)
static MgfColorContext cie_xp = {
    1, C_CDSPEC | C_CSSPEC | C_CSXY,
     {-174, -198, -195, -197, -202, -213, -235, -272, -333,
      -444, -688, -1232, -2393, -4497, -6876, -6758, -5256,
      -3100, -815, 1320, 3200, 4782, 5998, 6861, 7408, 7754,
      7980, 8120, 8199, 8240, 8271, 8292, 8309, 8283, 8469,
      8336, 8336, 8336, 8336, 8336, 8336},
             127424L, 1.0, 0.0
};

static MgfColorContext cie_yp = {
    1, C_CDSPEC | C_CSSPEC | C_CSXY,
         {-451, -431, -431, -430, -427, -417, -399, -366, -312,
          -204, 57, 691, 2142, 4990, 8810, 9871, 9122, 7321, 5145,
          3023, 1123, -473, -1704, -2572, -3127, -3474, -3704,
          -3846, -3927, -3968, -3999, -4021, -4038, -4012, -4201,
          -4066, -4066, -4066, -4066, -4066, -4066},
                 -23035L, 0.0, 1.0
};

static MgfColorContext cie_zp = {
    1, C_CDSPEC | C_CSSPEC | C_CSXY,
     {4051, 4054, 4052, 4053, 4054, 4056, 4059, 4064, 4071,
          4074, 4056, 3967, 3677, 2933, 1492, 313, -440, -795,
          -904, -918, -898, -884, -869, -863, -855, -855, -851,
          -848, -847, -846, -846, -846, -845, -846, -843, -845,
          -845, -845, -845, -845, -845},
                 36057L, 0.0, 0.0,
};

inline double
bBsp(double l, double t) {
    return C1 / (l * l * l * l * l * (std::exp(C2 / (t * l)) - 1.0));
}

inline double
bBlm(double t) {
    return C2 / 5.0 / t;
}

/**
Set black body spectrum
*/
static int
setBbTemp(MgfColorContext *clr, double tk)
{
    double sf;
    double wl;
    int i;

    if ( tk < 1000 ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    wl = bBlm(tk);
    // Scale factor based on peak
    if ( wl < C_CMINWL * 1e-9 ) {
        wl = C_CMINWL * 1e-9;
    } else if ( wl > C_CMAXWL * 1e-9 ) {
            wl = C_CMAXWL * 1e-9;
        }
    sf = C_CMAXV / bBsp(wl, tk);
    clr->spectralStraightSum = 0;
    for ( i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
        wl = (C_CMINWL + (float)i * C_CWLI) * 1e-9;
        clr->spectralStraightSum += clr->straightSamples[i] = (short)std::lround(sf * bBsp(wl, tk) + 0.5);
    }
    clr->flags = C_CDSPEC | C_CSSPEC;
    clr->clock++;
    return MGF_OK;
}

/**
Mix two colors according to weights given
*/
static void
mixColors(
    MgfColorContext *colorContext,
    double w1,
    MgfColorContext *c1,
    double w2,
    MgfColorContext *c2)
{
    double scale;
    float cMix[NUMBER_OF_SPECTRAL_SAMPLES];
    int i;

    if ( (c1->flags | c2->flags) & C_CDSPEC ) {
        // Spectral mixing
        mgfContextFixColorRepresentation(c1, C_CSSPEC | C_CSEFF);
        mgfContextFixColorRepresentation(c2, C_CSSPEC | C_CSEFF);
        w1 /= c1->eff * (float) c1->spectralStraightSum;
        w2 /= c2->eff * (float) c2->spectralStraightSum;
        scale = 0.0;
        for ( i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
            cMix[i] = (float)(w1 * c1->straightSamples[i] + w2 * c2->straightSamples[i]);
            if ( cMix[i] > scale ) {
                scale = cMix[i];
            }
        }
        scale = C_CMAXV / scale;
        colorContext->spectralStraightSum = 0;
        for ( i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
            colorContext->spectralStraightSum += colorContext->straightSamples[i] = (short)std::lround(scale * cMix[i] + 0.5);
        }
        colorContext->flags = C_CDSPEC | C_CSSPEC;
    } else {
        // CIE xy mixing
        mgfContextFixColorRepresentation(c1, C_CSXY);
        mgfContextFixColorRepresentation(c2, C_CSXY);
        scale = w1 / c1->cy + w2 / c2->cy;
        if ( scale == 0.0 ) {
            return;
        }
        scale = 1.0 / scale;
        colorContext->cx = (float)((c1->cx * w1 / c1->cy + c2->cx * w2 / c2->cy) * scale);
        colorContext->cy = (float)((w1 + w2) * scale);
        colorContext->flags = C_CDXY | C_CSXY;
    }
}

static int
setSpectrum(MgfColorContext *clr, double wlMinimum, double wlMaximum, int ac, char **av)    /* convert a spectrum */
{
    double scale;
    float va[NUMBER_OF_SPECTRAL_SAMPLES];
    int i, pos;
    int n, imax;
    int wl;
    double wl0;
    double wlStep;
    double boxPos;
    double boxStep;

    // Check getBoundingBox
    if ( wlMaximum <= C_CMINWL || wlMaximum <= wlMinimum || wlMinimum >= C_CMAXWL ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    wlStep = (wlMaximum - wlMinimum) / (ac - 1);
    while ( wlMinimum < C_CMINWL ) {
        wlMinimum += wlStep;
        ac--;
        av++;
    }
    while ( wlMaximum > C_CMAXWL ) {
        wlMaximum -= wlStep;
        ac--;
    }
    imax = ac; // Box filter if necessary
    boxPos = 0;
    boxStep = 1;
    if ( wlStep < (double)C_CWLI ) {
        imax = (int)std::lround((wlMaximum - wlMinimum) / C_CWLI + (1 - EPSILON));
        boxPos = (wlMinimum - C_CMINWL) / C_CWLI;
        boxStep = wlStep / C_CWLI;
        wlStep = C_CWLI;
    }
    scale = 0.0; // Get values and maximum
    pos = 0;
    for ( i = 0; i < imax; i++ ) {
        va[i] = 0.0;
        n = 0;
        while ( boxPos < i + 0.5 && pos < ac ) {
            if ( !isFloatWords(av[pos])) {
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
    if ( scale <= EPSILON) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    scale = C_CMAXV / scale;
    clr->spectralStraightSum = 0; // Convert to our spacing
    wl0 = wlMinimum;
    pos = 0;
    for ( i = 0, wl = C_CMINWL; i < NUMBER_OF_SPECTRAL_SAMPLES; i++, wl += C_CWLI) {
        if ( wl < wlMinimum || wl > wlMaximum ) {
            clr->straightSamples[i] = 0;
        } else {
            while ( wl0 + wlStep < wl + EPSILON) {
                wl0 += wlStep;
                pos++;
            }
            if ( wl + EPSILON >= wl0 && wl - EPSILON <= wl0 ) {
                clr->straightSamples[i] = (short)std::lround(scale * va[pos] + 0.5);
            } else {
                // Interpolate if necessary
                clr->straightSamples[i] = (short)std::lround(0.5 + scale / wlStep *
                                                                   (va[pos] * (wl0 + wlStep - wl) +
                                                                    va[pos + 1] * (wl - wl0)));
            }
            clr->spectralStraightSum += clr->straightSamples[i];
        }
    }
    clr->flags = C_CDSPEC | C_CSSPEC;
    clr->clock++;
    return MGF_OK;
}

/**
Handle color entity
*/
int
handleColorEntity(int ac, char **av, RadianceMethod * /*context*/)
{
    double w;
    double wSum;
    int i;
    LUENT *lp;

    switch ( mgfEntity(av[0]) ) {
        case MGF_ENTITY_COLOR:
            // Get/set color context
            if ( ac > 4 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( ac == 1 ) {
                // Set unnamed color context
                globalUnNamedColorContext = globalDefaultMgfColorContext;
                GLOBAL_mgf_currentColor = &globalUnNamedColorContext;
                return MGF_OK;
            }
            if ( !isNameWords(av[1])) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            lp = lookUpFind(&clr_tab, av[1]); // Lookup context
            if ( lp == nullptr) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            GLOBAL_mgf_currentColor = (MgfColorContext *) lp->data;
            if ( ac == 2 ) {
                // Re-establish previous context
                if ( GLOBAL_mgf_currentColor == nullptr) {
                    return MGF_ERROR_UNDEFINED_REFERENCE;
                }
                return MGF_OK;
            }
            if ( av[2][0] != '=' || av[2][1] ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            if ( GLOBAL_mgf_currentColor == nullptr) {    /* create new color context */
                lp->key = (char *) malloc(strlen(av[1]) + 1);
                if ( lp->key == nullptr) {
                    return MGF_ERROR_OUT_OF_MEMORY;
                }
                strcpy(lp->key, av[1]);
                lp->data = (char *) malloc(sizeof(MgfColorContext));
                if ( lp->data == nullptr) {
                    return MGF_ERROR_OUT_OF_MEMORY;
                }
                GLOBAL_mgf_currentColor = (MgfColorContext *) lp->data;
                GLOBAL_mgf_currentColor->clock = 0;
            }
            i = GLOBAL_mgf_currentColor->clock;
            if ( ac == 3 ) {
                // Use default template
                *GLOBAL_mgf_currentColor = globalDefaultMgfColorContext;
                GLOBAL_mgf_currentColor->clock = i + 1;
                return MGF_OK;
            }
            lp = lookUpFind(&clr_tab, av[3]);
            // Lookup template
            if ( lp == nullptr) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr) {
                return MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *GLOBAL_mgf_currentColor = *(MgfColorContext *) lp->data;
            GLOBAL_mgf_currentColor->clock = i + 1;
            return MGF_OK;
        case MGF_ENTITY_CXY:
            // Assign CIE XY value
            if ( ac != 3 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentColor->cx = strtof(av[1], nullptr);
            GLOBAL_mgf_currentColor->cy = strtof(av[2], nullptr);
            GLOBAL_mgf_currentColor->flags = C_CDXY | C_CSXY;
            if ( GLOBAL_mgf_currentColor->cx < 0.0 || GLOBAL_mgf_currentColor->cy < 0.0 ||
                 GLOBAL_mgf_currentColor->cx + GLOBAL_mgf_currentColor->cy > 1.0 ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentColor->clock++;
            return MGF_OK;
        case MGF_ENTITY_C_SPEC:
            // Assign spectral values
            if ( ac < 5 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            return setSpectrum(GLOBAL_mgf_currentColor, strtod(av[1], nullptr), strtod(av[2], nullptr),
                               ac - 3, av + 3);
        case MGF_ENTITY_CCT:
            // Assign black body spectrum
            if ( ac != 2 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            return setBbTemp(GLOBAL_mgf_currentColor, strtod(av[1], nullptr));
        case MGF_ENTITY_C_MIX:
            // Mix colors
            if ( ac < 5 || (ac - 1) % 2 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            wSum = strtod(av[1], nullptr);
            lp = lookUpFind(&clr_tab, av[2]);
            if ( lp == nullptr ) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr) {
                return MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *GLOBAL_mgf_currentColor = *(MgfColorContext *) lp->data;
            for ( i = 3; i < ac; i += 2 ) {
                if ( !isFloatWords(av[i]) ) {
                    return MGF_ERROR_ARGUMENT_TYPE;
                }
                w = strtod(av[i], nullptr);
                lp = lookUpFind(&clr_tab, av[i + 1]);
                if ( lp == nullptr ) {
                    return MGF_ERROR_OUT_OF_MEMORY;
                }
                if ( lp->data == nullptr ) {
                    return MGF_ERROR_UNDEFINED_REFERENCE;
                }
                mixColors(GLOBAL_mgf_currentColor, wSum, GLOBAL_mgf_currentColor,
                          w, (MgfColorContext *) lp->data);
                wSum += w;
            }
            if ( wSum <= 0.0 ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentColor->clock++;
            return MGF_OK;
    }
    return MGF_ERROR_UNKNOWN_ENTITY;
}

/**
Empty context tables
*/
void
clearContextTables()
{
    globalUnNamedColorContext = globalDefaultMgfColorContext;
    GLOBAL_mgf_currentColor = &globalUnNamedColorContext;
    lookUpDone(&clr_tab);
    GLOBAL_mgf_currentMaterialName = nullptr;
    GLOBAL_mgf_vertexContext = GLOBAL_mgf_defaultVertexContext;
    GLOBAL_mgf_currentVertex = &GLOBAL_mgf_vertexContext;
    GLOBAL_mgf_currentVertexName = nullptr;
    lookUpDone(&GLOBAL_mgf_vertexLookUpTable);
    mgfClearMaterialTables();
}

/**
Get a named vertex
*/
MgfVertexContext *
getNamedVertex(char *name)
{
    LUENT *lp = lookUpFind(&GLOBAL_mgf_vertexLookUpTable, name);

    if ( lp == nullptr ) {
        return nullptr;
    }
    return (MgfVertexContext *)lp->data;
}

/**
Convert color representations
*/
void
mgfContextFixColorRepresentation(MgfColorContext *clr, int fl)
{
    double x;
    double y;
    double z;
    int i;

    fl &= ~clr->flags; // Ignore what's done
    if ( !fl ) {
        // Everything's done!
        return;
    }
    if ( !(clr->flags & (C_CSXY | C_CSSPEC)) ) {
        // Nothing set!
        *clr = globalDefaultMgfColorContext;
    }
    if ( fl & C_CSXY ) {
        // spec -> cxy *
        x = 0.0;
        y = 0.0;
        z = 0.0;
        for ( i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
            x += cie_xf.straightSamples[i] * clr->straightSamples[i];
            y += cie_yf.straightSamples[i] * clr->straightSamples[i];
            z += cie_zf.straightSamples[i] * clr->straightSamples[i];
        }
        x /= (double) cie_xf.spectralStraightSum;
        y /= (double) cie_yf.spectralStraightSum;
        z /= (double) cie_zf.spectralStraightSum;
        z += x + y;
        clr->cx = (float)(x / z);
        clr->cy = (float)(y / z);
        clr->flags |= C_CSXY;
    } else if ( fl & C_CSSPEC ) {
        // cxy -> spec
        x = clr->cx;
        y = clr->cy;
        z = 1.0 - x - y;
        clr->spectralStraightSum = 0;
        for ( i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
            clr->straightSamples[i] = (short)std::lround(x * cie_xp.straightSamples[i] + y * cie_yp.straightSamples[i]
                                                         + z * cie_zp.straightSamples[i] + 0.5);
            if ( clr->straightSamples[i] < 0 ) {
                // Out of gamut!
                clr->straightSamples[i] = 0;
            } else {
                clr->spectralStraightSum += clr->straightSamples[i];
            }
        }
        clr->flags |= C_CSSPEC;
    }
    if ( fl & C_CSEFF ) {
        // Compute efficacy
        if ( clr->flags & C_CSSPEC ) {
            // From spectrum
            y = 0.0;
            for ( i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
                y += cie_yf.straightSamples[i] * clr->straightSamples[i];
            }
            clr->eff = C_CLPWM * y / (double)clr->spectralStraightSum;
        } else {
            // clr->flags & C_CS_XY from (x,y)
            clr->eff = (float)(clr->cx * cie_xf.eff + clr->cy * cie_yf.eff +
                       (1.0 - clr->cx - clr->cy) * cie_zf.eff);
        }
        clr->flags |= C_CSEFF;
    }
}

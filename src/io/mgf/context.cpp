/**
Context handlers
*/

#include "io/mgf/parser.h"
#include "io/mgf/lookup.h"

// W-m^2
#define C1 3.741832e-16

// m-K
#define C2 1.4388e-2

inline double
bbsp(double l, double t) {
    return C1 / (l * l * l * l * l * (std::exp(C2 / (t * l)) - 1.0));
}

inline double
bblm(double t) {
    return C2 / 5.0 / t;
}

/* default context values */
static MgfColorContext c_dfcolor = C_DEFCOLOR;
static MgfMaterialContext c_dfmaterial = C_DEFMATERIAL;
static MgfVertexContext c_dfvertex = C_DEFVERTEX;

/* the unnamed contexts */
static MgfColorContext c_uncolor = C_DEFCOLOR;
static MgfMaterialContext c_unmaterial = C_DEFMATERIAL;
static MgfVertexContext c_unvertex = C_DEFVERTEX;

/* the current contexts */
MgfColorContext *GLOBAL_mgf_currentColor = &c_uncolor;
MgfMaterialContext *GLOBAL_mgf_currentMaterial = &c_unmaterial;
char *GLOBAL_mgf_currentMaterialName = nullptr;
MgfVertexContext *GLOBAL_mgf_currentVertex = &c_unvertex;
char *GLOBAL_mgf_currentVertexName = nullptr;

static LUTAB clr_tab = LU_SINIT(free, free);    /* color lookup table */
static LUTAB mat_tab = LU_SINIT(free, free);    /* material lookup table */
static LUTAB vtx_tab = LU_SINIT(free, free);    /* vertex lookup table */

/* CIE 1931 Standard Observer curves */
static MgfColorContext cie_xf = {1, C_CDSPEC | C_CSSPEC | C_CSXY | C_CSEFF,
                                 {14, 42, 143, 435, 1344, 2839, 3483, 3362, 2908, 1954, 956,
                          320, 49, 93, 633, 1655, 2904, 4334, 5945, 7621, 9163, 10263,
                          10622, 10026, 8544, 6424, 4479, 2835, 1649, 874, 468, 227,
                          114, 58, 29, 14, 7, 3, 2, 1, 0}, 106836L, .467, .368, 362.230
};
static MgfColorContext cie_yf = {1, C_CDSPEC | C_CSSPEC | C_CSXY | C_CSEFF,
                                 {0, 1, 4, 12, 40, 116, 230, 380, 600, 910, 1390, 2080, 3230,
                          5030, 7100, 8620, 9540, 9950, 9950, 9520, 8700, 7570, 6310,
                          5030, 3810, 2650, 1750, 1070, 610, 320, 170, 82, 41, 21, 10,
                          5, 2, 1, 1, 0, 0}, 106856L, .398, .542, 493.525
};
static MgfColorContext cie_zf = {1, C_CDSPEC | C_CSSPEC | C_CSXY | C_CSEFF,
                                 {65, 201, 679, 2074, 6456, 13856, 17471, 17721, 16692,
                          12876, 8130, 4652, 2720, 1582, 782, 422, 203, 87, 39, 21, 17,
                          11, 8, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                                 106770L, 0.147, 0.077, 54.363
};
/* Derived CIE 1931 Primaries (imaginary) */
static MgfColorContext cie_xp = {1, C_CDSPEC | C_CSSPEC | C_CSXY,
                                 {-174, -198, -195, -197, -202, -213, -235, -272, -333,
                          -444, -688, -1232, -2393, -4497, -6876, -6758, -5256,
                          -3100, -815, 1320, 3200, 4782, 5998, 6861, 7408, 7754,
                          7980, 8120, 8199, 8240, 8271, 8292, 8309, 8283, 8469,
                          8336, 8336, 8336, 8336, 8336, 8336},
                                 127424L, 1.0, 0.0,
};
static MgfColorContext cie_yp = {1, C_CDSPEC | C_CSSPEC | C_CSXY,
                                 {-451, -431, -431, -430, -427, -417, -399, -366, -312,
                          -204, 57, 691, 2142, 4990, 8810, 9871, 9122, 7321, 5145,
                          3023, 1123, -473, -1704, -2572, -3127, -3474, -3704,
                          -3846, -3927, -3968, -3999, -4021, -4038, -4012, -4201,
                          -4066, -4066, -4066, -4066, -4066, -4066},
                                 -23035L, 0.0, 1.0,
};
static MgfColorContext cie_zp = {1, C_CDSPEC | C_CSSPEC | C_CSXY,
                                 {4051, 4054, 4052, 4053, 4054, 4056, 4059, 4064, 4071,
                          4074, 4056, 3967, 3677, 2933, 1492, 313, -440, -795,
                          -904, -918, -898, -884, -869, -863, -855, -855, -851,
                          -848, -847, -846, -846, -846, -845, -846, -843, -845,
                          -845, -845, -845, -845, -845},
                                 36057L, 0.0, 0.0,
};

static int setSpectrum(MgfColorContext *clr, double wlmin, double wlmax, int ac, char **av);

static int setbbtemp(MgfColorContext *clr, double tk);

static void mixColors(MgfColorContext *cres, double w1, MgfColorContext *c1, double w2, MgfColorContext *c2);

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
        case MG_E_COLOR:
            // Get/set color context
            if ( ac > 4 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( ac == 1 ) {
                // Set unnamed color context
                c_uncolor = c_dfcolor;
                GLOBAL_mgf_currentColor = &c_uncolor;
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
                *GLOBAL_mgf_currentColor = c_dfcolor;
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
        case MG_E_CXY:
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
        case MG_E_CSPEC:
            // Assign spectral values
            if ( ac < 5 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            return setSpectrum(GLOBAL_mgf_currentColor, strtod(av[1], nullptr), strtod(av[2], nullptr),
                               ac - 3, av + 3);
        case MG_E_CCT:        /* assign black body spectrum */
            if ( ac != 2 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            return setbbtemp(GLOBAL_mgf_currentColor, strtod(av[1], nullptr));
        case MG_E_CMIX:
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
Handle material entity
*/
int
handleMaterialEntity(int ac, char **av, RadianceMethod * /*context*/)
{
    int i;
    LUENT *lp;

    switch ( mgfEntity(av[0]) ) {
        case MG_E_MATERIAL:
            // get/set material context
            if ( ac > 4 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( ac == 1 ) {
                // Set unnamed material context
                c_unmaterial = c_dfmaterial;
                GLOBAL_mgf_currentMaterial = &c_unmaterial;
                GLOBAL_mgf_currentMaterialName = nullptr;
                return MGF_OK;
            }
            if ( !isNameWords(av[1]) ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            lp = lookUpFind(&mat_tab, av[1]);
            // Lookup context
            if ( lp == nullptr ) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            GLOBAL_mgf_currentMaterialName = lp->key;
            GLOBAL_mgf_currentMaterial = (MgfMaterialContext *) lp->data;
            if ( ac == 2 ) {
                // Re-establish previous context
                if ( GLOBAL_mgf_currentMaterial == nullptr) {
                    return MGF_ERROR_UNDEFINED_REFERENCE;
                }
                return MGF_OK;
            }
            if ( av[2][0] != '=' || av[2][1] ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            if ( GLOBAL_mgf_currentMaterial == nullptr ) {
                // Create new material
                lp->key = (char *) malloc(strlen(av[1]) + 1);
                if ( lp->key == nullptr) {
                    return MGF_ERROR_OUT_OF_MEMORY;
                }
                strcpy(lp->key, av[1]);
                lp->data = (char *) malloc(sizeof(MgfMaterialContext));
                if ( lp->data == nullptr) {
                    return MGF_ERROR_OUT_OF_MEMORY;
                }
                GLOBAL_mgf_currentMaterialName = lp->key;
                GLOBAL_mgf_currentMaterial = (MgfMaterialContext *) lp->data;
                GLOBAL_mgf_currentMaterial->clock = 0;
            }
            i = GLOBAL_mgf_currentMaterial->clock;
            if ( ac == 3 ) {
                // Use default template
                *GLOBAL_mgf_currentMaterial = c_dfmaterial;
                GLOBAL_mgf_currentMaterial->clock = i + 1;
                return MGF_OK;
            }
            lp = lookUpFind(&mat_tab, av[3]);
            // Lookup template
            if ( lp == nullptr ) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr ) {
                return MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *GLOBAL_mgf_currentMaterial = *(MgfMaterialContext *) lp->data;
            GLOBAL_mgf_currentMaterial->clock = i + 1;
            return MGF_OK;
        case MG_E_IR:
            // Set index of refraction
            if ( ac != 3 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->nr = strtof(av[1], nullptr);
            GLOBAL_mgf_currentMaterial->ni = strtof(av[2], nullptr);
            if ( GLOBAL_mgf_currentMaterial->nr <= FLOAT_TINY ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;
        case MG_E_RD:
            // Set diffuse reflectance
            if ( ac != 2 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->rd = strtof(av[1], nullptr);
            if ( GLOBAL_mgf_currentMaterial->rd < 0. || GLOBAL_mgf_currentMaterial->rd > 1.0 ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->rd_c = *GLOBAL_mgf_currentColor;
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;
        case MG_E_ED:
            // Set diffuse emittance
            if ( ac != 2 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->ed = strtof(av[1], nullptr);
            if ( GLOBAL_mgf_currentMaterial->ed < 0.0 ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->ed_c = *GLOBAL_mgf_currentColor;
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;
        case MG_E_TD:
            // Set diffuse transmittance
            if ( ac != 2 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->td = strtof(av[1], nullptr);
            if ( GLOBAL_mgf_currentMaterial->td < 0.0 || GLOBAL_mgf_currentMaterial->td > 1.0 ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->td_c = *GLOBAL_mgf_currentColor;
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;
        case MG_E_RS:
            // Set specular reflectance
            if ( ac != 3 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->rs = strtof(av[1], nullptr);
            GLOBAL_mgf_currentMaterial->rs_a = strtof(av[2], nullptr);
            if ( GLOBAL_mgf_currentMaterial->rs < 0.0 || GLOBAL_mgf_currentMaterial->rs > 1.0 ||
                 GLOBAL_mgf_currentMaterial->rs_a < 0.0 ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->rs_c = *GLOBAL_mgf_currentColor;
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;
        case MG_E_TS:
            // Set specular transmittance
            if ( ac != 3 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->ts = strtof(av[1], nullptr);
            GLOBAL_mgf_currentMaterial->ts_a = strtof(av[2], nullptr);
            if ( GLOBAL_mgf_currentMaterial->ts < 0.0 || GLOBAL_mgf_currentMaterial->ts > 1.0 ||
                 GLOBAL_mgf_currentMaterial->ts_a < 0.0 ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->ts_c = *GLOBAL_mgf_currentColor;
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;
        case MG_E_SIDES:
            // Set number of sides
            if ( ac != 2 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isIntWords(av[1]) ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            i = (int)strtol(av[1], nullptr, 10);
            if ( i == 1 ) {
                GLOBAL_mgf_currentMaterial->sided = 1;
            } else if ( i == 2 ) {
                GLOBAL_mgf_currentMaterial->sided = 0;
            } else {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;
    }
    return MGF_ERROR_UNKNOWN_ENTITY;
}

/**
Handle a vertex entity
*/
int
handleVertexEntity(int ac, char **av, RadianceMethod * /*context*/)
{
    LUENT *lp;

    switch ( mgfEntity(av[0]) ) {
        case MG_E_VERTEX:
            // get/set vertex context
            if ( ac > 4 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( ac == 1 ) {
                // Set unnamed vertex context
                c_unvertex = c_dfvertex;
                GLOBAL_mgf_currentVertex = &c_unvertex;
                GLOBAL_mgf_currentVertexName = nullptr;
                return MGF_OK;
            }
            if ( !isNameWords(av[1]) ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            lp = lookUpFind(&vtx_tab, av[1]);
            // Lookup context
            if ( lp == nullptr ) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            GLOBAL_mgf_currentVertexName = lp->key;
            GLOBAL_mgf_currentVertex = (MgfVertexContext *) lp->data;
            if ( ac == 2 ) {
                // Re-establish previous context
                if ( GLOBAL_mgf_currentVertex == nullptr) {
                    return MGF_ERROR_UNDEFINED_REFERENCE;
                }
                return MGF_OK;
            }
            if ( av[2][0] != '=' || av[2][1] ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            if ( GLOBAL_mgf_currentVertex == nullptr) {
                // Create new vertex context
                GLOBAL_mgf_currentVertexName = (char *) malloc(strlen(av[1]) + 1);
                if ( !GLOBAL_mgf_currentVertexName ) {
                    return MGF_ERROR_OUT_OF_MEMORY;
                }
                strcpy(GLOBAL_mgf_currentVertexName, av[1]);
                lp->key = GLOBAL_mgf_currentVertexName;
                GLOBAL_mgf_currentVertex = (MgfVertexContext *) malloc(sizeof(MgfVertexContext));
                if ( !GLOBAL_mgf_currentVertex ) {
                    return MGF_ERROR_OUT_OF_MEMORY;
                }
                lp->data = (char *) GLOBAL_mgf_currentVertex;
            }
            if ( ac == 3 ) {
                // Use default template
                *GLOBAL_mgf_currentVertex = c_dfvertex;
                return MGF_OK;
            }
            lp = lookUpFind(&vtx_tab, av[3]);
            // Lookup template
            if ( lp == nullptr) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr) {
                return MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *GLOBAL_mgf_currentVertex = *(MgfVertexContext *) lp->data;
            GLOBAL_mgf_currentVertex->clock++;
            return MGF_OK;
        case MG_E_POINT:
            // Set point
            if ( ac != 4 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2]) || !isFloatWords(av[3])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentVertex->p[0] = strtod(av[1], nullptr);
            GLOBAL_mgf_currentVertex->p[1] = strtod(av[2], nullptr);
            GLOBAL_mgf_currentVertex->p[2] = strtod(av[3], nullptr);
            GLOBAL_mgf_currentVertex->clock++;
            return MGF_OK;
        case MG_E_NORMAL:
            // Set normal
            if ( ac != 4 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2]) || !isFloatWords(av[3])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentVertex->n[0] = strtod(av[1], nullptr);
            GLOBAL_mgf_currentVertex->n[1] = strtod(av[2], nullptr);
            GLOBAL_mgf_currentVertex->n[2] = strtod(av[3], nullptr);
            normalize(GLOBAL_mgf_currentVertex->n);
            GLOBAL_mgf_currentVertex->clock++;
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
    c_uncolor = c_dfcolor;
    GLOBAL_mgf_currentColor = &c_uncolor;
    lookUpDone(&clr_tab);
    c_unmaterial = c_dfmaterial;
    GLOBAL_mgf_currentMaterial = &c_unmaterial;
    GLOBAL_mgf_currentMaterialName = nullptr;
    lookUpDone(&mat_tab);
    c_unvertex = c_dfvertex;
    GLOBAL_mgf_currentVertex = &c_unvertex;
    GLOBAL_mgf_currentVertexName = nullptr;
    lookUpDone(&vtx_tab);
}

/**
Get a named vertex
*/
MgfVertexContext *
getNamedVertex(char *name)
{
    LUENT *lp = lookUpFind(&vtx_tab, name);

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
        *clr = c_dfcolor;
    }
    if ( fl & C_CSXY ) {
        // cspec -> cxy *
        x = 0.0;
        y = 0.0;
        z = 0.0;
        for ( i = 0; i < C_CNSS; i++ ) {
            x += cie_xf.ssamp[i] * clr->ssamp[i];
            y += cie_yf.ssamp[i] * clr->ssamp[i];
            z += cie_zf.ssamp[i] * clr->ssamp[i];
        }
        x /= (double) cie_xf.ssum;
        y /= (double) cie_yf.ssum;
        z /= (double) cie_zf.ssum;
        z += x + y;
        clr->cx = (float)(x / z);
        clr->cy = (float)(y / z);
        clr->flags |= C_CSXY;
    } else if ( fl & C_CSSPEC ) {
        // cxy -> cspec
        x = clr->cx;
        y = clr->cy;
        z = 1.0 - x - y;
        clr->ssum = 0;
        for ( i = 0; i < C_CNSS; i++ ) {
            clr->ssamp[i] = (short)std::lround(x * cie_xp.ssamp[i] + y * cie_yp.ssamp[i]
                            + z * cie_zp.ssamp[i] + 0.5);
            if ( clr->ssamp[i] < 0 ) {
                // Out of gamut!
                clr->ssamp[i] = 0;
            } else {
                clr->ssum += clr->ssamp[i];
            }
        }
        clr->flags |= C_CSSPEC;
    }
    if ( fl & C_CSEFF ) {
        // Compute efficacy
        if ( clr->flags & C_CSSPEC ) {
            // From spectrum
            y = 0.0;
            for ( i = 0; i < C_CNSS; i++ ) {
                y += cie_yf.ssamp[i] * clr->ssamp[i];
            }
            clr->eff = C_CLPWM * y / (double)clr->ssum;
        } else {
            // clr->flags & C_CSXY from (x,y)
            clr->eff = (float)(clr->cx * cie_xf.eff + clr->cy * cie_yf.eff +
                       (1.0 - clr->cx - clr->cy) * cie_zf.eff);
        }
        clr->flags |= C_CSEFF;
    }
}

static int
setSpectrum(MgfColorContext *clr, double wlmin, double wlmax, int ac, char **av)    /* convert a spectrum */
{
    double scale;
    float va[C_CNSS];
    int i, pos;
    int n, imax;
    int wl;
    double wl0;
    double wlstep;
    double boxpos;
    double boxstep;

    // Check getBoundingBox
    if ( wlmax <= C_CMINWL || wlmax <= wlmin || wlmin >= C_CMAXWL ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    wlstep = (wlmax - wlmin) / (ac - 1);
    while ( wlmin < C_CMINWL ) {
        wlmin += wlstep;
        ac--;
        av++;
    }
    while ( wlmax > C_CMAXWL ) {
        wlmax -= wlstep;
        ac--;
    }
    imax = ac; // Box filter if necessary
    boxpos = 0;
    boxstep = 1;
    if ( wlstep < (double)C_CWLI ) {
        imax = (int)std::lround((wlmax - wlmin) / C_CWLI + (1 - FLOAT_TINY));
        boxpos = (wlmin - C_CMINWL) / C_CWLI;
        boxstep = wlstep / C_CWLI;
        wlstep = C_CWLI;
    }
    scale = 0.0; // Get values and maximum
    pos = 0;
    for ( i = 0; i < imax; i++ ) {
        va[i] = 0.0;
        n = 0;
        while ( boxpos < i + 0.5 && pos < ac ) {
            if ( !isFloatWords(av[pos])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            va[i] += strtof(av[pos++], nullptr);
            n++;
            boxpos += boxstep;
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
    if ( scale <= FLOAT_TINY) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    scale = C_CMAXV / scale;
    clr->ssum = 0; // Convert to our spacing
    wl0 = wlmin;
    pos = 0;
    for ( i = 0, wl = C_CMINWL; i < C_CNSS; i++, wl += C_CWLI) {
        if ( wl < wlmin || wl > wlmax ) {
            clr->ssamp[i] = 0;
        } else {
            while ( wl0 + wlstep < wl + FLOAT_TINY) {
                wl0 += wlstep;
                pos++;
            }
            if ( wl + FLOAT_TINY >= wl0 && wl - FLOAT_TINY <= wl0 ) {
                clr->ssamp[i] = (short)std::lround(scale * va[pos] + 0.5);
            } else {
                // Interpolate if necessary
                clr->ssamp[i] = (short)std::lround(0.5 + scale / wlstep *
                                     (va[pos] * (wl0 + wlstep - wl) +
                                      va[pos + 1] * (wl - wl0)));
            }
            clr->ssum += clr->ssamp[i];
        }
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
    MgfColorContext *cres,
    double w1,
    MgfColorContext *c1,
    double w2,
    MgfColorContext *c2)
{
    double scale;
    float cmix[C_CNSS];
    int i;

    if ( (c1->flags | c2->flags) & C_CDSPEC ) {
        // Spectral mixing
        mgfContextFixColorRepresentation(c1, C_CSSPEC | C_CSEFF);
        mgfContextFixColorRepresentation(c2, C_CSSPEC | C_CSEFF);
        w1 /= c1->eff * (float) c1->ssum;
        w2 /= c2->eff * (float) c2->ssum;
        scale = 0.0;
        for ( i = 0; i < C_CNSS; i++ ) {
            cmix[i] = (float)(w1 * c1->ssamp[i] + w2 * c2->ssamp[i]);
            if ( cmix[i] > scale ) {
                scale = cmix[i];
            }
        }
        scale = C_CMAXV / scale;
        cres->ssum = 0;
        for ( i = 0; i < C_CNSS; i++ ) {
            cres->ssum += cres->ssamp[i] = (short)std::lround(scale * cmix[i] + 0.5);
        }
        cres->flags = C_CDSPEC | C_CSSPEC;
    } else {
        // CIE xy mixing
        mgfContextFixColorRepresentation(c1, C_CSXY);
        mgfContextFixColorRepresentation(c2, C_CSXY);
        scale = w1 / c1->cy + w2 / c2->cy;
        if ( scale == 0.0 ) {
            return;
        }
        scale = 1.0 / scale;
        cres->cx = (float)((c1->cx * w1 / c1->cy + c2->cx * w2 / c2->cy) * scale);
        cres->cy = (float)((w1 + w2) * scale);
        cres->flags = C_CDXY | C_CSXY;
    }
}

/**
Set black body spectrum
*/
static int
setbbtemp(MgfColorContext *clr, double tk)
{
    double sf;
    double wl;
    int i;

    if ( tk < 1000 ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    wl = bblm(tk);
    // Scale factor based on peak
    if ( wl < C_CMINWL * 1e-9 ) {
        wl = C_CMINWL * 1e-9;
    } else if ( wl > C_CMAXWL * 1e-9 ) {
        wl = C_CMAXWL * 1e-9;
    }
    sf = C_CMAXV / bbsp(wl, tk);
    clr->ssum = 0;
    for ( i = 0; i < C_CNSS; i++ ) {
        wl = (C_CMINWL + (float)i * C_CWLI) * 1e-9;
        clr->ssum += clr->ssamp[i] = (short)std::lround(sf * bbsp(wl, tk) + 0.5);
    }
    clr->flags = C_CDSPEC | C_CSSPEC;
    clr->clock++;
    return MGF_OK;
}

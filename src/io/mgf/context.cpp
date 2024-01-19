/*
 * Context handlers
 */

#include <cstdlib>
#include <cstring>

#include "common/mymath.h"
#include "io/mgf/parser.h"
#include "io/mgf/lookup.h"
#include "io/mgf/words.h"

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
                                 106770L, .147, .077, 54.363
};
/* Derived CIE 1931 Primaries (imaginary) */
static MgfColorContext cie_xp = {1, C_CDSPEC | C_CSSPEC | C_CSXY,
                                 {-174, -198, -195, -197, -202, -213, -235, -272, -333,
                          -444, -688, -1232, -2393, -4497, -6876, -6758, -5256,
                          -3100, -815, 1320, 3200, 4782, 5998, 6861, 7408, 7754,
                          7980, 8120, 8199, 8240, 8271, 8292, 8309, 8283, 8469,
                          8336, 8336, 8336, 8336, 8336, 8336},
                                 127424L, 1., .0,
};
static MgfColorContext cie_yp = {1, C_CDSPEC | C_CSSPEC | C_CSXY,
                                 {-451, -431, -431, -430, -427, -417, -399, -366, -312,
                          -204, 57, 691, 2142, 4990, 8810, 9871, 9122, 7321, 5145,
                          3023, 1123, -473, -1704, -2572, -3127, -3474, -3704,
                          -3846, -3927, -3968, -3999, -4021, -4038, -4012, -4201,
                          -4066, -4066, -4066, -4066, -4066, -4066},
                                 -23035L, .0, 1.,
};
static MgfColorContext cie_zp = {1, C_CDSPEC | C_CSSPEC | C_CSXY,
                                 {4051, 4054, 4052, 4053, 4054, 4056, 4059, 4064, 4071,
                          4074, 4056, 3967, 3677, 2933, 1492, 313, -440, -795,
                          -904, -918, -898, -884, -869, -863, -855, -855, -851,
                          -848, -847, -846, -846, -846, -845, -846, -843, -845,
                          -845, -845, -845, -845, -845},
                                 36057L, .0, .0,
};

static int setspectrum(MgfColorContext *clr, double wlmin, double wlmax, int ac, char **av);

static int setbbtemp(MgfColorContext *clr, double tk);

static void mixcolors(MgfColorContext *cres, double w1, MgfColorContext *c1, double w2, MgfColorContext *c2);


int
handleColorEntity(int ac, char **av)        /* handle color entity */
{
    double w, wsum;
    int i;
    LUENT *lp;

    switch ( mgfEntity(av[0])) {
        case MG_E_COLOR:    /* get/set color context */
            if ( ac > 4 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( ac == 1 ) {
                // Set unnamed color context
                c_uncolor = c_dfcolor;
                GLOBAL_mgf_currentColor = &c_uncolor;
                return MGF_OK;
            }
            if ( !isnameWords(av[1])) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            lp = lu_find(&clr_tab, av[1]); // Lookup context
            if ( lp == nullptr) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            GLOBAL_mgf_currentColor = (MgfColorContext *) lp->data;
            if ( ac == 2 ) {        /* reestablish previous context */
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
            if ( ac == 3 ) {        /* use default template */
                *GLOBAL_mgf_currentColor = c_dfcolor;
                GLOBAL_mgf_currentColor->clock = i + 1;
                return MGF_OK;
            }
            lp = lu_find(&clr_tab, av[3]);    /* lookup template */
            if ( lp == nullptr) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr) {
                return MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *GLOBAL_mgf_currentColor = *(MgfColorContext *) lp->data;
            GLOBAL_mgf_currentColor->clock = i + 1;
            return MGF_OK;
        case MG_E_CXY:        /* assign CIE XY value */
            if ( ac != 3 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isfltWords(av[1]) | !isfltWords(av[2])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentColor->cx = atof(av[1]);
            GLOBAL_mgf_currentColor->cy = atof(av[2]);
            GLOBAL_mgf_currentColor->flags = C_CDXY | C_CSXY;
            if ( GLOBAL_mgf_currentColor->cx < 0. || GLOBAL_mgf_currentColor->cy < 0. ||
                 GLOBAL_mgf_currentColor->cx + GLOBAL_mgf_currentColor->cy > 1. ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentColor->clock++;
            return MGF_OK;
        case MG_E_CSPEC:    /* assign spectral values */
            if ( ac < 5 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isfltWords(av[1]) || !isfltWords(av[2])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            return setspectrum(GLOBAL_mgf_currentColor, atof(av[1]), atof(av[2]),
                                ac - 3, av + 3);
        case MG_E_CCT:        /* assign black body spectrum */
            if ( ac != 2 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isfltWords(av[1])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            return setbbtemp(GLOBAL_mgf_currentColor, atof(av[1]));
        case MG_E_CMIX:        /* mix colors */
            if ( ac < 5 || (ac - 1) % 2 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isfltWords(av[1])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            wsum = atof(av[1]);
            if ((lp = lu_find(&clr_tab, av[2])) == nullptr) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr) {
                return MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *GLOBAL_mgf_currentColor = *(MgfColorContext *) lp->data;
            for ( i = 3; i < ac; i += 2 ) {
                if ( !isfltWords(av[i])) {
                    return MGF_ERROR_ARGUMENT_TYPE;
                }
                w = atof(av[i]);
                if ((lp = lu_find(&clr_tab, av[i + 1])) == nullptr) {
                    return MGF_ERROR_OUT_OF_MEMORY;
                }
                if ( lp->data == nullptr) {
                    return MGF_ERROR_UNDEFINED_REFERENCE;
                }
                mixcolors(GLOBAL_mgf_currentColor, wsum, GLOBAL_mgf_currentColor,
                          w, (MgfColorContext *) lp->data);
                wsum += w;
            }
            if ( wsum <= 0. ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentColor->clock++;
            return MGF_OK;
    }
    return MGF_ERROR_UNKNOWN_ENTITY;
}


int
handleMaterialEntity(int ac, char **av)        /* handle material entity */
{
    int i;
    LUENT *lp;

    switch ( mgfEntity(av[0])) {
        case MG_E_MATERIAL:    /* get/set material context */
            if ( ac > 4 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( ac == 1 ) {        /* set unnamed material context */
                c_unmaterial = c_dfmaterial;
                GLOBAL_mgf_currentMaterial = &c_unmaterial;
                GLOBAL_mgf_currentMaterialName = nullptr;
                return MGF_OK;
            }
            if ( !isnameWords(av[1])) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            lp = lu_find(&mat_tab, av[1]);    /* lookup context */
            if ( lp == nullptr) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            GLOBAL_mgf_currentMaterialName = lp->key;
            GLOBAL_mgf_currentMaterial = (MgfMaterialContext *) lp->data;
            if ( ac == 2 ) {        /* reestablish previous context */
                if ( GLOBAL_mgf_currentMaterial == nullptr) {
                    return MGF_ERROR_UNDEFINED_REFERENCE;
                }
                return MGF_OK;
            }
            if ( av[2][0] != '=' || av[2][1] ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            if ( GLOBAL_mgf_currentMaterial == nullptr) {    /* create new material */
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
            if ( ac == 3 ) {        /* use default template */
                *GLOBAL_mgf_currentMaterial = c_dfmaterial;
                GLOBAL_mgf_currentMaterial->clock = i + 1;
                return MGF_OK;
            }
            lp = lu_find(&mat_tab, av[3]);    /* lookup template */
            if ( lp == nullptr) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr) {
                return MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *GLOBAL_mgf_currentMaterial = *(MgfMaterialContext *) lp->data;
            GLOBAL_mgf_currentMaterial->clock = i + 1;
            return MGF_OK;
        case MG_E_IR:        /* set index of refraction */
            if ( ac != 3 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isfltWords(av[1]) || !isfltWords(av[2])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->nr = atof(av[1]);
            GLOBAL_mgf_currentMaterial->ni = atof(av[2]);
            if ( GLOBAL_mgf_currentMaterial->nr <= FTINY) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;
        case MG_E_RD:        /* set diffuse reflectance */
            if ( ac != 2 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isfltWords(av[1])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->rd = atof(av[1]);
            if ( GLOBAL_mgf_currentMaterial->rd < 0. || GLOBAL_mgf_currentMaterial->rd > 1. ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->rd_c = *GLOBAL_mgf_currentColor;
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;
        case MG_E_ED:        /* set diffuse emittance */
            if ( ac != 2 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isfltWords(av[1])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->ed = atof(av[1]);
            if ( GLOBAL_mgf_currentMaterial->ed < 0. ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->ed_c = *GLOBAL_mgf_currentColor;
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;
        case MG_E_TD:        /* set diffuse transmittance */
            if ( ac != 2 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isfltWords(av[1])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->td = atof(av[1]);
            if ( GLOBAL_mgf_currentMaterial->td < 0. || GLOBAL_mgf_currentMaterial->td > 1. ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->td_c = *GLOBAL_mgf_currentColor;
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;
        case MG_E_RS:        /* set specular reflectance */
            if ( ac != 3 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isfltWords(av[1]) || !isfltWords(av[2])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->rs = atof(av[1]);
            GLOBAL_mgf_currentMaterial->rs_a = atof(av[2]);
            if ( GLOBAL_mgf_currentMaterial->rs < 0. || GLOBAL_mgf_currentMaterial->rs > 1. ||
                 GLOBAL_mgf_currentMaterial->rs_a < 0. ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->rs_c = *GLOBAL_mgf_currentColor;
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;
        case MG_E_TS:        /* set specular transmittance */
            if ( ac != 3 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isfltWords(av[1]) || !isfltWords(av[2])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentMaterial->ts = atof(av[1]);
            GLOBAL_mgf_currentMaterial->ts_a = atof(av[2]);
            if ( GLOBAL_mgf_currentMaterial->ts < 0. || GLOBAL_mgf_currentMaterial->ts > 1. ||
                 GLOBAL_mgf_currentMaterial->ts_a < 0. ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            GLOBAL_mgf_currentMaterial->ts_c = *GLOBAL_mgf_currentColor;
            GLOBAL_mgf_currentMaterial->clock++;
            return MGF_OK;
        case MG_E_SIDES:    /* set number of sides */
            if ( ac != 2 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isintWords(av[1])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            i = atoi(av[1]);
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

int
handleVertexEntity(int ac, char **av)        /* handle a vertex entity */
{
    /*	int	i; */
    LUENT *lp;

    switch ( mgfEntity(av[0])) {
        case MG_E_VERTEX:    /* get/set vertex context */
            if ( ac > 4 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( ac == 1 ) {        /* set unnamed vertex context */
                c_unvertex = c_dfvertex;
                GLOBAL_mgf_currentVertex = &c_unvertex;
                GLOBAL_mgf_currentVertexName = nullptr;
                return MGF_OK;
            }
            if ( !isnameWords(av[1])) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            lp = lu_find(&vtx_tab, av[1]);    /* lookup context */
            if ( lp == nullptr) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            GLOBAL_mgf_currentVertexName = lp->key;
            GLOBAL_mgf_currentVertex = (MgfVertexContext *) lp->data;
            if ( ac == 2 ) {        /* reestablish previous context */
                if ( GLOBAL_mgf_currentVertex == nullptr) {
                    return MGF_ERROR_UNDEFINED_REFERENCE;
                }
                return MGF_OK;
            }
            if ( av[2][0] != '=' || av[2][1] ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            if ( GLOBAL_mgf_currentVertex == nullptr) {    /* create new vertex context */
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
            /* i = c_cvertex->clock; */
            if ( ac == 3 ) {        /* use default template */
                *GLOBAL_mgf_currentVertex = c_dfvertex;
                /* c_cvertex->clock = i + 1; */
                return MGF_OK;
            }
            lp = lu_find(&vtx_tab, av[3]);    /* lookup template */
            if ( lp == nullptr) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr) {
                return MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *GLOBAL_mgf_currentVertex = *(MgfVertexContext *) lp->data;
            /* c_cvertex->clock = i + 1; */
            GLOBAL_mgf_currentVertex->clock++;
            return MGF_OK;
        case MG_E_POINT:    /* set point */
            if ( ac != 4 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isfltWords(av[1]) || !isfltWords(av[2]) || !isfltWords(av[3])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentVertex->p[0] = atof(av[1]);
            GLOBAL_mgf_currentVertex->p[1] = atof(av[2]);
            GLOBAL_mgf_currentVertex->p[2] = atof(av[3]);
            GLOBAL_mgf_currentVertex->clock++;
            return MGF_OK;
        case MG_E_NORMAL:    /* set normal */
            if ( ac != 4 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isfltWords(av[1]) || !isfltWords(av[2]) || !isfltWords(av[3])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentVertex->n[0] = atof(av[1]);
            GLOBAL_mgf_currentVertex->n[1] = atof(av[2]);
            GLOBAL_mgf_currentVertex->n[2] = atof(av[3]);
            normalize(GLOBAL_mgf_currentVertex->n);
            GLOBAL_mgf_currentVertex->clock++;
            return MGF_OK;
    }
    return MGF_ERROR_UNKNOWN_ENTITY;
}


void
clearContextTables()            /* empty context tables */
{
    c_uncolor = c_dfcolor;
    GLOBAL_mgf_currentColor = &c_uncolor;
    lu_done(&clr_tab);
    c_unmaterial = c_dfmaterial;
    GLOBAL_mgf_currentMaterial = &c_unmaterial;
    GLOBAL_mgf_currentMaterialName = nullptr;
    lu_done(&mat_tab);
    c_unvertex = c_dfvertex;
    GLOBAL_mgf_currentVertex = &c_unvertex;
    GLOBAL_mgf_currentVertexName = nullptr;
    lu_done(&vtx_tab);
}

MgfVertexContext *
getNamedVertex(char *name)            /* get a named vertex */
{
    LUENT *lp;

    if ((lp = lu_find(&vtx_tab, name)) == nullptr) {
        return nullptr;
    }
    return (MgfVertexContext *) lp->data;
}

void
mgfContextFixColorRepresentation(MgfColorContext *clr, int fl)            /* convert color representations */
{
    double x, y, z;
    int i;

    fl &= ~clr->flags;            /* ignore what's done */
    if ( !fl ) {                /* everything's done! */
        return;
    }
    if ( !(clr->flags & (C_CSXY | C_CSSPEC))) {    /* nothing set! */
        *clr = c_dfcolor;
    }
    if ( fl & C_CSXY ) {        /* cspec -> cxy */
        x = y = z = 0.;
        for ( i = 0; i < C_CNSS; i++ ) {
            x += cie_xf.ssamp[i] * clr->ssamp[i];
            y += cie_yf.ssamp[i] * clr->ssamp[i];
            z += cie_zf.ssamp[i] * clr->ssamp[i];
        }
        x /= (double) cie_xf.ssum;
        y /= (double) cie_yf.ssum;
        z /= (double) cie_zf.ssum;
        z += x + y;
        clr->cx = x / z;
        clr->cy = y / z;
        clr->flags |= C_CSXY;
    } else if ( fl & C_CSSPEC ) {    /* cxy -> cspec */
        x = clr->cx;
        y = clr->cy;
        z = 1. - x - y;
        clr->ssum = 0;
        for ( i = 0; i < C_CNSS; i++ ) {
            clr->ssamp[i] = x * cie_xp.ssamp[i] + y * cie_yp.ssamp[i]
                            + z * cie_zp.ssamp[i] + .5;
            if ( clr->ssamp[i] < 0 ) {        /* out of gamut! */
                clr->ssamp[i] = 0;
            } else {
                clr->ssum += clr->ssamp[i];
            }
        }
        clr->flags |= C_CSSPEC;
    }
    if ( fl & C_CSEFF ) {        /* compute efficacy */
        if ( clr->flags & C_CSSPEC ) {        /* from spectrum */
            y = 0.;
            for ( i = 0; i < C_CNSS; i++ ) {
                y += cie_yf.ssamp[i] * clr->ssamp[i];
            }
            clr->eff = C_CLPWM * y / clr->ssum;
        } else /* clr->flags & C_CSXY */ {    /* from (x,y) */
            clr->eff = clr->cx * cie_xf.eff + clr->cy * cie_yf.eff +
                       (1. - clr->cx - clr->cy) * cie_zf.eff;
        }
        clr->flags |= C_CSEFF;
    }
}

static int
setspectrum(MgfColorContext *clr, double wlmin, double wlmax, int ac, char **av)    /* convert a spectrum */
{
    double scale;
    float va[C_CNSS];
    int i, pos;
    int n, imax;
    int wl;
    double wl0, wlstep;
    double boxpos, boxstep;
    /* check getBoundingBox */
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
    imax = ac;            /* box filter if necessary */
    boxpos = 0;
    boxstep = 1;
    if ( wlstep < C_CWLI) {
        imax = (wlmax - wlmin) / C_CWLI + (1 - FTINY);
        boxpos = (wlmin - C_CMINWL) / C_CWLI;
        boxstep = wlstep / C_CWLI;
        wlstep = C_CWLI;
    }
    scale = 0.;            /* get values and maximum */
    pos = 0;
    for ( i = 0; i < imax; i++ ) {
        va[i] = 0.;
        n = 0;
        while ( boxpos < i + .5 && pos < ac ) {
            if ( !isfltWords(av[pos])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            va[i] += atof(av[pos++]);
            n++;
            boxpos += boxstep;
        }
        if ( n > 1 ) {
            va[i] /= (double) n;
        }
        if ( va[i] > scale ) {
            scale = va[i];
        } else if ( va[i] < -scale ) {
            scale = -va[i];
        }
    }
    if ( scale <= FTINY) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    scale = C_CMAXV / scale;
    clr->ssum = 0;            /* convert to our spacing */
    wl0 = wlmin;
    pos = 0;
    for ( i = 0, wl = C_CMINWL; i < C_CNSS; i++, wl += C_CWLI) {
        if ( wl < wlmin || wl > wlmax ) {
            clr->ssamp[i] = 0;
        } else {
            while ( wl0 + wlstep < wl + FTINY) {
                wl0 += wlstep;
                pos++;
            }
            if ( wl + FTINY >= wl0 && wl - FTINY <= wl0 ) {
                clr->ssamp[i] = scale * va[pos] + .5;
            } else {        /* interpolate if necessary */
                clr->ssamp[i] = .5 + scale / wlstep *
                                     (va[pos] * (wl0 + wlstep - wl) +
                                      va[pos + 1] * (wl - wl0));
            }
            clr->ssum += clr->ssamp[i];
        }
    }
    clr->flags = C_CDSPEC | C_CSSPEC;
    clr->clock++;
    return MGF_OK;
}

static void
mixcolors(MgfColorContext *cres, double w1, MgfColorContext *c1, double w2,
          MgfColorContext *c2)    /* mix two colors according to weights given */
{
    double scale;
    float cmix[C_CNSS];
    int i;

    if ((c1->flags | c2->flags) & C_CDSPEC ) {        /* spectral mixing */
        mgfContextFixColorRepresentation(c1, C_CSSPEC | C_CSEFF);
        mgfContextFixColorRepresentation(c2, C_CSSPEC | C_CSEFF);
        w1 /= c1->eff * (float) c1->ssum;
        w2 /= c2->eff * (float) c2->ssum;
        scale = 0.;
        for ( i = 0; i < C_CNSS; i++ ) {
            cmix[i] = w1 * c1->ssamp[i] + w2 * c2->ssamp[i];
            if ( cmix[i] > scale ) {
                scale = cmix[i];
            }
        }
        scale = C_CMAXV / scale;
        cres->ssum = 0;
        for ( i = 0; i < C_CNSS; i++ ) {
            cres->ssum += cres->ssamp[i] = scale * cmix[i] + .5;
        }
        cres->flags = C_CDSPEC | C_CSSPEC;
    } else {                    /* CIE xy mixing */
        mgfContextFixColorRepresentation(c1, C_CSXY);
        mgfContextFixColorRepresentation(c2, C_CSXY);
        scale = w1 / c1->cy + w2 / c2->cy;
        if ( scale == 0. ) {
            return;
        }
        scale = 1. / scale;
        cres->cx = (c1->cx * w1 / c1->cy + c2->cx * w2 / c2->cy) * scale;
        cres->cy = (w1 + w2) * scale;
        cres->flags = C_CDXY | C_CSXY;
    }
}

#define    C1        3.741832e-16    /* W-m^2 */
#define C2        1.4388e-2    /* m-K */

#define bbsp(l, t)    (C1/((l)*(l)*(l)*(l)*(l)*(exp(C2/((t)*(l)))-1.)))
#define bblm(t)        (C2/5./(t))

static int
setbbtemp(MgfColorContext *clr, double tk)        /* set black body spectrum */
{
    double sf, wl;
    int i;

    if ( tk < 1000 ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    wl = bblm(tk);            /* scalefactor based on peak */
    if ( wl < C_CMINWL * 1e-9 ) {
        wl = C_CMINWL * 1e-9;
    } else if ( wl > C_CMAXWL * 1e-9 ) {
        wl = C_CMAXWL * 1e-9;
    }
    sf = C_CMAXV / bbsp(wl, tk);
    clr->ssum = 0;
    for ( i = 0; i < C_CNSS; i++ ) {
        wl = (C_CMINWL + i * C_CWLI) * 1e-9;
        clr->ssum += clr->ssamp[i] = sf * bbsp(wl, tk) + .5;
    }
    clr->flags = C_CDSPEC | C_CSSPEC;
    clr->clock++;
    return MGF_OK;
}

#undef    C1
#undef    C2
#undef    bbsp
#undef    bblm

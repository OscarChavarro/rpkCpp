/**
Parse an MGF file, converting or discarding unsupported entities.
*/

#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <cstring>

#include "common/mymath.h"
#include "MGF/parser.h"
#include "MGF/lookup.h"
#include "MGF/messages.h"
#include "common/FileUncompressWrapper.h"
#include "MGF/words.h"

/*
 * Global definitions of variables declared in parser.h
 */
/* entity names */

char mg_ename[MG_NENTITIES][MG_MAXELEN] = MG_NAMELIST;

/* Handler routines for each entity */

int (*mg_ehand[MG_NENTITIES])(int argc, char **argv);

/* Handler routine for unknown entities */

int (*mg_uhand)(int argc, char **argv) = mg_defuhand;

unsigned mg_nunknown;    /* count of unknown entities */

/* error messages */

char *mg_err[MG_NERRS] = MG_ERRLIST;

MG_FCTXT *mg_file;    /* current file context pointer */

int mg_nqcdivs = MG_NQCD;    /* number of divisions per quarter circle */

/*
 * The idea with this parser is to compensate for any missing entries in
 * mg_ehand with alternate handlers that express these entities in terms
 * of others that the calling program can handle.
 * 
 * In some cases, no alternate handler is possible because the entity
 * has no approximate equivalent.  These entities are simply discarded.
 *
 * Certain entities are dependent on others, and mg_init() will fail
 * if the supported entities are not consistent.
 *
 * Some alternate entity handlers require that earlier entities be
 * noted in some fashion, and we therefore keep another array of
 * parallel support handlers to assist in this effort.
 */

/* temporary settings for testing */
#define e_ies e_any_toss

/* alternate handler support functions */

static int (*e_supp[MG_NENTITIES])(int argc, char **argv);
static char FLTFMT[] = "%.12g";
static int warpconends; // hack for generating good normals

/**
Discard unneeded/unwanted entity
*/
static int
e_any_toss(int ac, char **av) {
    return MG_OK;
}

/**
Compute u and v given w (normalized)
*/
static void
make_axes(double *u, double *v, double *w)
{
    int i;

    v[0] = v[1] = v[2] = 0.;
    for ( i = 0; i < 3; i++ ) {
        if ( w[i] < .6 && w[i] > -.6 ) {
            break;
        }
    }
    v[i] = 1.;
    fcross(u, v, w);
    normalize(u);
    fcross(v, w, u);
}

/**
Put out current xy chromaticities
*/
static int
put_cxy()
{
    static char xbuf[24], ybuf[24];
    static char *ccom[4] = {mg_ename[MG_E_CXY], xbuf, ybuf};

    sprintf(xbuf, "%.4f", c_ccolor->cx);
    sprintf(ybuf, "%.4f", c_ccolor->cy);
    return mg_handle(MG_E_CXY, 3, ccom);
}

/**
Put out current color spectrum
*/
static int
put_cspec()
{
    char wl[2][6], vbuf[C_CNSS][24];
    char *newav[C_CNSS + 4];
    double sf;
    int i;

    if ( mg_ehand[MG_E_CSPEC] != c_hcolor ) {
        sprintf(wl[0], "%d", C_CMINWL);
        sprintf(wl[1], "%d", C_CMAXWL);
        newav[0] = mg_ename[MG_E_CSPEC];
        newav[1] = wl[0];
        newav[2] = wl[1];
        sf = (double) C_CNSS / c_ccolor->ssum;
        for ( i = 0; i < C_CNSS; i++ ) {
            sprintf(vbuf[i], "%.4f", sf * c_ccolor->ssamp[i]);
            newav[i + 3] = vbuf[i];
        }
        newav[C_CNSS + 3] = nullptr;
        if ((i = mg_handle(MG_E_CSPEC, C_CNSS + 3, newav)) != MG_OK ) {
            return i;
        }
    }
    return MG_OK;
}

/**
Handle spectral color
*/
static int
e_cspec(int ac, char **av) {
    /* convert to xy chromaticity */
    c_ccvt(c_ccolor, C_CSXY);
    /* if it's really their handler, use it */
    if ( mg_ehand[MG_E_CXY] != c_hcolor ) {
        return put_cxy();
    }
    return MG_OK;
}

/**
Handle mixing of colors
*/
static int
e_cmix(int ac, char **av) {
    /*
     * Contorted logic works as follows:
     *	1. the colors are already mixed in c_hcolor() support function
     *	2. if we would handle a spectral result, make sure it's not
     *	3. if c_hcolor() would handle a spectral result, don't bother
     *	4. otherwise, make cspec entity and pass it to their handler
     *	5. if we have only xy results, handle it as c_spec() would
     */
    if ( mg_ehand[MG_E_CSPEC] == e_cspec ) {
        c_ccvt(c_ccolor, C_CSXY);
    } else if ( c_ccolor->flags & C_CDSPEC ) {
        return put_cspec();
    }
    if ( mg_ehand[MG_E_CXY] != c_hcolor ) {
        return put_cxy();
    }
    return MG_OK;
}

/**
Handle color temperature
*/
static int
e_cct(int ac, char **av)
{
    /*
     * Logic is similar to e_cmix here.  Support handler has already
     * converted temperature to spectral color.  Put it out as such
     * if they support it, otherwise convert to xy chromaticity and
     * put it out if they handle it.
     */
    if ( mg_ehand[MG_E_CSPEC] != e_cspec ) {
        return put_cspec();
    }
    c_ccvt(c_ccolor, C_CSXY);
    if ( mg_ehand[MG_E_CXY] != c_hcolor ) {
        return put_cxy();
    }
    return MG_OK;
}

/**
Initialize alternate entity handlers
*/
void
mg_init() {
    unsigned long ineed = 0;
    unsigned long uneed = 0;
    int i;

    // pick up slack
    if ( mg_ehand[MG_E_IES] == nullptr) {
        mg_ehand[MG_E_IES] = e_ies;
    }
    if ( mg_ehand[MG_E_INCLUDE] == nullptr) {
        mg_ehand[MG_E_INCLUDE] = e_include;
    }
    if ( mg_ehand[MG_E_SPH] == nullptr) {
        mg_ehand[MG_E_SPH] = e_sph;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( mg_ehand[MG_E_CYL] == nullptr) {
        mg_ehand[MG_E_CYL] = e_cyl;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( mg_ehand[MG_E_CONE] == nullptr) {
        mg_ehand[MG_E_CONE] = e_cone;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( mg_ehand[MG_E_RING] == nullptr) {
        mg_ehand[MG_E_RING] = e_ring;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_NORMAL | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_NORMAL | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( mg_ehand[MG_E_PRISM] == nullptr) {
        mg_ehand[MG_E_PRISM] = e_prism;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( mg_ehand[MG_E_TORUS] == nullptr) {
        mg_ehand[MG_E_TORUS] = e_torus;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_NORMAL | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_NORMAL | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( mg_ehand[MG_E_FACE] == nullptr) {
        mg_ehand[MG_E_FACE] = mg_ehand[MG_E_FACEH];
    } else if ( mg_ehand[MG_E_FACEH] == nullptr) {
        mg_ehand[MG_E_FACEH] = e_faceh;
    }
    if ( mg_ehand[MG_E_COLOR] != nullptr) {
        if ( mg_ehand[MG_E_CMIX] == nullptr) {
            mg_ehand[MG_E_CMIX] = e_cmix;
            ineed |= 1L << MG_E_COLOR | 1L << MG_E_CXY | 1L << MG_E_CSPEC | 1L << MG_E_CMIX | 1L << MG_E_CCT;
        }
        if ( mg_ehand[MG_E_CSPEC] == nullptr) {
            mg_ehand[MG_E_CSPEC] = e_cspec;
            ineed |= 1L << MG_E_COLOR | 1L << MG_E_CXY | 1L << MG_E_CSPEC | 1L << MG_E_CMIX | 1L << MG_E_CCT;
        }
        if ( mg_ehand[MG_E_CCT] == nullptr) {
            mg_ehand[MG_E_CCT] = e_cct;
            ineed |= 1L << MG_E_COLOR | 1L << MG_E_CXY | 1L << MG_E_CSPEC | 1L << MG_E_CMIX | 1L << MG_E_CCT;
        }
    }

    // check for consistency
    if ( mg_ehand[MG_E_FACE] != nullptr) {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( mg_ehand[MG_E_CXY] != nullptr || mg_ehand[MG_E_CSPEC] != nullptr ||
         mg_ehand[MG_E_CMIX] != nullptr) {
        uneed |= 1L << MG_E_COLOR;
    }
    if ( mg_ehand[MG_E_RD] != nullptr || mg_ehand[MG_E_TD] != nullptr ||
         mg_ehand[MG_E_IR] != nullptr ||
         mg_ehand[MG_E_ED] != nullptr ||
         mg_ehand[MG_E_RS] != nullptr ||
         mg_ehand[MG_E_TS] != nullptr ||
         mg_ehand[MG_E_SIDES] != nullptr) {
        uneed |= 1L << MG_E_MATERIAL;
    }
    for ( i = 0; i < MG_NENTITIES; i++ ) {
        if ( uneed & 1L << i && mg_ehand[i] == nullptr) {
            fprintf(stderr, "Missing support for \"%s\" entity\n",
                    mg_ename[i]);
            exit(1);
        }
    }

    // add support as needed
    if ( ineed & 1L << MG_E_VERTEX && mg_ehand[MG_E_VERTEX] != c_hvertex ) {
        e_supp[MG_E_VERTEX] = c_hvertex;
    }
    if ( ineed & 1L << MG_E_POINT && mg_ehand[MG_E_POINT] != c_hvertex ) {
        e_supp[MG_E_POINT] = c_hvertex;
    }
    if ( ineed & 1L << MG_E_NORMAL && mg_ehand[MG_E_NORMAL] != c_hvertex ) {
        e_supp[MG_E_NORMAL] = c_hvertex;
    }
    if ( ineed & 1L << MG_E_COLOR && mg_ehand[MG_E_COLOR] != c_hcolor ) {
        e_supp[MG_E_COLOR] = c_hcolor;
    }
    if ( ineed & 1L << MG_E_CXY && mg_ehand[MG_E_CXY] != c_hcolor ) {
        e_supp[MG_E_CXY] = c_hcolor;
    }
    if ( ineed & 1L << MG_E_CSPEC && mg_ehand[MG_E_CSPEC] != c_hcolor ) {
        e_supp[MG_E_CSPEC] = c_hcolor;
    }
    if ( ineed & 1L << MG_E_CMIX && mg_ehand[MG_E_CMIX] != c_hcolor ) {
        e_supp[MG_E_CMIX] = c_hcolor;
    }
    if ( ineed & 1L << MG_E_CCT && mg_ehand[MG_E_CCT] != c_hcolor ) {
        e_supp[MG_E_CCT] = c_hcolor;
    }

    // Discard remaining entities
    for ( i = 0; i < MG_NENTITIES; i++ ) {
        if ( mg_ehand[i] == nullptr) {
            mg_ehand[i] = e_any_toss;
        }
    }
}

/**
Get entity number from its name
*/
int
mg_entity(char *name)
{
    static LUTAB ent_tab = LU_SINIT(nullptr, nullptr);    /* lookup table */
    char *cp;

    if ( !ent_tab.tsiz ) {        /* initialize hash table */
        if ( !lu_init(&ent_tab, MG_NENTITIES)) {
            return -1;
        }        /* what to do? */
        for ( cp = mg_ename[MG_NENTITIES - 1]; cp >= mg_ename[0];
              cp -= sizeof(mg_ename[0])) {
            lu_find(&ent_tab, cp)->key = cp;
        }
    }
    cp = lu_find(&ent_tab, name)->key;
    if ( cp == nullptr) {
        return -1;
    }
    return (cp - mg_ename[0]) / sizeof(mg_ename[0]);
}

int
mg_handle(int en, int ac, char **av)        /* pass entity to appropriate handler */
{
    int rv;

    if ( en < 0 && (en = mg_entity(av[0])) < 0 ) {    /* unknown entity */
        if ( mg_uhand != nullptr) {
            return (*mg_uhand)(ac, av);
        }
        return MG_EUNK;
    }
    if ( e_supp[en] != nullptr) {            /* support handler */
        /* TODO SITHMASTER: Check number of arguments here */
        if ((rv = (*e_supp[en])(ac, av)) != MG_OK ) {
            return rv;
        }
    }
    return (*mg_ehand[en])(ac, av);        /* assigned handler */
}

int
mg_open(MG_FCTXT *ctx, char *fn)            /* open new input file */
{
    static int nfids;
    char *cp;
    int ispipe;

    ctx->fid = ++nfids;
    ctx->lineno = 0;
    ctx->ispipe = 0;
    if ( fn == nullptr) {
        strcpy(ctx->fname, "<stdin>");
        ctx->fp = stdin;
        ctx->prev = mg_file;
        mg_file = ctx;
        return MG_OK;
    }
    /* get name relative to this context */
    if ( mg_file != nullptr && (cp = strrchr(mg_file->fname, '/')) != nullptr) {
        strcpy(ctx->fname, mg_file->fname);
        strcpy(ctx->fname + (cp - mg_file->fname + 1), fn);
    } else {
        strcpy(ctx->fname, fn);
    }

    ctx->fp = OpenFile(ctx->fname, "r", &ispipe);
    ctx->ispipe = ispipe;

    if ( ctx->fp == nullptr) {
        return MG_ENOFILE;
    }

    ctx->prev = mg_file;        /* establish new context */
    mg_file = ctx;
    return MG_OK;
}


void
mg_close()            /* close input file */
{
    MG_FCTXT *ctx = mg_file;

    mg_file = ctx->prev;        /* restore enclosing context */
    if ( ctx->fp != stdin ) {        /* close file if it's a file */
        CloseFile(ctx->fp, ctx->ispipe);
    }
}


void
mg_fgetpos(MG_FPOS *pos)            /* get current position in input file */
{
    extern long ftell(FILE *);

    pos->fid = mg_file->fid;
    pos->lineno = mg_file->lineno;
    pos->offset = ftell(mg_file->fp);
}

int
mg_fgoto(MG_FPOS *pos)            /* reposition input file pointer */
{
    if ( pos->fid != mg_file->fid ) {
        return MG_ESEEK;
    }
    if ( pos->lineno == mg_file->lineno ) {
        return MG_OK;
    }
    if ( mg_file->fp == stdin || mg_file->ispipe ) {
        return MG_ESEEK;
    }    /* cannot seek on standard input */
    if ( fseek(mg_file->fp, pos->offset, 0) == EOF) {
        return MG_ESEEK;
    }
    mg_file->lineno = pos->lineno;
    return MG_OK;
}

int
mg_read()            /* read next line from file */
{
    int len = 0;

    do {
        if ( fgets(mg_file->inpline + len,
                   MG_MAXLINE - len, mg_file->fp) == nullptr) {
            return len;
        }
        len += strlen(mg_file->inpline + len);
        if ( len >= MG_MAXLINE - 1 ) {
            return len;
        }
        mg_file->lineno++;
    } while ( len > 1 && mg_file->inpline[len - 2] == '\\' );

    return len;
}

int
mg_parse()            /* parse current input line */
{
    char abuf[MG_MAXLINE];
    char *argv[MG_MAXARGC];
    /*	int	en; */
    char *cp, *cp2, **ap;
    /* copy line, removing escape chars */
    cp = abuf;
    cp2 = mg_file->inpline;
    while ((*cp++ = *cp2++)) {
        if ( cp2[0] == '\n' && cp2[-1] == '\\' ) {
            cp--;
        }
    }
    cp = abuf;
    ap = argv;        /* break into words */
    for ( ;; ) {
        while ( isspace(*cp)) {
            *cp++ = '\0';
        }
        if ( !*cp ) {
            break;
        }
        if ( ap - argv >= MG_MAXARGC - 1 ) {
            return MG_EARGC;
        }
        *ap++ = cp;
        while ( *++cp && !isspace(*cp)) {
        }
    }
    if ( ap == argv ) {
        return MG_OK;
    }        /* no words in line */
    *ap = nullptr;
    /* else handle it */
    return mg_handle(-1, ap - argv, argv);
}

int
mg_load(char *fn)            /* load an MGF file */
{
    MG_FCTXT cntxt;
    int rval;
    int nbr;

    if ((rval = mg_open(&cntxt, fn)) != MG_OK ) {
        fprintf(stderr, "%s: %s\n", fn, mg_err[rval]);
        return rval;
    }
    while ((nbr = mg_read()) > 0 ) {    /* parse each line */
        if ( nbr >= MG_MAXLINE - 1 ) {
            fprintf(stderr, "%s: %d: %s\n", cntxt.fname,
                    cntxt.lineno, mg_err[rval = MG_ELINE]);
            break;
        }
        if ((rval = mg_parse()) != MG_OK ) {
            fprintf(stderr, "%s: %d: %s:\n%s", cntxt.fname,
                    cntxt.lineno, mg_err[rval],
                    cntxt.inpline);
            break;
        }
    }
    mg_close();
    return rval;
}

int
mg_defuhand(int ac, char **av)        /* default handler for unknown entities */
{
    if ( mg_nunknown++ == 0 ) {        /* report first incident */
        fprintf(stderr, "%s: %d: %s: %s\n", mg_file->fname,
                mg_file->lineno, mg_err[MG_EUNK], av[0]);
    }
    return MG_OK;
}

void
mg_clear()            /* clear parser history */
{
    c_clearall();            /* clear context tables */
    while ( mg_file != nullptr) {        /* reset our file context */
        mg_close();
    }
}

/****************************************************************************
 *	The following routines handle unsupported entities
 */

int
e_include(int ac, char **av)        /* include file */
{
    char *xfarg[MG_MAXARGC];
    MG_FCTXT ictx;
    XfSpec *xf_orig = xf_context;
    int rv;

    if ( ac < 2 ) {
        return MG_EARGC;
    }
    if ((rv = mg_open(&ictx, av[1])) != MG_OK ) {
        return rv;
    }
    if ( ac > 2 ) {
        int i;

        xfarg[0] = mg_ename[MG_E_XF];
        for ( i = 1; i < ac - 1; i++ ) {
            xfarg[i] = av[i + 1];
        }
        xfarg[ac - 1] = nullptr;
        if ((rv = mg_handle(MG_E_XF, ac - 1, xfarg)) != MG_OK ) {
            mg_close();
            return rv;
        }
    }
    do {
        while ((rv = mg_read()) > 0 ) {
            if ( rv >= MG_MAXLINE - 1 ) {
                fprintf(stderr, "%s: %d: %s\n", ictx.fname,
                        ictx.lineno, mg_err[MG_ELINE]);
                mg_close();
                return MG_EINCL;
            }
            if ((rv = mg_parse()) != MG_OK ) {
                fprintf(stderr, "%s: %d: %s:\n%s", ictx.fname,
                        ictx.lineno, mg_err[rv],
                        ictx.inpline);
                mg_close();
                return MG_EINCL;
            }
        }
        if ( ac > 2 ) {
            if ((rv = mg_handle(MG_E_XF, 1, xfarg)) != MG_OK ) {
                mg_close();
                return rv;
            }
        }
    } while ( xf_context != xf_orig );
    mg_close();
    return MG_OK;
}

int
e_faceh(int ac, char **av)            /* replace face+holes with single contour */
{
    char *newav[MG_MAXARGC];
    int lastp = 0;
    int i, j;

    newav[0] = mg_ename[MG_E_FACE];
    for ( i = 1; i < ac; i++ ) {
        if ( av[i][0] == '-' ) {
            if ( i < 4 ) {
                return MG_EARGC;
            }
            if ( i >= ac - 1 ) {
                break;
            }
            if ( !lastp ) {
                lastp = i - 1;
            }
            for ( j = i + 1; j < ac - 1 && av[j + 1][0] != '-'; j++ ) {
            }
            if ( j - i < 3 ) {
                return MG_EARGC;
            }
            newav[i] = av[j];    /* connect hole loop */
        } else {
            newav[i] = av[i];
        }
    }    /* hole or perimeter vertex */
    if ( lastp ) {
        newav[i++] = av[lastp];
    }        /* finish seam to outside */
    newav[i] = nullptr;
    return mg_handle(MG_E_FACE, i, newav);
}

int
e_sph(int ac, char **av)            /* expand a sphere into cones */
{
    static char p2x[24], p2y[24], p2z[24], r1[24], r2[24];
    static char *v1ent[5] = {mg_ename[MG_E_VERTEX], (char *)"_sv1", (char *)"=", (char *)"_sv2"};
    static char *v2ent[4] = {mg_ename[MG_E_VERTEX], (char *)"_sv2", (char *)"="};
    static char *p2ent[5] = {mg_ename[MG_E_POINT], p2x, p2y, p2z};
    static char *conent[6] = {mg_ename[MG_E_CONE], (char *)"_sv1", r1, (char *)"_sv2", r2};
    C_VERTEX *cv;
    int i;
    int rval;
    double rad;
    double theta;

    if ( ac != 3 ) {
        return MG_EARGC;
    }
    if ((cv = c_getvert(av[1])) == nullptr) {
        return MG_EUNDEF;
    }
    if ( !isfltWords(av[2])) {
        return MG_ETYPE;
    }
    rad = atof(av[2]);
    /* initialize */
    warpconends = 1;
    if ((rval = mg_handle(MG_E_VERTEX, 3, v2ent)) != MG_OK ) {
        return rval;
    }
    sprintf(p2x, FLTFMT, cv->p[0]);
    sprintf(p2y, FLTFMT, cv->p[1]);
    sprintf(p2z, FLTFMT, cv->p[2] + rad);
    if ((rval = mg_handle(MG_E_POINT, 4, p2ent)) != MG_OK ) {
        return rval;
    }
    r2[0] = '0';
    r2[1] = '\0';
    for ( i = 1; i <= 2 * mg_nqcdivs; i++ ) {
        theta = i * (M_PI / 2) / mg_nqcdivs;
        if ((rval = mg_handle(MG_E_VERTEX, 4, v1ent)) != MG_OK ) {
            return rval;
        }
        sprintf(p2z, FLTFMT, cv->p[2] + rad * cos(theta));
        if ((rval = mg_handle(MG_E_VERTEX, 2, v2ent)) != MG_OK ) {
            return rval;
        }
        if ((rval = mg_handle(MG_E_POINT, 4, p2ent)) != MG_OK ) {
            return rval;
        }
        strcpy(r1, r2);
        sprintf(r2, FLTFMT, rad * sin(theta));
        if ((rval = mg_handle(MG_E_CONE, 5, conent)) != MG_OK ) {
            return rval;
        }
    }
    warpconends = 0;
    return MG_OK;
}

int
e_torus(int ac, char **av)            /* expand a torus into cones */
{
    static char p2[3][24], r1[24], r2[24];
    static char *v1ent[5] = {mg_ename[MG_E_VERTEX], (char *)"_tv1", (char *)"=", (char *)"_tv2"};
    static char *v2ent[5] = {mg_ename[MG_E_VERTEX], (char *)"_tv2", (char *)"="};
    static char *p2ent[5] = {mg_ename[MG_E_POINT], p2[0], p2[1], p2[2]};
    static char *conent[6] = {mg_ename[MG_E_CONE], (char *)"_tv1", r1, (char *)"_tv2", r2};
    C_VERTEX *cv;
    int i, j;
    int rval;
    int sgn;
    double minrad, maxrad, avgrad;
    double theta;

    if ( ac != 4 ) {
        return MG_EARGC;
    }
    if ((cv = c_getvert(av[1])) == nullptr) {
        return MG_EUNDEF;
    }
    if ( is0vect(cv->n)) {
        return MG_EILL;
    }
    if ( !isfltWords(av[2]) || !isfltWords(av[3])) {
        return MG_ETYPE;
    }
    minrad = atof(av[2]);
    round0(minrad);
    maxrad = atof(av[3]);
    /* check orientation */
    if ( minrad > 0. ) {
        sgn = 1;
    } else if ( minrad < 0. ) {
        sgn = -1;
    } else if ( maxrad > 0. ) {
        sgn = 1;
    } else if ( maxrad < 0. ) {
        sgn = -1;
    } else {
        return MG_EILL;
    }
    if ( sgn * (maxrad - minrad) <= 0. ) {
        return MG_EILL;
    }
    /* initialize */
    warpconends = 1;
    v2ent[3] = av[1];
    for ( j = 0; j < 3; j++ ) {
        sprintf(p2[j], FLTFMT, cv->p[j] +
                               .5 * sgn * (maxrad - minrad) * cv->n[j]);
    }
    if ((rval = mg_handle(MG_E_VERTEX, 4, v2ent)) != MG_OK ) {
        return rval;
    }
    if ((rval = mg_handle(MG_E_POINT, 4, p2ent)) != MG_OK ) {
        return rval;
    }
    sprintf(r2, FLTFMT, avgrad = .5 * (minrad + maxrad));
    /* run outer section */
    for ( i = 1; i <= 2 * mg_nqcdivs; i++ ) {
        theta = i * (M_PI / 2) / mg_nqcdivs;
        if ((rval = mg_handle(MG_E_VERTEX, 4, v1ent)) != MG_OK ) {
            return rval;
        }
        for ( j = 0; j < 3; j++ ) {
            sprintf(p2[j], FLTFMT, cv->p[j] +
                                   .5 * sgn * (maxrad - minrad) * cos(theta) * cv->n[j]);
        }
        if ((rval = mg_handle(MG_E_VERTEX, 2, v2ent)) != MG_OK ) {
            return rval;
        }
        if ((rval = mg_handle(MG_E_POINT, 4, p2ent)) != MG_OK ) {
            return rval;
        }
        strcpy(r1, r2);
        sprintf(r2, FLTFMT, avgrad + .5 * (maxrad - minrad) * sin(theta));
        if ((rval = mg_handle(MG_E_CONE, 5, conent)) != MG_OK ) {
            return rval;
        }
    }
    /* run inner section */
    sprintf(r2, FLTFMT, -.5 * (minrad + maxrad));
    for ( ; i <= 4 * mg_nqcdivs; i++ ) {
        theta = i * (M_PI / 2) / mg_nqcdivs;
        for ( j = 0; j < 3; j++ ) {
            sprintf(p2[j], FLTFMT, cv->p[j] +
                                   .5 * sgn * (maxrad - minrad) * cos(theta) * cv->n[j]);
        }
        if ((rval = mg_handle(MG_E_VERTEX, 4, v1ent)) != MG_OK ) {
            return rval;
        }
        if ((rval = mg_handle(MG_E_VERTEX, 2, v2ent)) != MG_OK ) {
            return rval;
        }
        if ((rval = mg_handle(MG_E_POINT, 4, p2ent)) != MG_OK ) {
            return rval;
        }
        strcpy(r1, r2);
        sprintf(r2, FLTFMT, -avgrad - .5 * (maxrad - minrad) * sin(theta));
        if ((rval = mg_handle(MG_E_CONE, 5, conent)) != MG_OK ) {
            return rval;
        }
    }
    warpconends = 0;
    return MG_OK;
}

int
e_cyl(int ac, char **av)            /* replace a cylinder with equivalent cone */
{
    static char *avnew[6] = {mg_ename[MG_E_CONE]};

    if ( ac != 4 ) {
        return MG_EARGC;
    }
    avnew[1] = av[1];
    avnew[2] = av[2];
    avnew[3] = av[3];
    avnew[4] = av[2];
    return mg_handle(MG_E_CONE, 5, avnew);
}

int
e_ring(int ac, char **av)            /* turn a ring into polygons */
{
    static char p3[3][24], p4[3][24];
    static char *nzent[5] = {mg_ename[MG_E_NORMAL], (char *)"0", (char *)"0", (char *)"0"};
    static char *v1ent[5] = {mg_ename[MG_E_VERTEX], (char *)"_rv1", (char *)"="};
    static char *v2ent[5] = {mg_ename[MG_E_VERTEX], (char *)"_rv2", (char *)"=", (char *)"_rv3"};
    static char *v3ent[4] = {mg_ename[MG_E_VERTEX], (char *)"_rv3", (char *)"="};
    static char *p3ent[5] = {mg_ename[MG_E_POINT], p3[0], p3[1], p3[2]};
    static char *v4ent[4] = {mg_ename[MG_E_VERTEX], (char *)"_rv4", (char *)"="};
    static char *p4ent[5] = {mg_ename[MG_E_POINT], p4[0], p4[1], p4[2]};
    static char *fent[6] = {mg_ename[MG_E_FACE], (char *)"_rv1", (char *)"_rv2", (char *)"_rv3", (char *)"_rv4"};
    C_VERTEX *cv;
    int i, j;
    FVECT u, v;
    double minrad, maxrad;
    int rv;
    double theta, d;

    if ( ac != 4 ) {
        return MG_EARGC;
    }
    if ((cv = c_getvert(av[1])) == nullptr) {
        return MG_EUNDEF;
    }
    if ( is0vect(cv->n)) {
        return MG_EILL;
    }
    if ( !isfltWords(av[2]) || !isfltWords(av[3])) {
        return MG_ETYPE;
    }
    minrad = atof(av[2]);
    round0(minrad);
    maxrad = atof(av[3]);
    if ( minrad < 0. || maxrad <= minrad ) {
        return MG_EILL;
    }
    /* initialize */
    make_axes(u, v, cv->n);
    for ( j = 0; j < 3; j++ ) {
        sprintf(p3[j], FLTFMT, cv->p[j] + maxrad * u[j]);
    }
    if ((rv = mg_handle(MG_E_VERTEX, 3, v3ent)) != MG_OK ) {
        return rv;
    }
    if ((rv = mg_handle(MG_E_POINT, 4, p3ent)) != MG_OK ) {
        return rv;
    }
    if ( minrad == 0. ) {        /* closed */
        v1ent[3] = av[1];
        if ((rv = mg_handle(MG_E_VERTEX, 4, v1ent)) != MG_OK ) {
            return rv;
        }
        if ((rv = mg_handle(MG_E_NORMAL, 4, nzent)) != MG_OK ) {
            return rv;
        }
        for ( i = 1; i <= 4 * mg_nqcdivs; i++ ) {
            theta = i * (M_PI / 2) / mg_nqcdivs;
            if ((rv = mg_handle(MG_E_VERTEX, 4, v2ent)) != MG_OK ) {
                return rv;
            }
            for ( j = 0; j < 3; j++ ) {
                sprintf(p3[j], FLTFMT, cv->p[j] +
                                       maxrad * u[j] * cos(theta) +
                                       maxrad * v[j] * sin(theta));
            }
            if ((rv = mg_handle(MG_E_VERTEX, 2, v3ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mg_handle(MG_E_POINT, 4, p3ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mg_handle(MG_E_FACE, 4, fent)) != MG_OK ) {
                return rv;
            }
        }
    } else {            /* open */
        if ((rv = mg_handle(MG_E_VERTEX, 3, v4ent)) != MG_OK ) {
            return rv;
        }
        for ( j = 0; j < 3; j++ ) {
            sprintf(p4[j], FLTFMT, cv->p[j] + minrad * u[j]);
        }
        if ((rv = mg_handle(MG_E_POINT, 4, p4ent)) != MG_OK ) {
            return rv;
        }
        v1ent[3] = (char *)"_rv4";
        for ( i = 1; i <= 4 * mg_nqcdivs; i++ ) {
            theta = i * (M_PI / 2) / mg_nqcdivs;
            if ((rv = mg_handle(MG_E_VERTEX, 4, v1ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mg_handle(MG_E_VERTEX, 4, v2ent)) != MG_OK ) {
                return rv;
            }
            for ( j = 0; j < 3; j++ ) {
                d = u[j] * cos(theta) + v[j] * sin(theta);
                sprintf(p3[j], FLTFMT, cv->p[j] + maxrad * d);
                sprintf(p4[j], FLTFMT, cv->p[j] + minrad * d);
            }
            if ((rv = mg_handle(MG_E_VERTEX, 2, v3ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mg_handle(MG_E_POINT, 4, p3ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mg_handle(MG_E_VERTEX, 2, v4ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mg_handle(MG_E_POINT, 4, p4ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mg_handle(MG_E_FACE, 5, fent)) != MG_OK ) {
                return rv;
            }
        }
    }
    return MG_OK;
}

int
e_cone(int ac, char **av)            /* turn a cone into polygons */
{
    static char p3[3][24], p4[3][24], n3[3][24], n4[3][24];
    static char *v1ent[5] = {mg_ename[MG_E_VERTEX], (char *)"_cv1", (char *)"="};
    static char *v2ent[5] = {mg_ename[MG_E_VERTEX], (char *)"_cv2", (char *)"=", (char *)"_cv3"};
    static char *v3ent[4] = {mg_ename[MG_E_VERTEX], (char *)"_cv3", (char *)"="};
    static char *p3ent[5] = {mg_ename[MG_E_POINT], p3[0], p3[1], p3[2]};
    static char *n3ent[5] = {mg_ename[MG_E_NORMAL], n3[0], n3[1], n3[2]};
    static char *v4ent[4] = {mg_ename[MG_E_VERTEX], (char *)"_cv4", (char *)"="};
    static char *p4ent[5] = {mg_ename[MG_E_POINT], p4[0], p4[1], p4[2]};
    static char *n4ent[5] = {mg_ename[MG_E_NORMAL], n4[0], n4[1], n4[2]};
    static char *fent[6] = {mg_ename[MG_E_FACE], (char *)"_cv1", (char *)"_cv2", (char *)"_cv3", (char *)"_cv4"};
    char *v1n;
    C_VERTEX *cv1, *cv2;
    int i, j;
    FVECT u, v, w;
    double rad1, rad2;
    int sgn;
    double n1off, n2off;
    double d;
    int rv;
    double theta;

    if ( ac != 5 ) {
        return MG_EARGC;
    }
    if ((cv1 = c_getvert(av[1])) == nullptr ||
        (cv2 = c_getvert(av[3])) == nullptr) {
        return MG_EUNDEF;
    }
    v1n = av[1];
    if ( !isfltWords(av[2]) || !isfltWords(av[4])) {
        return MG_ETYPE;
    }
    rad1 = atof(av[2]);
    round0(rad1);
    rad2 = atof(av[4]);
    round0(rad2);
    if ( rad1 == 0. ) {
        if ( rad2 == 0. ) {
            return MG_EILL;
        }
    } else if ( rad2 != 0. ) {
        if ((rad1 < 0.) ^ (rad2 < 0.)) {
            return MG_EILL;
        }
    } else {            /* swap */
        C_VERTEX *cv;

        cv = cv1;
        cv1 = cv2;
        cv2 = cv;
        v1n = av[3];
        d = rad1;
        rad1 = rad2;
        rad2 = d;
    }
    sgn = rad2 < 0. ? -1 : 1;
    /* initialize */
    for ( j = 0; j < 3; j++ ) {
        w[j] = cv1->p[j] - cv2->p[j];
    }
    if ((d = normalize(w)) == 0. ) {
        return MG_EILL;
    }
    n1off = n2off = (rad2 - rad1) / d;
    if ( warpconends ) {        /* hack for e_sph and e_torus */
        d = atan(n2off) - (M_PI / 4) / mg_nqcdivs;
        if ( d <= -M_PI / 2 + FTINY) {
            n2off = -FHUGE;
        } else {
            n2off = tan(d);
        }
    }
    make_axes(u, v, w);
    for ( j = 0; j < 3; j++ ) {
        sprintf(p3[j], FLTFMT, cv2->p[j] + rad2 * u[j]);
        if ( n2off <= -FHUGE) {
            sprintf(n3[j], FLTFMT, -w[j]);
        } else {
            sprintf(n3[j], FLTFMT, u[j] + w[j] * n2off);
        }
    }
    if ((rv = mg_handle(MG_E_VERTEX, 3, v3ent)) != MG_OK ) {
        return rv;
    }
    if ((rv = mg_handle(MG_E_POINT, 4, p3ent)) != MG_OK ) {
        return rv;
    }
    if ((rv = mg_handle(MG_E_NORMAL, 4, n3ent)) != MG_OK ) {
        return rv;
    }
    if ( rad1 == 0. ) {        /* triangles */
        v1ent[3] = v1n;
        if ((rv = mg_handle(MG_E_VERTEX, 4, v1ent)) != MG_OK ) {
            return rv;
        }
        for ( j = 0; j < 3; j++ ) {
            sprintf(n4[j], FLTFMT, w[j]);
        }
        if ((rv = mg_handle(MG_E_NORMAL, 4, n4ent)) != MG_OK ) {
            return rv;
        }
        for ( i = 1; i <= 4 * mg_nqcdivs; i++ ) {
            theta = sgn * i * (M_PI / 2) / mg_nqcdivs;
            if ((rv = mg_handle(MG_E_VERTEX, 4, v2ent)) != MG_OK ) {
                return rv;
            }
            for ( j = 0; j < 3; j++ ) {
                d = u[j] * cos(theta) + v[j] * sin(theta);
                sprintf(p3[j], FLTFMT, cv2->p[j] + rad2 * d);
                if ( n2off > -FHUGE) {
                    sprintf(n3[j], FLTFMT, d + w[j] * n2off);
                }
            }
            if ((rv = mg_handle(MG_E_VERTEX, 2, v3ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mg_handle(MG_E_POINT, 4, p3ent)) != MG_OK ) {
                return rv;
            }
            if ( n2off > -FHUGE &&
                 (rv = mg_handle(MG_E_NORMAL, 4, n3ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mg_handle(MG_E_FACE, 4, fent)) != MG_OK ) {
                return rv;
            }
        }
    } else {            /* quads */
        v1ent[3] = (char *)"_cv4";
        if ( warpconends ) {        /* hack for e_sph and e_torus */
            d = atan(n1off) + (M_PI / 4) / mg_nqcdivs;
            if ( d >= M_PI / 2 - FTINY) {
                n1off = FHUGE;
            } else {
                n1off = tan(atan(n1off) + (M_PI / 4) / mg_nqcdivs);
            }
        }
        for ( j = 0; j < 3; j++ ) {
            sprintf(p4[j], FLTFMT, cv1->p[j] + rad1 * u[j]);
            if ( n1off >= FHUGE) {
                sprintf(n4[j], FLTFMT, w[j]);
            } else {
                sprintf(n4[j], FLTFMT, u[j] + w[j] * n1off);
            }
        }
        if ((rv = mg_handle(MG_E_VERTEX, 3, v4ent)) != MG_OK ) {
            return rv;
        }
        if ((rv = mg_handle(MG_E_POINT, 4, p4ent)) != MG_OK ) {
            return rv;
        }
        if ((rv = mg_handle(MG_E_NORMAL, 4, n4ent)) != MG_OK ) {
            return rv;
        }
        for ( i = 1; i <= 4 * mg_nqcdivs; i++ ) {
            theta = sgn * i * (M_PI / 2) / mg_nqcdivs;
            if ((rv = mg_handle(MG_E_VERTEX, 4, v1ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mg_handle(MG_E_VERTEX, 4, v2ent)) != MG_OK ) {
                return rv;
            }
            for ( j = 0; j < 3; j++ ) {
                d = u[j] * cos(theta) + v[j] * sin(theta);
                sprintf(p3[j], FLTFMT, cv2->p[j] + rad2 * d);
                if ( n2off > -FHUGE) {
                    sprintf(n3[j], FLTFMT, d + w[j] * n2off);
                }
                sprintf(p4[j], FLTFMT, cv1->p[j] + rad1 * d);
                if ( n1off < FHUGE) {
                    sprintf(n4[j], FLTFMT, d + w[j] * n1off);
                }
            }
            if ((rv = mg_handle(MG_E_VERTEX, 2, v3ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mg_handle(MG_E_POINT, 4, p3ent)) != MG_OK ) {
                return rv;
            }
            if ( n2off > -FHUGE &&
                 (rv = mg_handle(MG_E_NORMAL, 4, n3ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mg_handle(MG_E_VERTEX, 2, v4ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mg_handle(MG_E_POINT, 4, p4ent)) != MG_OK ) {
                return rv;
            }
            if ( n1off < FHUGE &&
                 (rv = mg_handle(MG_E_NORMAL, 4, n4ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mg_handle(MG_E_FACE, 5, fent)) != MG_OK ) {
                return rv;
            }
        }
    }
    return MG_OK;
}

int
e_prism(int ac, char **av)            /* turn a prism into polygons */
{
    static char p[3][24];
    static char *vent[5] = {mg_ename[MG_E_VERTEX], nullptr, (char *)"="};
    static char *pent[5] = {mg_ename[MG_E_POINT], p[0], p[1], p[2]};
    static char *znorm[5] = {mg_ename[MG_E_NORMAL], (char *)"0", (char *)"0", (char *)"0"};
    char *newav[MG_MAXARGC], nvn[MG_MAXARGC - 1][8];
    double length;
    int hasnorm;
    FVECT v1, v2, v3, norm;
    C_VERTEX *cv;
    C_VERTEX *cv0;
    int rv;
    int i, j;
    /* check arguments */
    if ( ac < 5 ) {
        return MG_EARGC;
    }
    if ( !isfltWords(av[ac - 1])) {
        return MG_ETYPE;
    }
    length = atof(av[ac - 1]);
    if ( length <= FTINY && length >= -FTINY) {
        return MG_EILL;
    }
    /* compute face normal */
    if ((cv0 = c_getvert(av[1])) == nullptr) {
        return MG_EUNDEF;
    }
    hasnorm = 0;
    norm[0] = norm[1] = norm[2] = 0.;
    v1[0] = v1[1] = v1[2] = 0.;
    for ( i = 2; i < ac - 1; i++ ) {
        if ((cv = c_getvert(av[i])) == nullptr) {
            return MG_EUNDEF;
        }
        hasnorm += !is0vect(cv->n);
        v2[0] = cv->p[0] - cv0->p[0];
        v2[1] = cv->p[1] - cv0->p[1];
        v2[2] = cv->p[2] - cv0->p[2];
        fcross(v3, v1, v2);
        norm[0] += v3[0];
        norm[1] += v3[1];
        norm[2] += v3[2];
        VCOPY(v1, v2);
    }
    if ( normalize(norm) == 0. ) {
        return MG_EILL;
    }
    /* create moved vertices */
    for ( i = 1; i < ac - 1; i++ ) {
        sprintf(nvn[i - 1], "_pv%d", i);
        vent[1] = nvn[i - 1];
        vent[3] = av[i];
        if ((rv = mg_handle(MG_E_VERTEX, 4, vent)) != MG_OK ) {
            return rv;
        }
        cv = c_getvert(av[i]);        /* checked above */
        for ( j = 0; j < 3; j++ ) {
            sprintf(p[j], FLTFMT, cv->p[j] - length * norm[j]);
        }
        if ((rv = mg_handle(MG_E_POINT, 4, pent)) != MG_OK ) {
            return rv;
        }
    }
    /* make faces */
    newav[0] = mg_ename[MG_E_FACE];
    /* do the side faces */
    newav[5] = nullptr;
    newav[3] = av[ac - 2];
    newav[4] = nvn[ac - 3];
    for ( i = 1; i < ac - 1; i++ ) {
        newav[1] = nvn[i - 1];
        newav[2] = av[i];
        if ((rv = mg_handle(MG_E_FACE, 5, newav)) != MG_OK ) {
            return rv;
        }
        newav[3] = newav[2];
        newav[4] = newav[1];
    }
    /* do top face */
    for ( i = 1; i < ac - 1; i++ ) {
        if ( hasnorm ) {            /* zero normals */
            vent[1] = nvn[i - 1];
            if ((rv = mg_handle(MG_E_VERTEX, 2, vent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mg_handle(MG_E_NORMAL, 4, znorm)) != MG_OK ) {
                return rv;
            }
        }
        newav[ac - 1 - i] = nvn[i - 1];    /* reverse */
    }
    if ((rv = mg_handle(MG_E_FACE, ac - 1, newav)) != MG_OK ) {
        return rv;
    }
    /* do bottom face */
    if ( hasnorm ) {
        for ( i = 1; i < ac - 1; i++ ) {
            vent[1] = nvn[i - 1];
            vent[3] = av[i];
            if ((rv = mg_handle(MG_E_VERTEX, 4, vent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mg_handle(MG_E_NORMAL, 4, znorm)) != MG_OK ) {
                return rv;
            }
            newav[i] = nvn[i - 1];
        }
    } else {
        for ( i = 1; i < ac - 1; i++ ) {
            newav[i] = av[i];
        }
    }
    newav[i] = nullptr;
    if ((rv = mg_handle(MG_E_FACE, i, newav)) != MG_OK ) {
        return rv;
    }
    return MG_OK;
}

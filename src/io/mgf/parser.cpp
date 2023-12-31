/**
Parse an mgf file, converting or discarding unsupported entities.
*/

#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <cstring>

#include "common/mymath.h"
#include "io/FileUncompressWrapper.h"
#include "io/mgf/lookup.h"
#include "io/mgf/messages.h"
#include "io/mgf/parser.h"

/*
 * Global definitions of variables declared in parser.h
 */
/* entity names */

char mg_ename[MGF_TOTAL_NUMBER_OF_ENTITIES][MG_MAXELEN] = MG_NAMELIST;

/* Handler routines for each entity */

int (*GLOBAL_mgf_handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv);

/* Handler routine for unknown entities */

int (*GLOBAL_mgf_unknownEntityHandleCallback)(int argc, char **argv) = mg_defuhand;

unsigned mg_nunknown;    /* count of unknown entities */

/* error messages */

char *GLOBAL_mgf_errors[MG_NERRS] = MG_ERRLIST;

MgfReaderContext *GLOBAL_mgf_file;    /* current file context pointer */

int GLOBAL_mgf_divisionsPerQuarterCircle = MGF_DEFAULT_NUMBER_OF_DIVISIONS;    /* number of divisions per quarter circle */

/*
 * The idea with this parser is to compensate for any missing entries in
 * GLOBAL_mgf_handleCallbacks with alternate handlers that express these entities in terms
 * of others that the calling program can handle.
 * 
 * In some cases, no alternate handler is possible because the entity
 * has no approximate equivalent.  These entities are simply discarded.
 *
 * Certain entities are dependent on others, and mgfAlternativeInit() will fail
 * if the supported entities are not consistent.
 *
 * Some alternate entity handlers require that earlier entities be
 * noted in some fashion, and we therefore keep another array of
 * parallel support handlers to assist in this effort.
 */

/* temporary settings for testing */
#define e_ies e_any_toss

/* alternate handler support functions */

static int (*e_supp[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv);
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
    return mgfHandle(MG_E_CXY, 3, ccom);
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

    if ( GLOBAL_mgf_handleCallbacks[MG_E_CSPEC] != handleColorEntity ) {
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
        if ((i = mgfHandle(MG_E_CSPEC, C_CNSS + 3, newav)) != MG_OK ) {
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
    mgfContextFixColorRepresentation(c_ccolor, C_CSXY);
    /* if it's really their handler, use it */
    if ( GLOBAL_mgf_handleCallbacks[MG_E_CXY] != handleColorEntity ) {
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
     *	3. if handleColorEntity() would handle a spectral result, don't bother
     *	4. otherwise, make cspec entity and pass it to their handler
     *	5. if we have only xy results, handle it as c_spec() would
     */
    if ( GLOBAL_mgf_handleCallbacks[MG_E_CSPEC] == e_cspec ) {
        mgfContextFixColorRepresentation(c_ccolor, C_CSXY);
    } else if ( c_ccolor->flags & C_CDSPEC ) {
        return put_cspec();
    }
    if ( GLOBAL_mgf_handleCallbacks[MG_E_CXY] != handleColorEntity ) {
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
    if ( GLOBAL_mgf_handleCallbacks[MG_E_CSPEC] != e_cspec ) {
        return put_cspec();
    }
    mgfContextFixColorRepresentation(c_ccolor, C_CSXY);
    if ( GLOBAL_mgf_handleCallbacks[MG_E_CXY] != handleColorEntity ) {
        return put_cxy();
    }
    return MG_OK;
}

/**
Initialize alternate entity handlers
*/
void
mgfAlternativeInit(int (*handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int, char **)) {
    unsigned long ineed = 0;
    unsigned long uneed = 0;
    int i;

    // pick up slack
    if ( handleCallbacks[MG_E_IES] == nullptr) {
        handleCallbacks[MG_E_IES] = e_ies;
    }
    if ( handleCallbacks[MG_E_INCLUDE] == nullptr) {
        handleCallbacks[MG_E_INCLUDE] = e_include;
    }
    if ( handleCallbacks[MGF_ERROR_SPHERE] == nullptr) {
        handleCallbacks[MGF_ERROR_SPHERE] = mgfEntitySphere;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( handleCallbacks[MGF_ERROR_CYLINDER] == nullptr) {
        handleCallbacks[MGF_ERROR_CYLINDER] = mgfEntityCylinder;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( handleCallbacks[MGF_ERROR_CONE] == nullptr) {
        handleCallbacks[MGF_ERROR_CONE] = mgfEntityCone;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( handleCallbacks[MGF_ERROR_RING] == nullptr) {
        handleCallbacks[MGF_ERROR_RING] = mgfEntityRing;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_NORMAL | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_NORMAL | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( handleCallbacks[MGF_ERROR_PRISM] == nullptr) {
        handleCallbacks[MGF_ERROR_PRISM] = mgfEntityPrism;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( handleCallbacks[MGF_ERROR_TORUS] == nullptr) {
        handleCallbacks[MGF_ERROR_TORUS] = mgfEntityTorus;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_NORMAL | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_NORMAL | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( handleCallbacks[MG_E_FACE] == nullptr) {
        handleCallbacks[MG_E_FACE] = handleCallbacks[MG_E_FACEH];
    } else if ( handleCallbacks[MG_E_FACEH] == nullptr) {
        handleCallbacks[MG_E_FACEH] = mgfEntityFaceWithHoles;
    }
    if ( handleCallbacks[MG_E_COLOR] != nullptr) {
        if ( handleCallbacks[MG_E_CMIX] == nullptr) {
            handleCallbacks[MG_E_CMIX] = e_cmix;
            ineed |= 1L << MG_E_COLOR | 1L << MG_E_CXY | 1L << MG_E_CSPEC | 1L << MG_E_CMIX | 1L << MG_E_CCT;
        }
        if ( handleCallbacks[MG_E_CSPEC] == nullptr) {
            handleCallbacks[MG_E_CSPEC] = e_cspec;
            ineed |= 1L << MG_E_COLOR | 1L << MG_E_CXY | 1L << MG_E_CSPEC | 1L << MG_E_CMIX | 1L << MG_E_CCT;
        }
        if ( handleCallbacks[MG_E_CCT] == nullptr) {
            handleCallbacks[MG_E_CCT] = e_cct;
            ineed |= 1L << MG_E_COLOR | 1L << MG_E_CXY | 1L << MG_E_CSPEC | 1L << MG_E_CMIX | 1L << MG_E_CCT;
        }
    }

    // check for consistency
    if ( handleCallbacks[MG_E_FACE] != nullptr) {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( handleCallbacks[MG_E_CXY] != nullptr || handleCallbacks[MG_E_CSPEC] != nullptr ||
         handleCallbacks[MG_E_CMIX] != nullptr) {
        uneed |= 1L << MG_E_COLOR;
    }
    if ( handleCallbacks[MG_E_RD] != nullptr || handleCallbacks[MG_E_TD] != nullptr ||
         handleCallbacks[MG_E_IR] != nullptr ||
         handleCallbacks[MG_E_ED] != nullptr ||
         handleCallbacks[MG_E_RS] != nullptr ||
         handleCallbacks[MG_E_TS] != nullptr ||
         handleCallbacks[MG_E_SIDES] != nullptr) {
        uneed |= 1L << MG_E_MATERIAL;
    }
    for ( i = 0; i < MGF_TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( uneed & 1L << i && handleCallbacks[i] == nullptr) {
            fprintf(stderr, "Missing support for \"%s\" entity\n",
                    mg_ename[i]);
            exit(1);
        }
    }

    // add support as needed
    if ( ineed & 1L << MG_E_VERTEX && handleCallbacks[MG_E_VERTEX] != handleVertexEntity ) {
        e_supp[MG_E_VERTEX] = handleVertexEntity;
    }
    if ( ineed & 1L << MG_E_POINT && handleCallbacks[MG_E_POINT] != handleVertexEntity ) {
        e_supp[MG_E_POINT] = handleVertexEntity;
    }
    if ( ineed & 1L << MG_E_NORMAL && handleCallbacks[MG_E_NORMAL] != handleVertexEntity ) {
        e_supp[MG_E_NORMAL] = handleVertexEntity;
    }
    if ( ineed & 1L << MG_E_COLOR && handleCallbacks[MG_E_COLOR] != handleColorEntity ) {
        e_supp[MG_E_COLOR] = handleColorEntity;
    }
    if ( ineed & 1L << MG_E_CXY && handleCallbacks[MG_E_CXY] != handleColorEntity ) {
        e_supp[MG_E_CXY] = handleColorEntity;
    }
    if ( ineed & 1L << MG_E_CSPEC && handleCallbacks[MG_E_CSPEC] != handleColorEntity ) {
        e_supp[MG_E_CSPEC] = handleColorEntity;
    }
    if ( ineed & 1L << MG_E_CMIX && handleCallbacks[MG_E_CMIX] != handleColorEntity ) {
        e_supp[MG_E_CMIX] = handleColorEntity;
    }
    if ( ineed & 1L << MG_E_CCT && handleCallbacks[MG_E_CCT] != handleColorEntity ) {
        e_supp[MG_E_CCT] = handleColorEntity;
    }

    // Discard remaining entities
    for ( i = 0; i < MGF_TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( handleCallbacks[i] == nullptr) {
            handleCallbacks[i] = e_any_toss;
        }
    }
}

/**
Get entity number from its name
*/
int
mgfEntity(char *name)
{
    static LUTAB ent_tab = LU_SINIT(nullptr, nullptr);    /* lookup table */
    char *cp;

    if ( !ent_tab.tsiz ) {        /* initialize hash table */
        if ( !lu_init(&ent_tab, MGF_TOTAL_NUMBER_OF_ENTITIES)) {
            return -1;
        }        /* what to do? */
        for ( cp = mg_ename[MGF_TOTAL_NUMBER_OF_ENTITIES - 1]; cp >= mg_ename[0];
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
mgfHandle(int en, int ac, char **av)        /* pass entity to appropriate handler */
{
    int rv;

    if ( en < 0 && (en = mgfEntity(av[0])) < 0 ) {    /* unknown entity */
        if ( GLOBAL_mgf_unknownEntityHandleCallback != nullptr) {
            return (*GLOBAL_mgf_unknownEntityHandleCallback)(ac, av);
        }
        return MG_EUNK;
    }
    if ( e_supp[en] != nullptr) {            /* support handler */
        /* TODO SITHMASTER: Check number of arguments here */
        if ((rv = (*e_supp[en])(ac, av)) != MG_OK ) {
            return rv;
        }
    }
    return (*GLOBAL_mgf_handleCallbacks[en])(ac, av);        /* assigned handler */
}

int
mgfOpen(MgfReaderContext *ctx, char *fn)            /* open new input file */
{
    static int nfids;
    char *cp;
    int ispipe;

    ctx->fileContextId = ++nfids;
    ctx->lineNumber = 0;
    ctx->isPipe = 0;
    if ( fn == nullptr) {
        strcpy(ctx->fileName, "<stdin>");
        ctx->fp = stdin;
        ctx->prev = GLOBAL_mgf_file;
        GLOBAL_mgf_file = ctx;
        return MG_OK;
    }
    /* get name relative to this context */
    if ( GLOBAL_mgf_file != nullptr && (cp = strrchr(GLOBAL_mgf_file->fileName, '/')) != nullptr) {
        strcpy(ctx->fileName, GLOBAL_mgf_file->fileName);
        strcpy(ctx->fileName + (cp - GLOBAL_mgf_file->fileName + 1), fn);
    } else {
        strcpy(ctx->fileName, fn);
    }

    ctx->fp = OpenFile(ctx->fileName, "r", &ispipe);
    ctx->isPipe = ispipe;

    if ( ctx->fp == nullptr) {
        return MG_ENOFILE;
    }

    ctx->prev = GLOBAL_mgf_file;        /* establish new context */
    GLOBAL_mgf_file = ctx;
    return MG_OK;
}


void
mgfClose()            /* close input file */
{
    MgfReaderContext *ctx = GLOBAL_mgf_file;

    GLOBAL_mgf_file = ctx->prev;        /* restore enclosing context */
    if ( ctx->fp != stdin ) {        /* close file if it's a file */
        CloseFile(ctx->fp, ctx->isPipe);
    }
}


void
mgfGetFilePosition(MgdReaderFilePosition *pos)            /* get current position in input file */
{
    extern long ftell(FILE *);

    pos->fid = GLOBAL_mgf_file->fileContextId;
    pos->lineno = GLOBAL_mgf_file->lineNumber;
    pos->offset = ftell(GLOBAL_mgf_file->fp);
}

int
mgfGoToFilePosition(MgdReaderFilePosition *pos)            /* reposition input file pointer */
{
    if ( pos->fid != GLOBAL_mgf_file->fileContextId ) {
        return MG_ESEEK;
    }
    if ( pos->lineno == GLOBAL_mgf_file->lineNumber ) {
        return MG_OK;
    }
    if ( GLOBAL_mgf_file->fp == stdin || GLOBAL_mgf_file->isPipe ) {
        return MG_ESEEK;
    }    /* cannot seek on standard input */
    if ( fseek(GLOBAL_mgf_file->fp, pos->offset, 0) == EOF) {
        return MG_ESEEK;
    }
    GLOBAL_mgf_file->lineNumber = pos->lineno;
    return MG_OK;
}

int
mgfReadNextLine()            /* read next line from file */
{
    int len = 0;

    do {
        if ( fgets(GLOBAL_mgf_file->inputLine + len,
                   MGF_MAXIMUM_INPUT_LINE_LENGTH - len, GLOBAL_mgf_file->fp) == nullptr) {
            return len;
        }
        len += strlen(GLOBAL_mgf_file->inputLine + len);
        if ( len >= MGF_MAXIMUM_INPUT_LINE_LENGTH - 1 ) {
            return len;
        }
        GLOBAL_mgf_file->lineNumber++;
    } while ( len > 1 && GLOBAL_mgf_file->inputLine[len - 2] == '\\' );

    return len;
}

int
mgfParseCurrentLine()            /* parse current input line */
{
    char abuf[MGF_MAXIMUM_INPUT_LINE_LENGTH];
    char *argv[MG_MAXARGC];
    /*	int	en; */
    char *cp, *cp2, **ap;
    /* copy line, removing escape chars */
    cp = abuf;
    cp2 = GLOBAL_mgf_file->inputLine;
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
    return mgfHandle(-1, ap - argv, argv);
}

int
mg_load(char *fn)            /* load an mgf file */
{
    MgfReaderContext cntxt;
    int nbr;

    int rval = mgfOpen(&cntxt, fn);
    if ( rval != MG_OK ) {
        fprintf(stderr, "%s: %s\n", fn, GLOBAL_mgf_errors[rval]);
        return rval;
    }
    while ((nbr = mgfReadNextLine()) > 0 ) {    /* parse each line */
        if ( nbr >= MGF_MAXIMUM_INPUT_LINE_LENGTH - 1 ) {
            fprintf(stderr, "%s: %d: %s\n", cntxt.fileName,
                    cntxt.lineNumber, GLOBAL_mgf_errors[rval = MG_ELINE]);
            break;
        }
        if ((rval = mgfParseCurrentLine()) != MG_OK ) {
            fprintf(stderr, "%s: %d: %s:\n%s", cntxt.fileName,
                    cntxt.lineNumber, GLOBAL_mgf_errors[rval],
                    cntxt.inputLine);
            break;
        }
    }
    mgfClose();
    return rval;
}

int
mg_defuhand(int ac, char **av)        /* default handler for unknown entities */
{
    if ( mg_nunknown++ == 0 ) {        /* report first incident */
        fprintf(stderr, "%s: %d: %s: %s\n", GLOBAL_mgf_file->fileName,
                GLOBAL_mgf_file->lineNumber, GLOBAL_mgf_errors[MG_EUNK], av[0]);
    }
    return MG_OK;
}

void
mgfClear()            /* clear parser history */
{
    c_clearall();            /* clear context tables */
    while ( GLOBAL_mgf_file != nullptr) {        /* reset our file context */
        mgfClose();
    }
}

/****************************************************************************
 *	The following routines handle unsupported entities
 */

int
e_include(int ac, char **av)        /* include file */
{
    char *xfarg[MG_MAXARGC];
    MgfReaderContext ictx;
    XfSpec *xf_orig = xf_context;

    if ( ac < 2 ) {
        return MG_EARGC;
    }

    int rv = mgfOpen(&ictx, av[1]);
    if ( rv != MG_OK ) {
        return rv;
    }
    if ( ac > 2 ) {
        int i;

        xfarg[0] = mg_ename[MG_E_XF];
        for ( i = 1; i < ac - 1; i++ ) {
            xfarg[i] = av[i + 1];
        }
        xfarg[ac - 1] = nullptr;
        rv = mgfHandle(MG_E_XF, ac - 1, xfarg);
        if ( rv != MG_OK ) {
            mgfClose();
            return rv;
        }
    }
    do {
        while ((rv = mgfReadNextLine()) > 0 ) {
            if ( rv >= MGF_MAXIMUM_INPUT_LINE_LENGTH - 1 ) {
                fprintf(stderr, "%s: %d: %s\n", ictx.fileName,
                        ictx.lineNumber, GLOBAL_mgf_errors[MG_ELINE]);
                mgfClose();
                return MG_EINCL;
            }
            if ((rv = mgfParseCurrentLine()) != MG_OK ) {
                fprintf(stderr, "%s: %d: %s:\n%s", ictx.fileName,
                        ictx.lineNumber, GLOBAL_mgf_errors[rv],
                        ictx.inputLine);
                mgfClose();
                return MG_EINCL;
            }
        }
        if ( ac > 2 ) {
            if ((rv = mgfHandle(MG_E_XF, 1, xfarg)) != MG_OK ) {
                mgfClose();
                return rv;
            }
        }
    } while ( xf_context != xf_orig );
    mgfClose();
    return MG_OK;
}

int
mgfEntityFaceWithHoles(int ac, char **av)            /* replace face+holes with single contour */
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
    return mgfHandle(MG_E_FACE, i, newav);
}

int
mgfEntitySphere(int ac, char **av)            /* expand a sphere into cones */
{
    static char p2x[24], p2y[24], p2z[24], r1[24], r2[24];
    static char *v1ent[5] = {mg_ename[MG_E_VERTEX], (char *)"_sv1", (char *)"=", (char *)"_sv2"};
    static char *v2ent[4] = {mg_ename[MG_E_VERTEX], (char *)"_sv2", (char *)"="};
    static char *p2ent[5] = {mg_ename[MG_E_POINT], p2x, p2y, p2z};
    static char *conent[6] = {mg_ename[MGF_ERROR_CONE], (char *)"_sv1", r1, (char *)"_sv2", r2};
    MgfVertexContext *cv;
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
    if ((rval = mgfHandle(MG_E_VERTEX, 3, v2ent)) != MG_OK ) {
        return rval;
    }
    sprintf(p2x, FLTFMT, cv->p[0]);
    sprintf(p2y, FLTFMT, cv->p[1]);
    sprintf(p2z, FLTFMT, cv->p[2] + rad);
    if ((rval = mgfHandle(MG_E_POINT, 4, p2ent)) != MG_OK ) {
        return rval;
    }
    r2[0] = '0';
    r2[1] = '\0';
    for ( i = 1; i <= 2 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
        theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
        if ((rval = mgfHandle(MG_E_VERTEX, 4, v1ent)) != MG_OK ) {
            return rval;
        }
        sprintf(p2z, FLTFMT, cv->p[2] + rad * cos(theta));
        if ((rval = mgfHandle(MG_E_VERTEX, 2, v2ent)) != MG_OK ) {
            return rval;
        }
        if ((rval = mgfHandle(MG_E_POINT, 4, p2ent)) != MG_OK ) {
            return rval;
        }
        strcpy(r1, r2);
        sprintf(r2, FLTFMT, rad * sin(theta));
        if ((rval = mgfHandle(MGF_ERROR_CONE, 5, conent)) != MG_OK ) {
            return rval;
        }
    }
    warpconends = 0;
    return MG_OK;
}

int
mgfEntityTorus(int ac, char **av)            /* expand a torus into cones */
{
    static char p2[3][24], r1[24], r2[24];
    static char *v1ent[5] = {mg_ename[MG_E_VERTEX], (char *)"_tv1", (char *)"=", (char *)"_tv2"};
    static char *v2ent[5] = {mg_ename[MG_E_VERTEX], (char *)"_tv2", (char *)"="};
    static char *p2ent[5] = {mg_ename[MG_E_POINT], p2[0], p2[1], p2[2]};
    static char *conent[6] = {mg_ename[MGF_ERROR_CONE], (char *)"_tv1", r1, (char *)"_tv2", r2};
    MgfVertexContext *cv;
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
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
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
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( sgn * (maxrad - minrad) <= 0. ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    /* initialize */
    warpconends = 1;
    v2ent[3] = av[1];
    for ( j = 0; j < 3; j++ ) {
        sprintf(p2[j], FLTFMT, cv->p[j] +
                               .5 * sgn * (maxrad - minrad) * cv->n[j]);
    }
    if ((rval = mgfHandle(MG_E_VERTEX, 4, v2ent)) != MG_OK ) {
        return rval;
    }
    if ((rval = mgfHandle(MG_E_POINT, 4, p2ent)) != MG_OK ) {
        return rval;
    }
    sprintf(r2, FLTFMT, avgrad = .5 * (minrad + maxrad));
    /* run outer section */
    for ( i = 1; i <= 2 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
        theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
        if ((rval = mgfHandle(MG_E_VERTEX, 4, v1ent)) != MG_OK ) {
            return rval;
        }
        for ( j = 0; j < 3; j++ ) {
            sprintf(p2[j], FLTFMT, cv->p[j] +
                                   .5 * sgn * (maxrad - minrad) * cos(theta) * cv->n[j]);
        }
        if ((rval = mgfHandle(MG_E_VERTEX, 2, v2ent)) != MG_OK ) {
            return rval;
        }
        if ((rval = mgfHandle(MG_E_POINT, 4, p2ent)) != MG_OK ) {
            return rval;
        }
        strcpy(r1, r2);
        sprintf(r2, FLTFMT, avgrad + .5 * (maxrad - minrad) * sin(theta));
        if ((rval = mgfHandle(MGF_ERROR_CONE, 5, conent)) != MG_OK ) {
            return rval;
        }
    }
    /* run inner section */
    sprintf(r2, FLTFMT, -.5 * (minrad + maxrad));
    for ( ; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
        theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
        for ( j = 0; j < 3; j++ ) {
            sprintf(p2[j], FLTFMT, cv->p[j] +
                                   .5 * sgn * (maxrad - minrad) * cos(theta) * cv->n[j]);
        }
        if ((rval = mgfHandle(MG_E_VERTEX, 4, v1ent)) != MG_OK ) {
            return rval;
        }
        if ((rval = mgfHandle(MG_E_VERTEX, 2, v2ent)) != MG_OK ) {
            return rval;
        }
        if ((rval = mgfHandle(MG_E_POINT, 4, p2ent)) != MG_OK ) {
            return rval;
        }
        strcpy(r1, r2);
        sprintf(r2, FLTFMT, -avgrad - .5 * (maxrad - minrad) * sin(theta));
        if ((rval = mgfHandle(MGF_ERROR_CONE, 5, conent)) != MG_OK ) {
            return rval;
        }
    }
    warpconends = 0;
    return MG_OK;
}

int
mgfEntityCylinder(int ac, char **av)            /* replace a cylinder with equivalent cone */
{
    static char *avnew[6] = {mg_ename[MGF_ERROR_CONE]};

    if ( ac != 4 ) {
        return MG_EARGC;
    }
    avnew[1] = av[1];
    avnew[2] = av[2];
    avnew[3] = av[3];
    avnew[4] = av[2];
    return mgfHandle(MGF_ERROR_CONE, 5, avnew);
}

int
mgfEntityRing(int ac, char **av)            /* turn a ring into polygons */
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
    MgfVertexContext *cv;
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
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( !isfltWords(av[2]) || !isfltWords(av[3])) {
        return MG_ETYPE;
    }
    minrad = atof(av[2]);
    round0(minrad);
    maxrad = atof(av[3]);
    if ( minrad < 0. || maxrad <= minrad ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    /* initialize */
    make_axes(u, v, cv->n);
    for ( j = 0; j < 3; j++ ) {
        sprintf(p3[j], FLTFMT, cv->p[j] + maxrad * u[j]);
    }
    if ((rv = mgfHandle(MG_E_VERTEX, 3, v3ent)) != MG_OK ) {
        return rv;
    }
    if ((rv = mgfHandle(MG_E_POINT, 4, p3ent)) != MG_OK ) {
        return rv;
    }
    if ( minrad == 0. ) {        /* closed */
        v1ent[3] = av[1];
        if ((rv = mgfHandle(MG_E_VERTEX, 4, v1ent)) != MG_OK ) {
            return rv;
        }
        if ((rv = mgfHandle(MG_E_NORMAL, 4, nzent)) != MG_OK ) {
            return rv;
        }
        for ( i = 1; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
            theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
            if ((rv = mgfHandle(MG_E_VERTEX, 4, v2ent)) != MG_OK ) {
                return rv;
            }
            for ( j = 0; j < 3; j++ ) {
                sprintf(p3[j], FLTFMT, cv->p[j] +
                                       maxrad * u[j] * cos(theta) +
                                       maxrad * v[j] * sin(theta));
            }
            if ((rv = mgfHandle(MG_E_VERTEX, 2, v3ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mgfHandle(MG_E_POINT, 4, p3ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mgfHandle(MG_E_FACE, 4, fent)) != MG_OK ) {
                return rv;
            }
        }
    } else {            /* open */
        if ((rv = mgfHandle(MG_E_VERTEX, 3, v4ent)) != MG_OK ) {
            return rv;
        }
        for ( j = 0; j < 3; j++ ) {
            sprintf(p4[j], FLTFMT, cv->p[j] + minrad * u[j]);
        }
        if ((rv = mgfHandle(MG_E_POINT, 4, p4ent)) != MG_OK ) {
            return rv;
        }
        v1ent[3] = (char *)"_rv4";
        for ( i = 1; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
            theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
            if ((rv = mgfHandle(MG_E_VERTEX, 4, v1ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mgfHandle(MG_E_VERTEX, 4, v2ent)) != MG_OK ) {
                return rv;
            }
            for ( j = 0; j < 3; j++ ) {
                d = u[j] * cos(theta) + v[j] * sin(theta);
                sprintf(p3[j], FLTFMT, cv->p[j] + maxrad * d);
                sprintf(p4[j], FLTFMT, cv->p[j] + minrad * d);
            }
            if ((rv = mgfHandle(MG_E_VERTEX, 2, v3ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mgfHandle(MG_E_POINT, 4, p3ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mgfHandle(MG_E_VERTEX, 2, v4ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mgfHandle(MG_E_POINT, 4, p4ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mgfHandle(MG_E_FACE, 5, fent)) != MG_OK ) {
                return rv;
            }
        }
    }
    return MG_OK;
}

int
mgfEntityCone(int ac, char **av)            /* turn a cone into polygons */
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
    MgfVertexContext *cv1, *cv2;
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
            return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
    } else if ( rad2 != 0. ) {
        if ((rad1 < 0.) ^ (rad2 < 0.)) {
            return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
    } else {            /* swap */
        MgfVertexContext *cv;

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
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    n1off = n2off = (rad2 - rad1) / d;
    if ( warpconends ) {        /* hack for mgfEntitySphere and mgfEntityTorus */
        d = atan(n2off) - (M_PI / 4) / GLOBAL_mgf_divisionsPerQuarterCircle;
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
    if ((rv = mgfHandle(MG_E_VERTEX, 3, v3ent)) != MG_OK ) {
        return rv;
    }
    if ((rv = mgfHandle(MG_E_POINT, 4, p3ent)) != MG_OK ) {
        return rv;
    }
    if ((rv = mgfHandle(MG_E_NORMAL, 4, n3ent)) != MG_OK ) {
        return rv;
    }
    if ( rad1 == 0. ) {        /* triangles */
        v1ent[3] = v1n;
        if ((rv = mgfHandle(MG_E_VERTEX, 4, v1ent)) != MG_OK ) {
            return rv;
        }
        for ( j = 0; j < 3; j++ ) {
            sprintf(n4[j], FLTFMT, w[j]);
        }
        if ((rv = mgfHandle(MG_E_NORMAL, 4, n4ent)) != MG_OK ) {
            return rv;
        }
        for ( i = 1; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
            theta = sgn * i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
            if ((rv = mgfHandle(MG_E_VERTEX, 4, v2ent)) != MG_OK ) {
                return rv;
            }
            for ( j = 0; j < 3; j++ ) {
                d = u[j] * cos(theta) + v[j] * sin(theta);
                sprintf(p3[j], FLTFMT, cv2->p[j] + rad2 * d);
                if ( n2off > -FHUGE) {
                    sprintf(n3[j], FLTFMT, d + w[j] * n2off);
                }
            }
            if ((rv = mgfHandle(MG_E_VERTEX, 2, v3ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mgfHandle(MG_E_POINT, 4, p3ent)) != MG_OK ) {
                return rv;
            }
            if ( n2off > -FHUGE &&
                 (rv = mgfHandle(MG_E_NORMAL, 4, n3ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mgfHandle(MG_E_FACE, 4, fent)) != MG_OK ) {
                return rv;
            }
        }
    } else {            /* quads */
        v1ent[3] = (char *)"_cv4";
        if ( warpconends ) {        /* hack for mgfEntitySphere and mgfEntityTorus */
            d = atan(n1off) + (M_PI / 4) / GLOBAL_mgf_divisionsPerQuarterCircle;
            if ( d >= M_PI / 2 - FTINY) {
                n1off = FHUGE;
            } else {
                n1off = tan(atan(n1off) + (M_PI / 4) / GLOBAL_mgf_divisionsPerQuarterCircle);
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
        if ((rv = mgfHandle(MG_E_VERTEX, 3, v4ent)) != MG_OK ) {
            return rv;
        }
        if ((rv = mgfHandle(MG_E_POINT, 4, p4ent)) != MG_OK ) {
            return rv;
        }
        if ((rv = mgfHandle(MG_E_NORMAL, 4, n4ent)) != MG_OK ) {
            return rv;
        }
        for ( i = 1; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
            theta = sgn * i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
            if ((rv = mgfHandle(MG_E_VERTEX, 4, v1ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mgfHandle(MG_E_VERTEX, 4, v2ent)) != MG_OK ) {
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
            if ((rv = mgfHandle(MG_E_VERTEX, 2, v3ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mgfHandle(MG_E_POINT, 4, p3ent)) != MG_OK ) {
                return rv;
            }
            if ( n2off > -FHUGE &&
                 (rv = mgfHandle(MG_E_NORMAL, 4, n3ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mgfHandle(MG_E_VERTEX, 2, v4ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mgfHandle(MG_E_POINT, 4, p4ent)) != MG_OK ) {
                return rv;
            }
            if ( n1off < FHUGE &&
                 (rv = mgfHandle(MG_E_NORMAL, 4, n4ent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mgfHandle(MG_E_FACE, 5, fent)) != MG_OK ) {
                return rv;
            }
        }
    }
    return MG_OK;
}

int
mgfEntityPrism(int ac, char **av)            /* turn a prism into polygons */
{
    static char p[3][24];
    static char *vent[5] = {mg_ename[MG_E_VERTEX], nullptr, (char *)"="};
    static char *pent[5] = {mg_ename[MG_E_POINT], p[0], p[1], p[2]};
    static char *znorm[5] = {mg_ename[MG_E_NORMAL], (char *)"0", (char *)"0", (char *)"0"};
    char *newav[MG_MAXARGC], nvn[MG_MAXARGC - 1][8];
    double length;
    int hasnorm;
    FVECT v1, v2, v3, norm;
    MgfVertexContext *cv;
    MgfVertexContext *cv0;
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
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
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
        MGF_VERTEX_COPY(v1, v2);
    }
    if ( normalize(norm) == 0. ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    /* create moved vertices */
    for ( i = 1; i < ac - 1; i++ ) {
        sprintf(nvn[i - 1], "_pv%d", i);
        vent[1] = nvn[i - 1];
        vent[3] = av[i];
        if ((rv = mgfHandle(MG_E_VERTEX, 4, vent)) != MG_OK ) {
            return rv;
        }
        cv = c_getvert(av[i]);        /* checked above */
        for ( j = 0; j < 3; j++ ) {
            sprintf(p[j], FLTFMT, cv->p[j] - length * norm[j]);
        }
        if ((rv = mgfHandle(MG_E_POINT, 4, pent)) != MG_OK ) {
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
        if ((rv = mgfHandle(MG_E_FACE, 5, newav)) != MG_OK ) {
            return rv;
        }
        newav[3] = newav[2];
        newav[4] = newav[1];
    }
    /* do top face */
    for ( i = 1; i < ac - 1; i++ ) {
        if ( hasnorm ) {            /* zero normals */
            vent[1] = nvn[i - 1];
            if ((rv = mgfHandle(MG_E_VERTEX, 2, vent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mgfHandle(MG_E_NORMAL, 4, znorm)) != MG_OK ) {
                return rv;
            }
        }
        newav[ac - 1 - i] = nvn[i - 1];    /* reverse */
    }
    if ((rv = mgfHandle(MG_E_FACE, ac - 1, newav)) != MG_OK ) {
        return rv;
    }
    /* do bottom face */
    if ( hasnorm ) {
        for ( i = 1; i < ac - 1; i++ ) {
            vent[1] = nvn[i - 1];
            vent[3] = av[i];
            if ((rv = mgfHandle(MG_E_VERTEX, 4, vent)) != MG_OK ) {
                return rv;
            }
            if ((rv = mgfHandle(MG_E_NORMAL, 4, znorm)) != MG_OK ) {
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
    if ((rv = mgfHandle(MG_E_FACE, i, newav)) != MG_OK ) {
        return rv;
    }
    return MG_OK;
}

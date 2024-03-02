/*
Routines for 4x4 homogeneous, rigid-body transformations
*/

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "common/mymath.h"
#include "io/mgf/parser.h"
#include "io/mgf/badarg.h"

#define d2r(a) ((M_PI/180.)*(a))

#define checkarg(a, l) if ( av[i][(a)] || badarg(ac - i - 1, av + i + 1, (char *)(l))) { \
    finish(icnt, ret, xfmat, xfsca); \
    return i; \
}

MAT4 GLOBAL_mgf_m4Ident = MAT4IDENT;

static MAT4 m4tmp; // for efficiency

XfSpec *GLOBAL_mgf_xfContext; // current context
char **GLOBAL_mgf_xfLastTransform; // end of transform argument list
static char **xf_argbeg; // beginning of transform argument list

XfSpec *new_xf(int, char **);        /* allocate new transform */
void free_xf(XfSpec *);        /* free a transform */
int xf_aname(XfArray *);    /* name this instance */
long comp_xfid(MAT4);        /* compute unique ID */
int xf(XF *, int, char **);        /* interpret transform spec. */
void multmat4(MAT4, MAT4, MAT4);    /* m4a = m4b X m4c */
void multv3(FVECT, FVECT, MAT4);    /* v3a = v3b X m4 (vectors) */
void multp3(FVECT, FVECT, MAT4);    /* p3a = p3b X m4 (positions) */

/**
Handle xf entity
*/
int
handleTransformationEntity(int ac, char **av) {
    XfSpec *spec;
    int n;
    int rv;

    if ( ac == 1 ) {
        // something with existing transform
        if ((spec = GLOBAL_mgf_xfContext) == nullptr) {
            return MGF_ERROR_UNMATCHED_CONTEXT_CLOSE;
        }
        n = -1;
        if ( spec->xarr != nullptr) {
            // check for iteration
            XfArray *ap = spec->xarr;

            xf_aname(nullptr);
            n = ap->ndim;
            while ( n-- ) {
                if ( ++ap->aarg[n].i < ap->aarg[n].n ) {
                    break;
                }
                strcpy(ap->aarg[n].arg, "0");
                ap->aarg[n].i = 0;
            }
            if ( n >= 0 ) {
                if ((rv = mgfGoToFilePosition(&ap->spos)) != MGF_OK ) {
                    return rv;
                }
                snprintf(ap->aarg[n].arg, 8, "%d", ap->aarg[n].i);
                xf_aname(ap);
            }
        }
        if ( n < 0 ) {
            // Pop transform
            GLOBAL_mgf_xfContext = spec->prev;
            free_xf(spec);
            return MGF_OK;
        }
    } else {            /* else allocate transform */
        if ((spec = new_xf(ac - 1, av + 1)) == nullptr) {
            return MGF_ERROR_OUT_OF_MEMORY;
        }
        if ( spec->xarr != nullptr) {
            xf_aname(spec->xarr);
        }
        spec->prev = GLOBAL_mgf_xfContext;    /* push onto stack */
        GLOBAL_mgf_xfContext = spec;
    }
    /* translate new specification */
    n = xf_ac(spec);
    n -= xf_ac(spec->prev);        /* incremental comp. is more eff. */
    if ( xf(&spec->xf, n, xf_av(spec)) != n ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    /* check for vertex reversal */
    if ((spec->rev = (spec->xf.sca < 0.))) {
        spec->xf.sca = -spec->xf.sca;
    }
    /* compute total transformation */
    if ( spec->prev != nullptr) {
        multmat4(spec->xf.xfm, spec->xf.xfm, spec->prev->xf.xfm);
        spec->xf.sca *= spec->prev->xf.sca;
        spec->rev ^= spec->prev->rev;
    }
    spec->xid = comp_xfid(spec->xf.xfm);    /* compute unique ID */
    return MGF_OK;
}

XfSpec *
new_xf(int ac, char **av)            /* allocate new transform structure */
{
    XfSpec *spec;
    int i;
    char *cp;
    int n, ndim;

    ndim = 0;
    n = 0;                /* compute space req'd by arguments */
    for ( i = 0; i < ac; i++ ) {
        if ( !strcmp(av[i], "-a")) {
            ndim++;
            i++;
        } else {
            n += strlen(av[i]) + 1;
        }
    }
    if ( ndim > XF_MAXDIM ) {
        return nullptr;
    }
    spec = (XfSpec *) malloc(sizeof(XfSpec) + n);
    if ( spec == nullptr) {
        return nullptr;
    }
    if ( ndim ) {
        spec->xarr = (XfArray *) malloc(sizeof(XfArray));
        if ( spec->xarr == nullptr) {
            return nullptr;
        }
        mgfGetFilePosition(&spec->xarr->spos);
        spec->xarr->ndim = 0;        /* incremented below */
    } else {
        spec->xarr = nullptr;
    }
    spec->xac = ac + xf_argc;
    /* and store new xf arguments */
    if ( xf_argbeg == nullptr || xf_av(spec) < xf_argbeg ) {
        char **newav =
                (char **) malloc((spec->xac + 1) * sizeof(char *));
        if ( newav == nullptr) {
            return nullptr;
        }
        for ( i = xf_argc; i-- > 0; ) {
            newav[ac + i] = GLOBAL_mgf_xfLastTransform[i - GLOBAL_mgf_xfContext->xac];
        }
        *(GLOBAL_mgf_xfLastTransform = newav + spec->xac) = nullptr;
        if ( xf_argbeg != nullptr) {
            free((void *) xf_argbeg);
        }
        xf_argbeg = newav;
    }
    cp = (char *) (spec + 1);    /* use memory allocated above */
    for ( i = 0; i < ac; i++ ) {
        if ( !strcmp(av[i], "-a")) {
            xf_av(spec)[i++] = (char *)"-i";
            xf_av(spec)[i] = strcpy(
                    spec->xarr->aarg[spec->xarr->ndim].arg,
                    "0");
            spec->xarr->aarg[spec->xarr->ndim].i = 0;
            spec->xarr->aarg[spec->xarr->ndim++].n = atoi(av[i]);
        } else {
            xf_av(spec)[i] = strcpy(cp, av[i]);
            cp += strlen(av[i]) + 1;
        }
    }
    return spec;
}

void
free_xf(XfSpec *spec)            /* free a transform */
{
    if ( spec->xarr != nullptr) {
        free(spec->xarr);
    }
    free(spec);
}

int
xf_aname(XfArray *ap)            /* put out name for this instance */
{
    static char oname[10 * XF_MAXDIM];
    static char *oav[3] = {GLOBAL_mgf_entityNames[MG_E_OBJECT], oname};
    int i;
    char *cp1, *cp2;

    if ( ap == nullptr) {
        return mgfHandle(MG_E_OBJECT, 1, oav);
    }
    cp1 = oname;
    *cp1 = 'a';
    for ( i = 0; i < ap->ndim; i++ ) {
        for ( cp2 = ap->aarg[i].arg; *cp2; ) {
            *++cp1 = *cp2++;
        }
        *++cp1 = '.';
    }
    *cp1 = '\0';
    return mgfHandle(MG_E_OBJECT, 2, oav);
}


long
comp_xfid(double (*xfm)[4])            /* compute unique ID from matrix */
{
    static char shifttab[64] = {15, 5, 11, 5, 6, 3,
                                9, 15, 13, 2, 13, 5, 2, 12, 14, 11,
                                11, 12, 12, 3, 2, 11, 8, 12, 1, 12,
                                5, 4, 15, 9, 14, 5, 13, 14, 2, 10,
                                10, 14, 12, 3, 5, 5, 14, 6, 12, 11,
                                13, 9, 12, 8, 1, 6, 5, 12, 7, 13,
                                15, 8, 9, 2, 6, 11, 9, 11};
    long xid;

    xid = 0;
    // Compute unique transform id
    for ( long unsigned int i = 0; i < sizeof(MAT4) / sizeof(unsigned short); i++ ) {
        xid ^= (long) (((unsigned short *) xfm)[i]) << shifttab[i & 63];
    }
    return xid;
}

void
mgfTransformPoint(FVECT v1, FVECT v2)        /* transform a point by the current matrix */
{
    if ( GLOBAL_mgf_xfContext == nullptr) {
        mgfVertexCopy(v1, v2);
        return;
    }
    multp3(v1, v2, GLOBAL_mgf_xfContext->xf.xfm);
}

void
mgfTransformVector(FVECT v1, FVECT v2)        /* transform a vector using current matrix */
{
    if ( GLOBAL_mgf_xfContext == nullptr) {
        mgfVertexCopy(v1, v2);
        return;
    }
    multv3(v1, v2, GLOBAL_mgf_xfContext->xf.xfm);
}

void
xf_rotvect(double *v1, double *v2)        /* rotate a vector using current matrix */
{
    mgfTransformVector(v1, v2);
    if ( GLOBAL_mgf_xfContext == nullptr) {
        return;
    }
    v1[0] /= GLOBAL_mgf_xfContext->xf.sca;
    v1[1] /= GLOBAL_mgf_xfContext->xf.sca;
    v1[2] /= GLOBAL_mgf_xfContext->xf.sca;
}

double
xf_scale(double d)            /* scale a number by the current transform */
{
    if ( GLOBAL_mgf_xfContext == nullptr) {
        return d;
    }
    return d * GLOBAL_mgf_xfContext->xf.sca;
}

inline void
copyMat4(double (*m4a)[4], MAT4 m4b) {
    memcpy((char *)m4a,(char *)m4b,sizeof(MAT4));
}

inline void
setident4(double (*m4a)[4]) {
    copyMat4(m4a, GLOBAL_mgf_m4Ident);
}

void
multmat4(double (*m4a)[4], double (*m4b)[4],
         double (*m4c)[4])        /* multiply m4b X m4c and put into m4a */
{
    int i, j;

    for ( i = 4; i--; ) {
        for ( j = 4; j--; ) {
            m4tmp[i][j] = m4b[i][0] * m4c[0][j] +
                          m4b[i][1] * m4c[1][j] +
                          m4b[i][2] * m4c[2][j] +
                          m4b[i][3] * m4c[3][j];
        }
    }

    copyMat4(m4a, m4tmp);
}

void
multv3(double *v3a, double *v3b, double (*m4)[4])    /* transform vector v3b by m4 and put into v3a */
{
    m4tmp[0][0] = v3b[0] * m4[0][0] + v3b[1] * m4[1][0] + v3b[2] * m4[2][0];
    m4tmp[0][1] = v3b[0] * m4[0][1] + v3b[1] * m4[1][1] + v3b[2] * m4[2][1];
    m4tmp[0][2] = v3b[0] * m4[0][2] + v3b[1] * m4[1][2] + v3b[2] * m4[2][2];

    v3a[0] = m4tmp[0][0];
    v3a[1] = m4tmp[0][1];
    v3a[2] = m4tmp[0][2];
}

void
multp3(double *p3a, double *p3b, double (*m4)[4])        /* transform p3b by m4 and put into p3a */
{
    multv3(p3a, p3b, m4);    /* transform as vector */
    p3a[0] += m4[3][0];    /* translate */
    p3a[1] += m4[3][1];
    p3a[2] += m4[3][2];
}

void
finish(int icnt, XF *ret, MAT4 xfmat, double xfsca) {
    while ( icnt-- > 0 ) {
        multmat4(ret->xfm, ret->xfm, xfmat);
        ret->sca *= xfsca;
    }
}

/**
Get transform specification
*/
int
xf(XF *ret, int ac, char **av)
{
    MAT4 xfmat;
    MAT4 m4;
    double xfsca;
    double dtmp;
    int i;
    int icnt;

    setident4(ret->xfm);
    ret->sca = 1.0;

    icnt = 1;
    setident4(xfmat);
    xfsca = 1.0;

    for ( i = 0; i < ac && av[i][0] == '-'; i++ ) {
        setident4(m4);

        switch ( av[i][1] ) {

            case 't':
                // Translate
                checkarg(2, "fff");
                m4[3][0] = atof(av[++i]);
                m4[3][1] = atof(av[++i]);
                m4[3][2] = atof(av[++i]);
                break;

            case 'r':
                // Rotate
                switch ( av[i][2] ) {
                    case 'x':
                        checkarg(3, "f");
                        dtmp = d2r(atof(av[++i]));
                        m4[1][1] = m4[2][2] = cos(dtmp);
                        m4[2][1] = -(m4[1][2] = sin(dtmp));
                        break;
                    case 'y':
                        checkarg(3, "f");
                        dtmp = d2r(atof(av[++i]));
                        m4[0][0] = m4[2][2] = cos(dtmp);
                        m4[0][2] = -(m4[2][0] = sin(dtmp));
                        break;
                    case 'z':
                        checkarg(3, "f");
                        dtmp = d2r(atof(av[++i]));
                        m4[0][0] = m4[1][1] = cos(dtmp);
                        m4[1][0] = -(m4[0][1] = sin(dtmp));
                        break;
                    default: {
                        float x;
                        float y;
                        float z;
                        float a;
                        float c;
                        float s;
                        float t;
                        float A;
                        float B;
                        checkarg(2, "ffff");
                        x = atof(av[++i]);
                        y = atof(av[++i]);
                        z = atof(av[++i]);
                        a = d2r(atof(av[++i]));
                        s = std::sqrt(x * x + y * y + z * z);
                        x /= s;
                        y /= s;
                        z /= s;
                        c = cos(a);
                        s = sin(a);
                        t = 1 - c;
                        m4[0][0] = t * x * x + c;
                        m4[1][1] = t * y * y + c;
                        m4[2][2] = t * z * z + c;
                        A = t * x * y;
                        B = s * z;
                        m4[0][1] = A + B;
                        m4[1][0] = A - B;
                        A = t * x * z;
                        B = s * y;
                        m4[0][2] = A - B;
                        m4[2][0] = A + B;
                        A = t * y * z;
                        B = s * x;
                        m4[1][2] = A + B;
                        m4[2][1] = A - B;
                    }
                }
                break;

            case 's':
                // Scale
                switch ( av[i][2] ) {
                    case 'x':
                        checkarg(3, "f");
                        dtmp = atof(av[i + 1]);
                        if ( dtmp == 0.0 ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
                        m4[0][0] = dtmp;
                        break;
                    case 'y':
                        checkarg(3, "f");
                        dtmp = atof(av[i + 1]);
                        if ( dtmp == 0.0 ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
                        m4[1][1] = dtmp;
                        break;
                    case 'z':
                        checkarg(3, "f");
                        dtmp = atof(av[i + 1]);
                        if ( dtmp == 0.0 ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
                        m4[2][2] = dtmp;
                        break;
                    default:
                        checkarg(2, "f");
                        dtmp = atof(av[i + 1]);
                        if ( dtmp == 0.0 ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
                        xfsca *=
                        m4[0][0] =
                        m4[1][1] =
                        m4[2][2] = dtmp;
                        break;
                }
                i++;
                break;

            case 'm':
                // Mirror
                switch ( av[i][2] ) {
                    case 'x':
                        checkarg(3, "");
                        xfsca *=
                        m4[0][0] = -1.0;
                        break;
                    case 'y':
                        checkarg(3, "");
                        xfsca *=
                        m4[1][1] = -1.0;
                        break;
                    case 'z':
                        checkarg(3, "");
                        xfsca *=
                        m4[2][2] = -1.0;
                        break;
                    default:
                        finish(icnt, ret, xfmat, xfsca);
                        return i;
                }
                break;

            case 'i':
                // Iterate
                checkarg(2, "i");
                while ( icnt-- > 0 ) {
                    multmat4(ret->xfm, ret->xfm, xfmat);
                    ret->sca *= xfsca;
                }
                icnt = atoi(av[++i]);
                setident4(xfmat);
                xfsca = 1.0;
                continue;

            default:
                finish(icnt, ret, xfmat, xfsca);
                return i;

        }
        multmat4(xfmat, xfmat, m4);
    }

    finish(icnt, ret, xfmat, xfsca);
    return i;
}

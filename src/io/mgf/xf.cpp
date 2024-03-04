/**
Routines for 4x4 homogeneous, rigid-body transformations
*/

#include "io/mgf/parser.h"

static MAT4 globalM4Tmp; // For efficiency
static char **globalTransformArgumentListBeginning;

MAT4 GLOBAL_mgf_m4Ident = MAT4IDENT;
XfSpec *GLOBAL_mgf_xfContext; // Current context
char **GLOBAL_mgf_xfLastTransform; // End of transform argument list

/**
Transform vector v3b by m4 and put into v3a
*/
static void
multv3(double *v3a, const double *v3b, double (*m4)[4])
{
    globalM4Tmp[0][0] = v3b[0] * m4[0][0] + v3b[1] * m4[1][0] + v3b[2] * m4[2][0];
    globalM4Tmp[0][1] = v3b[0] * m4[0][1] + v3b[1] * m4[1][1] + v3b[2] * m4[2][1];
    globalM4Tmp[0][2] = v3b[0] * m4[0][2] + v3b[1] * m4[1][2] + v3b[2] * m4[2][2];

    v3a[0] = globalM4Tmp[0][0];
    v3a[1] = globalM4Tmp[0][1];
    v3a[2] = globalM4Tmp[0][2];
}

/**
Transform p3b by m4 and put into p3a
*/
static void
multp3(double *p3a, double *p3b, double (*m4)[4])
{
    multv3(p3a, p3b, m4); // Transform as vector
    p3a[0] += m4[3][0]; // Translate
    p3a[1] += m4[3][1];
    p3a[2] += m4[3][2];
}

static void
copyMat4(double (*m4a)[4], MAT4 m4b) {
    memcpy((char *)m4a,(char *)m4b,sizeof(MAT4));
}

static void
setident4(double (*m4a)[4]) {
    copyMat4(m4a, GLOBAL_mgf_m4Ident);
}

/**
Multiply m4b X m4c and put into m4a
*/
static void
multmat4(double (*m4a)[4], double (*m4b)[4], double (*m4c)[4])
{
    int i, j;

    for ( i = 4; i--; ) {
        for ( j = 4; j--; ) {
            globalM4Tmp[i][j] = m4b[i][0] * m4c[0][j] +
                                m4b[i][1] * m4c[1][j] +
                                m4b[i][2] * m4c[2][j] +
                                m4b[i][3] * m4c[3][j];
        }
    }

    copyMat4(m4a, globalM4Tmp);
}

/**
Compute unique ID from matrix
*/
static long
comp_xfid(double (*xfm)[4])
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

/**
Free a transform
*/
static void
free_xf(XfSpec *spec)
{
    if ( spec->xarr != nullptr) {
        free(spec->xarr);
    }
    free(spec);
}

static double
d2r(double a) {
    return (M_PI / 180.0) * a;
}

static bool
checkarg(int a, const char *l, int ac, char **av, int i) {
    if ( av[i][(a)] || checkForBadArguments(ac - i - 1, av + i + 1, (char *) (l)) ) {
        return false;
    }
    return true;
}

/**
Put out name for this instance
*/
static int
xf_aname(XfArray *ap)
{
    static char oname[10 * XF_MAXDIM];
    static char *oav[3] = {GLOBAL_mgf_entityNames[MG_E_OBJECT], oname};
    int i;
    char *cp1;
    char *cp2;

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

/**
Allocate new transform structure
*/
static XfSpec *
new_xf(int ac, char **av)
{
    XfSpec *spec;
    int i;
    char *cp;
    int n;
    int ndim;

    ndim = 0;
    n = 0;

    // Compute space req'd by arguments
    for ( i = 0; i < ac; i++ ) {
        if ( !strcmp(av[i], "-a") ) {
            ndim++;
            i++;
        } else {
            n += (int)strlen(av[i]) + 1;
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
        spec->xarr->ndim = 0; // Incremented below
    } else {
        spec->xarr = nullptr;
    }
    spec->xac = (short)(ac + xf_argc);

    // And store new xf arguments
    if ( globalTransformArgumentListBeginning == nullptr || xf_av(spec) < globalTransformArgumentListBeginning ) {
        char **newav =
                (char **) malloc((spec->xac + 1) * sizeof(char *));
        if ( newav == nullptr) {
            return nullptr;
        }
        for ( i = xf_argc; i-- > 0; ) {
            newav[ac + i] = GLOBAL_mgf_xfLastTransform[i - GLOBAL_mgf_xfContext->xac];
        }
        *(GLOBAL_mgf_xfLastTransform = newav + spec->xac) = nullptr;
        if ( globalTransformArgumentListBeginning != nullptr) {
            free((void *) globalTransformArgumentListBeginning);
        }
        globalTransformArgumentListBeginning = newav;
    }
    cp = (char *) (spec + 1);

    // Use memory allocated above
    for ( i = 0; i < ac; i++ ) {
        if ( !strcmp(av[i], "-a") ) {
            xf_av(spec)[i++] = (char *)"-i";
            xf_av(spec)[i] = strcpy(
                    spec->xarr->aarg[spec->xarr->ndim].arg,
                    "0");
            spec->xarr->aarg[spec->xarr->ndim].i = 0;
            spec->xarr->aarg[spec->xarr->ndim++].n = (short)strtol(av[i], nullptr, 10);
        } else {
            xf_av(spec)[i] = strcpy(cp, av[i]);
            cp += strlen(av[i]) + 1;
        }
    }
    return spec;
}

/**
Transform a point by the current matrix
*/
void
mgfTransformPoint(FVECT v1, FVECT v2)
{
    if ( GLOBAL_mgf_xfContext == nullptr) {
        mgfVertexCopy(v1, v2);
        return;
    }
    multp3(v1, v2, GLOBAL_mgf_xfContext->xf.xfm);
}

/**
Transform a vector using current matrix
*/
void
mgfTransformVector(FVECT v1, FVECT v2)
{
    if ( GLOBAL_mgf_xfContext == nullptr) {
        mgfVertexCopy(v1, v2);
        return;
    }
    multv3(v1, v2, GLOBAL_mgf_xfContext->xf.xfm);
}

static void
finish(int icnt, XF *ret, MAT4 xfmat, double xfsca) {
    while ( icnt-- > 0 ) {
        multmat4(ret->xfm, ret->xfm, xfmat);
        ret->sca *= xfsca;
    }
}

/**
Get transform specification
*/
static int
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
                if ( !checkarg(2, "fff", ac, av, i) ) {
                    finish(icnt, ret, xfmat, xfsca);
                    return i;
                }
                m4[3][0] = strtod(av[++i], nullptr);
                m4[3][1] = strtod(av[++i], nullptr);
                m4[3][2] = strtod(av[++i], nullptr);
                break;

            case 'r':
                // Rotate
                switch ( av[i][2] ) {
                    case 'x':
                        if ( !checkarg(3, "f", ac, av, i) ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
                        dtmp = d2r(strtod(av[++i], nullptr));
                        m4[1][1] = m4[2][2] = std::cos(dtmp);
                        m4[2][1] = -(m4[1][2] = std::sin(dtmp));
                        break;
                    case 'y':
                        if ( !checkarg(3, "f", ac, av, i) ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
                        dtmp = d2r(strtod(av[++i], nullptr));
                        m4[0][0] = m4[2][2] = std::cos(dtmp);
                        m4[0][2] = -(m4[2][0] = std::sin(dtmp));
                        break;
                    case 'z':
                        if ( !checkarg(3, "f", ac, av, i) ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
                        dtmp = d2r(strtod(av[++i], nullptr));
                        m4[0][0] = m4[1][1] = std::cos(dtmp);
                        m4[1][0] = -(m4[0][1] = std::sin(dtmp));
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

                        if ( !checkarg(2, "ffff", ac, av, i) ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
                        x = strtof(av[++i], nullptr);
                        y = strtof(av[++i], nullptr);
                        z = strtof(av[++i], nullptr);
                        a = (float)d2r(strtod(av[++i], nullptr));
                        s = std::sqrt(x * x + y * y + z * z);
                        x /= s;
                        y /= s;
                        z /= s;
                        c = std::cos(a);
                        s = std::sin(a);
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
                        if ( !checkarg(3, "f", ac, av, i) ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
                        dtmp = strtod(av[i + 1], nullptr);
                        if ( dtmp == 0.0 ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
                        m4[0][0] = dtmp;
                        break;
                    case 'y':
                        if ( !checkarg(3, "f", ac, av, i) ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
                        dtmp = strtod(av[i + 1], nullptr);
                        if ( dtmp == 0.0 ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
                        m4[1][1] = dtmp;
                        break;
                    case 'z':
                        if ( !checkarg(3, "f", ac, av, i) ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
                        dtmp = strtod(av[i + 1], nullptr);
                        if ( dtmp == 0.0 ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
                        m4[2][2] = dtmp;
                        break;
                    default:
                        if ( !checkarg(2, "f", ac, av, i) ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
                        dtmp = strtod(av[i + 1], nullptr);
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
                        if ( !checkarg(3, "", ac, av, i) ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
                        xfsca *=
                        m4[0][0] = -1.0;
                        break;
                    case 'y':
                        if ( !checkarg(3, "", ac, av, i) ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
                        xfsca *=
                        m4[1][1] = -1.0;
                        break;
                    case 'z':
                        if ( !checkarg(3, "", ac, av, i) ) {
                            finish(icnt, ret, xfmat, xfsca);
                            return i;
                        }
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
                if ( !checkarg(2, "i", ac, av, i) ) {
                    finish(icnt, ret, xfmat, xfsca);
                    return i;
                }
                while ( icnt-- > 0 ) {
                    multmat4(ret->xfm, ret->xfm, xfmat);
                    ret->sca *= xfsca;
                }
                icnt = (int)strtol(av[++i], nullptr, 10);
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

/**
Handle xf entity
*/
int
handleTransformationEntity(int ac, char **av) {
    XfSpec *spec;
    int n;
    int rv;

    if ( ac == 1 ) {
        // Something with existing transform
        if ( (spec = GLOBAL_mgf_xfContext) == nullptr ) {
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
                rv = mgfGoToFilePosition(&ap->spos);
                if ( rv != MGF_OK ) {
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
    } else {
        // Allocate transform
        spec = new_xf(ac - 1, av + 1);
        if ( spec == nullptr ) {
            return MGF_ERROR_OUT_OF_MEMORY;
        }
        if ( spec->xarr != nullptr) {
            xf_aname(spec->xarr);
        }
        spec->prev = GLOBAL_mgf_xfContext; // Push onto stack
        GLOBAL_mgf_xfContext = spec;
    }

    // Translate new specification
    n = xf_ac(spec);
    n -= xf_ac(spec->prev); // Incremental comp. is more eff.
    if ( xf(&spec->xf, n, xf_av(spec)) != n ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }

    // Check for vertex reversal
    if ( (spec->rev = (spec->xf.sca < 0.0)) ) {
        spec->xf.sca = -spec->xf.sca;
    }

    // Compute total transformation
    if ( spec->prev != nullptr) {
        multmat4(spec->xf.xfm, spec->xf.xfm, spec->prev->xf.xfm);
        spec->xf.sca *= spec->prev->xf.sca;
        spec->rev = static_cast<short>(spec->rev ^ spec->prev->rev);
    }
    spec->xid = comp_xfid(spec->xf.xfm); // Compute unique ID
    return MGF_OK;
}

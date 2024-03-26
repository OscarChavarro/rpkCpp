/**
Routines for 4x4 homogeneous, rigid-body transformations
*/

#include <cstring>

#include "common/linealAlgebra/Vector3Dd.h"
#include "io/mgf/badarg.h"
#include "io/mgf/MgfTransformContext.h"

static char **globalTransformArgumentListBeginning;
static char **globalLastTransform; // End of transform argument list (last transform argument)

/**
Compute unique ID from matrix
*/
static long
computeUniqueId(double (*xfm)[4]) {
    static char shiftTab[64] = {15, 5, 11, 5, 6, 3,
                                9, 15, 13, 2, 13, 5, 2, 12, 14, 11,
                                11, 12, 12, 3, 2, 11, 8, 12, 1, 12,
                                5, 4, 15, 9, 14, 5, 13, 14, 2, 10,
                                10, 14, 12, 3, 5, 5, 14, 6, 12, 11,
                                13, 9, 12, 8, 1, 6, 5, 12, 7, 13,
                                15, 8, 9, 2, 6, 11, 9, 11};
    long xid;

    xid = 0;
    // Compute unique transform id
    for ( long unsigned int i = 0; i < sizeof(MATRIX4Dd) / sizeof(unsigned short); i++ ) {
        xid ^= (long) (((unsigned short *) xfm)[i]) << shiftTab[i & 63];
    }
    return xid;
}

/**
Free a transform
*/
static void
free_xf(MgfTransformContext *spec) {
    if ( spec->transformationArray != nullptr ) {
        free(spec->transformationArray);
    }
    free(spec);
}

static double
d2r(double a) {
    return (M_PI / 180.0) * a;
}

static bool
checkArgument(int a, const char *l, int ac, char **av, int i) {
    if ( av[i][(a)] || checkForBadArguments(ac - i - 1, av + i + 1, (char *)(l)) ) {
        return false;
    }
    return true;
}

/**
Put out name for this instance
*/
static int
transformName(MgfTransformArray *ap, MgfContext *context) {
    static char oName[10 * TRANSFORM_MAXIMUM_DIMENSIONS];
    static char *oav[3] = {
            context->entityNames[MGF_ENTITY_OBJECT], oName
    };
    int i;
    char *cp1;
    char *cp2;

    if ( ap == nullptr ) {
        return mgfHandle(MGF_ENTITY_OBJECT, 1, oav, context);
    }
    cp1 = oName;
    *cp1 = 'a';
    for ( i = 0; i < ap->numberOfDimensions; i++ ) {
        for ( cp2 = ap->transformArguments[i].arg; *cp2; ) {
            *++cp1 = *cp2++;
        }
        *++cp1 = '.';
    }
    *cp1 = '\0';
    return mgfHandle(MGF_ENTITY_OBJECT, 2, oav, context);
}

/**
Allocate new transform structure
*/
static MgfTransformContext *
newTransform(int ac, char **av, MgfContext *context) {
    MgfTransformContext *spec;
    char *cp;
    int n;
    int nDim;

    nDim = 0;
    n = 0;

    // Compute space required by arguments
    for ( int i = 0; i < ac; i++ ) {
        if ( !strcmp(av[i], "-a") ) {
            nDim++;
            i++;
        } else {
            n += (int)strlen(av[i]) + 1;
        }
    }
    if ( nDim > TRANSFORM_MAXIMUM_DIMENSIONS ) {
        return nullptr;
    }
    spec = (MgfTransformContext *) malloc(sizeof(MgfTransformContext) + n);
    if ( spec == nullptr) {
        return nullptr;
    }
    if ( nDim ) {
        spec->transformationArray = (MgfTransformArray *) malloc(sizeof(MgfTransformArray));
        if ( spec->transformationArray == nullptr) {
            return nullptr;
        }
        mgfGetFilePosition(&spec->transformationArray->startingPosition, context);
        spec->transformationArray->numberOfDimensions = 0; // Incremented below
    } else {
        spec->transformationArray = nullptr;
    }
    spec->xac = (short)(ac + TRANSFORM_ARGC(context->transformContext));

    // And store new xf arguments
    if ( globalTransformArgumentListBeginning == nullptr || TRANSFORM_ARGV(spec) < globalTransformArgumentListBeginning ) {
        char **newAv =
                (char **) malloc((spec->xac + 1) * sizeof(char *));
        if ( newAv == nullptr) {
            return nullptr;
        }
        for ( int i = TRANSFORM_ARGC(context->transformContext); i-- > 0; ) {
            newAv[ac + i] = globalLastTransform[i - context->transformContext->xac];
        }
        *(globalLastTransform = newAv + spec->xac) = nullptr;
        if ( globalTransformArgumentListBeginning != nullptr) {
            free((void *) globalTransformArgumentListBeginning);
        }
        globalTransformArgumentListBeginning = newAv;
    }
    cp = (char *)(spec + 1);

    // Use memory allocated above
    for ( int i = 0; i < ac; i++ ) {
        if ( !strcmp(av[i], "-a") ) {
            TRANSFORM_ARGV(spec)[i++] = (char *)"-i";
            TRANSFORM_ARGV(spec)[i] = strcpy(
                    spec->transformationArray->transformArguments[spec->transformationArray->numberOfDimensions].arg,
                    "0");
            spec->transformationArray->transformArguments[spec->transformationArray->numberOfDimensions].i = 0;
            spec->transformationArray->transformArguments[spec->transformationArray->numberOfDimensions++].n = (short)strtol(av[i], nullptr, 10);
        } else {
            TRANSFORM_ARGV(spec)[i] = strcpy(cp, av[i]);
            cp += strlen(av[i]) + 1;
        }
    }
    return spec;
}

/**
Transform a point by the current matrix
*/
void
mgfTransformPoint(VECTOR3Dd v1, VECTOR3Dd v2, MgfContext *context) {
    if ( context->transformContext == nullptr) {
        mgfVertexCopy(v1, v2);
        return;
    }
    multiplyP3(v1, v2, context->transformContext->xf.xfm);
}

/**
Transform a vector using current matrix
*/
void
mgfTransformVector(VECTOR3Dd v1, VECTOR3Dd v2, MgfContext *context) {
    if ( context->transformContext == nullptr) {
        mgfVertexCopy(v1, v2);
        return;
    }
    multiplyV3(v1, v2, context->transformContext->xf.xfm);
}

static void
finish(int count, MgfTransform *ret, MATRIX4Dd transformMatrix, double scaTransform) {
    while ( count-- > 0 ) {
        multiplyMatrix4(ret->xfm, ret->xfm, transformMatrix);
        ret->sca *= scaTransform;
    }
}

/**
Get transform specification
*/
static int
xf(MgfTransform *ret, int ac, char **av) {
    MATRIX4Dd transformMatrix;
    MATRIX4Dd m4;
    double scaTransform;
    double tmp;
    int counter;

    setIdent4(ret->xfm);
    ret->sca = 1.0;

    counter = 1;
    setIdent4(transformMatrix);
    scaTransform = 1.0;

    int i;
    for ( i = 0; i < ac && av[i][0] == '-'; i++ ) {
        setIdent4(m4);

        switch ( av[i][1] ) {

            case 't':
                // Translate
                if ( !checkArgument(2, "fff", ac, av, i) ) {
                    finish(counter, ret, transformMatrix, scaTransform);
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
                        if ( !checkArgument(3, "f", ac, av, i) ) {
                            finish(counter, ret, transformMatrix, scaTransform);
                            return i;
                        }
                        tmp = d2r(strtod(av[++i], nullptr));
                        m4[1][1] = m4[2][2] = std::cos(tmp);
                        m4[2][1] = -(m4[1][2] = std::sin(tmp));
                        break;
                    case 'y':
                        if ( !checkArgument(3, "f", ac, av, i) ) {
                            finish(counter, ret, transformMatrix, scaTransform);
                            return i;
                        }
                        tmp = d2r(strtod(av[++i], nullptr));
                        m4[0][0] = m4[2][2] = std::cos(tmp);
                        m4[0][2] = -(m4[2][0] = std::sin(tmp));
                        break;
                    case 'z':
                        if ( !checkArgument(3, "f", ac, av, i) ) {
                            finish(counter, ret, transformMatrix, scaTransform);
                            return i;
                        }
                        tmp = d2r(strtod(av[++i], nullptr));
                        m4[0][0] = m4[1][1] = std::cos(tmp);
                        m4[1][0] = -(m4[0][1] = std::sin(tmp));
                        break;
                    default: {
                        if ( !checkArgument(2, "ffff", ac, av, i) ) {
                            finish(counter, ret, transformMatrix, scaTransform);
                            return i;
                        }
                        float x = strtof(av[++i], nullptr);
                        float y = strtof(av[++i], nullptr);
                        float z = strtof(av[++i], nullptr);
                        float a = (float)d2r(strtod(av[++i], nullptr));
                        float s = std::sqrt(x * x + y * y + z * z);
                        x /= s;
                        y /= s;
                        z /= s;
                        float c = std::cos(a);
                        s = std::sin(a);
                        float t = 1 - c;
                        m4[0][0] = t * x * x + c;
                        m4[1][1] = t * y * y + c;
                        m4[2][2] = t * z * z + c;
                        float A = t * x * y;
                        float B = s * z;
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
                        if ( !checkArgument(3, "f", ac, av, i) ) {
                            finish(counter, ret, transformMatrix, scaTransform);
                            return i;
                        }
                        tmp = strtod(av[i + 1], nullptr);
                        if ( tmp == 0.0 ) {
                            finish(counter, ret, transformMatrix, scaTransform);
                            return i;
                        }
                        m4[0][0] = tmp;
                        break;
                    case 'y':
                        if ( !checkArgument(3, "f", ac, av, i) ) {
                            finish(counter, ret, transformMatrix, scaTransform);
                            return i;
                        }
                        tmp = strtod(av[i + 1], nullptr);
                        if ( tmp == 0.0 ) {
                            finish(counter, ret, transformMatrix, scaTransform);
                            return i;
                        }
                        m4[1][1] = tmp;
                        break;
                    case 'z':
                        if ( !checkArgument(3, "f", ac, av, i) ) {
                            finish(counter, ret, transformMatrix, scaTransform);
                            return i;
                        }
                        tmp = strtod(av[i + 1], nullptr);
                        if ( tmp == 0.0 ) {
                            finish(counter, ret, transformMatrix, scaTransform);
                            return i;
                        }
                        m4[2][2] = tmp;
                        break;
                    default:
                        if ( !checkArgument(2, "f", ac, av, i) ) {
                            finish(counter, ret, transformMatrix, scaTransform);
                            return i;
                        }
                        tmp = strtod(av[i + 1], nullptr);
                        if ( tmp == 0.0 ) {
                            finish(counter, ret, transformMatrix, scaTransform);
                            return i;
                        }
                        scaTransform *=
                        m4[0][0] =
                        m4[1][1] =
                        m4[2][2] = tmp;
                        break;
                }
                i++;
                break;

            case 'm':
                // Mirror
                switch ( av[i][2] ) {
                    case 'x':
                        if ( !checkArgument(3, "", ac, av, i) ) {
                            finish(counter, ret, transformMatrix, scaTransform);
                            return i;
                        }
                        scaTransform *=
                        m4[0][0] = -1.0;
                        break;
                    case 'y':
                        if ( !checkArgument(3, "", ac, av, i) ) {
                            finish(counter, ret, transformMatrix, scaTransform);
                            return i;
                        }
                        scaTransform *=
                        m4[1][1] = -1.0;
                        break;
                    case 'z':
                        if ( !checkArgument(3, "", ac, av, i) ) {
                            finish(counter, ret, transformMatrix, scaTransform);
                            return i;
                        }
                        scaTransform *=
                        m4[2][2] = -1.0;
                        break;
                    default:
                        finish(counter, ret, transformMatrix, scaTransform);
                        return i;
                }
                break;

            case 'i':
                // Iterate
                if ( !checkArgument(2, "i", ac, av, i) ) {
                    finish(counter, ret, transformMatrix, scaTransform);
                    return i;
                }
                while ( counter-- > 0 ) {
                    multiplyMatrix4(ret->xfm, ret->xfm, transformMatrix);
                    ret->sca *= scaTransform;
                }
                counter = (int)strtol(av[++i], nullptr, 10);
                setIdent4(transformMatrix);
                scaTransform = 1.0;
                continue;

            default:
                finish(counter, ret, transformMatrix, scaTransform);
                return i;

        }
        multiplyMatrix4(transformMatrix, transformMatrix, m4);
    }

    finish(counter, ret, transformMatrix, scaTransform);
    return i;
}

/**
Handle xf entity
*/
int
handleTransformationEntity(int ac, char **av, MgfContext *context) {
    MgfTransformContext *spec;
    int n;
    int rv;

    if ( ac == 1 ) {
        // Something with existing transform
        spec = context->transformContext;
        if ( spec == nullptr ) {
            return MGF_ERROR_UNMATCHED_CONTEXT_CLOSE;
        }
        n = -1;
        if ( spec->transformationArray != nullptr) {
            // check for iteration
            MgfTransformArray *ap = spec->transformationArray;

            transformName(nullptr, context);
            n = ap->numberOfDimensions;
            while ( n-- ) {
                if ( ++ap->transformArguments[n].i < ap->transformArguments[n].n ) {
                    break;
                }
                strcpy(ap->transformArguments[n].arg, "0");
                ap->transformArguments[n].i = 0;
            }
            if ( n >= 0 ) {
                rv = mgfGoToFilePosition(&ap->startingPosition, context);
                if ( rv != MGF_OK ) {
                    return rv;
                }
                snprintf(ap->transformArguments[n].arg, 8, "%d", ap->transformArguments[n].i);
                transformName(ap, context);
            }
        }
        if ( n < 0 ) {
            // Pop transform
            context->transformContext = spec->prev;
            free_xf(spec);
            return MGF_OK;
        }
    } else {
        // Allocate transform
        spec = newTransform(ac - 1, av + 1, context);
        if ( spec == nullptr ) {
            return MGF_ERROR_OUT_OF_MEMORY;
        }
        if ( spec->transformationArray != nullptr) {
            transformName(spec->transformationArray, context);
        }
        spec->prev = context->transformContext; // Push onto stack
        context->transformContext = spec;
    }

    // Translate new specification
    n = TRANSFORM_ARGC(spec);
    n -= TRANSFORM_ARGC(spec->prev); // Incremental comp. is more eff.
    if ( xf(&spec->xf, n, TRANSFORM_ARGV(spec)) != n ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }

    // Check for vertex reversal
    if ( (spec->rev = (spec->xf.sca < 0.0)) ) {
        spec->xf.sca = -spec->xf.sca;
    }

    // Compute total transformation
    if ( spec->prev != nullptr) {
        multiplyMatrix4(spec->xf.xfm, spec->xf.xfm, spec->prev->xf.xfm);
        spec->xf.sca *= spec->prev->xf.sca;
        spec->rev = static_cast<short>(spec->rev ^ spec->prev->rev);
    }
    spec->xid = computeUniqueId(spec->xf.xfm); // Compute unique ID
    return MGF_OK;
}

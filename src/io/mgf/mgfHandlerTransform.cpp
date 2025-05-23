/**
Routines for 4x4 homogeneous, rigid-body transformations
*/

#include <cstring>

#include "java/lang/Math.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/mgf/badarg.h"
#include "io/mgf/MgfTransformContext.h"
#include "io/mgf/MgfContext.h"
#include "io/mgf/mgfDefinitions.h"

static char **globalTransformArgumentListBeginning;
static char **globalLastTransform; // End of transform argument list (last transform argument)

#define TRANSFORM_ARGC(xf) ( (xf) == nullptr ? 0 : (xf)->xac )
#define TRANSFORM_ARGV(xf) (globalLastTransform - (xf)->xac)

/**
Compute unique ID from matrix
*/
static long
computeUniqueId(MATRIX4Dd *xfm) {
    static char shiftTab[64] = {
        15, 5, 11, 5, 6, 3, 9, 15,
        13, 2, 13, 5, 2, 12, 14, 11,
        11, 12, 12, 3, 2, 11, 8, 12,
        1, 12, 5, 4, 15, 9, 14, 5,
        13, 14, 2, 10, 10, 14, 12, 3,
        5, 5, 14, 6, 12, 11, 13, 9,
        12, 8, 1, 6, 5, 12, 7, 13,
        15, 8, 9, 2, 6, 11, 9, 11
    };
    long xid = 0;

    // Compute unique transform id
    for ( long unsigned int i = 0; i < sizeof(MATRIX4Dd) / sizeof(unsigned short); i++ ) {
        xid ^= (long) (((unsigned short *) xfm->m)[i]) << shiftTab[i & 63];
    }
    return xid;
}

/**
Free a transform
*/
static void
free_xf(MgfTransformContext *spec) {
    if ( spec->transformationArray != nullptr ) {
        delete spec->transformationArray;
    }
    delete[] spec;
}

static double
d2r(double a) {
    return (M_PI / 180.0) * a;
}

static bool
checkArgument(int a, const char *l, int ac, char **av, int i) {
    if ( av[i][a] || checkForBadArguments(ac - i - 1, av + i + 1, l) ) {
        return false;
    }
    return true;
}

/**
Put out name for this instance
*/
static int
transformName(const MgfTransformArray *ap, MgfContext *context) {
    static char oName[10 * TRANSFORM_MAXIMUM_DIMENSIONS];
    static const char *oav[3] = {
        context->entityNames[MgfEntity::OBJECT], oName
    };
    char *cp1;

    if ( ap == nullptr ) {
        return mgfHandle(MgfEntity::OBJECT, 1, oav, context);
    }
    cp1 = oName;
    *cp1 = 'a';
    for ( int i = 0; i < ap->numberOfDimensions; i++ ) {
        for ( const char *cp2 = ap->transformArguments[i].arg; *cp2; ) {
            *++cp1 = *cp2++;
        }
        *++cp1 = '.';
    }
    *cp1 = '\0';
    return mgfHandle(MgfEntity::OBJECT, 2, oav, context);
}

/**
Allocate new transform structure
*/
static MgfTransformContext *
newTransform(int ac, const char **av, MgfContext *context) {
    char *cp;
    int nDim = 0;

    // Compute space required by arguments
    for ( int i = 0; i < ac; i++ ) {
        if ( !strcmp(av[i], "-a") ) {
            nDim++;
            i++;
        }
    }
    if ( nDim > TRANSFORM_MAXIMUM_DIMENSIONS ) {
        return nullptr;
    }

    MgfTransformContext *spec = new MgfTransformContext[2]; // TODO: Check why 2 works here but 1 does not
    if ( nDim != 0 ) {
        spec->transformationArray = new MgfTransformArray;
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
        char **newAv = new char *[spec->xac + 1];
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
            TRANSFORM_ARGV(spec)[i] = (char *)"-i";
            i++;
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
mgfTransformPoint(VECTOR3Dd *v1, const VECTOR3Dd *v2, const MgfContext *context) {
    if ( context->transformContext == nullptr) {
        v1->copy(v2);
        return;
    }
    context->transformContext->xf.transformMatrix.multiplyWithTranslation(v1, v2);
}

/**
Transform a vector using current matrix
*/
void
mgfTransformVector(VECTOR3Dd *v1, const VECTOR3Dd *v2, const MgfContext *context) {
    if ( context->transformContext == nullptr) {
        v1->copy(v2);
        return;
    }
    context->transformContext->xf.transformMatrix.multiply(v1, v2);
}

static void
finish(int count, MgfTransform *ret, const MATRIX4Dd *transformMatrix, double scaTransform) {
    while ( count-- > 0 ) {
        multiplyMatrix4(&ret->transformMatrix, &ret->transformMatrix, transformMatrix);
        ret->scaleFactor *= scaTransform;
    }
}

/**
Get transform specification
*/
static int
xf(MgfTransform *ret, int ac, char **av) {
    ret->transformMatrix.identity();
    ret->scaleFactor = 1.0;

    int counter = 1;
    MATRIX4Dd transformMatrix;
    transformMatrix.identity();
    double scaTransform = 1.0;

    int i;
    double tmp;
    for ( i = 0; i < ac && av[i][0] == '-'; i++ ) {
        MATRIX4Dd m4;

        switch ( av[i][1] ) {

            case 't':
                // Translate
                if ( !checkArgument(2, "fff", ac, av, i) ) {
                    finish(counter, ret, &transformMatrix, scaTransform);
                    return i;
                }
                m4.m[3][0] = strtod(av[++i], nullptr);
                m4.m[3][1] = strtod(av[++i], nullptr);
                m4.m[3][2] = strtod(av[++i], nullptr);
                break;

            case 'r':
                // Rotate
                switch ( av[i][2] ) {
                    case 'x':
                        if ( !checkArgument(3, "f", ac, av, i) ) {
                            finish(counter, ret, &transformMatrix, scaTransform);
                            return i;
                        }
                        tmp = d2r(strtod(av[++i], nullptr));
                        m4.m[1][1] = m4.m[2][2] = java::Math::cos(tmp);
                        m4.m[2][1] = -(m4.m[1][2] = java::Math::sin(tmp));
                        break;
                    case 'y':
                        if ( !checkArgument(3, "f", ac, av, i) ) {
                            finish(counter, ret, &transformMatrix, scaTransform);
                            return i;
                        }
                        tmp = d2r(strtod(av[++i], nullptr));
                        m4.m[0][0] = m4.m[2][2] = java::Math::cos(tmp);
                        m4.m[0][2] = -(m4.m[2][0] = java::Math::sin(tmp));
                        break;
                    case 'z':
                        if ( !checkArgument(3, "f", ac, av, i) ) {
                            finish(counter, ret, &transformMatrix, scaTransform);
                            return i;
                        }
                        tmp = d2r(strtod(av[++i], nullptr));
                        m4.m[0][0] = m4.m[1][1] = java::Math::cos(tmp);
                        m4.m[1][0] = -(m4.m[0][1] = java::Math::sin(tmp));
                        break;
                    default: {
                        if ( !checkArgument(2, "ffff", ac, av, i) ) {
                            finish(counter, ret, &transformMatrix, scaTransform);
                            return i;
                        }
                        float x = strtof(av[++i], nullptr);
                        float y = strtof(av[++i], nullptr);
                        float z = strtof(av[++i], nullptr);
                        float a = (float)d2r(strtod(av[++i], nullptr));
                        float s = java::Math::sqrt(x * x + y * y + z * z);
                        x /= s;
                        y /= s;
                        z /= s;
                        float c = java::Math::cos(a);
                        s = java::Math::sin(a);
                        float t = 1 - c;
                        m4.m[0][0] = t * x * x + c;
                        m4.m[1][1] = t * y * y + c;
                        m4.m[2][2] = t * z * z + c;
                        float A = t * x * y;
                        float B = s * z;
                        m4.m[0][1] = A + B;
                        m4.m[1][0] = A - B;
                        A = t * x * z;
                        B = s * y;
                        m4.m[0][2] = A - B;
                        m4.m[2][0] = A + B;
                        A = t * y * z;
                        B = s * x;
                        m4.m[1][2] = A + B;
                        m4.m[2][1] = A - B;
                    }
                }
                break;

            case 's':
                // Scale
                switch ( av[i][2] ) {
                    case 'x':
                        if ( !checkArgument(3, "f", ac, av, i) ) {
                            finish(counter, ret, &transformMatrix, scaTransform);
                            return i;
                        }
                        tmp = strtod(av[i + 1], nullptr);
                        if ( tmp == 0.0 ) {
                            finish(counter, ret, &transformMatrix, scaTransform);
                            return i;
                        }
                        m4.m[0][0] = tmp;
                        break;
                    case 'y':
                        if ( !checkArgument(3, "f", ac, av, i) ) {
                            finish(counter, ret, &transformMatrix, scaTransform);
                            return i;
                        }
                        tmp = strtod(av[i + 1], nullptr);
                        if ( tmp == 0.0 ) {
                            finish(counter, ret, &transformMatrix, scaTransform);
                            return i;
                        }
                        m4.m[1][1] = tmp;
                        break;
                    case 'z':
                        if ( !checkArgument(3, "f", ac, av, i) ) {
                            finish(counter, ret, &transformMatrix, scaTransform);
                            return i;
                        }
                        tmp = strtod(av[i + 1], nullptr);
                        if ( tmp == 0.0 ) {
                            finish(counter, ret, &transformMatrix, scaTransform);
                            return i;
                        }
                        m4.m[2][2] = tmp;
                        break;
                    default:
                        if ( !checkArgument(2, "f", ac, av, i) ) {
                            finish(counter, ret, &transformMatrix, scaTransform);
                            return i;
                        }
                        tmp = strtod(av[i + 1], nullptr);
                        if ( tmp == 0.0 ) {
                            finish(counter, ret, &transformMatrix, scaTransform);
                            return i;
                        }
                        scaTransform *=
                        m4.m[0][0] =
                        m4.m[1][1] =
                        m4.m[2][2] = tmp;
                        break;
                }
                i++;
                break;

            case 'm':
                // Mirror
                switch ( av[i][2] ) {
                    case 'x':
                        if ( !checkArgument(3, "", ac, av, i) ) {
                            finish(counter, ret, &transformMatrix, scaTransform);
                            return i;
                        }
                        scaTransform *=
                        m4.m[0][0] = -1.0;
                        break;
                    case 'y':
                        if ( !checkArgument(3, "", ac, av, i) ) {
                            finish(counter, ret, &transformMatrix, scaTransform);
                            return i;
                        }
                        scaTransform *=
                        m4.m[1][1] = -1.0;
                        break;
                    case 'z':
                        if ( !checkArgument(3, "", ac, av, i) ) {
                            finish(counter, ret, &transformMatrix, scaTransform);
                            return i;
                        }
                        scaTransform *=
                        m4.m[2][2] = -1.0;
                        break;
                    default:
                        finish(counter, ret, &transformMatrix, scaTransform);
                        return i;
                }
                break;

            case 'i':
                // Iterate
                if ( !checkArgument(2, "i", ac, av, i) ) {
                    finish(counter, ret, &transformMatrix, scaTransform);
                    return i;
                }
                while ( counter-- > 0 ) {
                    multiplyMatrix4(&ret->transformMatrix, &ret->transformMatrix, &transformMatrix);
                    ret->scaleFactor *= scaTransform;
                }
                counter = (int)strtol(av[++i], nullptr, 10);
                transformMatrix.identity();
                scaTransform = 1.0;
                continue;

            default:
                finish(counter, ret, &transformMatrix, scaTransform);
                return i;

        }
        multiplyMatrix4(&transformMatrix, &transformMatrix, &m4);
    }

    finish(counter, ret, &transformMatrix, scaTransform);
    return i;
}

/**
Handle xf entity
*/
int
handleTransformationEntity(int ac, const char **av, MgfContext *context) {
    MgfTransformContext *spec;
    int n;
    int rv;

    if ( ac == 1 ) {
        // Something with existing transform
        spec = context->transformContext;
        if ( spec == nullptr ) {
            return MgfErrorCode::MGF_ERROR_UNMATCHED_CONTEXT_CLOSE;
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
                if ( rv != MgfErrorCode::MGF_OK ) {
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
            return MgfErrorCode::MGF_OK;
        }
    } else {
        // Allocate transform
        spec = newTransform(ac - 1, av + 1, context);
        if ( spec == nullptr ) {
            return MgfErrorCode::MGF_ERROR_OUT_OF_MEMORY;
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
        return MgfErrorCode::MGF_ERROR_ARGUMENT_TYPE;
    }

    // Check for vertex reversal
    if ( (spec->rev = (spec->xf.scaleFactor < 0.0)) ) {
        spec->xf.scaleFactor = -spec->xf.scaleFactor;
    }

    // Compute total transformation
    if ( spec->prev != nullptr) {
        multiplyMatrix4(&spec->xf.transformMatrix, &spec->xf.transformMatrix, &spec->prev->xf.transformMatrix);
        spec->xf.scaleFactor *= spec->prev->xf.scaleFactor;
        spec->rev = static_cast<short>(spec->rev ^ spec->prev->rev);
    }
    spec->xid = computeUniqueId(&spec->xf.transformMatrix); // Compute unique ID
    return MgfErrorCode::MGF_OK;
}

void
mgfTransformFreeMemory() {
    delete[] globalTransformArgumentListBeginning;
}

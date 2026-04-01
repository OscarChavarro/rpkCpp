#include <cstdlib>
#include <cstring>

#include "java/util/Formatter.h"
#include "io/context/WordsContext.h"
#include "io/mgf/MgfDefinitions.h"
#include "io/mgf/MgfGeometry.h"
#include "io/mgf/MgfHandlerGeometry.h"

static constexpr int MGF_PV_SIZE = 24;
static constexpr char globalFloatFormat[] = "%.12g";

/**
Expand a sphere into cones
*/
int
MgfGeometry::mgfEntitySphere(int ac, const char **av, ParseSession *context) {
    char p2x[24];
    char p2y[24];
    char p2z[24];
    char r1[24];
    char r2[24];
    const char *v1Entity[5] = {
        context->entityNames[EntityContext::VERTEX],
        "_sv1",
        "=",
        "_sv2"
    };
    const char *v2Entity[4] = {
        context->entityNames[EntityContext::VERTEX],
        "_sv2",
        "="
    };
    const char *p2Entity[5] = {context->entityNames[EntityContext::MGF_POINT], p2x, p2y, p2z};
    const char *coneEntity[6] = {
        context->entityNames[EntityContext::CONE],
        "_sv1",
        r1,
        "_sv2",
        r2
    };

    if ( ac != 3 ) {
        return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    const VertexContext *cv = MgfHandlerGeometry::getNamedVertex(av[1], context);
    if ( cv == nullptr) {
        return ErrorCodeContext::MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( !WordsContext::isFloat(av[2]) ) {
        return ErrorCodeContext::MGF_ERROR_ARGUMENT_TYPE;
    }
    double rad = strtod(av[2], nullptr);

    // Initialize
    context->warpConeEnds = true;
    int rVal = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 3, v2Entity, context);
    if ( rVal != ErrorCodeContext::MGF_OK ) {
        return rVal;
    }
    java::Formatter::format(p2x, 24, globalFloatFormat, cv->p.x);
    java::Formatter::format(p2y, 24, globalFloatFormat, cv->p.y);
    java::Formatter::format(p2z, 24, globalFloatFormat, cv->p.z + rad);
    rVal = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p2Entity, context);
    if ( rVal != ErrorCodeContext::MGF_OK ) {
        return rVal;
    }
    r2[0] = '0';
    r2[1] = '\0';
    for ( int i = 1; i <= 2 * context->numberOfQuarterCircleDivisions; i++ ) {
        double theta = i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
        rVal = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v1Entity, context);
        if ( rVal != ErrorCodeContext::MGF_OK ) {
            return rVal;
        }
        java::Formatter::format(p2z, 24, globalFloatFormat, cv->p.z + rad * java::Math::cos(theta));
        rVal = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 2, v2Entity, context);
        if ( rVal != ErrorCodeContext::MGF_OK ) {
            return rVal;
        }
        rVal = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p2Entity, context);
        if ( rVal != ErrorCodeContext::MGF_OK ) {
            return rVal;
        }
        strcpy(r1, r2);
        java::Formatter::format(r2, 24, globalFloatFormat, rad * java::Math::sin(theta));
        rVal = MgfDefinitions::mgfHandle(EntityContext::CONE, 5, coneEntity, context);
        if ( rVal != ErrorCodeContext::MGF_OK ) {
            return rVal;
        }
    }
    context->warpConeEnds = false;
    return ErrorCodeContext::MGF_OK;
}

/**
Expand a torus into cones
*/
int
MgfGeometry::mgfEntityTorus(int ac, const char **av, ParseSession *context) {
    char p2[3][24];
    char r1[24];
    char r2[24];
    const char *v1Entity[5] = {
        context->entityNames[EntityContext::VERTEX],
        "_tv1",
        "=",
        "_tv2"
    };
    const char *v2Entity[5] = {
        context->entityNames[EntityContext::VERTEX],
        "_tv2",
        "="
    };
    const char *p2Entity[5] = {
        context->entityNames[EntityContext::MGF_POINT],
        p2[0],
        p2[1],
        p2[2]
    };
    const char *coneEntity[6] = {
        context->entityNames[EntityContext::CONE],
        "_tv1",
        r1,
        "_tv2",
        r2
    };
    const VertexContext *cv;
    double avgRad;
    double theta;

    if ( ac != 4 ) {
        return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if ( (cv = MgfHandlerGeometry::getNamedVertex(av[1], context)) == nullptr ) {
        return ErrorCodeContext::MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( cv->n.isNull(Numeric::EPSILON) ) {
        return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( !WordsContext::isFloat(av[2]) || !WordsContext::isFloat(av[3]) ) {
        return ErrorCodeContext::MGF_ERROR_ARGUMENT_TYPE;
    }
    double minRad = strtod(av[2], nullptr);
    Numeric::roundDeltaToZero(minRad, Numeric::EPSILON);
    double maxRad = strtod(av[3], nullptr);

    // Check orientation
    int sign;
    if ( minRad > 0.0 ) {
        sign = 1;
    } else if ( minRad < 0.0 ) {
        sign = -1;
    } else {
        return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( sign * (maxRad - minRad) <= 0.0 ) {
        return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Initialize
    context->warpConeEnds = true;
    v2Entity[3] = av[1];
    java::Formatter::format(p2[0], 24, globalFloatFormat, cv->p.x + 0.5 * sign * (maxRad - minRad) * cv->n.x);
    java::Formatter::format(p2[1], 24, globalFloatFormat, cv->p.y + 0.5 * sign * (maxRad - minRad) * cv->n.y);
    java::Formatter::format(p2[2], 24, globalFloatFormat, cv->p.z + 0.5 * sign * (maxRad - minRad) * cv->n.z);
    int rVal = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v2Entity, context);
    if ( rVal != ErrorCodeContext::MGF_OK ) {
        return rVal;
    }
    rVal = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p2Entity, context);
    if ( rVal != ErrorCodeContext::MGF_OK ) {
        return rVal;
    }
    java::Formatter::format(r2, 24, globalFloatFormat, avgRad = 0.5 * (minRad + maxRad));

    // Run outer section
    int i;
    for ( i = 1; i <= 2 * context->numberOfQuarterCircleDivisions; i++ ) {
        theta = i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
        rVal = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v1Entity, context);
        if ( rVal != ErrorCodeContext::MGF_OK ) {
            return rVal;
        }
        java::Formatter::format(p2[0], 24, globalFloatFormat, cv->p.x + 0.5 * sign * (maxRad - minRad) * java::Math::cos(theta) * cv->n.x);
        java::Formatter::format(p2[1], 24, globalFloatFormat, cv->p.y + 0.5 * sign * (maxRad - minRad) * java::Math::cos(theta) * cv->n.y);
        java::Formatter::format(p2[2], 24, globalFloatFormat, cv->p.z + 0.5 * sign * (maxRad - minRad) * java::Math::cos(theta) * cv->n.z);
        rVal = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 2, v2Entity, context);
        if ( rVal != ErrorCodeContext::MGF_OK ) {
            return rVal;
        }
        rVal = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p2Entity, context);
        if ( rVal != ErrorCodeContext::MGF_OK ) {
            return rVal;
        }
        strcpy(r1, r2);
        java::Formatter::format(r2, 24, globalFloatFormat, avgRad + 0.5 * (maxRad - minRad) * java::Math::sin(theta));
        rVal = MgfDefinitions::mgfHandle(EntityContext::CONE, 5, coneEntity, context);
        if ( rVal != ErrorCodeContext::MGF_OK ) {
            return rVal;
        }
    }

    // Run inner section
    java::Formatter::format(r2, 24, globalFloatFormat, -0.5 * (minRad + maxRad));
    for ( ; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
        theta = i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
        java::Formatter::format(p2[0], 24, globalFloatFormat, cv->p.x + 0.5 * sign * (maxRad - minRad) * java::Math::cos(theta) * cv->n.x);
        java::Formatter::format(p2[1], 24, globalFloatFormat, cv->p.y + 0.5 * sign * (maxRad - minRad) * java::Math::cos(theta) * cv->n.y);
        java::Formatter::format(p2[2], 24, globalFloatFormat, cv->p.z + 0.5 * sign * (maxRad - minRad) * java::Math::cos(theta) * cv->n.z);
        rVal = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v1Entity, context);
        if ( rVal != ErrorCodeContext::MGF_OK ) {
            return rVal;
        }
        rVal = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 2, v2Entity, context);
        if ( rVal != ErrorCodeContext::MGF_OK ) {
            return rVal;
        }
        rVal = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p2Entity, context);
        if ( rVal != ErrorCodeContext::MGF_OK ) {
            return rVal;
        }
        strcpy(r1, r2);
        java::Formatter::format(r2, 24, globalFloatFormat, -avgRad - .5 * (maxRad - minRad) * java::Math::sin(theta));
        rVal = MgfDefinitions::mgfHandle(EntityContext::CONE, 5, coneEntity, context);
        if ( rVal != ErrorCodeContext::MGF_OK ) {
            return rVal;
        }
    }
    context->warpConeEnds = false;
    return ErrorCodeContext::MGF_OK;
}

/**
Replace a cylinder with equivalent cone
*/
int
MgfGeometry::mgfEntityCylinder(int ac, const char **av, ParseSession *context) {
    const char *newArgV[6] = {context->entityNames[EntityContext::CONE]};

    if ( ac != 4 ) {
        return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    newArgV[1] = av[1];
    newArgV[2] = av[2];
    newArgV[3] = av[3];
    newArgV[4] = av[2];
    return MgfDefinitions::mgfHandle(EntityContext::CONE, 5, newArgV, context);
}

/**
Compute u and v given w (normalized)
*/
void
MgfGeometry::mgfMakeAxes(Vector3Dd *u, Vector3Dd *v, const Vector3Dd *w, double epsilon)
{
    v->x = 0.0;
    v->y = 0.0;
    v->z = 0.0;
    double vArr[3] = {v->x, v->y, v->z};
    double wArr[3] = {w->x, w->y, w->z};

    int i;
    for ( i = 0; i < 3; i++ ) {
        if ( wArr[i] > -0.6 && wArr[i] < 0.6 ) {
            break;
        }
    }

    if ( i < 3 ) {
        vArr[i] = 1.0;
    }

    v->x = vArr[0];
    v->y = vArr[1];
    v->z = vArr[2];

    u->crossProduct(v, w);
    u->normalizeAndGivePreviousNorm(epsilon);
    v->crossProduct(w, u);
}

/**
Turn a ring into polygons
*/
int
MgfGeometry::mgfEntityRing(int ac, const char **av, ParseSession *context) {
    char p3[3][24];
    char p4[3][24];
    const char *namesEntity[5] = {
        context->entityNames[EntityContext::MGF_NORMAL],
        "0",
        "0",
        "0"
    };
    const char *v1Entity[5] = {
        context->entityNames[EntityContext::VERTEX],
        "_rv1",
        "="
    };
    const char *v2Entity[5] = {
        context->entityNames[EntityContext::VERTEX],
        "_rv2",
        "=",
        "_rv3"
    };
    const char *v3Entity[4] = {
        context->entityNames[EntityContext::VERTEX],
        "_rv3",
        "="
    };
    const char *p3Entity[5] = {
        context->entityNames[EntityContext::MGF_POINT],
        p3[0],
        p3[1],
        p3[2]
    };
    const char *v4Entity[4] = {
        context->entityNames[EntityContext::VERTEX],
        "_rv4",
        "="
    };
    const char *p4Entity[5] = {
        context->entityNames[EntityContext::MGF_POINT],
        p4[0],
        p4[1],
        p4[2]
    };
    const char *faceEntity[6] = {
        context->entityNames[EntityContext::FACE],
        "_rv1",
        "_rv2",
        "_rv3",
        "_rv4"
    };
    double theta;

    if ( ac != 4 ) {
        return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }

    const VertexContext *vertexContext = MgfHandlerGeometry::getNamedVertex(av[1], context);
    if ( vertexContext == nullptr) {
        return ErrorCodeContext::MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( vertexContext->n.isNull(Numeric::EPSILON) ) {
        return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( !WordsContext::isFloat(av[2]) || !WordsContext::isFloat(av[3]) ) {
        return ErrorCodeContext::MGF_ERROR_ARGUMENT_TYPE;
    }
    double minRad = strtod(av[2], nullptr);
    Numeric::roundDeltaToZero(minRad, Numeric::EPSILON);
    double maxRad = strtod(av[3], nullptr);
    if ( minRad < 0.0 || maxRad <= minRad ) {
        return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Initialize
    Vector3Dd u;
    Vector3Dd v;

    mgfMakeAxes(&u, &v, &vertexContext->n, Numeric::EPSILON);
    java::Formatter::format(p3[0], 24, globalFloatFormat, vertexContext->p.x + maxRad * u.x);
    java::Formatter::format(p3[1], 24, globalFloatFormat, vertexContext->p.y + maxRad * u.y);
    java::Formatter::format(p3[2], 24, globalFloatFormat, vertexContext->p.z + maxRad * u.z);
    int rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 3, v3Entity, context);
    if ( rv != ErrorCodeContext::MGF_OK ) {
        return rv;
    }
    rv = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p3Entity, context);
    if ( rv != ErrorCodeContext::MGF_OK ) {
        return rv;
    }

    if ( Numeric::doubleEqual(minRad, 0.0, Numeric::EPSILON) ) {
        // Closed
        v1Entity[3] = av[1];
        rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v1Entity, context);
        if ( rv != ErrorCodeContext::MGF_OK ) {
            return rv;
        }
        rv = MgfDefinitions::mgfHandle(EntityContext::MGF_NORMAL, 4, namesEntity, context);
        if ( rv != ErrorCodeContext::MGF_OK ) {
            return rv;
        }
        for ( int i = 1; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
            theta = i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
            rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v2Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }

            java::Formatter::format(
                p3[0], 24, globalFloatFormat,
                vertexContext->p.x + maxRad * u.x * java::Math::cos(theta) + maxRad * v.x * java::Math::sin(theta));
            java::Formatter::format(
                p3[1], 24, globalFloatFormat,
                vertexContext->p.y + maxRad * u.y * java::Math::cos(theta) + maxRad * v.y * java::Math::sin(theta));
            java::Formatter::format(
                p3[2], 24, globalFloatFormat,
                vertexContext->p.z + maxRad * u.z * java::Math::cos(theta) + maxRad * v.z * java::Math::sin(theta));

            rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 2, v3Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p3Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::FACE, 4, faceEntity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
        }
    } else {
        // Open
        rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 3, v4Entity, context);
        if ( rv != ErrorCodeContext::MGF_OK ) {
            return rv;
        }

        java::Formatter::format(p4[0], 24, globalFloatFormat, vertexContext->p.x + minRad * u.x);
        java::Formatter::format(p4[1], 24, globalFloatFormat, vertexContext->p.y + minRad * u.y);
        java::Formatter::format(p4[2], 24, globalFloatFormat, vertexContext->p.z + minRad * u.z);

        rv = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p4Entity, context);
        if ( rv != ErrorCodeContext::MGF_OK ) {
            return rv;
        }
        v1Entity[3] = "_rv4";
        for ( int i = 1; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
            theta = i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
            rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v1Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v2Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }

            double d = u.x * java::Math::cos(theta) + v.x * java::Math::sin(theta);
            java::Formatter::format(p3[0], 24, globalFloatFormat, vertexContext->p.x + maxRad * d);
            java::Formatter::format(p4[0], 24, globalFloatFormat, vertexContext->p.x + minRad * d);

            d = u.y * java::Math::cos(theta) + v.y * java::Math::sin(theta);
            java::Formatter::format(p3[1], 24, globalFloatFormat, vertexContext->p.y + maxRad * d);
            java::Formatter::format(p4[1], 24, globalFloatFormat, vertexContext->p.y + minRad * d);

            d = u.z * java::Math::cos(theta) + v.z * java::Math::sin(theta);
            java::Formatter::format(p3[2], 24, globalFloatFormat, vertexContext->p.z + maxRad * d);
            java::Formatter::format(p4[2], 24, globalFloatFormat, vertexContext->p.z + minRad * d);

            rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 2, v3Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p3Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 2, v4Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p4Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::FACE, 5, faceEntity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
        }
    }
    return ErrorCodeContext::MGF_OK;
}

/**
Turn a cone into polygons
*/
int
MgfGeometry::mgfEntityCone(int ac, const char **av, ParseSession *context) {
    char p3[3][24];
    char p4[3][24];
    char n3[3][24];
    char n4[3][24];
    const char *v1Entity[5] = {
        context->entityNames[EntityContext::VERTEX],
        "_cv1",
        "="
    };
    const char *v2Entity[5] = {
        context->entityNames[EntityContext::VERTEX],
        "_cv2",
        "=",
        "_cv3"
    };
    const char *v3Entity[4] = {
        context->entityNames[EntityContext::VERTEX],
        "_cv3",
        "="
    };
    const char *p3Entity[5] = {
        context->entityNames[EntityContext::MGF_POINT], p3[0], p3[1], p3[2]};
    const char *n3Entity[5] = {
        context->entityNames[EntityContext::MGF_NORMAL], n3[0], n3[1], n3[2]};
    const char *v4Entity[4] = {
        context->entityNames[EntityContext::VERTEX],
        "_cv4",
        "="};
    const char *p4Entity[5] = {
        context->entityNames[EntityContext::MGF_POINT],
        p4[0],
        p4[1],
        p4[2]
    };
    const char *n4Entity[5] = {
        context->entityNames[EntityContext::MGF_NORMAL],
        n4[0],
        n4[1],
        n4[2]
    };
    const char *faceEntity[6] = {
        context->entityNames[EntityContext::FACE],
        "_cv1",
        "_cv2",
        "_cv3",
        "_cv4"
    };
    const char *v1n;
    VertexContext *cv1;
    VertexContext *cv2;
    double n1off;
    double n2off;
    double d;
    int rv;
    double theta;

    if ( ac != 5 ) {
        return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    cv1 = MgfHandlerGeometry::getNamedVertex(av[1], context);
    cv2 = MgfHandlerGeometry::getNamedVertex(av[3], context);
    if ( cv1 == nullptr || cv2 == nullptr) {
        return ErrorCodeContext::MGF_ERROR_UNDEFINED_REFERENCE;
    }
    v1n = av[1];
    if ( !WordsContext::isFloat(av[2]) || !WordsContext::isFloat(av[4]) ) {
        return ErrorCodeContext::MGF_ERROR_ARGUMENT_TYPE;
    }

    // Set up (radius1, radius2)
    double radius1 = strtod(av[2], nullptr);
    Numeric::roundDeltaToZero(radius1, Numeric::EPSILON);
    double radius2 = strtod(av[4], nullptr);
    Numeric::roundDeltaToZero(radius2, Numeric::EPSILON);

    if ( radius1 == 0.0 ) {
        if ( radius2 == 0.0 ) {
            return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
    } else if ( radius2 != 0.0 ) {
        bool a = radius1 < 0.0;
        bool b = radius2 < 0.0;
        bool check = (a && !b) || (!a && b); // Note: this is exclusive or / XOR a ^ b
        if ( check ) {
            return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
    } else {
        // Swap
        VertexContext *cv;

        cv = cv1;
        cv1 = cv2;
        cv2 = cv;
        v1n = av[3];
        d = radius1;
        radius1 = radius2;
        radius2 = d;
    }
    int sign = radius2 < 0.0 ? -1 : 1;

    // Initialize
    Vector3Dd w;

    w.x = cv1->p.x - cv2->p.x;
    w.y = cv1->p.y - cv2->p.y;
    w.z = cv1->p.z - cv2->p.z;

    d = w.normalizeAndGivePreviousNorm(Numeric::EPSILON);
    if ( Numeric::doubleEqual(d, 0.0, Numeric::EPSILON) ) {
        return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    n1off = n2off = (radius2 - radius1) / d;
    if ( context->warpConeEnds ) {
        // Hack for mgfEntitySphere and mgfEntityTorus
        d = java::Math::atan(n2off) - (M_PI / 4) / context->numberOfQuarterCircleDivisions;
        if ( d <= -M_PI / 2 + Numeric::EPSILON ) {
            n2off = -Numeric::HUGE_FLOAT_VALUE;
        } else {
            n2off = java::Math::tan(d);
        }
    }

    Vector3Dd u;
    Vector3Dd v;
    mgfMakeAxes(&u, &v, &w, Numeric::EPSILON);

    java::Formatter::format(p3[0], 24, globalFloatFormat, cv2->p.x + radius2 * u.x);
    if ( n2off <= -Numeric::HUGE_FLOAT_VALUE) {
        java::Formatter::format(n3[0], 24, globalFloatFormat, -w.x);
    } else {
        java::Formatter::format(n3[0], 24, globalFloatFormat, u.x + w.x * n2off);
    }

    java::Formatter::format(p3[1], 24, globalFloatFormat, cv2->p.y + radius2 * u.y);
    if ( n2off <= -Numeric::HUGE_FLOAT_VALUE) {
        java::Formatter::format(n3[1], 24, globalFloatFormat, -w.y);
    } else {
        java::Formatter::format(n3[1], 24, globalFloatFormat, u.y + w.y * n2off);
    }

    java::Formatter::format(p3[2], 24, globalFloatFormat, cv2->p.z + radius2 * u.z);
    if ( n2off <= -Numeric::HUGE_FLOAT_VALUE) {
        java::Formatter::format(n3[2], 24, globalFloatFormat, -w.z);
    } else {
        java::Formatter::format(n3[2], 24, globalFloatFormat, u.z + w.z * n2off);
    }

    rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 3, v3Entity, context);
    if ( rv != ErrorCodeContext::MGF_OK ) {
        return rv;
    }
    rv = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p3Entity, context);
    if ( rv != ErrorCodeContext::MGF_OK ) {
        return rv;
    }
    rv = MgfDefinitions::mgfHandle(EntityContext::MGF_NORMAL, 4, n3Entity, context);
    if ( rv != ErrorCodeContext::MGF_OK ) {
        return rv;
    }
    if ( radius1 == 0.0 ) {
        // TODO: Review floating point comparisons vs EPSILON
        // Triangles
        v1Entity[3] = v1n;
        rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v1Entity, context);
        if ( rv != ErrorCodeContext::MGF_OK ) {
            return rv;
        }

        java::Formatter::format(n4[0], 24, globalFloatFormat, w.x);
        java::Formatter::format(n4[1], 24, globalFloatFormat, w.y);
        java::Formatter::format(n4[2], 24, globalFloatFormat, w.z);

        rv = MgfDefinitions::mgfHandle(EntityContext::MGF_NORMAL, 4, n4Entity, context);
        if ( rv != ErrorCodeContext::MGF_OK ) {
            return rv;
        }
        for ( int i = 1; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
            theta = sign * i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
            rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v2Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }

            d = u.x * java::Math::cos(theta) + v.x * java::Math::sin(theta);
            java::Formatter::format(p3[0], 24, globalFloatFormat, cv2->p.x + radius2 * d);
            if ( n2off > -Numeric::HUGE_FLOAT_VALUE) {
                java::Formatter::format(n3[0], 24, globalFloatFormat, d + w.x * n2off);
            }

            d = u.y * java::Math::cos(theta) + v.y * java::Math::sin(theta);
            java::Formatter::format(p3[1], 24, globalFloatFormat, cv2->p.y + radius2 * d);
            if ( n2off > -Numeric::HUGE_FLOAT_VALUE) {
                java::Formatter::format(n3[1], 24, globalFloatFormat, d + w.y * n2off);
            }

            d = u.z * java::Math::cos(theta) + v.z * java::Math::sin(theta);
            java::Formatter::format(p3[2], 24, globalFloatFormat, cv2->p.z + radius2 * d);
            if ( n2off > -Numeric::HUGE_FLOAT_VALUE) {
                java::Formatter::format(n3[2], 24, globalFloatFormat, d + w.z * n2off);
            }

            rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 2, v3Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p3Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::MGF_NORMAL, 4, n3Entity, context);
            if ( n2off > -Numeric::HUGE_FLOAT_VALUE && rv != MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::FACE, 4, faceEntity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
        }
    } else {
        // Quads
        v1Entity[3] = "_cv4";
        if ( context->warpConeEnds ) {
            // Hack for mgfEntitySphere and mgfEntityTorus
            d = java::Math::atan(n1off) + (M_PI / 4) / context->numberOfQuarterCircleDivisions;
            if ( d >= M_PI / 2 - Numeric::EPSILON ) {
                n1off = Numeric::HUGE_FLOAT_VALUE;
            } else {
                n1off = java::Math::tan(java::Math::atan(n1off) + (M_PI / 4) / context->numberOfQuarterCircleDivisions);
            }
        }

        java::Formatter::format(p4[0], 24, globalFloatFormat, cv1->p.x + radius1 * u.x);
        if ( n1off >= Numeric::HUGE_FLOAT_VALUE) {
            java::Formatter::format(n4[0], 24, globalFloatFormat, w.x);
        } else {
            java::Formatter::format(n4[0], 24, globalFloatFormat, u.x + w.x * n1off);
        }

        java::Formatter::format(p4[1], 24, globalFloatFormat, cv1->p.y + radius1 * u.y);
        if ( n1off >= Numeric::HUGE_FLOAT_VALUE) {
            java::Formatter::format(n4[1], 24, globalFloatFormat, w.y);
        } else {
            java::Formatter::format(n4[1], 24, globalFloatFormat, u.y + w.y * n1off);
        }

        java::Formatter::format(p4[2], 24, globalFloatFormat, cv1->p.z + radius1 * u.z);
        if ( n1off >= Numeric::HUGE_FLOAT_VALUE) {
            java::Formatter::format(n4[2], 24, globalFloatFormat, w.z);
        } else {
            java::Formatter::format(n4[2], 24, globalFloatFormat, u.z + w.z * n1off);
        }

        rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 3, v4Entity, context);
        if ( rv != ErrorCodeContext::MGF_OK ) {
            return rv;
        }
        rv = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p4Entity, context);
        if ( rv != ErrorCodeContext::MGF_OK ) {
            return rv;
        }
        rv = MgfDefinitions::mgfHandle(EntityContext::MGF_NORMAL, 4, n4Entity, context);
        if ( rv != ErrorCodeContext::MGF_OK ) {
            return rv;
        }
        for ( int i = 1; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
            theta = sign * i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
            rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v1Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, v2Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }

            d = u.x * java::Math::cos(theta) + v.x * java::Math::sin(theta);
            java::Formatter::format(p3[0], 24, globalFloatFormat, cv2->p.x + radius2 * d);
            if ( n2off > -Numeric::HUGE_FLOAT_VALUE) {
                java::Formatter::format(n3[0], 24, globalFloatFormat, d + w.x * n2off);
            }
            java::Formatter::format(p4[0], 24, globalFloatFormat, cv1->p.x + radius1 * d);
            if ( n1off < Numeric::HUGE_FLOAT_VALUE) {
                java::Formatter::format(n4[0], 24, globalFloatFormat, d + w.x * n1off);
            }

            d = u.y * java::Math::cos(theta) + v.y * java::Math::sin(theta);
            java::Formatter::format(p3[1], 24, globalFloatFormat, cv2->p.y + radius2 * d);
            if ( n2off > -Numeric::HUGE_FLOAT_VALUE) {
                java::Formatter::format(n3[1], 24, globalFloatFormat, d + w.y * n2off);
            }
            java::Formatter::format(p4[1], 24, globalFloatFormat, cv1->p.y + radius1 * d);
            if ( n1off < Numeric::HUGE_FLOAT_VALUE) {
                java::Formatter::format(n4[1], 24, globalFloatFormat, d + w.y * n1off);
            }

            d = u.z * java::Math::cos(theta) + v.z * java::Math::sin(theta);
            java::Formatter::format(p3[2], 24, globalFloatFormat, cv2->p.z + radius2 * d);
            if ( n2off > -Numeric::HUGE_FLOAT_VALUE) {
                java::Formatter::format(n3[2], 24, globalFloatFormat, d + w.z * n2off);
            }
            java::Formatter::format(p4[2], 24, globalFloatFormat, cv1->p.z + radius1 * d);
            if ( n1off < Numeric::HUGE_FLOAT_VALUE) {
                java::Formatter::format(n4[2], 24, globalFloatFormat, d + w.z * n1off);
            }

            rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 2, v3Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p3Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::MGF_NORMAL, 4, n3Entity, context);
            if ( n2off > -Numeric::HUGE_FLOAT_VALUE && rv != MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 2, v4Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, p4Entity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::MGF_NORMAL, 4, n4Entity, context);
            if ( n1off < Numeric::HUGE_FLOAT_VALUE &&
                 rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::FACE, 5, faceEntity, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
        }
    }
    return ErrorCodeContext::MGF_OK;
}

/**
Turn a prism into polygons
*/
int
MgfGeometry::mgfEntityPrism(int ac, const char **av, ParseSession *context) {
    char p[3][24];
    const char *vent[5] = {
        context->entityNames[EntityContext::VERTEX],
        nullptr,
        "="
    };
    const char *pent[5] = {
        context->entityNames[EntityContext::MGF_POINT],
        p[0],
        p[1],
        p[2]
    };
    const char *zNormal[5] = {
        context->entityNames[EntityContext::MGF_NORMAL],
        "0",
        "0",
        "0"
    };
    const char *newArgV[MGF_MAXIMUM_ARGUMENT_COUNT];
    char nvn[MGF_MAXIMUM_ARGUMENT_COUNT - 1][MGF_PV_SIZE];
    const VertexContext *cv;
    int rv;
    int i;

    // Check arguments
    if ( ac < 5 ) {
        return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if ( !WordsContext::isFloat(av[ac - 1]) ) {
        return ErrorCodeContext::MGF_ERROR_ARGUMENT_TYPE;
    }
    double length = strtod(av[ac - 1], nullptr);
    if ( length <= Numeric::EPSILON && length >= -Numeric::EPSILON ) {
        return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Compute face normal
    const VertexContext *cv0 = MgfHandlerGeometry::getNamedVertex(av[1], context);
    if ( cv0 == nullptr ) {
        return ErrorCodeContext::MGF_ERROR_UNDEFINED_REFERENCE;
    }
    int hasNormal = 0;

    Vector3Dd norm(0.0, 0.0, 0.0);
    Vector3Dd v1(0.0, 0.0, 0.0);

    for ( i = 2; i < ac - 1; i++ ) {
        cv = MgfHandlerGeometry::getNamedVertex(av[i], context);
        if ( cv == nullptr) {
            return ErrorCodeContext::MGF_ERROR_UNDEFINED_REFERENCE;
        }

        if ( !cv->n.isNull(Numeric::EPSILON) ) {
            hasNormal++;
        }

        Vector3Dd v2;
        Vector3Dd v3;

        v2.x = cv->p.x - cv0->p.x;
        v2.y = cv->p.y - cv0->p.y;
        v2.z = cv->p.z - cv0->p.z;
        v3.crossProduct(&v1, &v2);
        norm.x += v3.x;
        norm.y += v3.y;
        norm.z += v3.z;
        v1.copy(&v2);
    }
    if ( norm.normalizeAndGivePreviousNorm(Numeric::EPSILON) == 0.0 ) {
        return ErrorCodeContext::MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Create moved vertices
    for ( i = 1; i < ac - 1; i++ ) {
        java::Formatter::format(nvn[i - 1], MGF_PV_SIZE, "_pv%d", i);
        vent[1] = nvn[i - 1];
        vent[3] = av[i];
        rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, vent, context);
        if ( rv != ErrorCodeContext::MGF_OK ) {
            return rv;
        }
        cv = MgfHandlerGeometry::getNamedVertex(av[i], context); // Checked above
        java::Formatter::format(p[0], 24, globalFloatFormat, cv->p.x - length * norm.x);
        java::Formatter::format(p[1], 24, globalFloatFormat, cv->p.y - length * norm.y);
        java::Formatter::format(p[2], 24, globalFloatFormat, cv->p.z - length * norm.z);
        rv = MgfDefinitions::mgfHandle(EntityContext::MGF_POINT, 4, pent, context);
        if ( rv != ErrorCodeContext::MGF_OK ) {
            return rv;
        }
    }

    // Make faces
    newArgV[0] = context->entityNames[EntityContext::FACE];
    // Do the side faces
    newArgV[5] = nullptr;
    newArgV[3] = av[ac - 2];
    newArgV[4] = nvn[ac - 3];
    for ( i = 1; i < ac - 1; i++ ) {
        newArgV[1] = nvn[i - 1];
        newArgV[2] = av[i];
        rv = MgfDefinitions::mgfHandle(EntityContext::FACE, 5, newArgV, context);
        if ( rv != ErrorCodeContext::MGF_OK ) {
            return rv;
        }
        newArgV[3] = newArgV[2];
        newArgV[4] = newArgV[1];
    }

    // Do top face
    for ( i = 1; i < ac - 1; i++ ) {
        if ( hasNormal ) {
            // Zero normals
            vent[1] = nvn[i - 1];
            rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 2, vent, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::MGF_NORMAL, 4, zNormal, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
        }
        newArgV[ac - 1 - i] = nvn[i - 1]; // Reverse
    }
    rv = MgfDefinitions::mgfHandle(EntityContext::FACE, ac - 1, newArgV, context);
    if ( rv != ErrorCodeContext::MGF_OK ) {
        return rv;
    }

    // Do bottom face
    if ( hasNormal != 0 ) {
        for ( i = 1; i < ac - 1; i++ ) {
            vent[1] = nvn[i - 1];
            vent[3] = av[i];
            rv = MgfDefinitions::mgfHandle(EntityContext::VERTEX, 4, vent, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            rv = MgfDefinitions::mgfHandle(EntityContext::MGF_NORMAL, 4, zNormal, context);
            if ( rv != ErrorCodeContext::MGF_OK ) {
                return rv;
            }
            newArgV[i] = nvn[i - 1];
        }
    } else {
        for ( i = 1; i < ac - 1; i++ ) {
            newArgV[i] = av[i];
        }
    }
    newArgV[i] = nullptr;
    rv = MgfDefinitions::mgfHandle(EntityContext::FACE, i, newArgV, context);
    if ( rv != ErrorCodeContext::MGF_OK ) {
        return rv;
    }
    return ErrorCodeContext::MGF_OK;
}

/**
Replace face + holes with single contour
*/
int
MgfGeometry::mgfEntityFaceWithHoles(int ac, const char **av, ParseSession *context) {
    const char *newArgV[MGF_MAXIMUM_ARGUMENT_COUNT];
    int lastP = 0;

    newArgV[0] = context->entityNames[EntityContext::FACE];
    int i;
    for ( i = 1; i < ac; i++ ) {
        if ( av[i][0] == '-' ) {
            if ( i < 4 ) {
                return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( i >= ac - 1 ) {
                break;
            }
            if ( !lastP ) {
                lastP = i - 1;
            }
            int j;
            for ( j = i + 1; j < ac - 1 && av[j + 1][0] != '-'; j++ ) {}
            if ( j - i < 3 ) {
                return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            newArgV[i] = av[j]; // Connect hole loop
        } else {
            // Hole or perimeter vertex
            newArgV[i] = av[i];
        }
    }
    if ( lastP ) {
        // Finish seam to outside
        newArgV[i++] = av[lastP];
    }
    newArgV[i] = nullptr;
    return MgfDefinitions::mgfHandle(EntityContext::FACE, i, newArgV, context);
}

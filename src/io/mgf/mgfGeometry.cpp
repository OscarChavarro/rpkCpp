#include <cstring>

#include "io/mgf/mgfDefinitions.h"
#include "io/mgf/words.h"
#include "io/mgf/mgfGeometry.h"

// Alternate handler support functions
static const int MGF_PV_SIZE = 24;
static const char globalFloatFormat[] = "%.12g";
static bool globalWarpConeEnds; // Hack for generating good normals

/**
Expand a sphere into cones
*/
int
mgfEntitySphere(int ac, const char **av, MgfContext *context) {
    char p2x[24];
    char p2y[24];
    char p2z[24];
    char r1[24];
    char r2[24];
    const char *v1Entity[5] = {
        context->entityNames[MgfEntity::VERTEX],
        "_sv1",
        "=",
        "_sv2"
    };
    const char *v2Entity[4] = {
        context->entityNames[MgfEntity::VERTEX],
        "_sv2",
        "="
    };
    const char *p2Entity[5] = {context->entityNames[MgfEntity::MGF_POINT], p2x, p2y, p2z};
    const char *coneEntity[6] = {
        context->entityNames[MgfEntity::CONE],
        "_sv1",
        r1,
        "_sv2",
        r2
    };
    const MgfVertexContext *cv;
    int rVal;
    double rad;
    double theta;

    if ( ac != 3 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    cv = getNamedVertex(av[1], context);
    if ( cv == nullptr) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( !isFloatWords(av[2]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    rad = strtod(av[2], nullptr);

    // Initialize
    globalWarpConeEnds = true;
    rVal = mgfHandle(MgfEntity::VERTEX, 3, v2Entity, context);
    if ( rVal != MGF_OK ) {
        return rVal;
    }
    snprintf(p2x, 24, globalFloatFormat, cv->p.x);
    snprintf(p2y, 24, globalFloatFormat, cv->p.y);
    snprintf(p2z, 24, globalFloatFormat, cv->p.z + rad);
    rVal = mgfHandle(MgfEntity::MGF_POINT, 4, p2Entity, context);
    if ( rVal != MGF_OK ) {
        return rVal;
    }
    r2[0] = '0';
    r2[1] = '\0';
    for ( int i = 1; i <= 2 * context->numberOfQuarterCircleDivisions; i++ ) {
        theta = i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
        rVal = mgfHandle(MgfEntity::VERTEX, 4, v1Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        snprintf(p2z, 24, globalFloatFormat, cv->p.z + rad * java::Math::cos(theta));
        rVal = mgfHandle(MgfEntity::VERTEX, 2, v2Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        rVal = mgfHandle(MgfEntity::MGF_POINT, 4, p2Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        strcpy(r1, r2);
        snprintf(r2, 24, globalFloatFormat, rad * java::Math::sin(theta));
        rVal = mgfHandle(MgfEntity::CONE, 5, coneEntity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
    }
    globalWarpConeEnds = false;
    return MGF_OK;
}

/**
Expand a torus into cones
*/
int
mgfEntityTorus(int ac, const char **av, MgfContext *context) {
    char p2[3][24];
    char r1[24];
    char r2[24];
    const char *v1Entity[5] = {
        context->entityNames[MgfEntity::VERTEX],
        "_tv1",
        "=",
        "_tv2"
    };
    const char *v2Entity[5] = {
        context->entityNames[MgfEntity::VERTEX],
        "_tv2",
        "="
    };
    const char *p2Entity[5] = {
        context->entityNames[MgfEntity::MGF_POINT],
        p2[0],
        p2[1],
        p2[2]
    };
    const char *coneEntity[6] = {
        context->entityNames[MgfEntity::CONE],
        "_tv1",
        r1,
        "_tv2",
        r2
    };
    const MgfVertexContext *cv;
    int rVal;
    double minRad;
    double maxRad;
    double avgRad;
    double theta;

    if ( ac != 4 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if ( (cv = getNamedVertex(av[1], context)) == nullptr ) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( is0Vector(&cv->n, Numeric::EPSILON ) ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( !isFloatWords(av[2]) || !isFloatWords(av[3]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    minRad = strtod(av[2], nullptr);
    round0(minRad, Numeric::EPSILON);
    maxRad = strtod(av[3], nullptr);

    // Check orientation
    int sign;
    if ( minRad > 0.0 ) {
        sign = 1;
    } else if ( minRad < 0.0 ) {
        sign = -1;
    } else {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( sign * (maxRad - minRad) <= 0.0 ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Initialize
    globalWarpConeEnds = true;
    v2Entity[3] = av[1];
    snprintf(p2[0], 24, globalFloatFormat, cv->p.x + 0.5 * sign * (maxRad - minRad) * cv->n.x);
    snprintf(p2[1], 24, globalFloatFormat, cv->p.y + 0.5 * sign * (maxRad - minRad) * cv->n.y);
    snprintf(p2[2], 24, globalFloatFormat, cv->p.z + 0.5 * sign * (maxRad - minRad) * cv->n.z);
    rVal = mgfHandle(MgfEntity::VERTEX, 4, v2Entity, context);
    if ( rVal != MGF_OK ) {
        return rVal;
    }
    rVal = mgfHandle(MgfEntity::MGF_POINT, 4, p2Entity, context);
    if ( rVal != MGF_OK ) {
        return rVal;
    }
    snprintf(r2, 24, globalFloatFormat, avgRad = 0.5 * (minRad + maxRad));

    // Run outer section
    int i;
    for ( i = 1; i <= 2 * context->numberOfQuarterCircleDivisions; i++ ) {
        theta = i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
        rVal = mgfHandle(MgfEntity::VERTEX, 4, v1Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        snprintf(p2[0], 24, globalFloatFormat, cv->p.x + 0.5 * sign * (maxRad - minRad) * java::Math::cos(theta) * cv->n.x);
        snprintf(p2[1], 24, globalFloatFormat, cv->p.y + 0.5 * sign * (maxRad - minRad) * java::Math::cos(theta) * cv->n.y);
        snprintf(p2[2], 24, globalFloatFormat, cv->p.z + 0.5 * sign * (maxRad - minRad) * java::Math::cos(theta) * cv->n.z);
        rVal = mgfHandle(MgfEntity::VERTEX, 2, v2Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        rVal = mgfHandle(MgfEntity::MGF_POINT, 4, p2Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        strcpy(r1, r2);
        snprintf(r2, 24, globalFloatFormat, avgRad + 0.5 * (maxRad - minRad) * java::Math::sin(theta));
        rVal = mgfHandle(MgfEntity::CONE, 5, coneEntity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
    }

    // Run inner section
    snprintf(r2, 24, globalFloatFormat, -0.5 * (minRad + maxRad));
    for ( ; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
        theta = i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
        snprintf(p2[0], 24, globalFloatFormat, cv->p.x + 0.5 * sign * (maxRad - minRad) * java::Math::cos(theta) * cv->n.x);
        snprintf(p2[1], 24, globalFloatFormat, cv->p.y + 0.5 * sign * (maxRad - minRad) * java::Math::cos(theta) * cv->n.y);
        snprintf(p2[2], 24, globalFloatFormat, cv->p.z + 0.5 * sign * (maxRad - minRad) * java::Math::cos(theta) * cv->n.z);
        rVal = mgfHandle(MgfEntity::VERTEX, 4, v1Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        rVal = mgfHandle(MgfEntity::VERTEX, 2, v2Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        rVal = mgfHandle(MgfEntity::MGF_POINT, 4, p2Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        strcpy(r1, r2);
        snprintf(r2, 24, globalFloatFormat, -avgRad - .5 * (maxRad - minRad) * java::Math::sin(theta));
        rVal = mgfHandle(MgfEntity::CONE, 5, coneEntity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
    }
    globalWarpConeEnds = false;
    return MGF_OK;
}

/**
Replace a cylinder with equivalent cone
*/
int
mgfEntityCylinder(int ac, const char **av, MgfContext *context) {
    const char *newArgV[6] = {context->entityNames[MgfEntity::CONE]};

    if ( ac != 4 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    newArgV[1] = av[1];
    newArgV[2] = av[2];
    newArgV[3] = av[3];
    newArgV[4] = av[2];
    return mgfHandle(MgfEntity::CONE, 5, newArgV, context);
}

/**
Compute u and v given w (normalized)
*/
static void
mgfMakeAxes(VECTOR3Dd *u, VECTOR3Dd *v, const VECTOR3Dd *w, double epsilon)
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

    floatCrossProduct(u, v, w);
    normalize(u, epsilon);
    floatCrossProduct(v, w, u);
}

/**
Turn a ring into polygons
*/
int
mgfEntityRing(int ac, const char **av, MgfContext *context) {
    char p3[3][24];
    char p4[3][24];
    const char *namesEntity[5] = {
        context->entityNames[MgfEntity::MGF_NORMAL],
        "0",
        "0",
        "0"
    };
    const char *v1Entity[5] = {
        context->entityNames[MgfEntity::VERTEX],
        "_rv1",
        "="
    };
    const char *v2Entity[5] = {
        context->entityNames[MgfEntity::VERTEX],
        "_rv2",
        "=",
        "_rv3"
    };
    const char *v3Entity[4] = {
        context->entityNames[MgfEntity::VERTEX],
        "_rv3",
        "="
    };
    const char *p3Entity[5] = {
        context->entityNames[MgfEntity::MGF_POINT],
        p3[0],
        p3[1],
        p3[2]
    };
    const char *v4Entity[4] = {
        context->entityNames[MgfEntity::VERTEX],
        "_rv4",
        "="
    };
    const char *p4Entity[5] = {
        context->entityNames[MgfEntity::MGF_POINT],
        p4[0],
        p4[1],
        p4[2]
    };
    const char *faceEntity[6] = {
        context->entityNames[MgfEntity::FACE],
        "_rv1",
        "_rv2",
        "_rv3",
        "_rv4"
    };
    const MgfVertexContext *vertexContext;
    double minRad;
    double maxRad;
    int rv;
    double theta;
    double d;

    if ( ac != 4 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }

    vertexContext = getNamedVertex(av[1], context);
    if ( vertexContext == nullptr) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( is0Vector(&vertexContext->n, Numeric::EPSILON) ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( !isFloatWords(av[2]) || !isFloatWords(av[3]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    minRad = strtod(av[2], nullptr);
    round0(minRad, Numeric::EPSILON);
    maxRad = strtod(av[3], nullptr);
    if ( minRad < 0.0 || maxRad <= minRad ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Initialize
    VECTOR3Dd u;
    VECTOR3Dd v;

    mgfMakeAxes(&u, &v, &vertexContext->n, Numeric::EPSILON);
    snprintf(p3[0], 24, globalFloatFormat, vertexContext->p.x + maxRad * u.x);
    snprintf(p3[1], 24, globalFloatFormat, vertexContext->p.y + maxRad * u.y);
    snprintf(p3[2], 24, globalFloatFormat, vertexContext->p.z + maxRad * u.z);
    rv = mgfHandle(MgfEntity::VERTEX, 3, v3Entity, context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    rv = mgfHandle(MgfEntity::MGF_POINT, 4, p3Entity, context);
    if ( rv != MGF_OK ) {
        return rv;
    }

    if ( Numeric::doubleEqual(minRad, 0.0, Numeric::EPSILON) ) {
        // Closed
        v1Entity[3] = av[1];
        rv = mgfHandle(MgfEntity::VERTEX, 4, v1Entity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        rv = mgfHandle(MgfEntity::MGF_NORMAL, 4, namesEntity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( int i = 1; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
            theta = i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
            rv = mgfHandle(MgfEntity::VERTEX, 4, v2Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }

            snprintf(
                p3[0], 24, globalFloatFormat,
                vertexContext->p.x + maxRad * u.x * java::Math::cos(theta) + maxRad * v.x * java::Math::sin(theta));
            snprintf(
                p3[1], 24, globalFloatFormat,
                vertexContext->p.y + maxRad * u.y * java::Math::cos(theta) + maxRad * v.y * java::Math::sin(theta));
            snprintf(
                p3[2], 24, globalFloatFormat,
                vertexContext->p.z + maxRad * u.z * java::Math::cos(theta) + maxRad * v.z * java::Math::sin(theta));

            rv = mgfHandle(MgfEntity::VERTEX, 2, v3Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::MGF_POINT, 4, p3Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::FACE, 4, faceEntity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
        }
    } else {
        // Open
        rv = mgfHandle(MgfEntity::VERTEX, 3, v4Entity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }

        snprintf(p4[0], 24, globalFloatFormat, vertexContext->p.x + minRad * u.x);
        snprintf(p4[1], 24, globalFloatFormat, vertexContext->p.y + minRad * u.y);
        snprintf(p4[2], 24, globalFloatFormat, vertexContext->p.z + minRad * u.z);

        rv = mgfHandle(MgfEntity::MGF_POINT, 4, p4Entity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        v1Entity[3] = "_rv4";
        for ( int i = 1; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
            theta = i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
            rv = mgfHandle(MgfEntity::VERTEX, 4, v1Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::VERTEX, 4, v2Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }

            d = u.x * java::Math::cos(theta) + v.x * java::Math::sin(theta);
            snprintf(p3[0], 24, globalFloatFormat, vertexContext->p.x + maxRad * d);
            snprintf(p4[0], 24, globalFloatFormat, vertexContext->p.x + minRad * d);

            d = u.y * java::Math::cos(theta) + v.y * java::Math::sin(theta);
            snprintf(p3[1], 24, globalFloatFormat, vertexContext->p.y + maxRad * d);
            snprintf(p4[1], 24, globalFloatFormat, vertexContext->p.y + minRad * d);

            d = u.z * java::Math::cos(theta) + v.z * java::Math::sin(theta);
            snprintf(p3[2], 24, globalFloatFormat, vertexContext->p.z + maxRad * d);
            snprintf(p4[2], 24, globalFloatFormat, vertexContext->p.z + minRad * d);

            rv = mgfHandle(MgfEntity::VERTEX, 2, v3Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::MGF_POINT, 4, p3Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::VERTEX, 2, v4Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::MGF_POINT, 4, p4Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::FACE, 5, faceEntity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
        }
    }
    return MGF_OK;
}

/**
Turn a cone into polygons
*/
int
mgfEntityCone(int ac, const char **av, MgfContext *context) {
    char p3[3][24];
    char p4[3][24];
    char n3[3][24];
    char n4[3][24];
    const char *v1Entity[5] = {
        context->entityNames[MgfEntity::VERTEX],
        "_cv1",
        "="
    };
    const char *v2Entity[5] = {
        context->entityNames[MgfEntity::VERTEX],
        "_cv2",
        "=",
        "_cv3"
    };
    const char *v3Entity[4] = {
        context->entityNames[MgfEntity::VERTEX],
        "_cv3",
        "="
    };
    const char *p3Entity[5] = {
        context->entityNames[MgfEntity::MGF_POINT], p3[0], p3[1], p3[2]};
    const char *n3Entity[5] = {
        context->entityNames[MgfEntity::MGF_NORMAL], n3[0], n3[1], n3[2]};
    const char *v4Entity[4] = {
        context->entityNames[MgfEntity::VERTEX],
        "_cv4",
        "="};
    const char *p4Entity[5] = {
        context->entityNames[MgfEntity::MGF_POINT],
        p4[0],
        p4[1],
        p4[2]
    };
    const char *n4Entity[5] = {
        context->entityNames[MgfEntity::MGF_NORMAL],
        n4[0],
        n4[1],
        n4[2]
    };
    const char *faceEntity[6] = {
        context->entityNames[MgfEntity::FACE],
        "_cv1",
        "_cv2",
        "_cv3",
        "_cv4"
    };
    const char *v1n;
    MgfVertexContext *cv1;
    MgfVertexContext *cv2;
    double n1off;
    double n2off;
    double d;
    int rv;
    double theta;

    if ( ac != 5 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    cv1 = getNamedVertex(av[1], context);
    cv2 = getNamedVertex(av[3], context);
    if ( cv1 == nullptr || cv2 == nullptr) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    v1n = av[1];
    if ( !isFloatWords(av[2]) || !isFloatWords(av[4]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }

    // Set up (radius1, radius2)
    double radius1 = strtod(av[2], nullptr);
    round0(radius1, Numeric::EPSILON);
    double radius2 = strtod(av[4], nullptr);
    round0(radius2, Numeric::EPSILON);

    if ( radius1 == 0.0 ) {
        if ( radius2 == 0.0 ) {
            return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
    } else if ( radius2 != 0.0 ) {
        bool a = radius1 < 0.0;
        bool b = radius2 < 0.0;
        bool check = (a && !b) || (!a && b); // Note: this is exclusive or / XOR a ^ b
        if ( check ) {
            return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
    } else {
        // Swap
        MgfVertexContext *cv;

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
    VECTOR3Dd w;

    w.x = cv1->p.x - cv2->p.x;
    w.y = cv1->p.y - cv2->p.y;
    w.z = cv1->p.z - cv2->p.z;

    d = normalize(&w, Numeric::EPSILON);
    if ( d == 0.0 ) {
        // TODO: Review floating point comparisons vs EPSILON
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    n1off = n2off = (radius2 - radius1) / d;
    if ( globalWarpConeEnds ) {
        // Hack for mgfEntitySphere and mgfEntityTorus
        d = java::Math::atan(n2off) - (M_PI / 4) / context->numberOfQuarterCircleDivisions;
        if ( d <= -M_PI / 2 + Numeric::EPSILON ) {
            n2off = -Numeric::HUGE_FLOAT_VALUE;
        } else {
            n2off = java::Math::tan(d);
        }
    }

    VECTOR3Dd u;
    VECTOR3Dd v;
    mgfMakeAxes(&u, &v, &w, Numeric::EPSILON);

    snprintf(p3[0], 24, globalFloatFormat, cv2->p.x + radius2 * u.x);
    if ( n2off <= -Numeric::HUGE_FLOAT_VALUE) {
        snprintf(n3[0], 24, globalFloatFormat, -w.x);
    } else {
        snprintf(n3[0], 24, globalFloatFormat, u.x + w.x * n2off);
    }

    snprintf(p3[1], 24, globalFloatFormat, cv2->p.y + radius2 * u.y);
    if ( n2off <= -Numeric::HUGE_FLOAT_VALUE) {
        snprintf(n3[1], 24, globalFloatFormat, -w.y);
    } else {
        snprintf(n3[1], 24, globalFloatFormat, u.y + w.y * n2off);
    }

    snprintf(p3[2], 24, globalFloatFormat, cv2->p.z + radius2 * u.z);
    if ( n2off <= -Numeric::HUGE_FLOAT_VALUE) {
        snprintf(n3[2], 24, globalFloatFormat, -w.z);
    } else {
        snprintf(n3[2], 24, globalFloatFormat, u.z + w.z * n2off);
    }

    rv = mgfHandle(MgfEntity::VERTEX, 3, v3Entity, context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    rv = mgfHandle(MgfEntity::MGF_POINT, 4, p3Entity, context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    rv = mgfHandle(MgfEntity::MGF_NORMAL, 4, n3Entity, context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    if ( radius1 == 0.0 ) {
        // TODO: Review floating point comparisons vs EPSILON
        // Triangles
        v1Entity[3] = v1n;
        rv = mgfHandle(MgfEntity::VERTEX, 4, v1Entity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }

        snprintf(n4[0], 24, globalFloatFormat, w.x);
        snprintf(n4[1], 24, globalFloatFormat, w.y);
        snprintf(n4[2], 24, globalFloatFormat, w.z);

        rv = mgfHandle(MgfEntity::MGF_NORMAL, 4, n4Entity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( int i = 1; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
            theta = sign * i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
            rv = mgfHandle(MgfEntity::VERTEX, 4, v2Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }

            d = u.x * java::Math::cos(theta) + v.x * java::Math::sin(theta);
            snprintf(p3[0], 24, globalFloatFormat, cv2->p.x + radius2 * d);
            if ( n2off > -Numeric::HUGE_FLOAT_VALUE) {
                snprintf(n3[0], 24, globalFloatFormat, d + w.x * n2off);
            }

            d = u.y * java::Math::cos(theta) + v.y * java::Math::sin(theta);
            snprintf(p3[1], 24, globalFloatFormat, cv2->p.y + radius2 * d);
            if ( n2off > -Numeric::HUGE_FLOAT_VALUE) {
                snprintf(n3[1], 24, globalFloatFormat, d + w.y * n2off);
            }

            d = u.z * java::Math::cos(theta) + v.z * java::Math::sin(theta);
            snprintf(p3[2], 24, globalFloatFormat, cv2->p.z + radius2 * d);
            if ( n2off > -Numeric::HUGE_FLOAT_VALUE) {
                snprintf(n3[2], 24, globalFloatFormat, d + w.z * n2off);
            }

            rv = mgfHandle(MgfEntity::VERTEX, 2, v3Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::MGF_POINT, 4, p3Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::MGF_NORMAL, 4, n3Entity, context);
            if ( n2off > -Numeric::HUGE_FLOAT_VALUE && rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::FACE, 4, faceEntity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
        }
    } else {
        // Quads
        v1Entity[3] = "_cv4";
        if ( globalWarpConeEnds ) {
            // Hack for mgfEntitySphere and mgfEntityTorus
            d = java::Math::atan(n1off) + (M_PI / 4) / context->numberOfQuarterCircleDivisions;
            if ( d >= M_PI / 2 - Numeric::EPSILON ) {
                n1off = Numeric::HUGE_FLOAT_VALUE;
            } else {
                n1off = java::Math::tan(java::Math::atan(n1off) + (M_PI / 4) / context->numberOfQuarterCircleDivisions);
            }
        }

        snprintf(p4[0], 24, globalFloatFormat, cv1->p.x + radius1 * u.x);
        if ( n1off >= Numeric::HUGE_FLOAT_VALUE) {
            snprintf(n4[0], 24, globalFloatFormat, w.x);
        } else {
            snprintf(n4[0], 24, globalFloatFormat, u.x + w.x * n1off);
        }

        snprintf(p4[1], 24, globalFloatFormat, cv1->p.y + radius1 * u.y);
        if ( n1off >= Numeric::HUGE_FLOAT_VALUE) {
            snprintf(n4[1], 24, globalFloatFormat, w.y);
        } else {
            snprintf(n4[1], 24, globalFloatFormat, u.y + w.y * n1off);
        }

        snprintf(p4[2], 24, globalFloatFormat, cv1->p.z + radius1 * u.z);
        if ( n1off >= Numeric::HUGE_FLOAT_VALUE) {
            snprintf(n4[2], 24, globalFloatFormat, w.z);
        } else {
            snprintf(n4[2], 24, globalFloatFormat, u.z + w.z * n1off);
        }

        rv = mgfHandle(MgfEntity::VERTEX, 3, v4Entity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        rv = mgfHandle(MgfEntity::MGF_POINT, 4, p4Entity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        rv = mgfHandle(MgfEntity::MGF_NORMAL, 4, n4Entity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( int i = 1; i <= 4 * context->numberOfQuarterCircleDivisions; i++ ) {
            theta = sign * i * (M_PI / 2) / context->numberOfQuarterCircleDivisions;
            rv = mgfHandle(MgfEntity::VERTEX, 4, v1Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::VERTEX, 4, v2Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }

            d = u.x * java::Math::cos(theta) + v.x * java::Math::sin(theta);
            snprintf(p3[0], 24, globalFloatFormat, cv2->p.x + radius2 * d);
            if ( n2off > -Numeric::HUGE_FLOAT_VALUE) {
                snprintf(n3[0], 24, globalFloatFormat, d + w.x * n2off);
            }
            snprintf(p4[0], 24, globalFloatFormat, cv1->p.x + radius1 * d);
            if ( n1off < Numeric::HUGE_FLOAT_VALUE) {
                snprintf(n4[0], 24, globalFloatFormat, d + w.x * n1off);
            }

            d = u.y * java::Math::cos(theta) + v.y * java::Math::sin(theta);
            snprintf(p3[1], 24, globalFloatFormat, cv2->p.y + radius2 * d);
            if ( n2off > -Numeric::HUGE_FLOAT_VALUE) {
                snprintf(n3[1], 24, globalFloatFormat, d + w.y * n2off);
            }
            snprintf(p4[1], 24, globalFloatFormat, cv1->p.y + radius1 * d);
            if ( n1off < Numeric::HUGE_FLOAT_VALUE) {
                snprintf(n4[1], 24, globalFloatFormat, d + w.y * n1off);
            }

            d = u.z * java::Math::cos(theta) + v.z * java::Math::sin(theta);
            snprintf(p3[2], 24, globalFloatFormat, cv2->p.z + radius2 * d);
            if ( n2off > -Numeric::HUGE_FLOAT_VALUE) {
                snprintf(n3[2], 24, globalFloatFormat, d + w.z * n2off);
            }
            snprintf(p4[2], 24, globalFloatFormat, cv1->p.z + radius1 * d);
            if ( n1off < Numeric::HUGE_FLOAT_VALUE) {
                snprintf(n4[2], 24, globalFloatFormat, d + w.z * n1off);
            }

            rv = mgfHandle(MgfEntity::VERTEX, 2, v3Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::MGF_POINT, 4, p3Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::MGF_NORMAL, 4, n3Entity, context);
            if ( n2off > -Numeric::HUGE_FLOAT_VALUE && rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::VERTEX, 2, v4Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::MGF_POINT, 4, p4Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::MGF_NORMAL, 4, n4Entity, context);
            if ( n1off < Numeric::HUGE_FLOAT_VALUE &&
                 rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::FACE, 5, faceEntity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
        }
    }
    return MGF_OK;
}

/**
Turn a prism into polygons
*/
int
mgfEntityPrism(int ac, const char **av, MgfContext *context) {
    char p[3][24];
    const char *vent[5] = {
        context->entityNames[MgfEntity::VERTEX],
        nullptr,
        "="
    };
    const char *pent[5] = {
        context->entityNames[MgfEntity::MGF_POINT],
        p[0],
        p[1],
        p[2]
    };
    const char *zNormal[5] = {
        context->entityNames[MgfEntity::MGF_NORMAL],
        "0",
        "0",
        "0"
    };
    const char *newArgV[MGF_MAXIMUM_ARGUMENT_COUNT];
    char nvn[MGF_MAXIMUM_ARGUMENT_COUNT - 1][MGF_PV_SIZE];
    double length;
    int hasNormal;
    const MgfVertexContext *cv;
    const MgfVertexContext *cv0;
    int rv;
    int i;

    // Check arguments
    if ( ac < 5 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if ( !isFloatWords(av[ac - 1]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    length = strtod(av[ac - 1], nullptr);
    if ( length <= Numeric::EPSILON && length >= -Numeric::EPSILON ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Compute face normal
    cv0 = getNamedVertex(av[1], context);
    if ( cv0 == nullptr ) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    hasNormal = 0;

    VECTOR3Dd norm(0.0, 0.0, 0.0);
    VECTOR3Dd v1(0.0, 0.0, 0.0);

    for ( i = 2; i < ac - 1; i++ ) {
        cv = getNamedVertex(av[i], context);
        if ( cv == nullptr) {
            return MGF_ERROR_UNDEFINED_REFERENCE;
        }

        if ( !is0Vector(&cv->n, Numeric::EPSILON) ) {
            hasNormal++;
        }

        VECTOR3Dd v2;
        VECTOR3Dd v3;

        v2.x = cv->p.x - cv0->p.x;
        v2.y = cv->p.y - cv0->p.y;
        v2.z = cv->p.z - cv0->p.z;
        floatCrossProduct(&v3, &v1, &v2);
        norm.x += v3.x;
        norm.y += v3.y;
        norm.z += v3.z;
        mgfVertexCopy(&v1, &v2);
    }
    if ( normalize(&norm, Numeric::EPSILON) == 0.0 ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Create moved vertices
    for ( i = 1; i < ac - 1; i++ ) {
        snprintf(nvn[i - 1], MGF_PV_SIZE, "_pv%d", i);
        vent[1] = nvn[i - 1];
        vent[3] = av[i];
        rv = mgfHandle(MgfEntity::VERTEX, 4, vent, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        cv = getNamedVertex(av[i], context); // Checked above
        snprintf(p[0], 24, globalFloatFormat, cv->p.x - length * norm.x);
        snprintf(p[1], 24, globalFloatFormat, cv->p.y - length * norm.y);
        snprintf(p[2], 24, globalFloatFormat, cv->p.z - length * norm.z);
        rv = mgfHandle(MgfEntity::MGF_POINT, 4, pent, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
    }

    // Make faces
    newArgV[0] = context->entityNames[MgfEntity::FACE];
    // Do the side faces
    newArgV[5] = nullptr;
    newArgV[3] = av[ac - 2];
    newArgV[4] = nvn[ac - 3];
    for ( i = 1; i < ac - 1; i++ ) {
        newArgV[1] = nvn[i - 1];
        newArgV[2] = av[i];
        rv = mgfHandle(MgfEntity::FACE, 5, newArgV, context);
        if ( rv != MGF_OK ) {
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
            rv = mgfHandle(MgfEntity::VERTEX, 2, vent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::MGF_NORMAL, 4, zNormal, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
        }
        newArgV[ac - 1 - i] = nvn[i - 1]; // Reverse
    }
    rv = mgfHandle(MgfEntity::FACE, ac - 1, newArgV, context);
    if ( rv != MGF_OK ) {
        return rv;
    }

    // Do bottom face
    if ( hasNormal != 0 ) {
        for ( i = 1; i < ac - 1; i++ ) {
            vent[1] = nvn[i - 1];
            vent[3] = av[i];
            rv = mgfHandle(MgfEntity::VERTEX, 4, vent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MgfEntity::MGF_NORMAL, 4, zNormal, context);
            if ( rv != MGF_OK ) {
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
    rv = mgfHandle(MgfEntity::FACE, i, newArgV, context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    return MGF_OK;
}

/**
Replace face + holes with single contour
*/
int
mgfEntityFaceWithHoles(int ac, const char **av, MgfContext *context) {
    const char *newArgV[MGF_MAXIMUM_ARGUMENT_COUNT];
    int lastP = 0;

    newArgV[0] = context->entityNames[MgfEntity::FACE];
    int i;
    for ( i = 1; i < ac; i++ ) {
        int j;

        if ( av[i][0] == '-' ) {
            if ( i < 4 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( i >= ac - 1 ) {
                break;
            }
            if ( !lastP ) {
                lastP = i - 1;
            }
            for ( j = i + 1; j < ac - 1 && av[j + 1][0] != '-'; j++ );
            if ( j - i < 3 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
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
    return mgfHandle(MgfEntity::FACE, i, newArgV, context);
}

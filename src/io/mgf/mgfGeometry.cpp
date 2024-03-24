#include <cstring>

#include "io/mgf/mgfDefinitions.h"
#include "io/mgf/words.h"
#include "io/mgf/mgfGeometry.h"

#define MGF_DEFAULT_NUMBER_OF_DIVISIONS 5

// Number of divisions per quarter circle
int GLOBAL_mgf_divisionsPerQuarterCircle = MGF_DEFAULT_NUMBER_OF_DIVISIONS;

// Alternate handler support functions
static char globalFloatFormat[] = "%.12g";
static bool globalWarpConeEnds; // Hack for generating good normals

/**
Expand a sphere into cones
*/
int
mgfEntitySphere(int ac, char **av, MgfContext *context)
{
    char p2x[24];
    char p2y[24];
    char p2z[24];
    char r1[24];
    char r2[24];
    char *v1Entity[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_sv1", (char *)"=", (char *)"_sv2"};
    char *v2Entity[4] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_sv2", (char *)"="};
    char *p2Entity[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_POINT], p2x, p2y, p2z};
    char *coneEntity[6] = {GLOBAL_mgf_entityNames[MGF_ENTITY_CONE], (char *)"_sv1", r1, (char *)"_sv2", r2};
    MgfVertexContext *cv;
    int rVal;
    double rad;
    double theta;

    if ( ac != 3 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    cv = getNamedVertex(av[1]);
    if ( cv == nullptr) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( !isFloatWords(av[2]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    rad = strtod(av[2], nullptr);

    // Initialize
    globalWarpConeEnds = true;
    rVal = mgfHandle(MGF_ENTITY_VERTEX, 3, v2Entity, context);
    if ( rVal != MGF_OK ) {
        return rVal;
    }
    snprintf(p2x, 24, globalFloatFormat, cv->p[0]);
    snprintf(p2y, 24, globalFloatFormat, cv->p[1]);
    snprintf(p2z, 24, globalFloatFormat, cv->p[2] + rad);
    rVal = mgfHandle(MGF_ENTITY_POINT, 4, p2Entity, context);
    if ( rVal != MGF_OK ) {
        return rVal;
    }
    r2[0] = '0';
    r2[1] = '\0';
    for ( int i = 1; i <= 2 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
        theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
        rVal = mgfHandle(MGF_ENTITY_VERTEX, 4, v1Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        snprintf(p2z, 24, globalFloatFormat, cv->p[2] + rad * std::cos(theta));
        rVal = mgfHandle(MGF_ENTITY_VERTEX, 2, v2Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        rVal = mgfHandle(MGF_ENTITY_POINT, 4, p2Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        strcpy(r1, r2);
        snprintf(r2, 24, globalFloatFormat, rad * std::sin(theta));
        rVal = mgfHandle(MGF_ENTITY_CONE, 5, coneEntity, context);
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
mgfEntityTorus(int ac, char **av, MgfContext *context)
{
    char p2[3][24];
    char r1[24];
    char r2[24];
    char *v1Entity[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_tv1", (char *)"=", (char *)"_tv2"};
    char *v2Entity[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_tv2", (char *)"="};
    char *p2Entity[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_POINT], p2[0], p2[1], p2[2]};
    char *coneEntity[6] = {GLOBAL_mgf_entityNames[MGF_ENTITY_CONE], (char *)"_tv1", r1, (char *)"_tv2", r2};
    MgfVertexContext *cv;
    int i;
    int rVal;
    double minRad;
    double maxRad;
    double avgRad;
    double theta;

    if ( ac != 4 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if ( (cv = getNamedVertex(av[1])) == nullptr ) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( is0Vector(cv->n) ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( !isFloatWords(av[2]) || !isFloatWords(av[3]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    minRad = strtod(av[2], nullptr);
    round0(minRad);
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
    for ( int j = 0; j < 3; j++ ) {
        snprintf(p2[j], 24, globalFloatFormat, cv->p[j] + 0.5 * sign * (maxRad - minRad) * cv->n[j]);
    }
    rVal = mgfHandle(MGF_ENTITY_VERTEX, 4, v2Entity, context);
    if ( rVal != MGF_OK ) {
        return rVal;
    }
    rVal = mgfHandle(MGF_ENTITY_POINT, 4, p2Entity, context);
    if ( rVal != MGF_OK ) {
        return rVal;
    }
    snprintf(r2, 24, globalFloatFormat, avgRad = 0.5 * (minRad + maxRad));

    // Run outer section
    for ( i = 1; i <= 2 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
        theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
        rVal = mgfHandle(MGF_ENTITY_VERTEX, 4, v1Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        for ( int j = 0; j < 3; j++ ) {
            snprintf(p2[j], 24, globalFloatFormat, cv->p[j] +
                                                   0.5 * sign * (maxRad - minRad) * std::cos(theta) * cv->n[j]);
        }
        rVal = mgfHandle(MGF_ENTITY_VERTEX, 2, v2Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        rVal = mgfHandle(MGF_ENTITY_POINT, 4, p2Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        strcpy(r1, r2);
        snprintf(r2, 24, globalFloatFormat, avgRad + 0.5 * (maxRad - minRad) * std::sin(theta));
        rVal = mgfHandle(MGF_ENTITY_CONE, 5, coneEntity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
    }

    // Run inner section
    snprintf(r2, 24, globalFloatFormat, -0.5 * (minRad + maxRad));
    for ( ; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
        theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
        for ( int j = 0; j < 3; j++ ) {
            snprintf(p2[j], 24, globalFloatFormat, cv->p[j] +
                                                   0.5 * sign * (maxRad - minRad) * std::cos(theta) * cv->n[j]);
        }
        rVal = mgfHandle(MGF_ENTITY_VERTEX, 4, v1Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        rVal = mgfHandle(MGF_ENTITY_VERTEX, 2, v2Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        rVal = mgfHandle(MGF_ENTITY_POINT, 4, p2Entity, context);
        if ( rVal != MGF_OK ) {
            return rVal;
        }
        strcpy(r1, r2);
        snprintf(r2, 24, globalFloatFormat, -avgRad - .5 * (maxRad - minRad) * std::sin(theta));
        rVal = mgfHandle(MGF_ENTITY_CONE, 5, coneEntity, context);
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
mgfEntityCylinder(int ac, char **av, MgfContext *context)
{
    char *newArgV[6] = {GLOBAL_mgf_entityNames[MGF_ENTITY_CONE]};

    if ( ac != 4 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    newArgV[1] = av[1];
    newArgV[2] = av[2];
    newArgV[3] = av[3];
    newArgV[4] = av[2];
    return mgfHandle(MGF_ENTITY_CONE, 5, newArgV, context);
}

/**
Turn a ring into polygons
*/
int
mgfEntityRing(int ac, char **av, MgfContext *context)
{
    char p3[3][24];
    char p4[3][24];
    char *namesEntity[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_NORMAL], (char *)"0", (char *)"0", (char *)"0"};
    char *v1Entity[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_rv1", (char *)"="};
    char *v2Entity[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_rv2", (char *)"=", (char *)"_rv3"};
    char *v3Entity[4] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_rv3", (char *)"="};
    char *p3Entity[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_POINT], p3[0], p3[1], p3[2]};
    char *v4Entity[4] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_rv4", (char *)"="};
    char *p4Entity[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_POINT], p4[0], p4[1], p4[2]};
    char *faceEntity[6] = {GLOBAL_mgf_entityNames[MGF_ENTITY_FACE], (char *)"_rv1", (char *)"_rv2", (char *)"_rv3", (char *)"_rv4"};
    MgfVertexContext *vertexContext;
    double minRad;
    double maxRad;
    int rv;
    double theta;
    double d;

    if ( ac != 4 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }

    vertexContext = getNamedVertex(av[1]);
    if ( vertexContext == nullptr) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( is0Vector(vertexContext->n) ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( !isFloatWords(av[2]) || !isFloatWords(av[3]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    minRad = strtod(av[2], nullptr);
    round0(minRad);
    maxRad = strtod(av[3], nullptr);
    if ( minRad < 0.0 || maxRad <= minRad ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Initialize
    VECTOR3Dd u;
    VECTOR3Dd v;

    mgfMakeAxes(u, v, vertexContext->n);
    for ( int j = 0; j < 3; j++ ) {
        snprintf(p3[j], 24, globalFloatFormat, vertexContext->p[j] + maxRad * u[j]);
    }
    rv = mgfHandle(MGF_ENTITY_VERTEX, 3, v3Entity, context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    rv = mgfHandle(MGF_ENTITY_POINT, 4, p3Entity, context);
    if ( rv != MGF_OK ) {
        return rv;
    }

    if ( minRad == 0.0 ) {
        // TODO: Review floating point comparisons vs EPSILON
        // Closed
        v1Entity[3] = av[1];
        rv = mgfHandle(MGF_ENTITY_VERTEX, 4, v1Entity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        rv = mgfHandle(MGF_ENTITY_NORMAL, 4, namesEntity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( int i = 1; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
            theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
            rv = mgfHandle(MGF_ENTITY_VERTEX, 4, v2Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            for ( int j = 0; j < 3; j++ ) {
                snprintf(p3[j], 24, globalFloatFormat, vertexContext->p[j] +
                                                       maxRad * u[j] * std::cos(theta) +
                                                       maxRad * v[j] * std::sin(theta));
            }
            rv = mgfHandle(MGF_ENTITY_VERTEX, 2, v3Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_POINT, 4, p3Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_FACE, 4, faceEntity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
        }
    } else {
        // Open
        rv = mgfHandle(MGF_ENTITY_VERTEX, 3, v4Entity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( int j = 0; j < 3; j++ ) {
            snprintf(p4[j], 24, globalFloatFormat, vertexContext->p[j] + minRad * u[j]);
        }
        rv = mgfHandle(MGF_ENTITY_POINT, 4, p4Entity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        v1Entity[3] = (char *)"_rv4";
        for ( int i = 1; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
            theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
            rv = mgfHandle(MGF_ENTITY_VERTEX, 4, v1Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_VERTEX, 4, v2Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            for ( int j = 0; j < 3; j++ ) {
                d = u[j] * std::cos(theta) + v[j] * std::sin(theta);
                snprintf(p3[j], 24, globalFloatFormat, vertexContext->p[j] + maxRad * d);
                snprintf(p4[j], 24, globalFloatFormat, vertexContext->p[j] + minRad * d);
            }
            rv = mgfHandle(MGF_ENTITY_VERTEX, 2, v3Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_POINT, 4, p3Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_VERTEX, 2, v4Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_POINT, 4, p4Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_FACE, 5, faceEntity, context);
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
mgfEntityCone(int ac, char **av, MgfContext *context)
{
    char p3[3][24];
    char p4[3][24];
    char n3[3][24];
    char n4[3][24];
    char *v1Entity[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_cv1", (char *)"="};
    char *v2Entity[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_cv2", (char *)"=", (char *)"_cv3"};
    char *v3Entity[4] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_cv3", (char *)"="};
    char *p3Entity[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_POINT], p3[0], p3[1], p3[2]};
    char *n3Entity[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_NORMAL], n3[0], n3[1], n3[2]};
    char *v4Entity[4] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_cv4", (char *)"="};
    char *p4Entity[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_POINT], p4[0], p4[1], p4[2]};
    char *n4Entity[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_NORMAL], n4[0], n4[1], n4[2]};
    char *faceEntity[6] = {GLOBAL_mgf_entityNames[MGF_ENTITY_FACE], (char *)"_cv1", (char *)"_cv2", (char *)"_cv3", (char *)"_cv4"};
    char *v1n;
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
    cv1 = getNamedVertex(av[1]);
    cv2 = getNamedVertex(av[3]);
    if ( cv1 == nullptr || cv2 == nullptr) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    v1n = av[1];
    if ( !isFloatWords(av[2]) || !isFloatWords(av[4]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }

    // Set up (radius1, radius2)
    double radius1 = strtod(av[2], nullptr);
    round0(radius1);
    double radius2 = strtod(av[4], nullptr);
    round0(radius2);

    if ( radius1 == 0.0 ) {
        if ( radius2 == 0.0 ) {
            return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
    } else if ( radius2 != 0.0 ) {
        if ( (radius1 < 0.0) ^ (radius2 < 0.0) ) {
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

    for ( int j = 0; j < 3; j++ ) {
        w[j] = cv1->p[j] - cv2->p[j];
    }
    d = normalize(w);
    if ( d == 0.0 ) {
        // TODO: Review floating point comparisons vs EPSILON
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    n1off = n2off = (radius2 - radius1) / d;
    if ( globalWarpConeEnds ) {
        // Hack for mgfEntitySphere and mgfEntityTorus
        d = std::atan(n2off) - (M_PI / 4) / GLOBAL_mgf_divisionsPerQuarterCircle;
        if ( d <= -M_PI / 2 + EPSILON ) {
            n2off = -FLOAT_HUGE;
        } else {
            n2off = std::tan(d);
        }
    }

    VECTOR3Dd u;
    VECTOR3Dd v;
    mgfMakeAxes(u, v, w);
    for ( int j = 0; j < 3; j++ ) {
        snprintf(p3[j], 24, globalFloatFormat, cv2->p[j] + radius2 * u[j]);
        if ( n2off <= -FLOAT_HUGE) {
            snprintf(n3[j], 24, globalFloatFormat, -w[j]);
        } else {
            snprintf(n3[j], 24, globalFloatFormat, u[j] + w[j] * n2off);
        }
    }
    rv = mgfHandle(MGF_ENTITY_VERTEX, 3, v3Entity, context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    rv = mgfHandle(MGF_ENTITY_POINT, 4, p3Entity, context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    rv = mgfHandle(MGF_ENTITY_NORMAL, 4, n3Entity, context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    if ( radius1 == 0.0 ) {
        // TODO: Review floating point comparisons vs EPSILON
        // Triangles
        v1Entity[3] = v1n;
        rv = mgfHandle(MGF_ENTITY_VERTEX, 4, v1Entity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( int j = 0; j < 3; j++ ) {
            snprintf(n4[j], 24, globalFloatFormat, w[j]);
        }
        rv = mgfHandle(MGF_ENTITY_NORMAL, 4, n4Entity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( int i = 1; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
            theta = sign * i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
            rv = mgfHandle(MGF_ENTITY_VERTEX, 4, v2Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            for ( int j = 0; j < 3; j++ ) {
                d = u[j] * std::cos(theta) + v[j] * std::sin(theta);
                snprintf(p3[j], 24, globalFloatFormat, cv2->p[j] + radius2 * d);
                if ( n2off > -FLOAT_HUGE) {
                    snprintf(n3[j], 24, globalFloatFormat, d + w[j] * n2off);
                }
            }
            rv = mgfHandle(MGF_ENTITY_VERTEX, 2, v3Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_POINT, 4, p3Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_NORMAL, 4, n3Entity, context);
            if ( n2off > -FLOAT_HUGE && rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_FACE, 4, faceEntity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
        }
    } else {
        // Quads
        v1Entity[3] = (char *)"_cv4";
        if ( globalWarpConeEnds ) {
            // Hack for mgfEntitySphere and mgfEntityTorus
            d = std::atan(n1off) + (M_PI / 4) / GLOBAL_mgf_divisionsPerQuarterCircle;
            if ( d >= M_PI / 2 - EPSILON) {
                n1off = FLOAT_HUGE;
            } else {
                n1off = std::tan(std::atan(n1off) + (M_PI / 4) / GLOBAL_mgf_divisionsPerQuarterCircle);
            }
        }
        for ( int j = 0; j < 3; j++ ) {
            snprintf(p4[j], 24, globalFloatFormat, cv1->p[j] + radius1 * u[j]);
            if ( n1off >= FLOAT_HUGE) {
                snprintf(n4[j], 24, globalFloatFormat, w[j]);
            } else {
                snprintf(n4[j], 24, globalFloatFormat, u[j] + w[j] * n1off);
            }
        }
        rv = mgfHandle(MGF_ENTITY_VERTEX, 3, v4Entity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        rv = mgfHandle(MGF_ENTITY_POINT, 4, p4Entity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        rv = mgfHandle(MGF_ENTITY_NORMAL, 4, n4Entity, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( int i = 1; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
            theta = sign * i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
            rv = mgfHandle(MGF_ENTITY_VERTEX, 4, v1Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_VERTEX, 4, v2Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            for ( int j = 0; j < 3; j++ ) {
                d = u[j] * std::cos(theta) + v[j] * std::sin(theta);
                snprintf(p3[j], 24, globalFloatFormat, cv2->p[j] + radius2 * d);
                if ( n2off > -FLOAT_HUGE) {
                    snprintf(n3[j], 24, globalFloatFormat, d + w[j] * n2off);
                }
                snprintf(p4[j], 24, globalFloatFormat, cv1->p[j] + radius1 * d);
                if ( n1off < FLOAT_HUGE) {
                    snprintf(n4[j], 24, globalFloatFormat, d + w[j] * n1off);
                }
            }
            rv = mgfHandle(MGF_ENTITY_VERTEX, 2, v3Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_POINT, 4, p3Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_NORMAL, 4, n3Entity, context);
            if ( n2off > -FLOAT_HUGE && rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_VERTEX, 2, v4Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_POINT, 4, p4Entity, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_NORMAL, 4, n4Entity, context);
            if ( n1off < FLOAT_HUGE &&
                 rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_FACE, 5, faceEntity, context);
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
mgfEntityPrism(int ac, char **av, MgfContext *context)
{
    char p[3][24];
    char *vent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], nullptr, (char *)"="};
    char *pent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_POINT], p[0], p[1], p[2]};
    char *zNormal[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_NORMAL], (char *)"0", (char *)"0", (char *)"0"};
    char *newArgV[MGF_MAXIMUM_ARGUMENT_COUNT];
    char nvn[MGF_MAXIMUM_ARGUMENT_COUNT - 1][8];
    double length;
    int hasNormal;
    MgfVertexContext *cv;
    MgfVertexContext *cv0;
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
    if ( length <= EPSILON && length >= -EPSILON ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Compute face normal
    cv0 = getNamedVertex(av[1]);
    if ( cv0 == nullptr ) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    hasNormal = 0;

    VECTOR3Dd norm;

    norm[0] = 0.0;
    norm[1] = 0.0;
    norm[2] = 0.0;

    VECTOR3Dd v1;
    v1[0] = 0.0;
    v1[1] = 0.0;
    v1[2] = 0.0;

    for ( i = 2; i < ac - 1; i++ ) {
        cv = getNamedVertex(av[i]);
        if ( cv == nullptr) {
            return MGF_ERROR_UNDEFINED_REFERENCE;
        }
        hasNormal += !is0Vector(cv->n);

        VECTOR3Dd v2;
        VECTOR3Dd v3;

        v2[0] = cv->p[0] - cv0->p[0];
        v2[1] = cv->p[1] - cv0->p[1];
        v2[2] = cv->p[2] - cv0->p[2];
        floatCrossProduct(v3, v1, v2);
        norm[0] += v3[0];
        norm[1] += v3[1];
        norm[2] += v3[2];
        mgfVertexCopy(v1, v2);
    }
    if ( normalize(norm) == 0.0 ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Create moved vertices
    for ( i = 1; i < ac - 1; i++ ) {
        snprintf(nvn[i - 1], MGF_MAXIMUM_ARGUMENT_COUNT, "_pv%d", i);
        vent[1] = nvn[i - 1];
        vent[3] = av[i];
        rv = mgfHandle(MGF_ENTITY_VERTEX, 4, vent, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        cv = getNamedVertex(av[i]); // Checked above
        for ( int j = 0; j < 3; j++ ) {
            snprintf(p[j], 24, globalFloatFormat, cv->p[j] - length * norm[j]);
        }
        rv = mgfHandle(MGF_ENTITY_POINT, 4, pent, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
    }

    // Make faces
    newArgV[0] = GLOBAL_mgf_entityNames[MGF_ENTITY_FACE];
    // Do the side faces
    newArgV[5] = nullptr;
    newArgV[3] = av[ac - 2];
    newArgV[4] = nvn[ac - 3];
    for ( i = 1; i < ac - 1; i++ ) {
        newArgV[1] = nvn[i - 1];
        newArgV[2] = av[i];
        rv = mgfHandle(MGF_ENTITY_FACE, 5, newArgV, context);
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
            rv = mgfHandle(MGF_ENTITY_VERTEX, 2, vent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_NORMAL, 4, zNormal, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
        }
        newArgV[ac - 1 - i] = nvn[i - 1]; // Reverse
    }
    rv = mgfHandle(MGF_ENTITY_FACE, ac - 1, newArgV, context);
    if ( rv != MGF_OK ) {
        return rv;
    }

    // Do bottom face
    if ( hasNormal != 0 ) {
        for ( i = 1; i < ac - 1; i++ ) {
            vent[1] = nvn[i - 1];
            vent[3] = av[i];
            rv = mgfHandle(MGF_ENTITY_VERTEX, 4, vent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_NORMAL, 4, zNormal, context);
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
    rv = mgfHandle(MGF_ENTITY_FACE, i, newArgV, context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    return MGF_OK;
}

/**
Replace face+holes with single contour
*/
int
mgfEntityFaceWithHoles(int ac, char **av, MgfContext *context)
{
    char *newArgV[MGF_MAXIMUM_ARGUMENT_COUNT];
    int lastP = 0;

    newArgV[0] = GLOBAL_mgf_entityNames[MGF_ENTITY_FACE];
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
            for ( j = i + 1; j < ac - 1 && av[j + 1][0] != '-'; j++ ) {
            }
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
    return mgfHandle(MGF_ENTITY_FACE, i, newArgV, context);
}

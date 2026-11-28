#include <stdlib.h>

#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/environment/geometry/volume/Cone.h"
#include "vsdk/toolkit/environment/geometry/volume/Prism.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/Torus.h"
#include "vsdk/toolkit/environment/geometry/volume/Tube.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/io/context/ParseErrorContext.h"
#include "vsdk/toolkit/io/context/ReaderContext.h"
#include "vsdk/toolkit/io/context/TokenValidationContext.h"
#include "vsdk/toolkit/io/mgf/MgfBoundaryFaceEmitter.h"
#include "vsdk/toolkit/io/mgf/MgfEntityControl.h"
#include "vsdk/toolkit/io/mgf/MgfSurfaceNormalModel.h"
#include "vsdk/toolkit/io/mgf/MgfTessellationMath.h"
#include "vsdk/toolkit/io/mgf/MgfVertexFaceEntitySupport.h"
#include "vsdk/toolkit/io/mgf/MgfVitralPrimitiveTessellator.h"

int
MgfVitralPrimitiveTessellator::circleDivisions(const ParseRuntimeContext *context) {
    return 4 * Math::max(1, context->numberOfQuarterCircleDivisions);
}

/**
sph center radius
*/
int
MgfVitralPrimitiveTessellator::handleSphere(int argumentCount, const char **argumentValues, ParseRuntimeContext *context) {
    if ( argumentCount != 3 ) {
        return MGF_ERRR_WRNG_NUM_O_ARGMN;
    }
    const VertexContext *center = MgfVertexFaceEntitySupport::getNamedVertex(argumentValues[1], context);
    if ( center == NULL ) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( !TokenValidationContext::isFloat(argumentValues[2]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    double radius = strtod(argumentValues[2], NULL);
    if ( Math::abs(radius) <= VSDK::EPSILON ) {
        return MGF_ERRR_ILLGL_ARGMN_VAL;
    }

    // As the classic tessellation, the axis of the sphere is +Z
    Vector3Dd u;
    Vector3Dd v;
    Vector3Dd w(0.0, 0.0, 1.0);
    MgfTessellationMath::mgfMakeAxes(&u, &v, &w, VSDK::EPSILON);

    int n = circleDivisions(context);
    Sphere sphere(Math::abs(radius));
    PolyhedralBoundedSolid *solid = sphere.exportToPolyhedralBoundedSolid(n, n / 2, true);
    MgfBoundaryFaceEmitter emitter(center->p, u, v, w, radius < 0.0);
    int errorCode = emitter.emitSolid(solid, MgfSurfaceNormalModel::sphere(), context);
    delete solid;
    return errorCode;
}

/**
cyl bottomCenter radius topCenter: a cone with equal radii
*/
int
MgfVitralPrimitiveTessellator::handleCylinder(int argumentCount, const char **argumentValues, ParseRuntimeContext *context) {
    const char *coneArguments[6] = {
        context->entityNames[CONE],
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    };

    if ( argumentCount != 4 ) {
        return MGF_ERRR_WRNG_NUM_O_ARGMN;
    }
    coneArguments[1] = argumentValues[1];
    coneArguments[2] = argumentValues[2];
    coneArguments[3] = argumentValues[3];
    coneArguments[4] = argumentValues[2];
    return handleCone(5, coneArguments, context);
}

/**
cone center1 radius1 center2 radius2: the open lateral surface of a cone.
Radii of opposite signs are illegal, one radius may be 0 (the apex) and
negative radii turn the surface inwards
*/
int
MgfVitralPrimitiveTessellator::handleCone(int argumentCount, const char **argumentValues, ParseRuntimeContext *context) {
    if ( argumentCount != 5 ) {
        return MGF_ERRR_WRNG_NUM_O_ARGMN;
    }
    const VertexContext *v1Context = MgfVertexFaceEntitySupport::getNamedVertex(argumentValues[1], context);
    const VertexContext *v2Context = MgfVertexFaceEntitySupport::getNamedVertex(argumentValues[3], context);
    if ( v1Context == NULL || v2Context == NULL ) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( !TokenValidationContext::isFloat(argumentValues[2]) || !TokenValidationContext::isFloat(argumentValues[4]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    double radius1 = strtod(argumentValues[2], NULL);
    Numeric::roundDeltaToZero(radius1, VSDK::EPSILON);
    double radius2 = strtod(argumentValues[4], NULL);
    Numeric::roundDeltaToZero(radius2, VSDK::EPSILON);

    if ( radius1 == 0.0 && radius2 == 0.0 ) {
        return MGF_ERRR_ILLGL_ARGMN_VAL;
    }
    if ( radius1 != 0.0 && radius2 != 0.0 && ((radius1 < 0.0) != (radius2 < 0.0)) ) {
        return MGF_ERRR_ILLGL_ARGMN_VAL;
    }

    // The base of the VITRAL cone is the end with the non zero radius,
    // the classic tessellation took the end 2 when it could
    if ( radius2 == 0.0 ) {
        return emitCone(v1Context->p, radius1, v2Context->p, radius2, context);
    }
    return emitCone(v2Context->p, radius2, v1Context->p, radius1, context);
}

int
MgfVitralPrimitiveTessellator::emitCone(
    const Vector3Dd &bottomCenter,
    double bottomRadius,
    const Vector3Dd &topCenter,
    double topRadius,
    ParseRuntimeContext *context)
{
    Vector3Dd w(topCenter.x() - bottomCenter.x(), topCenter.y() - bottomCenter.y(), topCenter.z() - bottomCenter.z());
    double height = MgfTessellationMath::normalizeAndGivePreviousNorm(&w, VSDK::EPSILON);
    if ( Numeric::doubleEqual(height, 0.0, VSDK::EPSILON) ) {
        return MGF_ERRR_ILLGL_ARGMN_VAL;
    }
    Vector3Dd u;
    Vector3Dd v;
    MgfTessellationMath::mgfMakeAxes(&u, &v, &w, VSDK::EPSILON);

    bool inwards = bottomRadius < 0.0;
    double bottom = Math::abs(bottomRadius);
    double top = Math::abs(topRadius);
    Cone cone(bottom, top, height);
    PolyhedralBoundedSolid *solid = cone.exportToPolyhedralBoundedSolid(circleDivisions(context), 1, false);
    MgfBoundaryFaceEmitter emitter(bottomCenter, u, v, w, inwards);
    int errorCode = emitter.emitSolid(solid, MgfSurfaceNormalModel::coneLamina(bottom, top, height), context);
    delete solid;
    return errorCode;
}

/**
torus center minorRadius maxRadius: the center vertex gives the axis as its
normal. The distance from the axis goes from the first radius to the second
one; both negative turn the surface inwards
*/
int
MgfVitralPrimitiveTessellator::handleTorus(int argumentCount, const char **argumentValues, ParseRuntimeContext *context) {
    if ( argumentCount != 4 ) {
        return MGF_ERRR_WRNG_NUM_O_ARGMN;
    }
    const VertexContext *center = MgfVertexFaceEntitySupport::getNamedVertex(argumentValues[1], context);
    if ( center == NULL ) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( MgfTessellationMath::isNull(center->n, VSDK::EPSILON) ) {
        return MGF_ERRR_ILLGL_ARGMN_VAL;
    }
    if ( !TokenValidationContext::isFloat(argumentValues[2]) || !TokenValidationContext::isFloat(argumentValues[3]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    double minRadius = strtod(argumentValues[2], NULL);
    Numeric::roundDeltaToZero(minRadius, VSDK::EPSILON);
    double maxRadius = strtod(argumentValues[3], NULL);
    if ( minRadius == 0.0 || (minRadius > 0.0 ? maxRadius - minRadius : minRadius - maxRadius) <= 0.0 ) {
        return MGF_ERRR_ILLGL_ARGMN_VAL;
    }

    Vector3Dd u;
    Vector3Dd v;
    MgfTessellationMath::mgfMakeAxes(&u, &v, &center->n, VSDK::EPSILON);

    double majorRadius = Math::abs(0.5 * (minRadius + maxRadius));
    double tubeRadius = Math::abs(0.5 * (maxRadius - minRadius));
    int n = circleDivisions(context);
    Torus torus(majorRadius, tubeRadius);
    PolyhedralBoundedSolid *solid = torus.exportToPolyhedralBoundedSolid(n, n);
    MgfBoundaryFaceEmitter emitter(center->p, u, v, center->n, minRadius < 0.0);
    int errorCode = emitter.emitSolid(solid, MgfSurfaceNormalModel::torus(majorRadius), context);
    delete solid;
    return errorCode;
}

/**
ring center innerRadius outerRadius: a planar disk (inner radius 0) or
annulus facing the normal of the center vertex
*/
int
MgfVitralPrimitiveTessellator::handleRing(int argumentCount, const char **argumentValues, ParseRuntimeContext *context) {
    if ( argumentCount != 4 ) {
        return MGF_ERRR_WRNG_NUM_O_ARGMN;
    }
    const VertexContext *center = MgfVertexFaceEntitySupport::getNamedVertex(argumentValues[1], context);
    if ( center == NULL ) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( MgfTessellationMath::isNull(center->n, VSDK::EPSILON) ) {
        return MGF_ERRR_ILLGL_ARGMN_VAL;
    }
    if ( !TokenValidationContext::isFloat(argumentValues[2]) || !TokenValidationContext::isFloat(argumentValues[3]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    double minRadius = strtod(argumentValues[2], NULL);
    Numeric::roundDeltaToZero(minRadius, VSDK::EPSILON);
    double maxRadius = strtod(argumentValues[3], NULL);
    if ( minRadius < 0.0 || maxRadius <= minRadius ) {
        return MGF_ERRR_ILLGL_ARGMN_VAL;
    }

    Vector3Dd u;
    Vector3Dd v;
    MgfTessellationMath::mgfMakeAxes(&u, &v, &center->n, VSDK::EPSILON);

    double innerRadius = Numeric::doubleEqual(minRadius, 0.0, VSDK::EPSILON) ? 0.0 : minRadius;
    PolyhedralBoundedSolid *solid = Tube::createWasherLamina(innerRadius, maxRadius, 0.0, circleDivisions(context), true);
    MgfBoundaryFaceEmitter emitter(center->p, u, v, center->n, false);
    int errorCode = emitter.emitSolid(solid, MgfSurfaceNormalModel::flatLamina(), context);
    delete solid;
    return errorCode;
}

/**
prism v1 v2 ... vn length: the polygon v1 .. vn (counterclockwise around its
normal) extruded `length` units against its normal; a negative length
extrudes it along the normal, with the surface facing inwards. The walls
take the normals of the vertices of the polygon, if any of them has one
*/
int
MgfVitralPrimitiveTessellator::handlePrism(int argumentCount, const char **argumentValues, ParseRuntimeContext *context) {
    const int maximumVertices = ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT - 2;
    const VertexContext *polygon[ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT];

    if ( argumentCount < 5 || argumentCount - 2 > maximumVertices ) {
        return MGF_ERRR_WRNG_NUM_O_ARGMN;
    }
    if ( !TokenValidationContext::isFloat(argumentValues[argumentCount - 1]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    double length = strtod(argumentValues[argumentCount - 1], NULL);
    if ( length <= VSDK::EPSILON && length >= -VSDK::EPSILON ) {
        return MGF_ERRR_ILLGL_ARGMN_VAL;
    }

    // Normal of the polygon, as the classic tessellation
    int numberOfVertices = argumentCount - 2;
    bool hasNormals = false;
    Vector3Dd normal(0.0, 0.0, 0.0);
    Vector3Dd previous(0.0, 0.0, 0.0);
    for ( int i = 0; i < numberOfVertices; i++ ) {
        polygon[i] = MgfVertexFaceEntitySupport::getNamedVertex(argumentValues[i + 1], context);
        if ( polygon[i] == NULL ) {
            return MGF_ERROR_UNDEFINED_REFERENCE;
        }
        if ( i == 0 ) {
            continue;
        }
        if ( !MgfTessellationMath::isNull(polygon[i]->n, VSDK::EPSILON) ) {
            hasNormals = true;
        }
        Vector3Dd current = polygon[i]->p.subtract(polygon[0]->p);
        normal = normal.add(previous.crossProduct(current));
        previous = current;
    }
    if ( MgfTessellationMath::normalizeAndGivePreviousNorm(&normal, VSDK::EPSILON) == 0.0 ) {
        return MGF_ERRR_ILLGL_ARGMN_VAL;
    }

    // Local frame: the polygon on z = 0, extruded along +Z
    Vector3Dd w = length > 0.0 ? normal.multiply(-1.0) : normal;
    Vector3Dd u;
    Vector3Dd v;
    MgfTessellationMath::mgfMakeAxes(&u, &v, &w, VSDK::EPSILON);
    MgfBoundaryFaceEmitter emitter(polygon[0]->p, u, v, w, length < 0.0);

    ArrayList<Vector2Dd> base;
    for ( int i = 0; i < numberOfVertices; i++ ) {
        Vector3Dd d = polygon[i]->p.subtract(polygon[0]->p);
        base.add(Vector2Dd(d.dotProduct(u), d.dotProduct(v)));
    }
    Prism prism(base, Math::abs(length));
    if ( !prism.hasValidParameters() ) {
        MgfEntityControl::doWarning("prism base is not a simple polygon", context);
        return MGF_ERRR_ILLGL_ARGMN_VAL;
    }
    PolyhedralBoundedSolid *solid = prism.exportToPolyhedralBoundedSolid();

    Vector3Dd points[ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT];
    Vector3Dd normals[ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT];
    int errorCode = MGF_OK;
    for ( long f = 0; f < solid->getPolygonsList().size() && errorCode == MGF_OK; f++ ) {
        int faceSize = MgfBoundaryFaceEmitter::faceVertices(solid, f, points, maximumVertices);
        if ( faceSize < 3 ) {
            errorCode = MGF_ERRR_ILLGL_ARGMN_VAL;
            break;
        }
        Vector3Dd faceNormal = MgfBoundaryFaceEmitter::polygonNormal(faceSize, points);
        bool wall = Math::abs(faceNormal.z()) < 0.5 * faceNormal.length();
        const Vector3Dd *faceNormals = NULL;
        if ( hasNormals && wall ) {
            // Each wall vertex takes the normal of its polygon vertex
            for ( int k = 0; k < faceSize; k++ ) {
                int closest = 0;
                double closestDistance = -1.0;
                for ( int i = 0; i < numberOfVertices; i++ ) {
                    double dx = points[k].x() - base[i].x;
                    double dy = points[k].y() - base[i].y;
                    double distance = dx * dx + dy * dy;
                    if ( closestDistance < 0.0 || distance < closestDistance ) {
                        closest = i;
                        closestDistance = distance;
                    }
                }
                // The classic tessellation kept them as given, also when
                // the faces are reversed (and so their normals negated)
                normals[k] = length < 0.0 ? polygon[closest]->n.multiply(-1.0) : polygon[closest]->n;
            }
            faceNormals = normals;
        }
        errorCode = emitter.emitFace(faceSize, points, faceNormals, context);
    }
    delete solid;
    return errorCode;
}

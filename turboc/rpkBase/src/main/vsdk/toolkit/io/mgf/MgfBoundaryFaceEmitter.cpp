#include "java/util/Formatter.h"
#include "vsdk/toolkit/common/dataStructures/CircularArrayList.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
#include "vsdk/toolkit/io/context/ParseErrorContext.h"
#include "vsdk/toolkit/io/context/ReaderContext.h"
#include "vsdk/toolkit/io/mgf/MgfEntityControl.h"
#include "vsdk/toolkit/io/mgf/MgfTessellationMath.h"
#include "vsdk/toolkit/io/mgf/MgfBoundaryFaceEmitter.h"

MgfBoundaryFaceEmitter::MgfBoundaryFaceEmitter(
    const Vector3Dd &origin,
    const Vector3Dd &u,
    const Vector3Dd &v,
    const Vector3Dd &w,
    bool reversed):
    origin(origin),
    u(u),
    v(v),
    w(w),
    reversed(reversed)
{
}

Vector3Dd
MgfBoundaryFaceEmitter::toMgfPoint(const Vector3Dd &localPoint) const {
    return Vector3Dd(
        origin.x() + localPoint.x() * u.x() + localPoint.y() * v.x() + localPoint.z() * w.x(),
        origin.y() + localPoint.x() * u.y() + localPoint.y() * v.y() + localPoint.z() * w.y(),
        origin.z() + localPoint.x() * u.z() + localPoint.y() * v.z() + localPoint.z() * w.z());
}

Vector3Dd
MgfBoundaryFaceEmitter::toMgfDirection(const Vector3Dd &localDirection) const {
    return Vector3Dd(
        localDirection.x() * u.x() + localDirection.y() * v.x() + localDirection.z() * w.x(),
        localDirection.x() * u.y() + localDirection.y() * v.y() + localDirection.z() * w.y(),
        localDirection.x() * u.z() + localDirection.y() * v.z() + localDirection.z() * w.z());
}

int
MgfBoundaryFaceEmitter::emitFace(
    int numberOfVertices,
    const Vector3Dd *localPoints,
    const Vector3Dd *mgfNormals,
    ParseRuntimeContext *context) const
{
    char names[ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT - 1][MgfTessellationMath::MGF_PV_SIZE];
    char p[3][MgfTessellationMath::MGF_PV_SIZE];
    char n[3][MgfTessellationMath::MGF_PV_SIZE];
    const char *vertexEntity[4] = {context->entityNames[VERTEX], NULL, "=", NULL};
    const char *pointEntity[5] = {context->entityNames[MGF_POINT], p[0], p[1], p[2], NULL};
    const char *normalEntity[5] = {context->entityNames[MGF_NORMAL], n[0], n[1], n[2], NULL};
    const char *faceEntity[ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT];

    if ( numberOfVertices < 3 || numberOfVertices > ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT - 2 ) {
        return MGF_ERRR_ILLGL_ARGMN_VAL;
    }

    faceEntity[0] = context->entityNames[FACE];
    for ( int i = 0; i < numberOfVertices; i++ ) {
        // The vertices of a reversed face are taken backwards
        int k = reversed ? numberOfVertices - 1 - i : i;
        Formatter::format(names[i], MgfTessellationMath::MGF_PV_SIZE, "_bv%d", i);
        vertexEntity[1] = names[i];
        int errorCode = MgfEntityControl::mgfHandle(VERTEX, 3, vertexEntity, context);
        if ( errorCode != MGF_OK ) {
            return errorCode;
        }

        Vector3Dd point = toMgfPoint(localPoints[k]);
        MgfTessellationMath::formatFloat(p[0], MgfTessellationMath::MGF_PV_SIZE, point.x());
        MgfTessellationMath::formatFloat(p[1], MgfTessellationMath::MGF_PV_SIZE, point.y());
        MgfTessellationMath::formatFloat(p[2], MgfTessellationMath::MGF_PV_SIZE, point.z());
        errorCode = MgfEntityControl::mgfHandle(MGF_POINT, 4, pointEntity, context);
        if ( errorCode != MGF_OK ) {
            return errorCode;
        }

        if ( mgfNormals != NULL && !MgfTessellationMath::isNull(mgfNormals[k], 0.0) ) {
            double sign = reversed ? -1.0 : 1.0;
            MgfTessellationMath::formatFloat(n[0], MgfTessellationMath::MGF_PV_SIZE, sign * mgfNormals[k].x());
            MgfTessellationMath::formatFloat(n[1], MgfTessellationMath::MGF_PV_SIZE, sign * mgfNormals[k].y());
            MgfTessellationMath::formatFloat(n[2], MgfTessellationMath::MGF_PV_SIZE, sign * mgfNormals[k].z());
            errorCode = MgfEntityControl::mgfHandle(MGF_NORMAL, 4, normalEntity, context);
            if ( errorCode != MGF_OK ) {
                return errorCode;
            }
        }
        faceEntity[i + 1] = names[i];
    }
    faceEntity[numberOfVertices + 1] = NULL;
    return MgfEntityControl::mgfHandle(FACE, numberOfVertices + 1, faceEntity, context);
}

int
MgfBoundaryFaceEmitter::faceVertices(
    PolyhedralBoundedSolid *solid,
    long faceIndex,
    Vector3Dd *localPoints,
    int maximumVertices)
{
    _PolyhedralBoundedSolidFace *face = solid->getPolygonsList().get(faceIndex);
    if ( face->boundariesList.size() != 1 ) {
        return -1;
    }
    _PolyhedralBoundedSolidHalfEdge *start = face->boundariesList.get(0)->boundaryStartHalfEdge;
    _PolyhedralBoundedSolidHalfEdge *he = start;
    int count = 0;
    do {
        if ( count >= maximumVertices ) {
            return -1;
        }
        localPoints[count++] = he->startingVertex->position;
        he = he->next();
    } while ( he != start );
    return count;
}

Vector3Dd
MgfBoundaryFaceEmitter::polygonNormal(int numberOfVertices, const Vector3Dd *points) {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    for ( int i = 0; i < numberOfVertices; i++ ) {
        const Vector3Dd &a = points[i];
        const Vector3Dd &b = points[(i + 1) % numberOfVertices];
        x += (a.y() - b.y()) * (a.z() + b.z());
        y += (a.z() - b.z()) * (a.x() + b.x());
        z += (a.x() - b.x()) * (a.y() + b.y());
    }
    return Vector3Dd(x, y, z);
}

int
MgfBoundaryFaceEmitter::emitSolid(
    PolyhedralBoundedSolid *solid,
    const MgfSurfaceNormalModel &model,
    ParseRuntimeContext *context) const
{
    Vector3Dd points[ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT];
    Vector3Dd normals[ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT];

    for ( long i = 0; i < solid->getPolygonsList().size(); i++ ) {
        int numberOfVertices = faceVertices(solid, i, points, ReaderContext::MGF_MAXIMUM_ARGUMENT_COUNT - 2);
        if ( numberOfVertices < 3 ) {
            return MGF_ERRR_ILLGL_ARGMN_VAL;
        }

        Vector3Dd centroid(0.0, 0.0, 0.0);
        for ( int k = 0; k < numberOfVertices; k++ ) {
            centroid = centroid.add(points[k]);
        }
        centroid = centroid.multiply(1.0 / numberOfVertices);
        if ( !model.selectsFace(centroid, polygonNormal(numberOfVertices, points)) ) {
            continue;
        }

        const Vector3Dd *faceNormals = NULL;
        if ( model.hasVertexNormals() ) {
            for ( int k = 0; k < numberOfVertices; k++ ) {
                normals[k] = toMgfDirection(model.vertexNormal(points[k]));
            }
            faceNormals = normals;
        }
        int errorCode = emitFace(numberOfVertices, points, faceNormals, context);
        if ( errorCode != MGF_OK ) {
            return errorCode;
        }
    }
    return MGF_OK;
}

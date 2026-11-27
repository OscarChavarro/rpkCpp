#include "java/lang/Math.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/io/mgf/MgfSurfaceNormalModel.h"

MgfSurfaceNormalModel::MgfSurfaceNormalModel(
    Kind kind,
    double bottomRadius,
    double topRadius,
    double height,
    double majorRadius):
    kind(kind),
    bottomRadius(bottomRadius),
    topRadius(topRadius),
    height(height),
    majorRadius(majorRadius)
{
}

MgfSurfaceNormalModel
MgfSurfaceNormalModel::flatSolid() {
    return MgfSurfaceNormalModel(FLAT_SOLID, 0.0, 0.0, 0.0, 0.0);
}

MgfSurfaceNormalModel
MgfSurfaceNormalModel::flatLamina() {
    return MgfSurfaceNormalModel(FLAT_LAMINA, 0.0, 0.0, 0.0, 0.0);
}

MgfSurfaceNormalModel
MgfSurfaceNormalModel::sphere() {
    return MgfSurfaceNormalModel(SPHERE, 0.0, 0.0, 0.0, 0.0);
}

MgfSurfaceNormalModel
MgfSurfaceNormalModel::coneLamina(double bottomRadius, double topRadius, double height) {
    return MgfSurfaceNormalModel(CONE_LAMINA, bottomRadius, topRadius, height, 0.0);
}

MgfSurfaceNormalModel
MgfSurfaceNormalModel::torus(double majorRadius) {
    return MgfSurfaceNormalModel(TORUS, 0.0, 0.0, 0.0, majorRadius);
}

bool
MgfSurfaceNormalModel::hasVertexNormals() const {
    return kind == SPHERE || kind == CONE_LAMINA || kind == TORUS;
}

Vector3Dd
MgfSurfaceNormalModel::vertexNormal(const Vector3Dd &localPoint) const {
    double radial = Math::sqrt(localPoint.x() * localPoint.x() + localPoint.y() * localPoint.y());

    switch ( kind ) {
        case SPHERE:
            return localPoint;
        case CONE_LAMINA:
            if ( radial <= VSDK::EPSILON * (bottomRadius + topRadius) ) {
                // The apex: the classic tessellation gives it the axis
                return Vector3Dd(0.0, 0.0, 1.0);
            }
            // Radial direction, raised by the slope of the generatrix
            return Vector3Dd(localPoint.x() / radial, localPoint.y() / radial, (bottomRadius - topRadius) / height);
        case TORUS:
            if ( radial <= 0.0 ) {
                return Vector3Dd(0.0, 0.0, 0.0);
            }
            // From the closest point of the core circle
            return Vector3Dd(
                localPoint.x() - majorRadius * localPoint.x() / radial,
                localPoint.y() - majorRadius * localPoint.y() / radial,
                localPoint.z());
        default:
            return Vector3Dd(0.0, 0.0, 0.0);
    }
}

bool
MgfSurfaceNormalModel::selectsFace(const Vector3Dd &localCentroid, const Vector3Dd &localFaceNormal) const {
    switch ( kind ) {
        case FLAT_LAMINA:
            return localFaceNormal.z() > 0.0;
        case CONE_LAMINA:
            return localFaceNormal.dotProduct(vertexNormal(localCentroid)) > 0.0;
        default:
            return true;
    }
}

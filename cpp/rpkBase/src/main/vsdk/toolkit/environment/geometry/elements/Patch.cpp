#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkRayHit.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"

int Patch::patchId = 1;

/**
This routine returns the ID number the next patch would get
*/
int
Patch::getNextId() {
    return patchId;
}

/**
With this routine, the ID number is set that the next patch will get.
Note that patch ID 0 is reserved. The smallest patch ID number should be 1
*/
void
Patch::setNextId(int id) {
    patchId = id;
}

bool
Patch::allVerticesHaveANormal() const {
    int i;

    for ( i = 0; i < numberOfVertices; i++ ) {
        if ( !vertex[i]->normal ) {
            break;
        }
    }
    return i >= numberOfVertices;
}

/**
Point IN Triangle: barycentric parametrisation
*/
inline void
Patch::pointInTriangle(
    const Vector3Dd &v0,
    const Vector3Dd &v1,
    const Vector3Dd &v2,
    const double u,
    const double v,
    Vector3Dd &p)
{
    p = Vector3Dd(v0.x() + u * (v1.x() - v0.x()) + v * (v2.x() - v0.x()), v0.y() + u * (v1.y() - v0.y()) + v * (v2.y() - v0.y()), v0.z() + u * (v1.z() - v0.z()) + v * (v2.z() - v0.z()));
}

/**
Point IN Quadrilateral: bi-linear parametrisation
*/
inline void
Patch::pointInQuadrilateral(
    const Vector3Dd &v0,
    const Vector3Dd &v1,
    const Vector3Dd &v2,
    const Vector3Dd &v3,
    const double u,
    const double v,
    Vector3Dd &p)
{
    double c = u * v;
    double b = u - c;
    double d = v - c;
    p = Vector3Dd(v0.x() + b * (v1.x() - v0.x()) + c * (v2.x() - v0.x()) + d * (v3.x() - v0.x()), v0.y() + b * (v1.y() - v0.y()) + c * (v2.y() - v0.y()) + d * (v3.y() - v0.y()), v0.z() + b * (v1.z() - v0.z()) + c * (v2.z() - v0.z()) + d * (v3.z() - v0.z()));
}

Vector3Dd
Patch::getInterpolatedNormalAtUv(double u, double v) const {
    Vector3Dd localNormal;
    const Vector3Dd *v1 = vertex[0]->normal;
    const Vector3Dd *v2 = vertex[1]->normal;
    const Vector3Dd *v3 = vertex[2]->normal;
    const Vector3Dd *v4;

    switch ( numberOfVertices ) {
        case 3:
            pointInTriangle(*v1, *v2, *v3, static_cast<double>(u), static_cast<double>(v), localNormal);
            break;
        case 4:
            v4 = vertex[3]->normal;
            pointInQuadrilateral(*v1, *v2, *v3, *v4, static_cast<double>(u), static_cast<double>(v), localNormal);
            break;
        default:
            Logger::reportMessage("Patch", Logger::FATAL_ERROR, "PatchNormalAtUV", java::String::format("Invalid number of vertices %d", numberOfVertices));
    }

    localNormal = localNormal.normalized(Numeric::Numeric::EPSILON);
    return localNormal;
}

/**
Adds the patch to the list of patches that share the vertex
*/
void
Patch::connectVertex(RpkVertex *paramVertex) {
    paramVertex->patches->add(this);
}

/**
Adds the patch to the list of patches sharing each vertex
*/
void
Patch::connectVertices() {
    for ( int i = 0; i < numberOfVertices; i++ ) {
        connectVertex(vertex[i]);
    }
}

/**
Return true if patch is virtual
*/
bool
Patch::hasZeroVertices() const {
    return numberOfVertices == 0;
}

/**
Creates a patch structure for a patch with given vertices
*/
Patch::Patch(
    int inNumberOfVertices,
    RpkVertex *v1,
    RpkVertex *v2,
    RpkVertex *v3,
    RpkVertex *v4):
    face(),
    material(),
    id(),
    faceIndex(-1),
    twin(),
    numberOfVertices(static_cast<char>(inNumberOfVertices)),
    vertex{v1, v2, v3, v4}
{
    if ( v1 == nullptr || v2 == nullptr || v3 == nullptr || (inNumberOfVertices == 4 && v4 == nullptr) ) {
        Logger::reportMessage("Patch", Logger::ERROR, "Patch::Patch", "Null vertex!");
        java::System::exit(1);
    }

    // It's sad but it's true
    if ( inNumberOfVertices != 3 && inNumberOfVertices != 4 ) {
        Logger::reportMessage("Patch", Logger::ERROR, "Patch::Patch", "Can only handle quadrilateral or triangular patches");
        java::System::exit(2);
    }

    Statistics::instance().reader.numberOfElements++;
    twin = nullptr;
    id = patchId;
    patchId++;

    material = nullptr;

    // Face geometry: normal, area and Jacobian, midpoint, plane, dominant axis
    const Vector3Dd positions[MAXIMUM_VERTICES_PER_PATCH] = {
        *v1->point,
        *v2->point,
        *v3->point,
        inNumberOfVertices == 4 ? *v4->point : Vector3Dd()
    };
    face = PlanarFace(inNumberOfVertices, positions);
    if ( !face.isValid() ) {
        Logger::reportMessage("Patch", Logger::WARNING, "patchNormal", java::String::format("degenerate patch (id %d)", id));
        Statistics::instance().reader.numberOfElements--;
        Logger::reportMessage("Patch", Logger::ERROR, "Patch::Patch", "Error computing patch normal");
        java::System::exit(3);
    }
    if ( face.getArea() < VSDK::EPSILON * VSDK::EPSILON ) {
        java::System::err.printf("Warning: very small patch id %d area = %g\n", id, face.getArea());
    }

    // Tell the vertices that there's a new Patch using them
    connectVertices();

}

Patch::~Patch() {

}

/**
Computes the patch's bounding box from its vertices, in vertex order. The
vertices do not move after construction, so every call gives the same box;
runtime queries read the scene's FaceBoundsTable, built once from this
*/
void
Patch::computeBounds(RpkAxisAlignedBoundingBox *bounds) const {
    *bounds = RpkAxisAlignedBoundingBox();
    for ( int i = 0; i < numberOfVertices; i++ ) {
        bounds->enlargeToIncludePoint(vertex[i]->point);
    }
}

/**
Computes interpolated (= shading) normal at the point with given parameters
on the patch
*/
Vector3Dd
Patch::interpolatedNormalAtUv(double u, double v) const {
    if ( !allVerticesHaveANormal() ) {
        return face.getNormal();
    }
    return getInterpolatedNormalAtUv(u, v);
}

/**
Computes shading frame at the given uv on the patch.
Computes 'a' interpolated (shading) frame at the uv or point with
given parameters on the patch. The frame is consistent over the
complete patch if the shading normals in the vertices do not differ
too much from the geometric normal. The Z axis is the interpolated
normal The X is determined by Z and the projection of the patch by
the dominant axis (patch->getDominantAxisIndex())
*/
void
Patch::interpolatedFrameAtUv(
    double u, double v, Vector3Dd *X, Vector3Dd *Y, Vector3Dd *Z) const {
    *Z = interpolatedNormalAtUv(u, v);

    if ( X && Y ) {
        double zz = java::Math::sqrt(1 - Z->z() * Z->z());
        if ( zz < VSDK::EPSILON ) {
            *X = Vector3Dd(1.0, 0.0, 0.0);
        } else {
            *X = Vector3Dd(static_cast<double>(Z->y() / zz), static_cast<double>(-Z->x() / zz), 0.0);
        }

        *Y = (*Z).crossProduct(*X); // *Y = (*Z) ^ (*X)
    }
}

/**
Returns texture coordinates determined from vertex texture coordinates and
given u and v bi-linear of barycentric coordinates on the patch
*/
Vector3Dd
Patch::textureCoordAtUv(const double u, const double v) const {
    Vector3Dd texCoord;
    texCoord = Vector3Dd(0.0, 0.0, 0.0);

    const Vector3Dd *t0 = vertex[0]->textureCoordinates;
    const Vector3Dd *t1 = vertex[1]->textureCoordinates;
    const Vector3Dd *t2 = vertex[2]->textureCoordinates;
    const Vector3Dd *t3;
    switch ( numberOfVertices ) {
        case 3:
            if ( !t0 || !t1 || !t2 ) {
                texCoord = Vector3Dd(static_cast<double>(u), static_cast<double>(v), 0.0);
            } else {
                pointInTriangle(*t0, *t1, *t2, static_cast<double>(u), static_cast<double>(v), texCoord);
            }
            break;
        case 4:
            t3 = vertex[3]->textureCoordinates;
            if ( !t0 || !t1 || !t2 || !t3 ) {
                texCoord = Vector3Dd(static_cast<double>(u), static_cast<double>(v), 0.0);
            } else {
                pointInQuadrilateral(*t0, *t1, *t2, *t3, static_cast<double>(u), static_cast<double>(v), texCoord);
            }
            break;
        default:
            Logger::reportMessage("Patch", Logger::FATAL_ERROR, "textureCoordAtUv", java::String::format("Invalid nr of vertices %d", numberOfVertices));
    }
    return texCoord;
}

/**
Ray-patch intersection test, for computing form factors, creating ray-cast
images ... Returns nullptr if the Ray doesn't hit the patch. Fills in the
'hit' record if there is a new hit and returns a pointer to it.
Fills in the distance to the patch in maximumDistance if the patch
is hit. Intersections closer than minimumDistance or further than *maximumDistance are
ignored. The hitFlags determine what information to return about an
intersection and whether front/back facing patches are to be
considered and are described in ray.h.
*/
RpkRayHit *
Patch::intersect(
    const Ray *ray,
    double minimumDistance,
    double *maximumDistance,
    int hitFlags,
    RpkRayHit *hitStore,
    const RayExclusionSet &exclusions) const
{
    RpkRayHit hit;

    if ( exclusions.excludesPatch(this) || exclusions.excludesFace(getFaceIndex()) ) {
        return nullptr;
    }

    // Side, plane hit, hit point and (u, v) in one call into the face
    PlanarFace::RayHit faceHit;
    const bool inside = face.rayHit(
        ray->getOrigin(),
        ray->getDirection(),
        (hitFlags & RayHitFlag::FRONT) != 0,
        (hitFlags & RayHitFlag::BACK) != 0,
        minimumDistance,
        *maximumDistance,
        &faceHit);
    if ( inside ) {
        const double distance = faceHit.getDistance();
        Vector3Dd position(faceHit.getPointX(), faceHit.getPointY(), faceHit.getPointZ());
        hit.setPoint(&position);
        hit.setUv(faceHit.getU(), faceHit.getV());
        hit.setFlags(hit.getFlags()
            | (faceHit.getSide() == PlanarFace::Side::BACK ? RayHitFlag::BACK : RayHitFlag::FRONT)
            | RayHitFlag::UV);
        hit.setPatch(this);
        hit.setMaterial(material);
        const Vector3Dd geometricNormal = face.getNormal();
        hit.setGeometricNormal(&geometricNormal);
        unsigned int newFlags = hit.getFlags()
            | RayHitFlag::PATCH
            | RayHitFlag::POINT
            | RayHitFlag::MATERIAL
            | RayHitFlag::GEOMETRIC_NORMAL
            | RayHitFlag::DISTANCE;
        hit.setFlags(newFlags);
        if ( hitFlags & RayHitFlag::UV && !(hit.getFlags() & RayHitFlag::UV) ) {
            position = hit.getPoint();
            double u;
            double v;
            hit.getPatch()->uv(&position, &u, &v);
            hit.setUv(u, v);
            hit.setPoint(&position);
            newFlags = hit.getFlags() & RayHitFlag::UV;
            hit.setFlags(newFlags);
        }
        *hitStore = hit;
        *maximumDistance = distance;

        return hitStore;
    }

    return nullptr;
}

/**
Given the parameter (u,v) of a point on the patch, this routine
computes the 3D space coordinates of the same point, using barycentric
mapping for a triangle and bi-linear mapping for a quadrilateral.
u and v should be numbers in the range [0,1]. If u+v>1 for a triangle,
(1-u) and (1-v) are used instead
*/
Vector3Dd *
Patch::pointBarycentricMapping(double u, double v, Vector3Dd *point) const {
    if ( hasZeroVertices() ) {
        return nullptr;
    }

    *point = face.pointAt(u, v);
    return point;
}

/**
Like above, except that always a uniform mapping is used (one that
preserves area, with this mapping you'll have more positions in "stretched"
regions of an irregular quadrilateral, irregular quadrilaterals are the
only once for which this routine will yield other positions than the above
routine)
*/
Vector3Dd *
Patch::uniformPoint(double u, double v, Vector3Dd *point) const {
    if ( hasZeroVertices() ) {
        return nullptr;
    }
    *point = face.uniformPointAt(u, v);
    return point;
}

/**
Converts bi-linear coordinates into uniform (area preserving) coordinates,
for an irregular quadrilateral (see PlanarFace::biLinearToUniform)
*/
void
Patch::biLinearToUniform(double *u, double *v) const {
    face.biLinearToUniform(u, v);
}

/**
Computes (u, v) parameters of the point on the patch (barycentric or bi-linear
parametrisation). Returns true if the point is inside the patch and false if
not.
WARNING: The (u, v) coordinates are correctly computed only for positions inside
the patch. For positions outside, they can be garbage!
*/
bool
Patch::uv(const Vector3Dd *point, double *u, double *v) const {
    return face.parameters(*point, u, v);
}

/**
Computes a vertex color for the vertices of the patch
*/
void
Patch::computeVertexColors(const FaceColorTable *faceColors) const {
    for ( int i = 0; i < numberOfVertices; i++ ) {
        vertex[i]->computeColor(faceColors);
    }
}

/**
Returns true if the two patches can "see" each other: P and Q see each
other if at least a part of P is in front of Q and vice versa
*/
bool
Patch::facing(const Patch *other) const {
    return face.facing(other->face);
}

#ifdef RAYTRACING_ENABLED
/**
Like 'uv', but returns uniform coordinates (inverse of uniformPoint())
*/
bool
Patch::uniformUv(const Vector3Dd *point, double *u, double *v) const {
    return face.uniformParameters(*point, u, v);
}

#endif

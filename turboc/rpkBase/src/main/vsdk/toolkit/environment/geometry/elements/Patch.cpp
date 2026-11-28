#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/CoordinateAxis.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/logging/LoggerFormat.h"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkRayHit.h"
#include "vsdk/toolkit/environment/geometry/elements/RayHitFlag.h"

static inline double
vector2DdDeterminant(const Vector2Dd &a, const Vector2Dd &b) {
    return a.x * b.y - a.y * b.x;
}

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
            pointInTriangle(*v1, *v2, *v3, ((double)(u)), ((double)(v)), localNormal);
            break;
        case 4:
            v4 = vertex[3]->normal;
            pointInQuadrilateral(*v1, *v2, *v3, *v4, ((double)(u)), ((double)(v)), localNormal);
            break;
        default:
            Logger::reportMessage("Patch", Logger::FATAL_ERROR, "PatchNormalAtUV", LoggerFormat::format("Invalid number of vertices %d", numberOfVertices));
    }

    localNormal = localNormal.normalized(Numeric::Numeric::EPSILON);
    return localNormal;
}

/**
Looks for solution of the quadratic equation A.u^2 + B.u + C = 0
in the interval [0,1]. There must be exactly one such solution.
Returns true if one such solution is found, or false if the equation
has no real solutions, both solutions are in the interval [0,1] or
none of them is. In case of problems, best guess solution
is returned. Problems seem to be due to numerical inaccuracy
*/
bool
Patch::solveQuadraticUnitInterval(double A, double B, double C, double *x) {
    double D = B * B - 4.0 * A * C;
    double x1;
    double x2;

    if ( A < TOLERANCE && A > -TOLERANCE ) {
        // Degenerate case, solve B*x + C = 0
        x1 = -1.0;
        x2 = -C / B;
    } else {
        if ( D < -TOLERANCE * TOLERANCE ) {
            *x = -B / (2. * A);
            Logger::reportMessage("Patch", Logger::ERROR, "", LoggerFormat::format("Bi-linear->Uniform mapping has negative discriminant D = %g.\nTaking 0 as discriminant and %g as solution.", D, *x));
            return false;
        }

        D = D > TOLERANCE * TOLERANCE ? Math::sqrt(D) : 0.0;
        A = 1.0 / (2.0 * A);
        x1 = (-B + D) * A;
        x2 = (-B - D) * A;

        if ( x1 > -TOLERANCE && x1 < 1.0 + TOLERANCE ) {
            *x = x1;
            if ( x2 > -TOLERANCE && x2 < 1.0 + TOLERANCE ) {
                // Error: Bi-linear->Uniform mapping ambiguous: x1, x2, taking x1 as solution
                return false;
            } else {
                return true;
            }
        }
    }

    if ( x2 > -TOLERANCE && x2 < 1.0 + TOLERANCE ) {
        *x = x2;
        return true;
    } else {
        double d;

        // May happen due to numerical errors chose the root closest to [0,1]
        if ( x1 > 1.0 ) {
            d = x1 - 1.0;
        } else {
            // x1 < 0.0
            d = -x1;
        }
        *x = x1;
        if ( x2 > 1.0 ) {
            if ( x2 - 1.0 < d ) {
                *x = x2;
            }
        } else {
            if ( 0.0 - x2 < d ) {
                *x = x2;
            }
        }

        // Clip it to [0,1]
        if ( *x < 0.0 ) {
            *x = 0.0;
        }
        if ( *x > 1.0 ) {
            *x = 1.0;
        }
        // Error(NULL, "Bi-linear->Uniform mapping has no valid solution: x1=%g, x2=%g, taking %g as solution", x1, x2, *x);
        return false;
    }
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
Compute the jacobian J(u, v) of the coordinate mapping from the unit
square [0, 1] ^ 2 or the standard triangle (0, 0), (1, 0), (0, 1) to the patch.
- For triangles: J(u, v) = the area of the triangle
- For quadrilaterals: J(u, v) = A + B.u + C.v
  the area of the patch = A + (B + C) / 2
  for parallelograms holds that B = C = 0
  the coefficients A, B, C are only stored if B and C are non-zero.
The normal of the patch should have been computed before calling this routine
*/
double
Patch::computeRandomWalkRadiosityArea() {
    const Vector3Dd *p1;
    const Vector3Dd *p2;
    const Vector3Dd *p3;
    const Vector3Dd *p4;
    Vector3Dd d1;
    Vector3Dd d2;
    Vector3Dd d3;
    Vector3Dd d4;
    Vector3Dd cp1;
    Vector3Dd cp2;
    Vector3Dd cp3;
    double a;
    double b;
    double c;

    // Explicit compute the area and jacobian
    switch ( this->numberOfVertices ) {
        case 3:
            // Jacobian J(u,v) for the mapping of the triangle
            // (0,0),(0,1),(1,0) to a triangular patch is constant and equal
            // to the area of the triangle ... so there is no need to store
            // any coefficients explicitly
            this->jacobian = NULL;

            p1 = this->vertex[0]->point;
            p2 = this->vertex[1]->point;
            p3 = this->vertex[2]->point;
            d1 = (*p2).subtract(*p1);
            d2 = (*p3).subtract(*p2);
            cp1 = d1.crossProduct(d2);
            this->area = 0.5 * cp1.length();
            break;
        case 4:
            p1 = this->vertex[0]->point;
            p2 = this->vertex[1]->point;
            p3 = this->vertex[2]->point;
            p4 = this->vertex[3]->point;
            d1 = (*p2).subtract(*p1);
            d2 = (*p3).subtract(*p2);
            d3 = (*p3).subtract(*p4);
            d4 = (*p4).subtract(*p1);
            cp1 = d1.crossProduct(d4);
            cp2 = d1.crossProduct(d3);
            cp3 = d2.crossProduct(d4);
            a = cp1.dotProduct(this->normal);
            b = cp2.dotProduct(this->normal);
            c = cp3.dotProduct(this->normal);

            this->area = a + 0.5 * (b + c);
            if ( this->area < 0.0 ) {
                // May happen if the normal direction and
                // a = -a; vertex order are not consistent
                b = -b;
                c = -c;
                this->area = -this->area;
            }

            // 'b' and 'c' are zero for parallelograms. In that case, the area is equal to
            // 'a', so we don't need to store the coefficients
            if ( Math::abs(b) / this->area < VSDK::EPSILON && Math::abs(c) / this->area < VSDK::EPSILON ) {
                this->jacobian = NULL;
            } else {
                this->jacobian = new Jacobian(a, b, c);
            }
            break;
        default:
            Logger::reportMessage("Patch", Logger::FATAL_ERROR, "computeRandomWalkRadiosityArea", "Can only handle triangular and quadrilateral patches.\n");
            this->jacobian = NULL;
            this->area = 0.0;
    }

    if ( this->area < VSDK::EPSILON * VSDK::EPSILON ) {
        System::err.printf("Warning: very small patch id %d area = %g\n", this->id, this->area);
    }

    return this->area;
}

/**
Computes the mid point of the patch, stores the result in p and
returns a pointer to p
*/
void
Patch::computeMidpoint(Vector3Dd *p) const {
    *p = Vector3Dd(0, 0, 0);
    for ( int i = 0; i < numberOfVertices; i++ ) {
        *p = (*p).add(*(vertex[i]->point));
    }
    *p = (*p).multiplyByInverse(numberOfVertices, Numeric::EPSILON);
}

/**
Computes a certain "width" for the plane, e.g. for co-planar testing
*/
double
Patch::computeTolerance() const {
    // Fill in the vertices in the plane equation + take into account the vertex position tolerance
    double localTolerance = 0.0;
    for ( int i = 0; i < numberOfVertices; i++ ) {
        const Vector3Dd *p = vertex[i]->point;
        double e = Math::abs(normal.dotProduct(*p) + planeConstant) + (Numeric::EPSILON) * p->manhattanLength();
        if ( e > localTolerance ) {
            localTolerance = e;
        }
    }

    return localTolerance;
}

/**
Returns (u, v) coordinates of the point in the triangle
Didier Badouel, Graphics Gems I, p390
*/
bool
Patch::triangleUv(const Vector3Dd *point, Vector2Dd *uv) const {
    double u0;
    double v0;
    double alpha;
    double beta;
    Vector2Dd p0;
    Vector2Dd p1;
    Vector2Dd p2;

    // Project to 2D
    int vertexIndex = 0;
    switch ( index ) {
        case X:
            u0 = vertex[vertexIndex]->point->y();
            v0 = vertex[vertexIndex]->point->z();
            p0 = Vector2Dd(point->y() - u0, point->z() - v0);
            vertexIndex++;
            p1 = Vector2Dd(vertex[vertexIndex]->point->y() - u0, vertex[vertexIndex]->point->z() - v0);
            vertexIndex++;
            p2 = Vector2Dd(vertex[vertexIndex]->point->y() - u0, vertex[vertexIndex]->point->z() - v0);
            break;

        case Y:
            u0 = vertex[vertexIndex]->point->x();
            v0 = vertex[vertexIndex]->point->z();
            p0 = Vector2Dd(point->x() - u0, point->z() - v0);
            vertexIndex++;
            p1 = Vector2Dd(vertex[vertexIndex]->point->x() - u0, vertex[vertexIndex]->point->z() - v0);
            vertexIndex++;
            p2 = Vector2Dd(vertex[vertexIndex]->point->x() - u0, vertex[vertexIndex]->point->z() - v0);
            break;

        case Z:
            u0 = vertex[vertexIndex]->point->x();
            v0 = vertex[vertexIndex]->point->y();
            p0 = Vector2Dd(point->x() - u0, point->y() - v0);
            vertexIndex++;
            p1 = Vector2Dd(vertex[vertexIndex]->point->x() - u0, vertex[vertexIndex]->point->y() - v0);
            vertexIndex++;
            p2 = Vector2Dd(vertex[vertexIndex]->point->x() - u0, vertex[vertexIndex]->point->y() - v0);
            break;

        default:
            break;
    }

    if ( p1.x < -VSDK::EPSILON || p1.x > VSDK::EPSILON ) {
        // p1.u non zero
        beta = (p0.y * p1.x - p0.x * p1.y) / (p2.y * p1.x - p2.x * p1.y);
        if ( beta >= 0.0 && beta <= 1.0 ) {
            alpha = (p0.x - beta * p2.x) / p1.x;
        } else {
            return false;
        }
    } else {
        beta = p0.x / p2.x;
        if ( beta >= 0.0 && beta <= 1.0 ) {
            alpha = (p0.y - beta * p2.y) / p1.y;
        } else {
            return false;
        }
    }
    uv->x = alpha;
    uv->y = beta;
    if ( alpha < 0.0 || (alpha + beta) > 1.0 ) {
        return false;
    }
    return true;
}

/**
Christophe Schlick and Gilles Subrenat (15 May 1994)
"Ray Intersection of Tessellated Surfaces : Quadrangles versus Triangles"
in Graphics Gems V (edited by A. Paeth), Academic Press, pages 232-241
*/
bool
Patch::quadUv(const Patch *patch, const Vector3Dd *point, Vector2Dd *uv) {
    Vector2Dd A; // Projected vertices
    Vector2Dd B;
    Vector2Dd C;
    Vector2Dd D;
    Vector2Dd M; // Projected intersection point
    Vector2Dd AB; // AE = DC - AB = DA - CB
    Vector2Dd BC;
    Vector2Dd CD;
    Vector2Dd AD;
    Vector2Dd AM;
    Vector2Dd AE;
    double u = -1.0; // Parametric coordinates
    double v = -1.0;
    Vector2Dd Vector; // Temporary 2D-vector
    bool isInside = false;

    // Projection on the plane that is most parallel to the facet
    int vertexIndex = 0;
    switch ( patch->index ) {
        case X:
            A = Vector2Dd(patch->vertex[vertexIndex]->point->y(), patch->vertex[vertexIndex]->point->z());
            vertexIndex++;
            B = Vector2Dd(patch->vertex[vertexIndex]->point->y(), patch->vertex[vertexIndex]->point->z());
            vertexIndex++;
            C = Vector2Dd(patch->vertex[vertexIndex]->point->y(), patch->vertex[vertexIndex]->point->z());
            vertexIndex++;
            D = Vector2Dd(patch->vertex[vertexIndex]->point->y(), patch->vertex[vertexIndex]->point->z());
            M = Vector2Dd(point->y(), point->z());
            break;

        case Y:
            A = Vector2Dd(patch->vertex[vertexIndex]->point->x(), patch->vertex[vertexIndex]->point->z());
            vertexIndex++;
            B = Vector2Dd(patch->vertex[vertexIndex]->point->x(), patch->vertex[vertexIndex]->point->z());
            vertexIndex++;
            C = Vector2Dd(patch->vertex[vertexIndex]->point->x(), patch->vertex[vertexIndex]->point->z());
            vertexIndex++;
            D = Vector2Dd(patch->vertex[vertexIndex]->point->x(), patch->vertex[vertexIndex]->point->z());
            M = Vector2Dd(point->x(), point->z());
            break;

        case Z:
            A = Vector2Dd(patch->vertex[vertexIndex]->point->x(), patch->vertex[vertexIndex]->point->y());
            vertexIndex++;
            B = Vector2Dd(patch->vertex[vertexIndex]->point->x(), patch->vertex[vertexIndex]->point->y());
            vertexIndex++;
            C = Vector2Dd(patch->vertex[vertexIndex]->point->x(), patch->vertex[vertexIndex]->point->y());
            vertexIndex++;
            D = Vector2Dd(patch->vertex[vertexIndex]->point->x(), patch->vertex[vertexIndex]->point->y());
            M = Vector2Dd(point->x(), point->y());
            break;

        default:
            break;
    }

    AB = Vector2Dd(B.x - A.x, B.y - A.y);
    BC = Vector2Dd(C.x - B.x, C.y - B.y);
    CD = Vector2Dd(D.x - C.x, D.y - C.y);
    AD = Vector2Dd(D.x - A.x, D.y - A.y);
    AE = Vector2Dd(CD.x + AB.x, CD.y + AB.y);
    AE = Vector2Dd(-AE.x, -AE.y);
    AM = Vector2Dd(M.x - A.x, M.y - A.y);

    double a; // Quadratic equation
    double b;
    double c;

    if ( Math::abs(vector2DdDeterminant(AB, CD)) < VSDK::EPSILON ) {
        // Case AB // CD
        Vector = Vector2Dd(AB.x - CD.x, AB.y - CD.y);
        v = vector2DdDeterminant(AM, Vector) / vector2DdDeterminant(AD, Vector);
        if ( (v >= 0.0) && (v <= 1.0) ) {
            b = vector2DdDeterminant(AB, AD) - vector2DdDeterminant(AM, AE);
            c = vector2DdDeterminant(AM, AD);
            u = Math::abs(b) < VSDK::EPSILON ? -1 : c / b;
            isInside = ((u >= 0.0) && (u <= 1.0));
        }
    } else if ( Math::abs(vector2DdDeterminant(BC, AD)) < VSDK::EPSILON ) {
        // Case AD // BC
        Vector = Vector2Dd(AD.x + BC.x, AD.y + BC.y);
        u = vector2DdDeterminant(AM, Vector) / vector2DdDeterminant(AB, Vector);
        if ( (u >= 0.0) && (u <= 1.0) ) {
            b = vector2DdDeterminant(AD, AB) - vector2DdDeterminant(AM, AE);
            c = vector2DdDeterminant(AM, AB);
            v = Math::abs(b) < VSDK::EPSILON ? -1 : c / b;
            isInside = ((v >= 0.0) && (v <= 1.0));
        }
    } else {
        // General case
        a = vector2DdDeterminant(AB, AE);
        c = -vector2DdDeterminant(AM, AD);
        b = vector2DdDeterminant(AB, AD) - vector2DdDeterminant(AM, AE);
        a = -0.5 / a;
        b *= a;
        c *= (a + a);
        double SqrtDelta = b * b + c;
        if ( SqrtDelta >= 0.0 ) {
            SqrtDelta = Math::sqrt(SqrtDelta);
            u = b - SqrtDelta;
            if ( (u < 0.0) || (u > 1.0) ) {
                // To choose u between 0 and 1
                u = b + SqrtDelta;
            }
            if ( (u >= 0.0) && (u <= 1.0) ) {
                v = AD.x + u * AE.x;
                if ( Math::abs(v) < VSDK::EPSILON ) {
                    v = (AM.y - u * AB.y) / (AD.y + u * AE.y);
                } else {
                    v = (AM.x - u * AB.x) / v;
                }
                isInside = ((v >= 0.0) && (v <= 1.0));
            }
        } else {
            u = v = -1.0;
        }
    }

    uv->x = u;
    uv->y = v;
    uv->x = clipToUnitInterval(uv->x);
    uv->y = clipToUnitInterval(uv->y);

    return isInside;
}

/**
Badouels and Schlicks method from graphics gems: slower, but consumes less storage and computes
(u,v) parameters as a side result
*/
bool
Patch::hitInPatch(RpkRayHit *hit, const Patch *patch) const {
    unsigned int newFlags = hit->getFlags() | UV;
    hit->setFlags(newFlags); // uv parameters computed as a side result
    Vector3Dd position = hit->getPoint();
    Vector2Dd uv;
    bool result = (patch->numberOfVertices == 3)
       ? triangleUv(&position, &uv)
       : quadUv(patch, &position, &uv);
    hit->setUv(&uv);
    return result;
}

/**
Returns a pointer to the normal vector if everything is OK. NULL pointer if degenerate polygon
*/
Vector3Dd *
Patch::patchNormal(const Patch *patch, Vector3Dd *normal) {
    Vector3Dd current;

    *normal = Vector3Dd(0, 0, 0);
    current = (*patch->vertex[patch->numberOfVertices - 1]->point).subtract(*patch->vertex[0]->point);
    for ( int i = 0; i < patch->numberOfVertices; i++ ) {
        Vector3Dd previous = current;
        current = (*patch->vertex[i]->point).subtract(*patch->vertex[0]->point);
        *normal = normal->withX(normal->x() + (previous.y() - current.y()) * (previous.z() + current.z()));
        *normal = normal->withY(normal->y() + (previous.z() - current.z()) * (previous.x() + current.x()));
        *normal = normal->withZ(normal->z() + (previous.x() - current.x()) * (previous.y() + current.y()));
    }

    const double localNorm = normal->length();
    if ( localNorm < VSDK::EPSILON ) {
        Logger::reportMessage("Patch", Logger::WARNING, "patchNormal", LoggerFormat::format("degenerate patch (id %d)", patch->id));
        return NULL;
    }
    *normal = (*normal).multiplyByInverse(localNorm, Numeric::EPSILON);

    return normal;
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
    faceIndex(-1),
    id(),
    twin(),
    vertex(),
    numberOfVertices(),
    boundingBox(),
    normal(),
    planeConstant(),
    tolerance(),
    area(),
    midPoint(),
    jacobian(),
    index(),
    omit(),
    material()
{
    if ( v1 == NULL || v2 == NULL || v3 == NULL || (inNumberOfVertices == 4 && v4 == NULL) ) {
        Logger::reportMessage("Patch", Logger::ERROR, "Patch::Patch", "Null vertex!");
        System::exit(1);
    }

    // It's sad but it's true
    if ( inNumberOfVertices != 3 && inNumberOfVertices != 4 ) {
        Logger::reportMessage("Patch", Logger::ERROR, "Patch::Patch", "Can only handle quadrilateral or triangular patches");
        System::exit(2);
    }

    Statistics::instance().reader.numberOfElements++;
    twin = NULL;
    id = patchId;
    patchId++;

    material = NULL;

    for ( int i = 0; i < MAXIMUM_VERTICES_PER_PATCH; i++ ) {
        vertex[i] = NULL;
    }

    numberOfVertices = ((char)(inNumberOfVertices));
    vertex[0] = v1;
    vertex[1] = v2;
    vertex[2] = v3;
    vertex[3] = v4;

    // A bounding box will be computed the first time it is needed
    boundingBox = NULL;

    // Compute normal
    if ( Patch::patchNormal(this, &normal) == NULL ) {
        Statistics::instance().reader.numberOfElements--;
        Logger::reportMessage("Patch", Logger::ERROR, "Patch::Patch", "Error computing patch normal");
        System::exit(3);
    }

    // Also computes the jacobian
    area = computeRandomWalkRadiosityArea();

    // Patch midpoint
    computeMidpoint(&midPoint);

    // Plane constant
    planeConstant = -normal.dotProduct(midPoint);

    // Plane tolerance
    tolerance = computeTolerance();

    // Dominant part of normal
    index = ((char)(normal.dominantAxis()));

    // Tell the vertices that there's a new Patch using them
    connectVertices();

    omit = false;
}

Patch::~Patch() {

    if ( boundingBox != NULL ) {
        delete boundingBox;
    }
}

/**
Computes a bounding box for the patch. fills it in 'getBoundingBox' and returns
a pointer to 'getBoundingBox'
*/
void
Patch::computeAndGetBoundingBox(RpkAxisAlignedBoundingBox *bounds) {
    computeBoundingBox();
    bounds->copyFrom(boundingBox);
}

void
Patch::computeBoundingBox() {
    if ( boundingBox == NULL ) {
        boundingBox = new RpkAxisAlignedBoundingBox();
        for ( int i = 0; i < numberOfVertices; i++ ) {
            boundingBox->enlargeToIncludePoint(vertex[i]->point);
        }
    }
}

/**
Computes interpolated (= shading) normal at the point with given parameters
on the patch
*/
Vector3Dd
Patch::interpolatedNormalAtUv(double u, double v) const {
    if ( !allVerticesHaveANormal() ) {
        return normal;
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
the dominant axis (patch->index)
*/
void
Patch::interpolatedFrameAtUv(
    double u, double v, Vector3Dd *X, Vector3Dd *Y, Vector3Dd *Z) const {
    *Z = interpolatedNormalAtUv(u, v);

    if ( X && Y ) {
        double zz = Math::sqrt(1 - Z->z() * Z->z());
        if ( zz < VSDK::EPSILON ) {
            *X = Vector3Dd(1.0, 0.0, 0.0);
        } else {
            *X = Vector3Dd(((double)(Z->y() / zz)), ((double)(-Z->x() / zz)), 0.0);
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
                texCoord = Vector3Dd(((double)(u)), ((double)(v)), 0.0);
            } else {
                pointInTriangle(*t0, *t1, *t2, ((double)(u)), ((double)(v)), texCoord);
            }
            break;
        case 4:
            t3 = vertex[3]->textureCoordinates;
            if ( !t0 || !t1 || !t2 || !t3 ) {
                texCoord = Vector3Dd(((double)(u)), ((double)(v)), 0.0);
            } else {
                pointInQuadrilateral(*t0, *t1, *t2, *t3, ((double)(u)), ((double)(v)), texCoord);
            }
            break;
        default:
            Logger::reportMessage("Patch", Logger::FATAL_ERROR, "textureCoordAtUv", LoggerFormat::format("Invalid nr of vertices %d", numberOfVertices));
    }
    return texCoord;
}

/**
Ray-patch intersection test, for computing form factors, creating ray-cast
images ... Returns NULL if the Ray doesn't hit the patch. Fills in the
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
        return NULL;
    }

    double distance = normal.dotProduct(ray->getDirection());
    if ( distance > VSDK::EPSILON ) {
        // Back facing patch
        if ( !(hitFlags & BACK) ) {
            return NULL;
        } else {
            unsigned int newFlags = hit.getFlags() | BACK;
            hit.setFlags(newFlags);
        }
    } else if ( distance < -VSDK::EPSILON ) {
        // Front facing patch
        if ( !(hitFlags & FRONT) ) {
            return NULL;
        } else {
            unsigned int newFlags = hit.getFlags() | FRONT;
            hit.setFlags(newFlags);
        }
    } else {
        // Ray is parallel with the plane of the patch
        return NULL;
    }

    distance = -(normal.dotProduct(ray->getOrigin()) + planeConstant) / distance;

    if ( distance > *maximumDistance || distance < minimumDistance ) {
        // Intersection too far or too close
        return NULL;
    }

    // Intersection point of ray with plane of patch
    Vector3Dd position;
    position = ray->getOrigin().add(ray->getDirection().multiply(((double)(distance))));
    hit.setPoint(&position);

    // Test whether it lays inside or outside the patch
    if ( hitInPatch(&hit, this) ) {
        hit.setPatch(this);
        hit.setMaterial(material);
        hit.setGeometricNormal(&normal);
        unsigned int newFlags = hit.getFlags()
            | PATCH
            | POINT
            | MATERIAL
            | GEOMETRIC_NORMAL
            | DISTANCE;
        hit.setFlags(newFlags);
        if ( hitFlags & UV && !(hit.getFlags() & UV) ) {
            position = hit.getPoint();
            double u;
            double v;
            hit.getPatch()->uv(&position, &u, &v);
            hit.setUv(u, v);
            hit.setPoint(&position);
            newFlags = hit.getFlags() & UV;
            hit.setFlags(newFlags);
        }
        *hitStore = hit;
        *maximumDistance = distance;

        return hitStore;
    }

    return NULL;
}

/**
Converts bi-linear coordinates into uniform (area preserving)
coordinates for a polygon with given jacobian. Uniform coordinates
are such that the area left of the line of positions with u-coordinate 'u',
will be u.A and the area under the line of positions with v-coordinate 'v'
will be v.A
*/
void
Patch::biLinearToUniform(double *u, double *v) const {
    double a = jacobian->A;
    double b = jacobian->B;
    double c = jacobian->C;
    *u = ((a + 0.5 * c) + 0.5 * b * (*u)) * (*u) / area;
    *v = ((a + 0.5 * b) + 0.5 * c * (*v)) * (*v) / area;
}

/**
Converts uniform to bi-linear coordinates
*/
void
Patch::uniformToBiLinear(double *u, double *v) const {
    double a = jacobian->A;
    double b = jacobian->B;
    double c = jacobian->C;
    double A = 0.5 * b / area;
    double B = (a + 0.5 * c) / area;
    double C = -(*u);
    if ( !solveQuadraticUnitInterval(A, B, C, u) ) {
        //Logger::reportMessage("Patch", Logger::ERROR, "", LoggerFormat::format("Tried to solve %g*u^2 + %g*u = %g for patch %d", A, B, -C, id));
        //fprintf(stderr, "Jacobian: %g + %g*u + %g*v\n", a, b, c);
    }

    A = 0.5 * c / area;
    B = (a + 0.5 * b) / area;
    C = -(*v);
    if ( !solveQuadraticUnitInterval(A, B, C, v) ) {
        //Logger::reportMessage("Patch", Logger::ERROR, "", LoggerFormat::format("Tried to solve %g*v^2 + %g*v = %g for patch %d", A, B, -C, id));
        //fprintf(stderr, "Jacobian: %g + %g*u + %g*v\n", a, b, c);
    }
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
        return NULL;
    }

    const Vector3Dd *v1 = vertex[0]->point;
    const Vector3Dd *v2 = vertex[1]->point;
    const Vector3Dd *v3 = vertex[2]->point;

    if ( numberOfVertices == 3 ) {
        if ( u + v > 1.0 ) {
            u = 1.0 - u;
            v = 1.0 - v;
        }
        pointInTriangle(*v1, *v2, *v3, ((double)(u)), ((double)(v)), *point);
    } else if ( numberOfVertices == 4 ) {
        const Vector3Dd *v4 = vertex[3]->point;
        pointInQuadrilateral(*v1, *v2, *v3, *v4, ((double)(u)), ((double)(v)), *point);
    } else {
        Logger::reportMessage("Patch", Logger::FATAL_ERROR, "pointBarycentricMapping", "Can only handle triangular or quadrilateral patches");
    }

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
    if ( jacobian != NULL ) {
        uniformToBiLinear(&u, &v);
    }
    return pointBarycentricMapping(u, v, point);
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
    Vector2Dd uv;
    bool inside = false;

    switch ( numberOfVertices ) {
        case 3:
            inside = triangleUv(point, &uv);
            break;
        case 4:
            inside = quadUv(this, point, &uv);
            break;
        default:
            Logger::reportMessage("Patch", Logger::FATAL_ERROR, "uv", "Can only handle triangular or quadrilateral patches");
    }

    *u = uv.x;
    *v = uv.y;

    return inside;
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
Returns true if P is at least partly in front the plane of Q. Returns false
if P is coplanar with or behind Q. It suffices to test the vertices of P
with respect to the plane of Q
*/
bool
Patch::isAtLeastPartlyInFront(const Patch *other) const {
    for ( int i = 0; i < numberOfVertices; i++ ) {
        const Vector3Dd *vp = vertex[i]->point;
        double ep = other->normal.dotProduct(*vp) + other->planeConstant;
        double localTolerance = other->tolerance + (Numeric::EPSILON) * vp->manhattanLength();
        if ( ep > localTolerance ) {
            // P is at least partly in front of Q
            return true;
        }
    }
    return false; // P is behind or coplanar with Q
}

/**
Returns true if the two patches can "see" each other: P and Q see each
other if at least a part of P is in front of Q and vice versa
*/
bool
Patch::facing(const Patch *other) const {
    return isAtLeastPartlyInFront(other) && other->isAtLeastPartlyInFront(this);
}

#ifdef RAYTRACING_ENABLED

/**
Like 'uv', but returns uniform coordinates (inverse of uniformPoint())
*/
bool
Patch::uniformUv(const Vector3Dd *point, double *u, double *v) const {
    bool inside = uv(point, u, v);
    if ( jacobian != NULL ) {
        biLinearToUniform(u, v);
    }
    return inside;
}

#endif

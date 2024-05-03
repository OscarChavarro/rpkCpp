#include <cstdarg>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "material/statistics.h"
#include "QMC/nied31.h"
#include "skin/Patch.h"

#define TOLERANCE 1e-5

int Patch::globalPatchId = 1;
Patch *Patch::globalExcludedPatches[MAX_EXCLUDED_PATCHES] = {nullptr, nullptr, nullptr, nullptr};

/**
This routine returns the ID number the next patch would get
*/
int
Patch::getNextId() {
    return globalPatchId;
}

/**
With this routine, the ID number is set that the next patch will get.
Note that patch ID 0 is reserved. The smallest patch ID number should be 1
*/
void
Patch::setNextId(int id) {
    globalPatchId = id;
}

bool
Patch::isExcluded() const {
    Patch **excl = globalExcludedPatches;
    // MAX_EXCLUDED_PATCHES tests!
    for ( int i = 0; i < MAX_EXCLUDED_PATCHES; i++ ) {
        if ( *excl == this ) {
            return true;
        }
        excl++;
    }
    return false;
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

Vector3D
Patch::getInterpolatedNormalAtUv(double u, double v) const {
    Vector3D localNormal;
    const Vector3D *v1 = vertex[0]->normal;
    const Vector3D *v2 = vertex[1]->normal;
    const Vector3D *v3 = vertex[2]->normal;
    const Vector3D *v4;

    switch ( numberOfVertices ) {
        case 3:
            vectorPointInTriangle(*v1, *v2, *v3, (float) u, (float) v, localNormal);
            break;
        case 4:
            v4 = vertex[3]->normal;
            vectorPointInQuadrilateral(*v1, *v2, *v3, *v4, (float) u, (float) v, localNormal);
            break;
        default:
            logFatal(-1, "PatchNormalAtUV", "Invalid number of vertices %d", numberOfVertices);
    }

    vectorNormalize(localNormal);
    return localNormal;
}

/**
Looks for solution of the quadratic equation A.u^2 + B.u + C = 0
in the interval [0,1]. There must be exactly one such solution.
Returns true if one such solution is found, or false if the equation
has no real solutions, both solutions are in the interval [0,1] or
none of them is. In case of problems, a best guess solution
is returned. Problems seem to be due to numerical inaccuracy
*/
int
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
            logError(nullptr,
                     "Bi-linear->Uniform mapping has negative discriminant D = %g.\nTaking 0 as discriminant and %g as solution.",
                     D, *x);
            return false;
        }

        D = D > TOLERANCE * TOLERANCE ? std::sqrt(D) : 0.0;
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
        // Error(nullptr, "Bi-linear->Uniform mapping has no valid solution: x1=%g, x2=%g, taking %g as solution", x1, x2, *x);
        return false;
    }
}

/**
Adds the patch to the list of patches that share the vertex
*/
void
Patch::patchConnectVertex(Vertex *paramVertex) {
    paramVertex->patches->add(this);
}

/**
Adds the patch to the list of patches sharing each vertex
*/
void
Patch::patchConnectVertices() {
    for ( int i = 0; i < numberOfVertices; i++ ) {
        patchConnectVertex(vertex[i]);
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
float
Patch::randomWalkRadiosityPatchArea() {
    const Vector3D *p1;
    const Vector3D *p2;
    const Vector3D *p3;
    const Vector3D *p4;
    Vector3D d1;
    Vector3D d2;
    Vector3D d3;
    Vector3D d4;
    Vector3D cp1;
    Vector3D cp2;
    Vector3D cp3;
    float a;
    float b;
    float c;

    // Explicit compute the area and jacobian
    switch ( this->numberOfVertices ) {
        case 3:
            // Jacobian J(u,v) for the mapping of the triangle
            // (0,0),(0,1),(1,0) to a triangular patch is constant and equal
            // to the area of the triangle ... so there is no need to store
            // any coefficients explicitly
            this->jacobian = nullptr;

            p1 = this->vertex[0]->point;
            p2 = this->vertex[1]->point;
            p3 = this->vertex[2]->point;
            vectorSubtract(*p2, *p1, d1);
            vectorSubtract(*p3, *p2, d2);
            vectorCrossProduct(d1, d2, cp1);
            this->area = 0.5f * vectorNorm(cp1);
            break;
        case 4:
            p1 = this->vertex[0]->point;
            p2 = this->vertex[1]->point;
            p3 = this->vertex[2]->point;
            p4 = this->vertex[3]->point;
            vectorSubtract(*p2, *p1, d1);
            vectorSubtract(*p3, *p2, d2);
            vectorSubtract(*p3, *p4, d3);
            vectorSubtract(*p4, *p1, d4);
            vectorCrossProduct(d1, d4, cp1);
            vectorCrossProduct(d1, d3, cp2);
            vectorCrossProduct(d2, d4, cp3);
            a = vectorDotProduct(cp1, this->normal);
            b = vectorDotProduct(cp2, this->normal);
            c = vectorDotProduct(cp3, this->normal);

            this->area = a + 0.5f * (b + c);
            if ( this->area < 0.0 ) {
                // May happen if the normal direction and
                // a = -a; vertex order are not consistent
                b = -b;
                c = -c;
                this->area = -this->area;
            }

            // 'b' and 'c' are zero for parallelograms. In that case, the area is equal to
            // 'a', so we don't need to store the coefficients
            if ( std::fabs(b) / this->area < EPSILON && std::fabs(c) / this->area < EPSILON ) {
                this->jacobian = nullptr;
            } else {
                this->jacobian = jacobianCreate(a, b, c);
            }
            break;
        default:
            logFatal(2, "randomWalkRadiosityPatchArea", "Can only handle triangular and quadrilateral patches.\n");
            this->jacobian = nullptr;
            this->area = 0.0;
    }

    if ( this->area < EPSILON * EPSILON ) {
        fprintf(stderr, "Warning: very small patch id %d area = %g\n", this->id, this->area);
    }

    return this->area;
}

/**
Computes the mid point of the patch, stores the result in p and
returns a pointer to p
*/
Vector3D *
Patch::computeMidpoint(Vector3D *p) {
    p->set(0, 0, 0);
    for ( int i = 0; i < numberOfVertices; i++ ) {
        vectorAdd(*p, *(vertex[i]->point), *p);
    }
    vectorScaleInverse((float) numberOfVertices, *p, *p);

    return p;
}

/**
Computes a certain "width" for the plane, e.g. for co-planar testing
*/
float
Patch::computeTolerance() const {
    // Fill in the vertices in the plane equation + take into account the vertex position tolerance
    float localTolerance = 0.0f;
    for ( int i = 0; i < numberOfVertices; i++ ) {
        const Vector3D *p = vertex[i]->point;
        float e = std::fabs(vectorDotProduct(normal, *p) + planeConstant)
                  + vectorTolerance(*p);
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
Patch::triangleUv(const Vector3D *point, Vector2Dd *uv) {
    double u0;
    double v0;
    double alpha;
    double beta;
    Vertex **v;
    Vector2Dd p0;
    Vector2Dd p1;
    Vector2Dd p2;

    // Project to 2D
    v = vertex;
    switch ( index ) {
        case X_NORMAL:
            u0 = (*v)->point->y;
            v0 = (*v)->point->z;
            vector2DSet(p0, point->y - u0, point->z - v0);
            v++;
            vector2DSet(p1, (*v)->point->y - u0, (*v)->point->z - v0);
            v++;
            vector2DSet(p2, (*v)->point->y - u0, (*v)->point->z - v0);
            break;

        case Y_NORMAL:
            u0 = (*v)->point->x;
            v0 = (*v)->point->z;
            vector2DSet(p0, point->x - u0, point->z - v0);
            v++;
            vector2DSet(p1, (*v)->point->x - u0, (*v)->point->z - v0);
            v++;
            vector2DSet(p2, (*v)->point->x - u0, (*v)->point->z - v0);
            break;

        case Z_NORMAL:
            u0 = (*v)->point->x;
            v0 = (*v)->point->y;
            vector2DSet(p0, point->x - u0, point->y - v0);
            v++;
            vector2DSet(p1, (*v)->point->x - u0, (*v)->point->y - v0);
            v++;
            vector2DSet(p2, (*v)->point->x - u0, (*v)->point->y - v0);
            break;

        default:
            break;
    }

    if ( p1.u < -EPSILON || p1.u > EPSILON ) {
        // p1.u non zero
        beta = (p0.v * p1.u - p0.u * p1.v) / (p2.v * p1.u - p2.u * p1.v);
        if ( beta >= 0. && beta <= 1.0 ) {
            alpha = (p0.u - beta * p2.u) / p1.u;
        } else {
            return false;
        }
    } else {
        beta = p0.u / p2.u;
        if ( beta >= 0. && beta <= 1.0 ) {
            alpha = (p0.v - beta * p2.v) / p1.v;
        } else {
            return false;
        }
    }
    uv->u = alpha;
    uv->v = beta;
    if ( alpha < 0. || (alpha + beta) > 1.0 ) {
        return false;
    }
    return true;
}

/**
Christophe Schlick and Gilles Subrenat (15 May 1994)
"Ray Intersection of Tessellated Surfaces : Quadrangles versus Triangles"
in Graphics Gems V (edited by A. Paeth), Academic Press, pages 232-241
*/
int
Patch::quadUv(Patch *patch, const Vector3D *point, Vector2Dd *uv) {
    Vertex **p;
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
    double a; // For the quadratic equation
    double b;
    double c;
    double SqrtDelta;
    Vector2Dd Vector; // Temporary 2D-vector
    int isInside = false;

    // Projection on the plane that is most parallel to the facet
    p = patch->vertex;
    switch ( patch->index ) {
        case X_NORMAL:
            vector2DSet(A, (*p)->point->y, (*p)->point->z);
            p++;
            vector2DSet(B, (*p)->point->y, (*p)->point->z);
            p++;
            vector2DSet(C, (*p)->point->y, (*p)->point->z);
            p++;
            vector2DSet(D, (*p)->point->y, (*p)->point->z);
            vector2DSet(M, point->y, point->z);
            break;

        case Y_NORMAL:
            vector2DSet(A, (*p)->point->x, (*p)->point->z);
            p++;
            vector2DSet(B, (*p)->point->x, (*p)->point->z);
            p++;
            vector2DSet(C, (*p)->point->x, (*p)->point->z);
            p++;
            vector2DSet(D, (*p)->point->x, (*p)->point->z);
            vector2DSet(M, point->x, point->z);
            break;

        case Z_NORMAL:
            vector2DSet(A, (*p)->point->x, (*p)->point->y);
            p++;
            vector2DSet(B, (*p)->point->x, (*p)->point->y);
            p++;
            vector2DSet(C, (*p)->point->x, (*p)->point->y);
            p++;
            vector2DSet(D, (*p)->point->x, (*p)->point->y);
            vector2DSet(M, point->x, point->y);
            break;

        default:
            break;
    }

    vector2DSubtract(B, A, AB);
    vector2DSubtract(C, B, BC);
    vector2DSubtract(D, C, CD);
    vector2DSubtract(D, A, AD);
    vector2DAdd(CD, AB, AE);
    vector2DNegate(AE);
    vector2DSubtract(M, A, AM);

    if ( std::fabs(vector2DDeterminant(AB, CD)) < EPSILON ) {
        // Case AB // CD
        vector2DSubtract(AB, CD, Vector);
        v = vector2DDeterminant(AM, Vector) / vector2DDeterminant(AD, Vector);
        if ( (v >= 0.0) && (v <= 1.0) ) {
            b = vector2DDeterminant(AB, AD) - vector2DDeterminant(AM, AE);
            c = vector2DDeterminant(AM, AD);
            u = realAbs(b) < EPSILON ? -1 : c / b;
            isInside = ((u >= 0.0) && (u <= 1.0));
        }
    } else if ( std::fabs(vector2DDeterminant(BC, AD)) < EPSILON ) {
        // Case AD // BC
        vector2DAdd(AD, BC, Vector);
        u = vector2DDeterminant(AM, Vector) / vector2DDeterminant(AB, Vector);
        if ( (u >= 0.0) && (u <= 1.0) ) {
            b = vector2DDeterminant(AD, AB) - vector2DDeterminant(AM, AE);
            c = vector2DDeterminant(AM, AB);
            v = realAbs(b) < EPSILON ? -1 : c / b;
            isInside = ((v >= 0.0) && (v <= 1.0));
        }
    } else {
        // General case
        a = vector2DDeterminant(AB, AE);
        c = -vector2DDeterminant(AM, AD);
        b = vector2DDeterminant(AB, AD) - vector2DDeterminant(AM, AE);
        a = -0.5 / a;
        b *= a;
        c *= (a + a);
        SqrtDelta = b * b + c;
        if ( SqrtDelta >= 0.0 ) {
            SqrtDelta = std::sqrt(SqrtDelta);
            u = b - SqrtDelta;
            if ( (u < 0.0) || (u > 1.0) ) {
                // To choose u between 0 and 1
                u = b + SqrtDelta;
            }
            if ( (u >= 0.0) && (u <= 1.0) ) {
                v = AD.u + u * AE.u;
                if ( realAbs(v) < EPSILON ) {
                    v = (AM.v - u * AB.v) / (AD.v + u * AE.v);
                } else {
                    v = (AM.u - u * AB.u) / v;
                }
                isInside = ((v >= 0.0) && (v <= 1.0));
            }
        } else {
            u = v = -1.0;
        }
    }

    uv->u = u;
    uv->v = v;
    uv->u = clipToUnitInterval(uv->u);
    uv->v = clipToUnitInterval(uv->v);

    return isInside;
}

/**
Badouels and Schlicks method from graphics gems: slower, but consumes less storage and computes
(u,v) parameters as a side result
*/
bool
Patch::hitInPatch(RayHit *hit, Patch *patch) {
    hit->flags |= HIT_UV; // uv parameters computed as a side result
    return (patch->numberOfVertices == 3)
           ? triangleUv(&hit->point, &hit->uv)
           : quadUv(patch, &hit->point, &hit->uv);
}

/**
Returns a pointer to the normal vector if everything is OK. nullptr pointer if degenerate polygon
*/
static Vector3D *
patchNormal(const Patch *patch, Vector3D *normal) {
    float norm;
    Vector3D previous;
    Vector3D current;

    normal->set(0, 0, 0);
    vectorSubtract(*patch->vertex[patch->numberOfVertices - 1]->point,
                   *patch->vertex[0]->point, current);
    for ( int i = 0; i < patch->numberOfVertices; i++ ) {
        previous = current;
        vectorSubtract(*patch->vertex[i]->point, *patch->vertex[0]->point, current);
        normal->x += (previous.y - current.y) * (previous.z + current.z);
        normal->y += (previous.z - current.z) * (previous.x + current.x);
        normal->z += (previous.x - current.x) * (previous.y + current.y);
    }

    norm = vectorNorm(*normal);
    if ( norm < EPSILON ) {
        logWarning("patchNormal", "degenerate patch (id %d)", patch->id);
        return nullptr;
    }
    vectorScaleInverse(norm, *normal, *normal);

    return normal;
}

/**
Return true if patch is virtual
*/
int
Patch::hasZeroVertices() const {
    return numberOfVertices == 0;
}

/**
Creates a patch structure for a patch with given vertices
*/
Patch::Patch(
    int inNumberOfVertices,
    Vertex *v1,
    Vertex *v2,
    Vertex *v3,
    Vertex *v4):
    flags(),
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
    directPotential(),
    index(),
    omit(),
    color(),
    radianceData(),
    material()
{
    if ( v1 == nullptr || v2 == nullptr || v3 == nullptr || (inNumberOfVertices == 4 && v4 == nullptr) ) {
        logError("Patch::Patch", "Null vertex!");
        exit(1);
    }

    // It's sad but it's true
    if ( inNumberOfVertices != 3 && inNumberOfVertices != 4 ) {
        logError("Patch::Patch", "Can only handle quadrilateral or triangular patches");
        exit(2);
    }

    GLOBAL_statistics.numberOfElements++;
    twin = nullptr;
    id = globalPatchId;
    globalPatchId++;

    material = nullptr;

    for ( int i = 0; i < MAXIMUM_VERTICES_PER_PATCH; i++ ) {
        vertex[i] = nullptr;
    }

    numberOfVertices = (char)inNumberOfVertices;
    vertex[0] = v1;
    vertex[1] = v2;
    vertex[2] = v3;
    vertex[3] = v4;

    // A bounding box will be computed the first time it is needed
    boundingBox = nullptr;

    // Compute normal
    if ( patchNormal(this, &normal) == nullptr ) {
        GLOBAL_statistics.numberOfElements--;
        logError("Patch::Patch", "Error computing patch normal");
        exit(3);
    }

    // Also computes the jacobian
    area = randomWalkRadiosityPatchArea();

    // Patch midpoint
    computeMidpoint(&midPoint);

    // Plane constant
    planeConstant = -vectorDotProduct(normal, midPoint);

    // Plane tolerance
    tolerance = computeTolerance();

    // Dominant part of normal
    index = (char)vector3DDominantCoord(&normal);

    // Tell the vertices that there's a new Patch using them
    patchConnectVertices();

    directPotential = 0.0;
    color.set(0.0, 0.0, 0.0);

    omit = false;
    flags = 0; // Other flags

    radianceData = nullptr;
}

Patch::~Patch() {
    if ( jacobian != nullptr ) {
        delete jacobian;
    }

    if ( boundingBox != nullptr ) {
        delete boundingBox;
    }

    if ( radianceData != nullptr ) {
        delete radianceData;
    }
}

/**
Computes a bounding box for the patch. fills it in 'getBoundingBox' and returns
a pointer to 'getBoundingBox'
*/
void
Patch::computeAndGetBoundingBox(BoundingBox *bounds) {
    computeBoundingBox();
    bounds->copyFrom(boundingBox);
}

void
Patch::computeBoundingBox() {
    if ( boundingBox == nullptr ) {
        boundingBox = new BoundingBox();
        for ( int i = 0; i < numberOfVertices; i++ ) {
            boundingBox->enlargeToIncludePoint(vertex[i]->point);
        }
    }
}

int
Patch::getNumberOfSamples() const {
    int numberOfSamples = 1;
    if ( bsdfIsTextured(material->bsdf) ) {
        if ( vertex[0]->textureCoordinates == vertex[1]->textureCoordinates &&
             vertex[0]->textureCoordinates == vertex[2]->textureCoordinates &&
             (numberOfVertices == 3 || vertex[0]->textureCoordinates == vertex[3]->textureCoordinates) &&
             vertex[0]->textureCoordinates != nullptr ) {
            // All vertices have same texture coordinates (important special case)
            numberOfSamples = 1;
        } else {
            numberOfSamples = 100;
        }
    }
    return numberOfSamples;
}

/**
Use next function (with PatchListIterate) to close any open files of the patch use for recording
Computes average scattered power and emittance of the Patch
*/
ColorRgb
Patch::averageNormalAlbedo(BSDF_FLAGS components) {
    int numberOfSamples;
    ColorRgb albedo;
    RayHit hit;

    hit.init(this, nullptr, &midPoint, &normal, material, 0.0);

    numberOfSamples = getNumberOfSamples();
    albedo.clear();
    for ( int i = 0; i < numberOfSamples; i++ ) {
        ColorRgb sample;
        const unsigned *xi = Nied31(i);
        hit.uv.u = (double) xi[0] * RECIP;
        hit.uv.v = (double) xi[1] * RECIP;
        hit.flags |= HIT_UV;
        pointBarycentricMapping(hit.uv.u, hit.uv.v, &hit.point);
        sample = bsdfScatteredPower(material->bsdf, &hit, &normal, components);
        albedo.add(albedo, sample);
    }
    albedo.scaleInverse((float) numberOfSamples, albedo);

    return albedo;
}

ColorRgb
Patch::averageEmittance(char components) {
    int numberOfSamples;
    ColorRgb emittance;
    RayHit hit;
    hit.init(this, nullptr, &midPoint, &normal, material, 0.0);

    numberOfSamples = getNumberOfSamples();
    emittance.clear();
    for ( int i = 0; i < numberOfSamples; i++ ) {
        ColorRgb sample;
        const unsigned *xi = Nied31(i);
        hit.uv.u = (double) xi[0] * RECIP;
        hit.uv.v = (double) xi[1] * RECIP;
        hit.flags |= HIT_UV;
        pointBarycentricMapping(hit.uv.u, hit.uv.v, &hit.point);
        sample = edfEmittance(material->edf, &hit, components);
        emittance.add(emittance, sample);
    }
    emittance.scaleInverse((float) numberOfSamples, emittance);

    return emittance;
}

/**
Specify up to MAX_EXCLUDED_PATCHES patches not to test for intersections with.
Used to avoid self intersections when raytracing. First argument is number of
patches to include. next arguments are pointers to the patches to exclude.
Call with first parameter == 0 to clear the list
*/
void
Patch::dontIntersect(int n, ...) {
    va_list ap;
    int i;

    if ( n > MAX_EXCLUDED_PATCHES ) {
        logFatal(-1, "dontIntersect", "Too many patches to exclude from intersection tests (maximum is %d)",
                 MAX_EXCLUDED_PATCHES);
        return;
    }

    va_start(ap, n);
    for ( i = 0; i < n; i++ ) {
        globalExcludedPatches[i] = va_arg(ap, Patch *);
    }
    va_end(ap);

    while ( i < MAX_EXCLUDED_PATCHES ) {
        globalExcludedPatches[i++] = nullptr;
    }
}

/**
Computes interpolated (= shading) normal at the point with given parameters
on the patch
*/
Vector3D
Patch::interpolatedNormalAtUv(double u, double v) const {
    if ( !allVerticesHaveANormal() ) {
        return normal;
    }
    return getInterpolatedNormalAtUv(u, v);
}

/**
Computes shading frame at the given uv on the patch.
Computes a interpolated (shading) frame at the uv or point with
given parameters on the patch. The frame is consistent over the
complete patch if the shading normals in the vertices do not differ
too much from the geometric normal. The Z axis is the interpolated
normal The X is determined by Z and the projection of the patch by
the dominant axis (patch->index)
*/
void
Patch::interpolatedFrameAtUv(
    double u, double v, Vector3D *X, Vector3D *Y, Vector3D *Z) const {
    *Z = interpolatedNormalAtUv(u, v);

    if ( X && Y ) {
        double zz = std::sqrt(1 - Z->z * Z->z);
        if ( zz < EPSILON ) {
            X->set(1.0, 0.0, 0.0);
        } else {
            X->set((float) (Z->y / zz), (float) (-Z->x / zz), 0.0f);
        }

        vectorCrossProduct(*Z, *X, *Y); // *Y = (*Z) ^ (*X)
    }
}

/**
Returns texture coordinates determined from vertex texture coordinates and
given u and v bi-linear of barycentric coordinates on the patch
*/
Vector3D
Patch::textureCoordAtUv(const double u, const double v) const {
    const Vector3D *t0;
    const Vector3D *t1;
    const Vector3D *t2;
    const Vector3D *t3;
    Vector3D texCoord;
    texCoord.set(0.0, 0.0, 0.0);

    t0 = vertex[0]->textureCoordinates;
    t1 = vertex[1]->textureCoordinates;
    t2 = vertex[2]->textureCoordinates;
    switch ( numberOfVertices ) {
        case 3:
            if ( !t0 || !t1 || !t2 ) {
                texCoord.set((float) u, (float) v, 0.0f);
            } else {
                vectorPointInTriangle(*t0, *t1, *t2, (float) u, (float) v, texCoord);
            }
            break;
        case 4:
            t3 = vertex[3]->textureCoordinates;
            if ( !t0 || !t1 || !t2 || !t3 ) {
                texCoord.set((float) u, (float) v, 0.0f);
            } else {
                vectorPointInQuadrilateral(*t0, *t1, *t2, *t3, (float) u, (float) v, texCoord);
            }
            break;
        default:
            logFatal(-1, "textureCoordAtUv", "Invalid nr of vertices %d", numberOfVertices);
    }
    return texCoord;
}

/**
Ray-patch intersection test, for computing form factors, creating ray-cast
images ... Returns nullptr if the Ray doesn't hit the patch. Fills in the
'the_hit' hit record if there is a new hit and returns a pointer to it.
Fills in the distance to the patch in maximumDistance if the patch
is hit. Intersections closer than minimumDistance or further than *maximumDistance are
ignored. The hitFlags determine what information to return about an
intersection and whether or not front/back facing patches are to be 
considered and are described in ray.h.
*/
RayHit *
Patch::intersect(
    const Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore)
{
    RayHit hit;
    float dist;

    if ( isExcluded() ) {
        return nullptr;
    }

    dist = vectorDotProduct(normal, ray->dir);
    if ( dist > EPSILON ) {
        // Back facing patch
        if ( !(hitFlags & HIT_BACK) ) {
            return nullptr;
        } else {
            hit.flags = HIT_BACK;
        }
    } else if ( dist < -EPSILON ) {
        // Front facing patch
        if ( !(hitFlags & HIT_FRONT) ) {
            return nullptr;
        } else {
            hit.flags = HIT_FRONT;
        }
    } else {
        // Ray is parallel with the plane of the patch
        return nullptr;
    }

    dist = -(vectorDotProduct(normal, ray->pos) + planeConstant) / dist;

    if ( dist > *maximumDistance || dist < minimumDistance ) {
        // Intersection too far or too close
        return nullptr;
    }

    // Intersection point of ray with plane of patch
    hit.distance = dist;
    vectorSumScaled(ray->pos, dist, ray->dir, hit.point);

    // Test whether it lays inside or outside the patch
    if ( hitInPatch(&hit, this) ) {
        hit.patch = this;
        hit.material = material;
        hit.geometricNormal = normal;
        hit.flags |= HIT_PATCH | HIT_POINT | HIT_MATERIAL | HIT_GEOMETRIC_NORMAL | HIT_DIST;
        if ( hitFlags & HIT_UV && !(hit.flags & HIT_UV) ) {
            hit.patch->uv(&hit.point, &hit.uv.u, &hit.uv.v);
            hit.flags &= HIT_UV;
        }
        *hitStore = hit;
        *maximumDistance = dist;

        return hitStore;
    }

    return nullptr;
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
        logError(nullptr, "Tried to solve %g*u^2 + %g*u = %g for patch %d",
          A, B, -C, id);
        fprintf(stderr, "Jacobian: %g + %g*u + %g*v\n", a, b, c);
    }

    A = 0.5 * c / area;
    B = (a + 0.5 * b) / area;
    C = -(*v);
    if ( !solveQuadraticUnitInterval(A, B, C, v) ) {
        logError(nullptr, "Tried to solve %g*v^2 + %g*v = %g for patch %d",
          A, B, -C, id);
        fprintf(stderr, "Jacobian: %g + %g*u + %g*v\n", a, b, c);
    }
}

/**
Given the parameter (u,v) of a point on the patch, this routine
computes the 3D space coordinates of the same point, using barycentric
mapping for a triangle and bi-linear mapping for a quadrilateral.
u and v should be numbers in the range [0,1]. If u+v>1 for a triangle,
(1-u) and (1-v) are used instead
*/
Vector3D *
Patch::pointBarycentricMapping(double u, double v, Vector3D *point) const {
    if ( hasZeroVertices() ) {
        return nullptr;
    }

    const Vector3D *v1 = vertex[0]->point;
    const Vector3D *v2 = vertex[1]->point;
    const Vector3D *v3 = vertex[2]->point;
    const Vector3D *v4;

    if ( numberOfVertices == 3 ) {
        if ( u + v > 1.0 ) {
            u = 1.0 - u;
            v = 1.0 - v;
        }
        vectorPointInTriangle(*v1, *v2, *v3, (float) u, (float) v, *point);
    } else if ( numberOfVertices == 4 ) {
        v4 = vertex[3]->point;
            vectorPointInQuadrilateral(*v1, *v2, *v3, *v4, (float) u, (float) v, *point);
    } else {
        logFatal(4, "pointBarycentricMapping", "Can only handle triangular or quadrilateral patches");
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
Vector3D *
Patch::uniformPoint(double u, double v, Vector3D *point) const {
    if ( jacobian != nullptr ) {
        uniformToBiLinear(&u, &v);
    }
    return pointBarycentricMapping(u, v, point);
}

/**
Computes (u,v) parameters of the point on the patch (barycentric or bi-linear
parametrisation). Returns true if the point is inside the patch and false if
not.
WARNING: The (u,v) coordinates are correctly computed only for positions inside
the patch. For positions outside, they can be garbage!
*/
int
Patch::uv(const Vector3D *point, double *u, double *v) {
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
            logFatal(3, "uv", "Can only handle triangular or quadrilateral patches");
    }

    *u = uv.u;
    *v = uv.v;

    return inside;
}

/**
Like above, but returns uniform coordinates (inverse of uniformPoint())
*/
int
Patch::uniformUv(const Vector3D *point, double *u, double *v) {
    int inside = uv(point, u, v);
    if ( jacobian != nullptr ) {
        biLinearToUniform(u, v);
    }
    return inside;
}

/**
Computes a vertex color for the vertices of the patch
*/
void
Patch::computeVertexColors() const {
    for ( int i = 0; i < numberOfVertices; i++ ) {
        vertex[i]->computeColor();
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
        const Vector3D *vp = vertex[i]->point;
        double ep = vectorDotProduct(other->normal, *vp) + other->planeConstant;
        double localTolerance = other->tolerance + vectorTolerance(*vp);
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

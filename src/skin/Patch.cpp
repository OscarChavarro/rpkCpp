#include <cstdarg>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "material/statistics.h"
#include "QMC/nied31.h"
#include "skin/radianceinterfaces.h"
#include "skin/Patch.h"

#define TOLERANCE 1e-5
#define MAX_EXCLUDED_PATCHES 4

static inline double
clipToUnitInterval(double x) {
    return x < EPSILON ? EPSILON : (x > (1.0 - EPSILON) ? 1.0 - EPSILON : x);
}

static Patch *excludedPatches[MAX_EXCLUDED_PATCHES] = {nullptr, nullptr, nullptr, nullptr};

/**
A static counter which is increased every time a Patch is created in
order to make a unique Patch id
*/
static int globalPatchId = 1;

Patch::Patch():
    id{}, twin{}, brepData{}, vertex{}, numberOfVertices{}, boundingBox{}, normal{}, planeConstant{},
    tolerance{}, area{}, midpoint{}, jacobian{}, directPotential{}, index{}, omit{}, flags{},
    color{}, radianceData{}, surface{}
{

}

/**
This routine returns the ID number the next patch would get
*/
int
patchGetNextId() {
    return globalPatchId;
}

/**
With this routine, the ID number is set that the next patch will get.
Note that patch ID 0 is reserved. The smallest patch ID number should be 1
*/
void
patchSetNextId(int id) {
    globalPatchId = id;
}

/**
Adds the patch to the list of patches that share the vertex
*/
void
patchConnectVertex(Patch *patch, Vertex *vertex) {
    if ( patch == nullptr) {
        return;
    }
    vertex->patches->add(patch);
}

/**
Adds the patch to the list of patches sharing each vertex
*/
void
patchConnectVertices(Patch *patch) {
    int i;

    for ( i = 0; i < patch->numberOfVertices; i++ ) {
        patchConnectVertex(patch, patch->vertex[i]);
    }
}

/**
Returns a pointer to the normal vector if everything is OK. nullptr pointer if degenerate polygon
*/
Vector3D *
patchNormal(Patch *patch, Vector3D *normal) {
    float norm;
    Vector3Dd prev;
    Vector3Dd cur;
    int i;

    VECTORSET(*normal, 0, 0, 0);
    VECTORSUBTRACT(*patch->vertex[patch->numberOfVertices - 1]->point,
                   *patch->vertex[0]->point, cur);
    for ( i = 0; i < patch->numberOfVertices; i++ ) {
        prev = cur;
        VECTORSUBTRACT(*patch->vertex[i]->point, *patch->vertex[0]->point, cur);
        normal->x += (float)((prev.y - cur.y) * (prev.z + cur.z));
        normal->y += (float)((prev.z - cur.z) * (prev.x + cur.x));
        normal->z += (float)((prev.x - cur.x) * (prev.y + cur.y));
    }

    if ((norm = VECTORNORM(*normal)) < EPSILON ) {
        logWarning("patchNormal", "degenerate patch (id %d)", patch->id);
        return (Vector3D *) nullptr;
    }
    VECTORSCALEINVERSE(norm, *normal, *normal);

    return normal;
}

/**
We also compute the jacobian J(u,v) of the coordinate mapping from the unit
square [0,1]^2 or the standard triangle (0,0),(1,0),(0,1) to the patch.

- for triangles: J(u,v) = the area of the triangle
- for quadrilaterals: J(u,v) = A + B.u + C.v
  the area of the patch = A + (B+C)/2
  for parallelograms holds that B=C=0
  the coefficients A,B,C are only stored if B and C are nonzero.

The normal of the patch should have been computed before calling this routine
*/
float
randomWalkRadiosityPatchArea(Patch *P) {
    Vector3D *p1;
    Vector3D *p2;
    Vector3D *p3;
    Vector3D *p4;
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
    switch ( P->numberOfVertices ) {
        case 3:
            /* jacobian J(u,v) for the mapping of the triangle
             * (0,0),(0,1),(1,0) to a triangular patch is constant and equal
             * to the area of the triangle ... so there is no need to store
             * any coefficients explicitely */
            P->jacobian = (Jacobian *) nullptr;

            p1 = P->vertex[0]->point;
            p2 = P->vertex[1]->point;
            p3 = P->vertex[2]->point;
            VECTORSUBTRACT(*p2, *p1, d1);
            VECTORSUBTRACT(*p3, *p2, d2);
            VECTORCROSSPRODUCT(d1, d2, cp1);
            P->area = 0.5f * VECTORNORM(cp1);
            break;
        case 4:
            p1 = P->vertex[0]->point;
            p2 = P->vertex[1]->point;
            p3 = P->vertex[2]->point;
            p4 = P->vertex[3]->point;
            VECTORSUBTRACT(*p2, *p1, d1);
            VECTORSUBTRACT(*p3, *p2, d2);
            VECTORSUBTRACT(*p3, *p4, d3);
            VECTORSUBTRACT(*p4, *p1, d4);
            VECTORCROSSPRODUCT(d1, d4, cp1);
            VECTORCROSSPRODUCT(d1, d3, cp2);
            VECTORCROSSPRODUCT(d2, d4, cp3);
            a = VECTORDOTPRODUCT(cp1, P->normal);
            b = VECTORDOTPRODUCT(cp2, P->normal);
            c = VECTORDOTPRODUCT(cp3, P->normal);

            P->area = a + 0.5f * (b + c);
            if ( P->area < 0. ) {    /* may happen if the normal direction and
      a = -a;			 * vertex order are not consistent. */
                b = -b;
                c = -c;
                P->area = -P->area;
            }

            // b and c are zero for parallelograms. In that case, the area is equal to
            // a, so we don't need to store the coefficients
            if ( fabs(b) / P->area < EPSILON && fabs(c) / P->area < EPSILON ) {
                P->jacobian = (Jacobian *) nullptr;
            } else {
                P->jacobian = jacobianCreate(a, b, c);
            }

            break;
        default:
            logFatal(2, "randomWalkRadiosityPatchArea", "Can only handle triangular and quadrilateral patches.\n");
            P->jacobian = (Jacobian *) nullptr;
            P->area = 0.0;
    }

    if ( P->area < EPSILON * EPSILON ) {
        fprintf(stderr, "Warning: very small patch id %d area = %g\n", P->id, P->area);
    }

    return P->area;
}

/**
Computes the midpoint of the patch, stores the result in p and
returns a pointer to p
*/
Vector3D *
patchMidpoint(Patch *patch, Vector3D *p) {
    int i;

    VECTORSET(*p, 0, 0, 0);
    for ( i = 0; i < patch->numberOfVertices; i++ ) VECTORADD(*p, *(patch->vertex[i]->point), *p);
    VECTORSCALEINVERSE((float) patch->numberOfVertices, *p, *p);

    return p;
}

/**
Computes a certain "width" for the plane, e.g. for co-planarity testing
*/
float
patchTolerance(Patch *patch) {
    int i;
    float tolerance;

    // Fill in the vertices in the plane equation + take into account the vertex position tolerance
    tolerance = 0.0f;
    for ( i = 0; i < patch->numberOfVertices; i++ ) {
        Vector3D *p = patch->vertex[i]->point;
        float e = (float)std::fabs(VECTORDOTPRODUCT(patch->normal, *p) + patch->planeConstant)
                   + VECTORTOLERANCE(*p);
        if ( e > tolerance ) {
            tolerance = e;
        }
    }

    return tolerance;
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
Patch *
patchCreate(int numberOfVertices, Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4) {
    Patch *patch;

    // This may occur after an error parsing the input file, and some other routine
    // will already complain
    if ( !v1 || !v2 || !v3 || (numberOfVertices == 4 && !v4)) {
        return (Patch *) nullptr;
    }

    // It's sad but it's true
    if ( numberOfVertices != 3 && numberOfVertices != 4 ) {
        logError("patchCreate", "Can only handle quadrilateral or triangular patches");
        return (Patch *) nullptr;
    }

    patch = (Patch *)malloc(sizeof(Patch));
    GLOBAL_statistics_numberOfElements++;
    patch->twin = (Patch *) nullptr;
    patch->id = globalPatchId;
    globalPatchId++;

    patch->surface = (MeshSurface *) nullptr;

    patch->numberOfVertices = (char)numberOfVertices;
    patch->vertex[0] = v1;
    patch->vertex[1] = v2;
    patch->vertex[2] = v3;
    patch->vertex[3] = v4;

    patch->brepData = (BREP_FACE *) nullptr;

    // A bounding box will be computed the first time it is needed
    patch->boundingBox = (float *) nullptr;

    // Compute normal
    if ( !patchNormal(patch, &patch->normal)) {
        free(patch);
        GLOBAL_statistics_numberOfElements--;
        return (Patch *) nullptr;
    }

    // Also computes the jacobian
    patch->area = randomWalkRadiosityPatchArea(patch);

    // Patch midpoint
    patchMidpoint(patch, &patch->midpoint);

    // Plane constant
    patch->planeConstant = -VECTORDOTPRODUCT(patch->normal, patch->midpoint);

    // Plane tolerance
    patch->tolerance = patchTolerance(patch);

    // Dominant part of normal
    patch->index = (char)vector3DDominantCoord(&patch->normal);

    // Tell the vertices that there's a new Patch using them
    patchConnectVertices(patch);

    patch->directPotential = 0.0;
    setRGB(patch->color, 0.0, 0.0, 0.0);

    patch->omit = false;
    patch->flags = 0; // Other flags

    // If we are doing radiance computations, create radiance data for the patch
    patch->radianceData = (GLOBAL_radiance_currentRadianceMethodHandle && GLOBAL_radiance_currentRadianceMethodHandle->CreatePatchData) ?
                          GLOBAL_radiance_currentRadianceMethodHandle->CreatePatchData(patch) : nullptr;

    return patch;
}

/**
Disposes the memory allocated for the patch, does not remove
the pointers to the patch in the vertices of the patch
*/
void
patchDestroy(Patch *patch) {
    if ( GLOBAL_radiance_currentRadianceMethodHandle && patch->radianceData ) {
        if ( GLOBAL_radiance_currentRadianceMethodHandle->DestroyPatchData ) {
            GLOBAL_radiance_currentRadianceMethodHandle->DestroyPatchData(patch);
        }
    }

    if ( patch->boundingBox ) {
        boundsDestroy(patch->boundingBox);
    }

    if ( patch->jacobian ) {
        jacobianDestroy(patch->jacobian);
    }

    if ( patch->brepData ) {
        // Also destroys all contours, and edges if not used in other faces as
        // well. Not the vertices: these are destroyed when destroying the corresponding Vertex
        brepDestroyFace(patch->brepData);
    }

    if ( patch->twin ) {
        patch->twin->twin = (Patch *) nullptr;
    }

    free(patch);
    GLOBAL_statistics_numberOfElements--;
}

/**
Computes a bounding box for the patch. fills it in 'getBoundingBox' and returns
a pointer to 'getBoundingBox'
*/
float *
patchBounds(Patch *patch, float *bounds) {
    if ( !patch->boundingBox ) {
        patch->boundingBox = boundsCreate();
        boundsInit(patch->boundingBox);
        for ( int i = 0; i < patch->numberOfVertices; i++ ) {
            boundsEnlargePoint(patch->boundingBox, patch->vertex[i]->point);
        }
    }

    boundsCopy(patch->boundingBox, bounds);

    return bounds;
}

static int
getNumberOfSamples(Patch *patch) {
    int numberOfSamples = 1;
    if ( bsdfIsTextured(patch->surface->material->bsdf)) {
        if ( patch->vertex[0]->texCoord == patch->vertex[1]->texCoord &&
             patch->vertex[0]->texCoord == patch->vertex[2]->texCoord &&
             (patch->numberOfVertices == 3 || patch->vertex[0]->texCoord == patch->vertex[3]->texCoord) &&
             patch->vertex[0]->texCoord != nullptr) {
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
COLOR
patchAverageNormalAlbedo(Patch *patch, BSDFFLAGS components) {
    int numberOfSamples;
    COLOR albedo;
    RayHit hit;
    hitInit(&hit, patch, nullptr, &patch->midpoint, &patch->normal, patch->surface->material, 0.);

    numberOfSamples = getNumberOfSamples(patch);
    colorClear(albedo);
    for ( int i = 0; i < numberOfSamples; i++ ) {
        COLOR sample;
        unsigned *xi = Nied31(i);
        hit.uv.u = (double) xi[0] * RECIP;
        hit.uv.v = (double) xi[1] * RECIP;
        hit.flags |= HIT_UV;
        patchPoint(patch, hit.uv.u, hit.uv.v, &hit.point);
        sample = bsdfScatteredPower(patch->surface->material->bsdf, &hit, &patch->normal, components);
        colorAdd(albedo, sample, albedo);
    }
    colorScaleInverse((float) numberOfSamples, albedo, albedo);

    return albedo;
}

COLOR
patchAverageEmittance(Patch *patch, XXDFFLAGS components) {
    int numberOfSamples;
    COLOR emittance;
    RayHit hit;
    hitInit(&hit, patch, nullptr, &patch->midpoint, &patch->normal, patch->surface->material, 0.);

    numberOfSamples = getNumberOfSamples(patch);
    colorClear(emittance);
    for ( int i = 0; i < numberOfSamples; i++ ) {
        COLOR sample;
        unsigned *xi = Nied31(i);
        hit.uv.u = (double) xi[0] * RECIP;
        hit.uv.v = (double) xi[1] * RECIP;
        hit.flags |= HIT_UV;
        patchPoint(patch, hit.uv.u, hit.uv.v, &hit.point);
        sample = edfEmittance(patch->surface->material->edf, &hit, components);
        colorAdd(emittance, sample, emittance);
    }
    colorScaleInverse((float) numberOfSamples, emittance, emittance);

    return emittance;
}

void
patchPrintId(FILE *out, Patch *patch) {
    fprintf(out, "%d ", patch->id);
}

void
patchPrint(FILE *out, Patch *patch) {
    COLOR Rd;
    COLOR Ed;

    fprintf(out, "Patch id %d:\n", patch->id);
    fprintf(out, "%d vertices:\n", patch->numberOfVertices);
    for ( int i = 0; i < patch->numberOfVertices; i++ ) {
        vertexPrint(out, patch->vertex[i]);
    }
    fprintf(out, "\n");

    fprintf(out, "midpoint = ");
    vector3DPrint(out, patch->midpoint);
    fprintf(out, ", normal = ");
    vector3DPrint(out, patch->normal);
    fprintf(out, ", plane constant = %g, tolerance = %g\narea = %g, ",
            patch->planeConstant, patch->tolerance, patch->area);
    if ( patch->jacobian ) {
        fprintf(out, "Jacobian: %g %+g*u %+g*v \n",
                patch->jacobian->A, patch->jacobian->B, patch->jacobian->C);
    } else {
        fprintf(out, "No explicitely stored jacobian\n");
    }

    fprintf(out, "sided = %d, material = '%s'\n",
            patch->surface->material->sided,
            patch->surface->material->name ? patch->surface->material->name : "(default)");
    colorClear(Rd);
    if ( patch->surface->material->bsdf ) {
        Rd = patchAverageNormalAlbedo(patch, BRDF_DIFFUSE_COMPONENT);
    }
    colorClear(Ed);
    if ( patch->surface->material->edf ) {
        Ed = patchAverageEmittance(patch, DIFFUSE_COMPONENT);
    }
    fprintf(out, ", reflectance = ");
    Rd.print(out);
    fprintf(out, ", self-emitted luminosity = %g\n", colorLuminance(Ed));

    fprintf(out, "color: ");
    printRGB(out, patch->color);
    fprintf(out, "\n");
    fprintf(out, "directly received potential: %g\n", patch->directPotential);

    fprintf(out, "flags: %s\n",
            patch->isVisible() ? "PATCH_IS_VISIBLE" : "");

    if ( GLOBAL_radiance_currentRadianceMethodHandle ) {
        if ( patch->radianceData && GLOBAL_radiance_currentRadianceMethodHandle->PrintPatchData ) {
            fprintf(out, "Radiance data:\n");
            GLOBAL_radiance_currentRadianceMethodHandle->PrintPatchData(out, patch);
        } else {
            fprintf(out, "No radiance data\n");
        }
    }
}

/**
Specify up to MAX_EXCLUDED_PATCHES patches not to test for intersections with.
Used to avoid self intersections when raytracing. First argument is number of
patches to include. next arguments are pointers to the patches to exclude.
Call with first parameter == 0 to clear the list
*/
void
patchDontIntersect(int n, ...) {
    va_list ap;
    int i;

    if ( n > MAX_EXCLUDED_PATCHES ) {
        logFatal(-1, "patchDontIntersect", "Too many patches to exclude from intersection tests (maximum is %d)",
                 MAX_EXCLUDED_PATCHES);
        return;
    }

    va_start(ap, n);
    for ( i = 0; i < n; i++ ) {
        excludedPatches[i] = va_arg(ap, Patch *);
    }
    va_end(ap);

    while ( i < MAX_EXCLUDED_PATCHES ) {
        excludedPatches[i++] = nullptr;
    }
}

int
isExcludedPatch(Patch *p) {
    Patch **excl = excludedPatches;
    // MAX_EXCLUDED_PATCHES tests!
    return (*excl == p || *++excl == p || *++excl == p || *++excl == p);
}

static int
allVerticesHaveANormal(Patch *patch) {
    int i;

    for ( i = 0; i < patch->numberOfVertices; i++ ) {
        if ( !patch->vertex[i]->normal ) {
            break;
        }
    }
    return i >= patch->numberOfVertices;
}


static Vector3D
getInterpolatedNormalAtUv(Patch *patch, double u, double v) {
    Vector3D normal;
    Vector3D *v1 = patch->vertex[0]->normal;
    Vector3D *v2 = patch->vertex[1]->normal;
    Vector3D *v3 = patch->vertex[2]->normal;
    Vector3D *v4;

    switch ( patch->numberOfVertices ) {
        case 3:
            PINT(*v1, *v2, *v3, (float)u, (float)v, normal);
            break;
        case 4:
            v4 = patch->vertex[3]->normal;
            PINQ(*v1, *v2, *v3, *v4, (float)u, (float)v, normal);
            break;
        default:
            logFatal(-1, "PatchNormalAtUV", "Invalid number of vertices %d", patch->numberOfVertices);
    }

    VECTORNORMALIZE(normal);
    return normal;
}

/**
Computes interpolated (= shading) normal at the point with given parameters
on the patch
*/
Vector3D
patchInterpolatedNormalAtUv(Patch *patch, double u, double v) {
    if ( !allVerticesHaveANormal(patch)) {
        return patch->normal;
    }
    return getInterpolatedNormalAtUv(patch, u, v);
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
patchInterpolatedFrameAtUv(Patch *patch, double u, double v,
                           Vector3D *X, Vector3D *Y, Vector3D *Z) {
    *Z = patchInterpolatedNormalAtUv(patch, u, v);

    if ( X && Y ) {
        double zz = sqrt(1 - Z->z * Z->z);
        if ( zz < EPSILON ) {
            VECTORSET(*X, 1., 0., 0.);
        } else {
            VECTORSET(*X, (float)(Z->y / zz), (float)(-Z->x / zz), 0.0f);
        }

        VECTORCROSSPRODUCT(*Z, *X, *Y); /* *Y = (*Z) ^ (*X); */
    }
}

/**
Returns texture coordinates determined from vertex texture coordinates and
given u and v bilinear of barycentric coordinates on the patch
*/
Vector3D
patchTextureCoordAtUv(Patch *patch, double u, double v) {
    Vector3D *t0;
    Vector3D *t1;
    Vector3D *t2;
    Vector3D *t3;
    Vector3D texCoord;
    VECTORSET(texCoord, 0.0, 0.0, 0.0);

    t0 = patch->vertex[0]->texCoord;
    t1 = patch->vertex[1]->texCoord;
    t2 = patch->vertex[2]->texCoord;
    switch ( patch->numberOfVertices ) {
        case 3:
            if ( !t0 || !t1 || !t2 ) {
                VECTORSET(texCoord, (float)u, (float)v, 0.0f);
            } else {
                PINT(*t0, *t1, *t2,(float)u, (float)v, texCoord);
            }
            break;
        case 4:
            t3 = patch->vertex[3]->texCoord;
            if ( !t0 || !t1 || !t2 || !t3 ) {
                VECTORSET(texCoord, (float)u, (float)v, 0.0f);
            } else {
                PINQ(*t0, *t1, *t2, *t3, (float)u, (float)v, texCoord);
            }
            break;
        default:
            logFatal(-1, "patchTextureCoordAtUv", "Invalid nr of vertices %d", patch->numberOfVertices);
    }
    return texCoord;
}

/**
Returns (u,v) coordinates of the point in the triangle
Didier Badouel, Graphics Gems I, p390
*/
static int
triangleUv(Patch *patch, Vector3D *point, Vector2Dd *uv) {
    static Patch *cachedPatch = nullptr;
    double u0;
    double v0;
    REAL alpha;
    REAL beta;
    Vertex **v;
    Vector2Dd p0;
    Vector2Dd p1;
    Vector2Dd p2;

    if ( !patch ) {
        patch = cachedPatch;
    }
    cachedPatch = patch;

    // Project to 2D
    v = patch->vertex;
    switch ( patch->index ) {
        case XNORMAL:
            u0 = (*v)->point->y;
            v0 = (*v)->point->z;
            V2Set(p0, point->y - u0, point->z - v0);
            v++;
            V2Set(p1, (*v)->point->y - u0, (*v)->point->z - v0);
            v++;
            V2Set(p2, (*v)->point->y - u0, (*v)->point->z - v0);
            break;

        case YNORMAL:
            u0 = (*v)->point->x;
            v0 = (*v)->point->z;
            V2Set(p0, point->x - u0, point->z - v0);
            v++;
            V2Set(p1, (*v)->point->x - u0, (*v)->point->z - v0);
            v++;
            V2Set(p2, (*v)->point->x - u0, (*v)->point->z - v0);
            break;

        case ZNORMAL:
            u0 = (*v)->point->x;
            v0 = (*v)->point->y;
            V2Set(p0, point->x - u0, point->y - v0);
            v++;
            V2Set(p1, (*v)->point->x - u0, (*v)->point->y - v0);
            v++;
            V2Set(p2, (*v)->point->x - u0, (*v)->point->y - v0);
            break;
    }

    if ( p1.u < -EPSILON || p1.u > EPSILON ) {  /* p1.u non zero */
        beta = (p0.v * p1.u - p0.u * p1.v) / (p2.v * p1.u - p2.u * p1.v);
        if ( beta >= 0. && beta <= 1. ) {
            alpha = (p0.u - beta * p2.u) / p1.u;
        } else {
            return false;
        }
    } else {
        beta = p0.u / p2.u;
        if ( beta >= 0. && beta <= 1. ) {
            alpha = (p0.v - beta * p2.v) / p1.v;
        } else {
            return false;
        }
    }
    uv->u = alpha;
    uv->v = beta;
    if ( alpha < 0. || (alpha + beta) > 1. ) {
        return false;
    }
    return true;
}

/**
Christophe Schlick and Gilles Subrenat (15 May 1994)
"Ray Intersection of Tessellated Surfaces : Quadrangles versus Triangles"
in Graphics Gems V (edited by A. Paeth), Academic Press
*/
static int
quadUv(Patch *patch, Vector3D *point, Vector2Dd *uv) {
    static Patch *cachedPatch = nullptr;
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
    REAL u = -1.0; // Parametric coordinates
    REAL v = -1.0;
    REAL a; // For the quadratic equation
    REAL b;
    REAL c;
    REAL SqrtDelta;
    Vector2Dd Vector; // Temporary 2D-vector
    int isInside = false;

    if ( patch == nullptr ) {
        patch = cachedPatch;
    }
    cachedPatch = patch;

    // Projection on the plane that is most parallel to the facet
    p = patch->vertex;
    switch ( patch->index ) {
        case XNORMAL: V2Set(A, (*p)->point->y, (*p)->point->z);
            p++;
            V2Set(B, (*p)->point->y, (*p)->point->z);
            p++;
            V2Set(C, (*p)->point->y, (*p)->point->z);
            p++;
            V2Set(D, (*p)->point->y, (*p)->point->z);
            V2Set(M, point->y, point->z);
            break;

        case YNORMAL: V2Set(A, (*p)->point->x, (*p)->point->z);
            p++;
            V2Set(B, (*p)->point->x, (*p)->point->z);
            p++;
            V2Set(C, (*p)->point->x, (*p)->point->z);
            p++;
            V2Set(D, (*p)->point->x, (*p)->point->z);
            V2Set(M, point->x, point->z);
            break;

        case ZNORMAL: V2Set(A, (*p)->point->x, (*p)->point->y);
            p++;
            V2Set(B, (*p)->point->x, (*p)->point->y);
            p++;
            V2Set(C, (*p)->point->x, (*p)->point->y);
            p++;
            V2Set(D, (*p)->point->x, (*p)->point->y);
            V2Set(M, point->x, point->y);
            break;
    }

    V2Sub(B, A, AB);
    V2Sub(C, B, BC);
    V2Sub(D, C, CD);
    V2Sub(D, A, AD);
    V2Add(CD, AB, AE);
    V2Negate(AE);
    V2Sub(M, A, AM);

    if ( fabs(DETERMINANT(AB, CD)) < EPSILON ) {
	    // Case AB // CD
        V2Sub (AB, CD, Vector);
        v = DETERMINANT(AM, Vector) / DETERMINANT(AD, Vector);
        if ((v >= 0.0) && (v <= 1.0)) {
            b = DETERMINANT(AB, AD) - DETERMINANT(AM, AE);
            c = DETERMINANT (AM, AD);
            u = ABS(b) < EPSILON ? -1 : c / b;
            isInside = ((u >= 0.0) && (u <= 1.0));
        }
    } else if ( fabs(DETERMINANT(BC, AD)) < EPSILON ) {
        // Case AD // BC
        V2Add (AD, BC, Vector);
        u = DETERMINANT(AM, Vector) / DETERMINANT(AB, Vector);
        if ((u >= 0.0) && (u <= 1.0)) {
            b = DETERMINANT(AD, AB) - DETERMINANT(AM, AE);
            c = DETERMINANT (AM, AB);
            v = ABS(b) < EPSILON ? -1 : c / b;
            isInside = ((v >= 0.0) && (v <= 1.0));
        }
    } else {
        // General case
        a = DETERMINANT(AB, AE);
        c = -DETERMINANT (AM, AD);
        b = DETERMINANT(AB, AD) - DETERMINANT(AM, AE);
        a = -0.5 / a;
        b *= a;
        c *= (a + a);
        SqrtDelta = b * b + c;
        if ( SqrtDelta >= 0.0 ) {
            SqrtDelta = sqrt(SqrtDelta);
            u = b - SqrtDelta;
            if ( (u < 0.0) || (u > 1.0) ) {
                // To choose u between 0 and 1
                u = b + SqrtDelta;
            }
            if ( (u >= 0.0) && (u <= 1.0) ) {
                v = AD.u + u * AE.u;
                if ( ABS(v) < EPSILON ) {
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
static int
hitInPatch(RayHit *hit, Patch *patch) {
    hit->flags |= HIT_UV; // uv parameters computed as a side result
    return (patch->numberOfVertices == 3)
           ? triangleUv(patch, &hit->point, &hit->uv)
           : quadUv(patch, &hit->point, &hit->uv);
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
patchIntersect(
    Patch *patch,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore)
{
    RayHit hit;
    float dist;

    if ( isExcludedPatch(patch) ) {
        return nullptr;
    }

    dist = VECTORDOTPRODUCT(patch->normal, ray->dir);
    if ( dist > EPSILON ) {
        // Back facing patch
        if ( !(hitFlags & HIT_BACK) ) {
            return (RayHit *) nullptr;
        } else {
            hit.flags = HIT_BACK;
        }
    } else if ( dist < -EPSILON ) {
        // Front facing patch
        if ( !(hitFlags & HIT_FRONT)) {
            return (RayHit *) nullptr;
        } else {
            hit.flags = HIT_FRONT;
        }
    } else {
        // Ray is parallel with the plane of the patch
        return (RayHit *) nullptr;
    }

    dist = -(VECTORDOTPRODUCT(patch->normal, ray->pos) + patch->planeConstant) / dist;

    if ( dist > *maximumDistance || dist < minimumDistance ) {
        // Intersection too far or too close
        return (RayHit *) nullptr;
    }

    // Intersection point of ray with plane of patch
    hit.dist = dist;
    VECTORSUMSCALED(ray->pos, dist, ray->dir, hit.point);

    // Test whether it lays inside or outside the patch
    if ( hitInPatch(&hit, patch)) {
        hit.geom = (Geometry *) nullptr; // we don't know it
        hit.patch = patch;
        hit.material = patch->surface->material;
        hit.gnormal = patch->normal;
        hit.flags |= HIT_PATCH | HIT_POINT | HIT_MATERIAL | HIT_GNORMAL | HIT_DIST;
        if ( hitFlags & HIT_UV ) {
            if ( !(hit.flags & HIT_UV)) {
                patchUv(hit.patch, &hit.point, &hit.uv.u, &hit.uv.v);
                hit.flags &= HIT_UV;
            }
        }
        *hitStore = hit;
        *maximumDistance = dist;

        return hitStore;
    }

    return (RayHit *) nullptr;
}

/**
Converts bi-linear coordinates into uniform (area preserving)
coordinates for a polygon with given jacobian. Uniform coordinates
are such that the area left of the line of positions with u-coordinate 'u',
will be u.A and the area under the line of positions with v-coordinate 'v'
will be v.A.
*/
void
biLinearToUniform(Patch *patch, double *u, double *v) {
    double a = patch->jacobian->A, b = patch->jacobian->B, c = patch->jacobian->C;
    *u = ((a + 0.5 * c) + 0.5 * b * (*u)) * (*u) / patch->area;
    *v = ((a + 0.5 * b) + 0.5 * c * (*v)) * (*v) / patch->area;
}

/**
Looks for solution of the quadratic equation A.u^2 + B.u + C = 0
in the interval [0,1]. There must be exactly one such solution.
Returns true if one such solution is found, or false if the equation
has no real solutions, both solutions are in the interval [0,1] or
none of them is. In case of problems, a best guess solution
is returned. Problems seem to be due to numerical inaccuracy
*/
static int
solveQuadraticUnitInterval(double A, double B, double C, double *x) {
    double D = B * B - 4. * A * C, x1, x2;

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

        D = D > TOLERANCE * TOLERANCE ? sqrt(D) : 0.;
        A = 1. / (2. * A);
        x1 = (-B + D) * A;
        x2 = (-B - D) * A;

        if ( x1 > -TOLERANCE && x1 < 1.0 + TOLERANCE ) {
            *x = x1;
            if ( x2 > -TOLERANCE && x2 < 1. + TOLERANCE ) {
                // Error(nullptr, "Bi-linear->Uniform mapping ambiguous: x1=%g, x2=%g, taking %g as solution", x1, x2, x1);
                return false;
            } else {
                return true;
            }
        }
    }

    if ( x2 > -TOLERANCE && x2 < 1. + TOLERANCE ) {
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
Converts uniform to bi-linear coordinates
*/
void
uniformToBiLinear(Patch *patch, double *u, double *v) {
    double a = patch->jacobian->A, b = patch->jacobian->B, c = patch->jacobian->C, A, B, C;

    A = 0.5 * b / patch->area;
    B = (a + 0.5 * c) / patch->area;
    C = -(*u);
    if ( !solveQuadraticUnitInterval(A, B, C, u)) {
        /*
        Error(nullptr, "Tried to solve %g*u^2 + %g*u = %g for patch %d",
          A, B, -C, patch->id);
        fprintf(stderr, "Jacobian: %g + %g*u + %g*v\n", a, b, c);
        */
    }

    A = 0.5 * c / patch->area;
    B = (a + 0.5 * b) / patch->area;
    C = -(*v);
    if ( !solveQuadraticUnitInterval(A, B, C, v)) {
        /*
        Error(nullptr, "Tried to solve %g*v^2 + %g*v = %g for patch %d",
          A, B, -C, patch->id);
        fprintf(stderr, "Jacobian: %g + %g*u + %g*v\n", a, b, c);
        */
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
patchPoint(Patch *patch, double u, double v, Vector3D *point) {
    Vector3D *v1;
    Vector3D *v2;
    Vector3D *v3;
    Vector3D *v4;

    if ( patch->hasZeroVertices() ) {
        return nullptr;
    }

    v1 = patch->vertex[0]->point;
    v2 = patch->vertex[1]->point;
    v3 = patch->vertex[2]->point;

    if ( patch->numberOfVertices == 3 ) {
        if ( u + v > 1.0 ) {
            u = 1.0 - u;
            v = 1.0 - v;
            // Warning("patchPoint", "(u,v) outside unit triangle");
        }
        PINT(*v1, *v2, *v3, (float)u, (float)v, *point);
    } else if ( patch->numberOfVertices == 4 ) {
        v4 = patch->vertex[3]->point;
        PINQ(*v1, *v2, *v3, *v4, (float)u, (float)v, *point);
    } else {
        logFatal(4, "patchPoint", "Can only handle triangular or quadrilateral patches");
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
patchUniformPoint(Patch *patch, double u, double v, Vector3D *point) {
    if ( patch->jacobian ) {
        uniformToBiLinear(patch, &u, &v);
    }
    return patchPoint(patch, u, v, point);
}

/**
Computes (u,v) parameters of the point on the patch (barycentric or bi-linear
parametrisation). Returns true if the point is inside the patch and false if
not.
WARNING: The (u,v) coordinates are correctly computed only for positions inside
the patch. For positions outside, they can be garbage!
*/
int
patchUv(Patch *poly, Vector3D *point, double *u, double *v) {
    static Patch *cached;
    Patch *thePoly;
    Vector2Dd uv;
    int inside = false;

    thePoly = poly ? poly : cached;
    cached = thePoly;

    switch ( thePoly->numberOfVertices ) {
        case 3:
            inside = triangleUv(thePoly, point, &uv);
            break;
        case 4:
            inside = quadUv(thePoly, point, &uv);
            break;
        default:
            logFatal(3, "patchUv", "Can only handle triangular or quadrilateral patches");
    }

    *u = uv.u;
    *v = uv.v;

    return inside;
}

/**
Like above, but returns uniform coordinates (inverse of patchUniformPoint())
*/
int
patchUniformUv(Patch *poly, Vector3D *point, double *u, double *v) {
    int inside = patchUv(poly, point, u, v);
    if ( poly->jacobian ) {
        biLinearToUniform(poly, u, v);
    }
    return inside;
}

/**
Computes shading frame at hit point. Z is the shading normal. Returns FALSE
if the shading frame could not be determined.
If X and Y are null pointers, only the shading normal is returned in Z
possibly avoiding computations of the X and Y axis
*/
int
materialShadingFrame(RayHit *hit, Vector3D *X, Vector3D *Y, Vector3D *Z) {
    int success = false;

    if ( !hitInitialised(hit)) {
        logWarning("materialShadingFrame", "uninitialised hit structure");
        return false;
    }

    if ( hit->material && hit->material->bsdf && hit->material->bsdf->methods->ShadingFrame ) {
        success = bsdfShadingFrame(hit->material->bsdf, hit, X, Y, Z);
    }

    if ( !success && hit->material && hit->material->edf && hit->material->edf->methods->ShadingFrame ) {
        success = edfShadingFrame(hit->material->edf, hit, X, Y, Z);
    }

    if ( !success && hitUv(hit, &hit->uv)) {
        // Make default shading frame
        patchInterpolatedFrameAtUv(hit->patch, hit->uv.u, hit->uv.v, X, Y, Z);
        success = true;
    }

    return success;
}

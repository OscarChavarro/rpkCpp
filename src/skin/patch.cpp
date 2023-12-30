#include <cstdarg>
#include <cstdlib>

#include "common/error.h"
#include "material/statistics.h"
#include "BREP/BREP_SOLID.h"
#include "QMC/nied31.h"
#include "skin/patch_flags.h"
#include "skin/patch.h"
#include "skin/Vertex.h"
#include "skin/radianceinterfaces.h"

/* a static counter which is increased every time a PATCH is created in
 * order to make a unique PATCH id. */
static int patchid = 1;

/* This routine returns the ID number the next patch would get */
int PatchGetNextID() {
    return patchid;
}

/* With this routine, the ID nummer is set that the next patch will get. 
 * Note that patch ID 0 is reserved. The smallest patch ID number should be 1. */
void PatchSetNextID(int id) {
    patchid = id;
}

/* adds the patch to the list of patches that share the vertex */
void PatchConnectVertex(PATCH *patch, VERTEX *vertex) {
    vertex->patches = PatchListAdd(vertex->patches, patch);
}

/* adds the patch to the list of patches sharing each vertex */
void PatchConnectVertices(PATCH *patch) {
    int i;

    for ( i = 0; i < patch->nrvertices; i++ ) {
        PatchConnectVertex(patch, patch->vertex[i]);
    }
}

/* returns a pointer to the normal vector if everything is OK. nullptr pointer if
 * degenerate polygon. */
Vector3D *PatchNormal(PATCH *patch, Vector3D *normal) {
    double norm;
    Vector3Dd prev, cur;
    int i;

    VECTORSET(*normal, 0, 0, 0);
    VECTORSUBTRACT(*patch->vertex[patch->nrvertices - 1]->point,
                   *patch->vertex[0]->point, cur);
    for ( i = 0; i < patch->nrvertices; i++ ) {
        prev = cur;
        VECTORSUBTRACT(*patch->vertex[i]->point, *patch->vertex[0]->point, cur);
        normal->x += (prev.y - cur.y) * (prev.z + cur.z);
        normal->y += (prev.z - cur.z) * (prev.x + cur.x);
        normal->z += (prev.x - cur.x) * (prev.y + cur.y);
    }

    if ((norm = VECTORNORM(*normal)) < EPSILON ) {
        Warning("PatchNormal", "degenerate patch (id %d)", patch->id);
        return (Vector3D *) nullptr;
    }
    VECTORSCALEINVERSE(norm, *normal, *normal);

    return normal;
}

/* We also compute the jacobian J(u,v) of the coordinate mapping from the unit
 * square [0,1]^2 or the standard triangle (0,0),(1,0),(0,1) to the patch.
 *
 * - for triangles: J(u,v) = the area of the triangle 
 * - for quadrilaterals: J(u,v) = A + B.u + C.v
 *	the area of the patch = A + (B+C)/2
 *	for paralellograms holds that B=C=0
 * 	the coefficients A,B,C are only explicitely stored if B and C are nonzero.
 *
 * The normal of the patch should have been computed before calling this routine. */
float PatchArea(PATCH *patch) {
    Vector3D *p1, *p2, *p3, *p4;
    Vector3Dd d1, d2, d3, d4, cp1, cp2, cp3;
    double a, b, c;

    /* we need to compute the area and jacobian explicitely */
    switch ( patch->nrvertices ) {
        case 3:
            /* jacobian J(u,v) for the mapping of the triangle
             * (0,0),(0,1),(1,0) to a triangular patch is constant and equal
             * to the area of the triangle ... so there is no need to store
             * any coefficients explicitely */
            patch->jacobian = (JACOBIAN *) nullptr;

            p1 = patch->vertex[0]->point;
            p2 = patch->vertex[1]->point;
            p3 = patch->vertex[2]->point;
            VECTORSUBTRACT(*p2, *p1, d1);
            VECTORSUBTRACT(*p3, *p2, d2);
            VECTORCROSSPRODUCT(d1, d2, cp1);
            patch->area = 0.5 * VECTORNORM(cp1);
            break;
        case 4:
            p1 = patch->vertex[0]->point;
            p2 = patch->vertex[1]->point;
            p3 = patch->vertex[2]->point;
            p4 = patch->vertex[3]->point;
            VECTORSUBTRACT(*p2, *p1, d1);
            VECTORSUBTRACT(*p3, *p2, d2);
            VECTORSUBTRACT(*p3, *p4, d3);
            VECTORSUBTRACT(*p4, *p1, d4);
            VECTORCROSSPRODUCT(d1, d4, cp1);
            VECTORCROSSPRODUCT(d1, d3, cp2);
            VECTORCROSSPRODUCT(d2, d4, cp3);
            a = VECTORDOTPRODUCT(cp1, patch->normal);
            b = VECTORDOTPRODUCT(cp2, patch->normal);
            c = VECTORDOTPRODUCT(cp3, patch->normal);

            patch->area = a + 0.5 * (b + c);
            if ( patch->area < 0. ) {    /* may happen if the normal direction and
      a = -a;			 * vertex order are not consistent. */
                b = -b;
                c = -c;
                patch->area = -patch->area;
            }

            /* b and c are zero for parallellograms. In that case, the area is equal to
             * a, so we don't need to store the coefficients. */
            if ( fabs(b) / patch->area < EPSILON && fabs(c) / patch->area < EPSILON ) {
                patch->jacobian = (JACOBIAN *) nullptr;
            } else {
                patch->jacobian = JacobianCreate(a, b, c);
            }

            break;
        default:
            Fatal(2, "PatchArea", "Can only handle triangular and quadrilateral patches.\n");
            patch->jacobian = (JACOBIAN *) nullptr;
            patch->area = 0.0;
    }

    if ( patch->area < EPSILON * EPSILON ) {
        fprintf(stderr, "Warning: very small patch id %d area = %g\n", patch->id, patch->area);
    }

    return patch->area;
}

/* Computes the midpoint of the patch, stores the result in p and
 * returns a pointer to p. */
Vector3D *PatchMidpoint(PATCH *patch, Vector3D *p) {
    int i;

    VECTORSET(*p, 0, 0, 0);
    for ( i = 0; i < patch->nrvertices; i++ ) VECTORADD(*p, *(patch->vertex[i]->point), *p);
    VECTORSCALEINVERSE((float) patch->nrvertices, *p, *p);

    return p;
}

/* Computes a certain "width" for the plane, e.g. for coplanarity testing. */
float PatchTolerance(PATCH *patch) {
    int i;
    double tolerance;

    /* fill in the vertices in the plane equation + take into account the vertex
     * position tolerance */
    tolerance = 0.;
    for ( i = 0; i < patch->nrvertices; i++ ) {
        Vector3D *p = patch->vertex[i]->point;
        double e = fabs(VECTORDOTPRODUCT(patch->normal, *p) + patch->plane_constant)
                   + VECTORTOLERANCE(*p);
        if ( e > tolerance ) {
            tolerance = e;
        }
    }

    return tolerance;
}

/* edge flags */
#define U_DOMINANT 0x04
#define REVERSE_DIST 0x08

/* computes some ray tracing constants for the patch */
static void PatchInitRayTracing(PATCH *patch) {
}

int IsPatchVirtual(PATCH *patch) {
    return (patch->nrvertices == 0);
}

/* Creates a patch structure for a patch with given vertices. */
PATCH *PatchCreate(int nrvertices,
                   VERTEX *v1, VERTEX *v2, VERTEX *v3, VERTEX *v4) {
    PATCH *patch;

    /* this may occur after an error parsing the input file, and some other routine
     * will already complain */
    if ( !v1 || !v2 || !v3 || (nrvertices == 4 && !v4)) {
        return (PATCH *) nullptr;
    }

    /* it's sad but it's true */
    if ( nrvertices != 3 && nrvertices != 4 ) {
        Error("PatchCreate", "Can only handle quadrilateral or triagular patches");
        return (PATCH *) nullptr;
    }

    patch = (PATCH *)malloc(sizeof(PATCH));
    GLOBAL_statistics_numberOfElements++;
    patch->twin = (PATCH *) nullptr;
    patch->id = patchid;
    patchid++;

    patch->surface = (SURFACE *) nullptr;

    patch->nrvertices = nrvertices;
    patch->vertex[0] = v1;
    patch->vertex[1] = v2;
    patch->vertex[2] = v3;
    patch->vertex[3] = v4;

    patch->brep_data = (BREP_FACE *) nullptr;

    /* a bounding box will be computed the first time it is needed. */
    patch->bounds = (float *) nullptr;

    /* compute normal */
    if ( !PatchNormal(patch, &patch->normal)) {
        free(patch);
        GLOBAL_statistics_numberOfElements--;
        return (PATCH *) nullptr;
    }

    /* also computes the jacobian */
    patch->area = PatchArea(patch);

    /* patch midpoint */
    PatchMidpoint(patch, &patch->midpoint);

    /* plane constant */
    patch->plane_constant = -VECTORDOTPRODUCT(patch->normal, patch->midpoint);

    /* plane tolerance */
    patch->tolerance = PatchTolerance(patch);

    /* dominant part of normal */
    patch->index = VectorDominantCoord(&patch->normal);

    /* ray tracing constants */
    PatchInitRayTracing(patch);

    /* tell the vertices that there's a new PATCH using them */
    PatchConnectVertices(patch);

    patch->direct_potential = 0.0;
    RGBSET(patch->color, 0.0, 0.0, 0.0);

    patch->omit = false;
    patch->flags = 0;    /* other flags */

    /* if we are doing radiance computations, create radiance data for the
       patch */
    patch->radiance_data = (Radiance && Radiance->CreatePatchData) ?
                           Radiance->CreatePatchData(patch) : (void *) nullptr;

    return patch;
}

/* disposes the memory allocated for the patch, does not remove
 * the pointers to the patch in the VERTEXes of the patch. */
void PatchDestroy(PATCH *patch) {
    if ( Radiance && patch->radiance_data ) {
        if ( Radiance->DestroyPatchData ) {
            Radiance->DestroyPatchData(patch);
        }
    }

    if ( patch->bounds ) {
        BoundsDestroy(patch->bounds);
    }

    if ( patch->jacobian ) {
        JacobianDestroy(patch->jacobian);
    }

    if ( patch->brep_data ) {    /* also destroys all contours, and edges if not used in
			 * other faces as well. Not the vertices: these are
			 * destroyed when destroying the corresponding VERTEX. */
        BrepDestroyFace(patch->brep_data);
    }

    if ( patch->twin ) {
        patch->twin->twin = (PATCH *) nullptr;
    }

    free(patch);
    GLOBAL_statistics_numberOfElements--;
}

/* computes a bounding box for the patch. fills it in in 'bounds' and returns
 * a pointer to 'bounds'. */
float *PatchBounds(PATCH *patch, float *bounds) {
    int i;

    if ( !patch->bounds ) {
        patch->bounds = BoundsCreate();
        BoundsInit(patch->bounds);
        for ( i = 0; i < patch->nrvertices; i++ ) {
            BoundsEnlargePoint(patch->bounds, patch->vertex[i]->point);
        }
    }

    BoundsCopy(patch->bounds, bounds);

    return bounds;
}

static int nrofsamples(PATCH *patch) {
    int nrsamples = 1;
    if ( BsdfIsTextured(patch->surface->material->bsdf)) {
        if ( patch->vertex[0]->texCoord == patch->vertex[1]->texCoord &&
             patch->vertex[0]->texCoord == patch->vertex[2]->texCoord &&
             (patch->nrvertices == 3 || patch->vertex[0]->texCoord == patch->vertex[3]->texCoord) &&
             patch->vertex[0]->texCoord != nullptr) {
            /* all vertices have same texture coordinates (important special case) */
            nrsamples = 1;
        } else {
            nrsamples = 100;
        }
    }
    return nrsamples;
}

COLOR PatchAverageNormalAlbedo(PATCH *patch, BSDFFLAGS components) {
    int i, nrsamples;
    COLOR albedo;
    HITREC hit;
    InitHit(&hit, patch, nullptr, &patch->midpoint, &patch->normal, patch->surface->material, 0.);

    nrsamples = nrofsamples(patch);
    COLORCLEAR(albedo);
    for ( i = 0; i < nrsamples; i++ ) {
        COLOR sample;
        unsigned *xi = Nied31(i);
        hit.uv.u = (double) xi[0] * RECIP;
        hit.uv.v = (double) xi[1] * RECIP;
        hit.flags |= HIT_UV;
        PatchPoint(patch, hit.uv.u, hit.uv.v, &hit.point);
        sample = BsdfScatteredPower(patch->surface->material->bsdf, &hit, &patch->normal, components);
        COLORADD(albedo, sample, albedo);
    }
    COLORSCALEINVERSE((float) nrsamples, albedo, albedo);

    return albedo;
}

COLOR PatchAverageEmittance(PATCH *patch, XXDFFLAGS components) {
    int i, nrsamples;
    COLOR emittance;
    HITREC hit;
    InitHit(&hit, patch, nullptr, &patch->midpoint, &patch->normal, patch->surface->material, 0.);

    nrsamples = nrofsamples(patch);
    COLORCLEAR(emittance);
    for ( i = 0; i < nrsamples; i++ ) {
        COLOR sample;
        unsigned *xi = Nied31(i);
        hit.uv.u = (double) xi[0] * RECIP;
        hit.uv.v = (double) xi[1] * RECIP;
        hit.flags |= HIT_UV;
        PatchPoint(patch, hit.uv.u, hit.uv.v, &hit.point);
        sample = EdfEmittance(patch->surface->material->edf, &hit, components);
        COLORADD(emittance, sample, emittance);
    }
    COLORSCALEINVERSE((float) nrsamples, emittance, emittance);

    return emittance;
}

void PatchPrintID(FILE *out, PATCH *patch) {
    fprintf(out, "%d ", patch->id);
}

void PatchPrint(FILE *out, PATCH *patch) {
    int i;
    COLOR Rd, Ed;

    fprintf(out, "Patch id %d:\n", patch->id);

    fprintf(out, "%d vertices:\n", patch->nrvertices);
    for ( i = 0; i < patch->nrvertices; i++ ) {
        VertexPrint(out, patch->vertex[i]);
    }
    fprintf(out, "\n");

    fprintf(out, "midpoint = ");
    VectorPrint(out, patch->midpoint);
    fprintf(out, ", normal = ");
    VectorPrint(out, patch->normal);
    fprintf(out, ", plane constant = %g, tolerance = %g\narea = %g, ",
            patch->plane_constant, patch->tolerance, patch->area);
    if ( patch->jacobian ) {
        fprintf(out, "Jacobian: %g %+g*u %+g*v \n",
                patch->jacobian->A, patch->jacobian->B, patch->jacobian->C);
    } else {
        fprintf(out, "No explicitely stored jacobian\n");
    }

    fprintf(out, "sided = %d, material = '%s'\n",
            patch->surface->material->sided,
            patch->surface->material->name ? patch->surface->material->name : "(default)");
    COLORCLEAR(Rd);
    if ( patch->surface->material->bsdf ) {
        Rd = PatchAverageNormalAlbedo(patch, BRDF_DIFFUSE_COMPONENT);
    }
    COLORCLEAR(Ed);
    if ( patch->surface->material->edf ) {
        Ed = PatchAverageEmittance(patch, DIFFUSE_COMPONENT);
    }
    fprintf(out, ", reflectance = ");
    ColorPrint(out, Rd);
    fprintf(out, ", self-emitted luminosity = %g\n", ColorLuminance(Ed));

    fprintf(out, "color: ");
    RGBPrint(out, patch->color);
    fprintf(out, "\n");
    fprintf(out, "directly received potential: %g\n", patch->direct_potential);

    fprintf(out, "flags: %s\n",
            PATCH_IS_VISIBLE(patch) ? "PATCH_IS_VISIBLE" : "");

    if ( Radiance ) {
        if ( patch->radiance_data && Radiance->PrintPatchData ) {
            fprintf(out, "Radiance data:\n");
            Radiance->PrintPatchData(out, patch);
        } else {
            fprintf(out, "No radiance data\n");
        }
    }
}

/* Specify up to MAX_EXCLUDED_PATCHES patches not to test for intersections with. 
 * Used to avoid selfintersections when raytracing. First argument is number of
 * patches to include. Next arguments are pointers to the patches to exclude.
 * Call with first parameter == 0 to clear the list. */
#define MAX_EXCLUDED_PATCHES 4
static PATCH *excludedPatches[MAX_EXCLUDED_PATCHES] = {nullptr, nullptr, nullptr, nullptr};

void PatchDontIntersect(int n, ...) {
    va_list ap;
    int i;

    if ( n > MAX_EXCLUDED_PATCHES ) {
        Fatal(-1, "PatchDontIntersect", "Too many patches to exclude from intersection tests (maximum is %d)",
              MAX_EXCLUDED_PATCHES);
        return;
    }

    va_start(ap, n);
    for ( i = 0; i < n; i++ ) {
        excludedPatches[i] = va_arg(ap, PATCH *);
    }
    va_end(ap);

    while ( i < MAX_EXCLUDED_PATCHES ) {
        excludedPatches[i++] = nullptr;
    }
}

int IsExcludedPatch(PATCH *p) {
    PATCH **excl = excludedPatches;
    /* MAX_EXCLUDED_PATCHES tests!! */
    return (*excl == p || *++excl == p || *++excl == p || *++excl == p);
}

static int AllVerticesHaveANormal(PATCH *patch) {
    int i;

    for ( i = 0; i < patch->nrvertices; i++ ) {
        if ( !patch->vertex[i]->normal ) {
            break;
        }
    }
    return i >= patch->nrvertices;
}


static Vector3D GetInterpolatedNormalAtUV(PATCH *patch, double u, double v) {
    Vector3D normal, *v1, *v2, *v3, *v4;

    v1 = patch->vertex[0]->normal;
    v2 = patch->vertex[1]->normal;
    v3 = patch->vertex[2]->normal;

    switch ( patch->nrvertices ) {
        case 3: PINT(*v1, *v2, *v3, u, v, normal);
            break;
        case 4:
            v4 = patch->vertex[3]->normal;
            PINQ(*v1, *v2, *v3, *v4, u, v, normal);
            break;
        default:
            Fatal(-1, "PatchNormalAtUV", "Invalid number of vertices %d", patch->nrvertices);
    }

    VECTORNORMALIZE(normal);
    return normal;
}

/* computes interpolated (= shading) normal at the point with given parameters 
 * on the patch */
Vector3D PatchInterpolatedNormalAtUV(PATCH *patch, double u, double v) {
    if ( !AllVerticesHaveANormal(patch)) {
        return patch->normal;
    }
    return GetInterpolatedNormalAtUV(patch, u, v);
}

/* computes interpolated (= shading) normal at the given point on the patch */
Vector3D PatchInterpolatedNormalAtPoint(PATCH *patch, Vector3D *point) {
    double u, v;

    if ( !AllVerticesHaveANormal(patch)) {
        return patch->normal;
    }

    PatchUV(patch, point, &u, &v);
    return GetInterpolatedNormalAtUV(patch, u, v);
}


/* computes shading frame at the given uv on the patch */
void PatchInterpolatedFrameAtUV(PATCH *patch, double u, double v,
                                Vector3D *X, Vector3D *Y, Vector3D *Z) {
    *Z = PatchInterpolatedNormalAtUV(patch, u, v);

    /* fprintf(stderr, "*Z %g %g %g, index %i\n", Z->x, Z->y, Z->z, patch->index); */

    if ( X && Y ) {
        double zz = sqrt(1 - Z->z * Z->z);
        if ( zz < EPSILON ) {
            VECTORSET(*X, 1., 0., 0.);
        } else {
            VECTORSET(*X, Z->y / zz, -Z->x / zz, 0.);
        }

        VECTORCROSSPRODUCT(*Z, *X, *Y); /* *Y = (*Z) ^ (*X); */
    }
}

Vector3D PatchTextureCoordAtUV(PATCH *patch, double u, double v) {
    Vector3D *t0, *t1, *t2, *t3;
    Vector3D texCoord;
    VECTORSET(texCoord, 0., 0., 0.);

    t0 = patch->vertex[0]->texCoord;
    t1 = patch->vertex[1]->texCoord;
    t2 = patch->vertex[2]->texCoord;
    switch ( patch->nrvertices ) {
        case 3:
            if ( !t0 || !t1 || !t2 ) {
                VECTORSET(texCoord, u, v, 0.);
            } else {
                PINT(*t0, *t1, *t2, u, v, texCoord);
            }
            break;
        case 4:
            t3 = patch->vertex[3]->texCoord;
            if ( !t0 || !t1 || !t2 || !t3 ) {
                VECTORSET(texCoord, u, v, 0.);
            } else {
                PINQ(*t0, *t1, *t2, *t3, u, v, texCoord);
            }
            break;
        default:
            Fatal(-1, "PatchTextureCoordAtUV", "Invalid nr of vertices %d", patch->nrvertices);
    }
    return texCoord;
}

/* returns (u,v) parameters for the point in the polygon
 * ref: Eric Haines, Essential Ray-tracing algorithms, in A. Glassner,
 * Introduction to Ray-Tracing, p 59.
 *
 * If the symbol QUADUV_CLIPBOUNDS is defined, the resulting coordinates will be
 * clipped to lay within the unit interval. In that case, this routine shall only 
 * be used for points that are known to lay inside the quadrilateral (you can use
 * PointInPatch() to test this.) */
#define QUADUV_CLIPBOUNDS
#define DISTANCE_TO_UNIT_INTERVAL(x)    ((x < 0.) ? -x : ((x > 1.) ? (x-1.) : 0.))
#define CLIP_TO_UNIT_INTERVAL(x)    ((x < EPSILON) ? EPSILON : ((x > (1.-EPSILON)) ? (1.-EPSILON) : x))

/* Now comes same as above, but more efficient .... - PhB Oct 2000 */

#define REAL double    /* yes, float or double makes a difference!!! */

#define V2Set(V, a, b) { (V).u = (a) ; (V).v = (b) ; }
#define V2Sub(p, q, r) { (r).u = (p).u - (q).u; (r).v = (p).v - (q).v; }
#define V2Add(p, q, r) { (r).u = (p).u + (q).u; (r).v = (p).v + (q).v; }
#define V2Negate(p)    { (p).u = -(p).u;  (p).v = -(p).v; }
#define DETERMINANT(A, B)    (((REAL)(A).u * (REAL)(B).v - (REAL)(A).v * (REAL)(B).u))
#define ABS(A) ((A)<0. ? -(A) : (A))

/* Returns (u,v) coordinates of the point in the triangle. */
/* Didier Badouel, Graphics Gems I, p390 */
static int TriangleUV(PATCH *patch, Vector3D *point, Vector2Dd *uv) {
    static PATCH *cachedpatch = (PATCH *) nullptr;
    double u0, v0;
    REAL alpha = -1., beta = 1.;
    VERTEX **v;
    Vector2Dd p0, p1, p2;

    if ( !patch ) {
        patch = cachedpatch;
    }
    cachedpatch = patch;

    /* project to 2D */
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
QuadUV(PATCH *patch, Vector3D *point, Vector2Dd *uv) {
    static PATCH *cachedpatch = (PATCH *) nullptr;
    VERTEX **p;
    Vector2Dd A, B, C, D;                // Projected vertices
    Vector2Dd M;                         // Projected intersection point
    Vector2Dd AB, BC, CD, AD, AM, AE;    // AE = DC - AB = DA - CB
    REAL u = -1., v = -1.;               // Parametric coordinates
    REAL a, b, c, SqrtDelta;             // for the quadratic equation
    Vector2Dd Vector;                    // temporary 2D-vector
    int IsInside = false;

    if ( !patch ) {
        patch = cachedpatch;
    }
    cachedpatch = patch;

    /*
    ** Projection on the plane that is most parallel to the facet
    */
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
	/* case AB // CD */
        V2Sub (AB, CD, Vector);
        v = DETERMINANT(AM, Vector) / DETERMINANT(AD, Vector);
        if ((v >= 0.0) && (v <= 1.0)) {
            b = DETERMINANT(AB, AD) - DETERMINANT(AM, AE);
            c = DETERMINANT (AM, AD);
            u = ABS(b) < EPSILON ? -1 : c / b;
            IsInside = ((u >= 0.0) && (u <= 1.0));
        }
    } else if ( fabs(DETERMINANT(BC, AD)) < EPSILON )          /* case AD // BC */
    {
        V2Add (AD, BC, Vector);
        u = DETERMINANT(AM, Vector) / DETERMINANT(AB, Vector);
        if ((u >= 0.0) && (u <= 1.0)) {
            b = DETERMINANT(AD, AB) - DETERMINANT(AM, AE);
            c = DETERMINANT (AM, AB);
            v = ABS(b) < EPSILON ? -1 : c / b;
            IsInside = ((v >= 0.0) && (v <= 1.0));
        }
    } else                                                   /* general case */
    {
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
            if ((u < 0.0) || (u > 1.0)) {      /* to choose u between 0 and 1 */
                u = b + SqrtDelta;
            }
            if ((u >= 0.0) && (u <= 1.0)) {
                v = AD.u + u * AE.u;
                if ( ABS(v) < EPSILON ) {
                    v = (AM.v - u * AB.v) / (AD.v + u * AE.v);
                } else {
                    v = (AM.u - u * AB.u) / v;
                }
                IsInside = ((v >= 0.0) && (v <= 1.0));
            }
        } else {
            u = v = -1.;
        }
    }

    uv->u = u;
    uv->v = v;
    uv->u = CLIP_TO_UNIT_INTERVAL(uv->u);
    uv->v = CLIP_TO_UNIT_INTERVAL(uv->v);

    return IsInside;
}

 /* LOWMEM_INTERSECT defined: use Badouels and Schlicks method from
       * graphics gems: slower, but consumes less storage and computes
       * (u,v) parameters as a side result */

static int HitInPatch(HITREC *hit, PATCH *patch) {
    hit->flags |= HIT_UV;        /* uv parameters computed as a side result */
    return (patch->nrvertices == 3)
           ? TriangleUV(patch, &hit->point, &hit->uv)
           : QuadUV(patch, &hit->point, &hit->uv);
}

/**
ray-patch intersection test, for computing formfactors, creating raycast 
images ... Returns nullptr if the Ray doesn't hit the patch. Fills in the
'the_hit' hit record if there is a new hit and returns a pointer to it.
Fills in the distance to the patch in maxdist if the patch
is hit. Intersections closer than mindist or further than *maxdist are
ignored. The hitflags determine what information to return about an
intersection and whether or not front/back facing patches are to be 
considered and are described in ray.h.
*/
HITREC *
PatchIntersect(
    PATCH *patch,
    Ray *ray,
    float mindist,
    float *maxdist,
    int hitflags,
    HITREC *hitstore)
{
    HITREC hit;
    float dist;

    if ( IsExcludedPatch(patch)) {
        return (HITREC *) nullptr;
    }

    if ( (dist = VECTORDOTPRODUCT(patch->normal, ray->dir)) > EPSILON ) {
        // backfacing patch
        if ( !(hitflags & HIT_BACK)) {
            return (HITREC *) nullptr;
        } else {
            hit.flags = HIT_BACK;
        }
    } else if ( dist < -EPSILON ) {
        // frontfacing patch
        if ( !(hitflags & HIT_FRONT)) {
            return (HITREC *) nullptr;
        } else {
            hit.flags = HIT_FRONT;
        }
    } else {
        // ray is parallel with the plane of the patch
        return (HITREC *) nullptr;
    }

    dist = -(VECTORDOTPRODUCT(patch->normal, ray->pos) + patch->plane_constant) / dist;

    if ( dist > *maxdist || dist < mindist ) {
        // intersection too far or too close
        return (HITREC *) nullptr;
    }

    // intersection point of ray with plane of patch
    hit.dist = dist;
    VECTORSUMSCALED(ray->pos, dist, ray->dir, hit.point);

    // test whether it lays inside or outside the patch
    if ( HitInPatch(&hit, patch)) {
        hit.geom = (GEOM *) nullptr; // we don't know it
        hit.patch = patch;
        hit.material = patch->surface->material;
        hit.gnormal = patch->normal;
        hit.flags |= HIT_PATCH | HIT_POINT | HIT_MATERIAL | HIT_GNORMAL | HIT_DIST;
        if ( hitflags & HIT_UV ) {
            if ( !(hit.flags & HIT_UV)) {
                PatchUV(hit.patch, &hit.point, &hit.uv.u, &hit.uv.v);
                hit.flags &= HIT_UV;
            }
        }
        *hitstore = hit;
        *maxdist = dist;

        return hitstore;
    }

    return (HITREC *) nullptr;
}

/* Converts bilinear coordinates into uniform (area preserving)
 * coordinates for a polygon with given jacobian. Uniform coordinates
 * are such that the area left of the line of points with u-coordinate 'u', 
 * will be u.A and the area under the line of points with v-coordinate 'v'
 * will be v.A. */
void BilinearToUniform(PATCH *patch, double *u, double *v) {
    double a = patch->jacobian->A, b = patch->jacobian->B, c = patch->jacobian->C;
    *u = ((a + 0.5 * c) + 0.5 * b * (*u)) * (*u) / patch->area;
    *v = ((a + 0.5 * b) + 0.5 * c * (*v)) * (*v) / patch->area;
}

/* Looks for solution of the quadratic equation A.u^2 + B.u + C = 0 
 * in the interval [0,1]. There must be exactly one such solution. 
 * Returns true if one such solution is found, or false if the equation
 * has no real solutions, both solutions are in the interval [0,1] or
 * none of them is. In case of problems, a best guess solution
 * is returned. Problems seem to be due to numerical inaccuracy. */
static int SolveQuadraticUnitInterval(double A, double B, double C, double *x) {
    double D = B * B - 4. * A * C, x1, x2;
#define TOLERANCE 1e-5

    if ( A < TOLERANCE && A > -TOLERANCE ) {
        /* degenerate case, solve B*x + C = 0 */
        x1 = -1.;
        x2 = -C / B;
    } else {
        if ( D < -TOLERANCE * TOLERANCE ) {
            *x = -B / (2. * A);
            Error(nullptr,
                  "Bilinear->Uniform mapping has negative discriminant D = %g.\nTaking 0 as discriminant and %g as solution.",
                  D, *x);
            return false;
        }

        D = D > TOLERANCE * TOLERANCE ? sqrt(D) : 0.;
        A = 1. / (2. * A);
        x1 = (-B + D) * A;
        x2 = (-B - D) * A;

        if ( x1 > -TOLERANCE && x1 < 1. + TOLERANCE ) {
            *x = x1;
            if ( x2 > -TOLERANCE && x2 < 1. + TOLERANCE ) {
                /*
                Error(nullptr, "Bilinear->Uniform mapping ambiguous: x1=%g, x2=%g, taking %g as solution", x1, x2, x1);
                */
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

        /* may happen due to numerical errors */
        /* chose the root closest to [0,1] */
        if ( x1 > 1. ) {
            d = x1 - 1.;
        } else { /*x1<0.*/
            d = -x1;
        }
        *x = x1;
        if ( x2 > 1. ) {
            if ( x2 - 1. < d ) {
                *x = x2;
            }
        } else {
            if ( 0. - x2 < d ) {
                *x = x2;
            }
        }

        /* clip it to [0,1] */
        if ( *x < 0. ) {
            *x = 0.;
        }
        if ( *x > 1. ) {
            *x = 1.;
        }
        /*
        Error(nullptr, "Bilinear->Uniform mapping has no valid solution: x1=%g, x2=%g, taking %g as solution", x1, x2, *x);
        */
        return false;
    }

    /*  return true; */
}

/* Converts uniform to bilinear coordinates. */
void UniformToBilinear(PATCH *patch, double *u, double *v) {
    double a = patch->jacobian->A, b = patch->jacobian->B, c = patch->jacobian->C, A, B, C;

    A = 0.5 * b / patch->area;
    B = (a + 0.5 * c) / patch->area;
    C = -(*u);
    if ( !SolveQuadraticUnitInterval(A, B, C, u)) {
        /*
        Error(nullptr, "Tried to solve %g*u^2 + %g*u = %g for patch %d",
          A, B, -C, patch->id);
        fprintf(stderr, "Jacobian: %g + %g*u + %g*v\n", a, b, c);
        */
    }

    A = 0.5 * c / patch->area;
    B = (a + 0.5 * b) / patch->area;
    C = -(*v);
    if ( !SolveQuadraticUnitInterval(A, B, C, v)) {
        /*
        Error(nullptr, "Tried to solve %g*v^2 + %g*v = %g for patch %d",
          A, B, -C, patch->id);
        fprintf(stderr, "Jacobian: %g + %g*u + %g*v\n", a, b, c);
        */
    }
}

/* Given the parameter (u,v) of a point on the patch, this routine 
 * computes the 3D space coordinates of the same point, using barycentric
 * mapping for a triangle and bilinear mapping for a quadrilateral. 
 * u and v should be numbers in the range [0,1]. If u+v>1 for a triangle,
 * (1-u) and (1-v) are used instead. */
Vector3D *PatchPoint(PATCH *patch, double u, double v, Vector3D *point) {
    Vector3D *v1, *v2, *v3, *v4;

    if ( IsPatchVirtual(patch)) {
        return (nullptr);
    }

    v1 = patch->vertex[0]->point;
    v2 = patch->vertex[1]->point;
    v3 = patch->vertex[2]->point;

    if ( patch->nrvertices == 3 ) {
        if ( u + v > 1. ) {
            u = 1. - u;
            v = 1. - v;
            /*	Warning("PatchPoint", "(u,v) outside unit triangle"); */
        }
        PINT(*v1, *v2, *v3, u, v, *point);
    } else if ( patch->nrvertices == 4 ) {
        v4 = patch->vertex[3]->point;
        PINQ(*v1, *v2, *v3, *v4, u, v, *point);
    } else {
        Fatal(4, "PatchPoint", "Can only handle triangular or quadrilateral patches");
    }

    return point;
}

/* Like above, except that always a uniform mapping is used (one that
 * preserves area, with this mapping you'll have more points in "stretched"
 * regions of an irregular quadrilateral, irregular quadrilaterals are the
 * only onces for which this routine will yield other points than the above
 * routine). */
Vector3D *PatchUniformPoint(PATCH *patch, double u, double v, Vector3D *point) {
    if ( patch->jacobian ) {
        UniformToBilinear(patch, &u, &v);
    }
    return PatchPoint(patch, u, v, point);
}

/* computes (u,v) parameters of the point on the patch (barycentric or bilinear
 * parametrisation). Returns true if the point is inside the patch and false if 
 * not. 
 * WARNING: The (u,v) coordinates are correctly computed only for points inside 
 * the patch. For points outside, they can be garbage!!! */
int PatchUV(PATCH *poly, Vector3D *point, double *u, double *v) {
    static PATCH *cached;
    PATCH *thepoly;
    Vector2Dd uv;
    int inside = false;

    thepoly = poly ? poly : cached;
    cached = thepoly;

    switch ( thepoly->nrvertices ) {
        case 3:
            inside = TriangleUV(thepoly, point, &uv);
            break;
        case 4:
            inside = QuadUV(thepoly, point, &uv);
            break;
        default:
            Fatal(3, "PatchUV", "Can only handle triangular or quadrilateral patches");
    }

    *u = uv.u;
    *v = uv.v;

    return inside;
}

/* Like above, but returns uniform coordinates. */
int PatchUniformUV(PATCH *poly, Vector3D *point, double *u, double *v) {
    int inside = PatchUV(poly, point, u, v);
    if ( poly->jacobian ) {
        BilinearToUniform(poly, u, v);
    }
    return inside;
}

int MaterialShadingFrame(HITREC *hit, Vector3D *X, Vector3D *Y, Vector3D *Z) {
    int succes = false;

    if ( !HitInitialised(hit)) {
        Warning("MaterialShadingFrame", "uninitialised hit structure");
        return false;
    }

    if ( hit->material && hit->material->bsdf && hit->material->bsdf->methods->ShadingFrame ) {
        succes = BsdfShadingFrame(hit->material->bsdf, hit, X, Y, Z);
    }

    if ( !succes && hit->material && hit->material->edf && hit->material->edf->methods->ShadingFrame ) {
        succes = EdfShadingFrame(hit->material->edf, hit, X, Y, Z);
    }

    if ( !succes && HitUV(hit, &hit->uv)) {
        /* make default shading frame */
        PatchInterpolatedFrameAtUV(hit->patch, hit->uv.u, hit->uv.v, X, Y, Z);
        succes = true;
    }

    return succes;
}

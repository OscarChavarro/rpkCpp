#include <cstring>

#include "java/util/ArrayList.txx"
#include "common/error.h"
#include "scene/scene.h"
#include "scene/splitbsdf.h"
#include "scene/phong.h"
#include "io/mgf/parser.h"
#include "io/mgf/vectoroctree.h"
#include "io/mgf/fileopts.h"
#include "io/mgf/readmgf.h"

// Objects 'o' contexts can be nested this deep
#define MAXIMUM_GEOMETRY_STACK_DEPTH 100

// No face can have more than this vertices
#define MAXIMUM_FACE_VERTICES 100

#define NUMBER_OF_SAMPLES 3

static VECTOROCTREE *globalPointsOctree;
static VECTOROCTREE *globalNormalsOctree;

// Elements for surface currently being created
static java::ArrayList<Vector3D *> *globalCurrentPointList;
static java::ArrayList<Vector3D *> *globalCurrentNormalList;
static java::ArrayList<Vertex *> *globalCurrentVertexList;
static java::ArrayList<Patch *> *globalCurrentFaceList;
static java::ArrayList<Geometry *> *globalCurrentGeometryList = nullptr;
static Material *globalCurrentMaterial;

// Geometry stack: used for building a hierarchical representation of the scene
static java::ArrayList<Geometry *> *globalGeometryStack[MAXIMUM_GEOMETRY_STACK_DEPTH];
static java::ArrayList<Geometry *> **globalGeometryStackPtr;

static int globalInComplex = false; // true if reading a sphere, torus or other unsupported
static int globalInSurface = false; // true if busy creating a new surface
static int globalAllSurfacesSided = false; // when set to true, all surfaces will be considered one-sided

void freeLists();

static void
doError(const char *errmsg) {
    logError(nullptr, (char *) "%s line %d: %s", GLOBAL_mgf_file->fileName, GLOBAL_mgf_file->lineNumber, errmsg);
}

static void
doWarning(const char *errmsg) {
    logWarning(nullptr, (char *) "%s line %d: %s", GLOBAL_mgf_file->fileName, GLOBAL_mgf_file->lineNumber, errmsg);
}

static void
pushCurrentGeometryList() {
    if ( globalGeometryStackPtr - globalGeometryStack >= MAXIMUM_GEOMETRY_STACK_DEPTH ) {
        doError(
                "Objects are nested too deep for this program. Recompile with larger MAXIMUM_GEOMETRY_STACK_DEPTH constant in readmgf.cpp");
        return;
    } else {
        *globalGeometryStackPtr = globalCurrentGeometryList;
        globalGeometryStackPtr++;
        globalCurrentGeometryList = nullptr;
    }
}

static void
popCurrentGeometryList() {
    if ( globalGeometryStackPtr <= globalGeometryStack ) {
        doError("Object stack underflow ... unbalanced 'o' contexts?");
        globalCurrentGeometryList = nullptr;
        return;
    } else {
        globalGeometryStackPtr--;
        globalCurrentGeometryList = *globalGeometryStackPtr;
    }
}

/**
Returns squared distance between the two FVECTs
*/
static double
distanceSquared(FVECT *v1, FVECT *v2) {
    FVECT d;

    d[0] = (*v2)[0] - (*v1)[0];
    d[1] = (*v2)[1] - (*v1)[1];
    d[2] = (*v2)[2] - (*v1)[2];
    return d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
}

/**
The mgf parser already contains some good routines for discretizing spheres, ...
into polygons. In the official release of the parser library, these routines
are internal (declared static in parse.c and no reference to them in parser.h).
The parser was changed so we can call them in order not to have to duplicate
the code
*/
static int
doDiscretize(int argc, char **argv) {
    int en = mgfEntity(argv[0]);

    switch ( en ) {
        case MGF_ERROR_SPHERE:
            return mgfEntitySphere(argc, argv);
        case MGF_ERROR_TORUS:
            return mgfEntityTorus(argc, argv);
        case MGF_ERROR_CYLINDER:
            return mgfEntityCylinder(argc, argv);
        case MGF_ERROR_RING:
            return mgfEntityRing(argc, argv);
        case MGF_ERROR_CONE:
            return mgfEntityCone(argc, argv);
        case MGF_ERROR_PRISM:
            return mgfEntityPrism(argc, argv);
        default:
            logFatal(4, "mgf.c: doDiscretize", "Unsupported geometry entity number %d", en);
    }

    return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE; // Definitely illegal when this point is reached
}

static void
specSamples(COLOR &col, float *rgb) {
    rgb[0] = col.spec[0];
    rgb[1] = col.spec[1];
    rgb[2] = col.spec[2];
}

/**
Sets the number of quarter circle divisions for discretizing cylinders, spheres, cones, etc.
*/
static void
mgfSetNrQuartCircDivs(int divs) {
    if ( divs <= 0 ) {
        logError(nullptr, "Number of quarter circle divisions (%d) should be positive", divs);
        return;
    }

    GLOBAL_mgf_divisionsPerQuarterCircle = divs;
}

/**
Sets a flag indicating whether all surfaces should be considered
1-sided (the best for "good" models) or not.

When the argument is false, surfaces are considered two-sided unless
explicitely specified 1-sided in the mgf file. When the argument is true,
the specified sidedness in the mgf file are ignored and all surfaces
considered 1-sided. This may yield a significant speedup for "good"
models (containing only solids with consistently defined
face normals and vertices in counter clockwise order as seen from the
normal direction).
*/
static void
mgfSetIgnoreSidedness(int yesno) {
    globalAllSurfacesSided = yesno;
}

/**
If yesno is true, all materials will be converted to be GLOBAL_fileOptions_monochrome.
*/
static void
mgfSetMonochrome(int yesno) {
    GLOBAL_fileOptions_monochrome = yesno;
}

static void
newSurface() {
    globalCurrentPointList = new java::ArrayList<Vector3D *>();
    globalCurrentNormalList = new java::ArrayList<Vector3D *>();
    globalCurrentVertexList = new java::ArrayList<Vertex *>();
    globalCurrentFaceList = new java::ArrayList<Patch *>();
    globalInSurface = true;
}

static void
surfaceDone() {
    if ( globalCurrentGeometryList == nullptr ) {
        globalCurrentGeometryList = new java::ArrayList<Geometry *>();
    }

    if ( globalCurrentFaceList != nullptr ) {
        Geometry *newGeometry = geomCreateSurface(
            surfaceCreate(
                globalCurrentMaterial,
                globalCurrentPointList,
                globalCurrentNormalList,
                nullptr, // null texture coordinate list
                globalCurrentVertexList,
                globalCurrentFaceList,
                MaterialColorFlags::NO_COLORS));
        globalCurrentGeometryList->add(0, newGeometry);
    }
    globalInSurface = false;
}

/**
This routine returns true if the current material has changed
*/
static int
materialChanged() {
    char *materialName;

    materialName = GLOBAL_mgf_currentMaterialName;
    if ( !materialName || *materialName == '\0' ) {
        // This might cause strcmp to crash!
        materialName = (char *) "unnamed";
    }

    // Is it another material than the one used for the previous face? If not, the
    // globalCurrentMaterial remains the same
    if ( strcmp(materialName, globalCurrentMaterial->name) == 0 && GLOBAL_mgf_currentMaterial->clock == 0 ) {
        return false;
    }

    return true;
}

/**
Translates mgf color into out color representation
*/
static void
mgfGetColor(MgfColorContext *cin, float intensity, COLOR *colorOut) {
    float xyz[3];
    float rgb[3];

    mgfContextFixColorRepresentation(cin, C_CSXY);
    if ( cin->cy > EPSILON ) {
        xyz[0] = cin->cx / cin->cy * intensity;
        xyz[1] = 1.0f * intensity;
        xyz[2] = (1.0f - cin->cx - cin->cy) / cin->cy * intensity;
    } else {
        doWarning("invalid color specification (Y<=0) ... setting to black");
        xyz[0] = xyz[1] = xyz[2] = 0.;
    }

    if ( xyz[0] < 0. || xyz[1] < 0. || xyz[2] < 0. ) {
        doWarning("invalid color specification (negative CIE XYZ components) ... clipping to zero");
        if ( xyz[0] < 0.0 ) {
            xyz[0] = 0.0;
        }
        if ( xyz[1] < 1.0 ) {
            xyz[1] = 0.0;
        }
        if ( xyz[2] < 2.0 ) {
            xyz[2] = 0.0;
        }
    }

    transformColorFromXYZ2RGB(xyz, rgb);
    if ( clipGamut(rgb)) {
        doWarning("color desaturated during gamut clipping");
    }
    colorSet(*colorOut, rgb[0], rgb[1], rgb[2]);
}

float
colorMax(COLOR col) {
    // We should check every wavelength in the visible spectrum, but
    // as a first approximation, only the three RGB primary colors
    // are checked
    float samples[NUMBER_OF_SAMPLES], mx;
    int i;

    specSamples(col, samples);

    mx = -HUGE;
    for ( i = 0; i < NUMBER_OF_SAMPLES; i++ ) {
        if ( samples[i] > mx ) {
            mx = samples[i];
        }
    }

    return mx;
}

/**
Looks up a material with given name in the given material list. Returns
a pointer to the material if found, or (Material *)nullptr if not found
*/
static Material *
materialLookup(char *name) {
    for ( int i = 0; GLOBAL_scene_materials != nullptr && i < GLOBAL_scene_materials->size(); i++ ) {
        Material *m = GLOBAL_scene_materials->get(i);
        if ( m != nullptr && m->name != nullptr && strcmp(m->name, name) == 0 ) {
            return m;
        }
    }
    return nullptr;
}

/**
This routine checks whether the mgf material being used has changed. If it
changed, this routine converts to our representation of materials and
creates a new MATERIAL, which is added to the global material library.
The routine returns true if the material being used has changed
*/
static int
getCurrentMaterial() {
    COLOR Ed;
    COLOR Es;
    COLOR Rd;
    COLOR Td;
    COLOR Rs;
    COLOR Ts;
    COLOR A;
    float Ne;
    float Nr;
    float Nt;
    float a;
    char *materialName;

    materialName = GLOBAL_mgf_currentMaterialName;
    if ( !materialName || *materialName == '\0' ) {
        // This might cause strcmp to crash!
        materialName = (char *)"unnamed";
    }

    // Is it another material than the one used for the previous face ?? If not, the
    // globalCurrentMaterial remains the same
    if ( strcmp(materialName, globalCurrentMaterial->name) == 0 && GLOBAL_mgf_currentMaterial->clock == 0 ) {
        return false;
    }

    Material *theMaterial = materialLookup(materialName);
    if ( theMaterial != nullptr ) {
        if ( GLOBAL_mgf_currentMaterial->clock == 0 ) {
            globalCurrentMaterial = theMaterial;
            return true;
        }
    }

    // New material, or a material that changed. Convert intensities and chromaticities
    // to our color model
    mgfGetColor(&GLOBAL_mgf_currentMaterial->ed_c, GLOBAL_mgf_currentMaterial->ed, &Ed);
    mgfGetColor(&GLOBAL_mgf_currentMaterial->rd_c, GLOBAL_mgf_currentMaterial->rd, &Rd);
    mgfGetColor(&GLOBAL_mgf_currentMaterial->td_c, GLOBAL_mgf_currentMaterial->td, &Td);
    mgfGetColor(&GLOBAL_mgf_currentMaterial->rs_c, GLOBAL_mgf_currentMaterial->rs, &Rs);
    mgfGetColor(&GLOBAL_mgf_currentMaterial->ts_c, GLOBAL_mgf_currentMaterial->ts, &Ts);

    // Check/correct range of reflectances and transmittances
    colorAdd(Rd, Rs, A);
    if ( (a = colorMax(A)) > 1.0f - (float)EPSILON ) {
        doWarning("invalid material specification: total reflectance shall be < 1");
        a = (1.0f - (float)EPSILON) / a;
        colorScale(a, Rd, Rd);
        colorScale(a, Rs, Rs);
    }

    colorAdd(Td, Ts, A);
    if ( (a = colorMax(A)) > 1.0f - (float)EPSILON ) {
        doWarning("invalid material specification: total transmittance shall be < 1");
        a = (1.0f - (float)EPSILON) / a;
        colorScale(a, Td, Td);
        colorScale(a, Ts, Ts);
    }

    // Convert lumen / m^2 to W / m^2
    colorScale((1.0 / WHITE_EFFICACY), Ed, Ed);

    colorClear(Es);
    Ne = 0.0;

    // Specular power = (0.6/roughness)^2 (see mgf docs)
    if ( GLOBAL_mgf_currentMaterial->rs_a != 0.0 ) {
        Nr = 0.6f / GLOBAL_mgf_currentMaterial->rs_a;
        Nr *= Nr;
    } else {
        Nr = 0.0;
    }

    if ( GLOBAL_mgf_currentMaterial->ts_a != 0.0 ) {
        Nt = 0.6f / GLOBAL_mgf_currentMaterial->ts_a;
        Nt *= Nt;
    } else {
        Nt = 0.0;
    }

    if ( GLOBAL_fileOptions_monochrome ) {
        colorSetMonochrome(Ed, colorGray(Ed));
        colorSetMonochrome(Es, colorGray(Es));
        colorSetMonochrome(Rd, colorGray(Rd));
        colorSetMonochrome(Rs, colorGray(Rs));
        colorSetMonochrome(Td, colorGray(Td));
        colorSetMonochrome(Ts, colorGray(Ts));
    }

    theMaterial = materialCreate(materialName,
                                 (colorNull(Ed) && colorNull(Es)) ? nullptr : edfCreate(
                                         phongEdfCreate(&Ed, &Es, Ne), &GLOBAL_scene_phongEdfMethods),
                                 bsdfCreate(splitBsdfCreate(
                                                    (colorNull(Rd) && colorNull(Rs)) ? nullptr : brdfCreate(
                                                            phongBrdfCreate(&Rd, &Rs, Nr), &GLOBAL_scene_phongBrdfMethods),
                                                    (colorNull(Td) && colorNull(Ts)) ? nullptr : btdfCreate(
                                                            phongBtdfCreate(&Td, &Ts, Nt,
                                                                            GLOBAL_mgf_currentMaterial->nr,
                                                                            GLOBAL_mgf_currentMaterial->ni),
                                                            &GLOBAL_scene_phongBtdfMethods), nullptr),
                                            &GLOBAL_scene_splitBsdfMethods),
                                 globalAllSurfacesSided ? 1 : GLOBAL_mgf_currentMaterial->sided);

    GLOBAL_scene_materials->add(0, theMaterial);
    globalCurrentMaterial = theMaterial;

    // Reset the clock value to be aware of possible changes in future
    GLOBAL_mgf_currentMaterial->clock = 0;

    return true;
}

static Vector3D *
installPoint(float x, float y, float z) {
    Vector3D *coord = VectorCreate(x, y, z);
    globalCurrentPointList->add(0, coord);
    return coord;
}

static Vector3D *
installNormal(float x, float y, float z) {
    Vector3D *norm = VectorCreate(x, y, z);
    globalCurrentNormalList->add(0, norm);
    return norm;
}

static Vertex *
installVertex(Vector3D *coord, Vector3D *norm, char *name) {
    java::ArrayList<Patch *> *newPatchList = new java::ArrayList<Patch *>();
    Vertex *v = vertexCreate(coord, norm, nullptr, newPatchList);
    globalCurrentVertexList->add(v);
    return v;
}

static Vertex *
getVertex(char *name) {
    MgfVertexContext *vp;
    Vertex *theVertex;

    vp = getNamedVertex(name);
    if ( vp == nullptr ) {
        return nullptr;
    }

    theVertex = (Vertex *) (vp->client_data);
    if ( !theVertex || vp->clock >= 1 || vp->xid != xf_xid(GLOBAL_mgf_xfContext) || is0vect(vp->n)) {
        // New vertex, or updated vertex or same vertex, but other transform, or
        // vertex without normal: create a new Vertex
        FVECT vert;
        FVECT norm;
        Vector3D *theNormal;
        Vector3D *thePoint;

        mgfTransformPoint(vert, vp->p);
        thePoint = installPoint((float)vert[0], (float)vert[1], (float)vert[2]);
        if ( is0vect(vp->n)) {
            theNormal = nullptr;
        } else {
            mgfTransformVector(norm, vp->n);
            theNormal = installNormal((float)norm[0], (float)norm[1], (float)norm[2]);
        }
        theVertex = installVertex(thePoint, theNormal, name);
        vp->client_data = (void *) theVertex;
        vp->xid = xf_xid(GLOBAL_mgf_xfContext);
    }
    vp->clock = 0;

    return theVertex;
}

/**
Create a vertex with given name, but with reversed normal as
the given vertex. For back-faces of two-sided surfaces
*/
static Vertex *
getBackFaceVertex(Vertex *v, char *name) {
    Vertex *back = v->back;

    if ( !back ) {
        Vector3D *the_point, *the_normal;

        the_point = v->point;
        the_normal = v->normal;
        if ( the_normal ) {
            the_normal = installNormal(-the_normal->x, -the_normal->y, -the_normal->z);
        }

        back = v->back = installVertex(the_point, the_normal, name);
        back->back = v;
    }

    return back;
}

static Patch *
newFace(Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4, Vector3D *normal) {
    Patch *theFace;
    int numberOfVertices = v4 ? 4 : 3;

    if ( v1 == nullptr || v2 == nullptr || v3 == nullptr || (numberOfVertices == 4 && v4 == nullptr) ) {
        return nullptr;
    }

    if ( GLOBAL_mgf_xfContext && GLOBAL_mgf_xfContext->rev ) {
        theFace = new Patch(numberOfVertices, v3, v2, v1, v4);
    } else {
        theFace = new Patch(numberOfVertices, v1, v2, v3, v4);
    }

    if ( theFace != nullptr ) {
        globalCurrentFaceList->add(0, theFace);
    }

    return theFace;
}

/**
Computes the normal to the patch plane
*/
static Vector3D *
faceNormal(int numberOfVertices, Vertex **v, Vector3D *normal) {
    double norm;
    Vector3D prev;
    Vector3D cur;
    Vector3D n;
    int i;

    VECTORSET(n, 0, 0, 0);
    VECTORSUBTRACT(*(v[numberOfVertices - 1]->point), *(v[0]->point), cur);
    for ( i = 0; i < numberOfVertices; i++ ) {
        prev = cur;
        VECTORSUBTRACT(*(v[i]->point), *(v[0]->point), cur);
        n.x += (prev.y - cur.y) * (prev.z + cur.z);
        n.y += (prev.z - cur.z) * (prev.x + cur.x);
        n.z += (prev.x - cur.x) * (prev.y + cur.y);
    }
    norm = VECTORNORM(n);

    if ( norm < EPSILON ) {
        // Degenerate normal --> degenerate polygon
        return nullptr;
    }
    VECTORSCALEINVERSE((float)norm, n, n);
    *normal = n;

    return normal;
}

/**
Tests whether the polygon is convex or concave. This is accomplished by projecting
onto the coordinate plane "most parallel" to the polygon and checking the signs
the cross product of succeeding edges: the signs are all equal for a convex polygon
*/
static int
faceIsConvex(int numberOfVertices, Vertex **v, Vector3D *normal) {
    Vector2Dd v2d[MAXIMUM_FACE_VERTICES + 1], p, c;
    int i, index, sign;

    index = vector3DDominantCoord(normal);
    for ( i = 0; i < numberOfVertices; i++ ) {
	VECTORPROJECT(v2d[i], *(v[i]->point), index);
    }

    p.u = v2d[3].u - v2d[2].u;
    p.v = v2d[3].v - v2d[2].v;
    c.u = v2d[0].u - v2d[3].u;
    c.v = v2d[0].v - v2d[3].v;
    sign = (p.u * c.v > c.u * p.v) ? 1 : -1;

    for ( i = 1; i < numberOfVertices; i++ ) {
        p.u = c.u;
        p.v = c.v;
        c.u = v2d[i].u - v2d[i - 1].u;
        c.v = v2d[i].v - v2d[i - 1].v;
        if (((p.u * c.v > c.u * p.v) ? 1 : -1) != sign ) {
            return false;
        }
    }

    return true;
}

/**
Returns true if the 2D point p is inside the 2D triangle p1-p2-p3
*/
static int
pointInsideTriangle2D(Vector2D *p, Vector2D *p1, Vector2D *p2, Vector2D *p3) {
    double u0, v0, u1, v1, u2, v2, a, b;

    // From Graphics Gems I, Didier Badouel, An Efficient Ray-Polygon Intersection, p390
    u0 = p->u - p1->u;
    v0 = p->v - p1->v;
    u1 = p2->u - p1->u;
    v1 = p2->v - p1->v;
    u2 = p3->u - p1->u;
    v2 = p3->v - p1->v;

    a = 10.0;
    b = 10.0; // Values large enough so the result would be false
    if ( std::fabs(u1) < EPSILON ) {
        if ( std::fabs(u2) > EPSILON && std::fabs(v1) > EPSILON ) {
            b = u0 / u2;
            if ( b < EPSILON || b > 1. - EPSILON ) {
                return false;
            } else {
                a = (v0 - b * v2) / v1;
            }
        }
    } else {
        b = v2 * u1 - u2 * v1;
        if ( std::fabs(b) > EPSILON ) {
            b = (v0 * u1 - u0 * v1) / b;
            if ( b < EPSILON || b > 1.0 - EPSILON ) {
                return false;
            } else {
                a = (u0 - b * u2) / u1;
            }
        }
    }

    return (a >= EPSILON && a <= 1.0 - EPSILON && (a + b) <= 1.0 - EPSILON);
}

/**
Returns true if the 2D segments p1-p2 and p3-p4 intersect
*/
static int
segmentsIntersect2D(Vector2D *p1, Vector2D *p2, Vector2D *p3, Vector2D *p4) {
    double a, b, c, du, dv, r1, r2, r3, r4;
    int colinear = false;

    // From Graphics Gems II, Mukesh Prasad, Intersection of Line Segments, p7
    du = fabs(p2->u - p1->u);
    dv = fabs(p2->v - p1->v);
    if ( du > EPSILON || dv > EPSILON ) {
        if ( dv > du ) {
            a = 1.0;
            b = -(p2->u - p1->u) / (p2->v - p1->v);
            c = -(p1->u + b * p1->v);
        } else {
            a = -(p2->v - p1->v) / (p2->u - p1->u);
            b = 1.0;
            c = -(a * p1->u + p1->v);
        }

        r3 = a * p3->u + b * p3->v + c;
        r4 = a * p4->u + b * p4->v + c;

        if ( fabs(r3) < EPSILON && fabs(r4) < EPSILON ) {
            colinear = true;
        } else if ((r3 > -EPSILON && r4 > -EPSILON) || (r3 < EPSILON && r4 < EPSILON)) {
            return false;
        }
    }

    if ( !colinear ) {
        du = fabs(p4->u - p3->u);
        dv = fabs(p4->v - p3->v);
        if ( du > EPSILON || dv > EPSILON ) {
            if ( dv > du ) {
                a = 1.0;
                b = -(p4->u - p3->u) / (p4->v - p3->v);
                c = -(p3->u + b * p3->v);
            } else {
                a = -(p4->v - p3->v) / (p4->u - p3->u);
                b = 1.0;
                c = -(a * p3->u + p3->v);
            }

            r1 = a * p1->u + b * p1->v + c;
            r2 = a * p2->u + b * p2->v + c;

            if ( fabs(r1) < EPSILON && fabs(r2) < EPSILON ) {
                colinear = true;
            } else if ((r1 > -EPSILON && r2 > -EPSILON) || (r1 < EPSILON && r2 < EPSILON)) {
                return false;
            }
        }
    }

    if ( !colinear ) {
        return true;
    }

    return false; // Co-linear segments never intersect: do as if they are always
		 // a bit apart from each other
}

/**
Handles concave faces and faces with >4 vertices. This routine started as an
ANSI-C version of mgflib/face2tri.C, but I changed it a lot to make it more robust.
Inspiration comes from Burger and Gillis, Interactive Computer Graphics and
the (indispensable) Graphics Gems books
*/
static int
doComplexFace(int n, Vertex **v, Vector3D *normal, Vertex **backv, Vector3D *backnormal) {
    int i;
    int j;
    int max;
    int p0;
    int p1;
    int p2;
    int corners;
    int start;
    int good;
    int index;
    double maxD;
    double d;
    double a;
    Vector3D center;
    char out[MAXIMUM_FACE_VERTICES + 1];
    Vector2D q[MAXIMUM_FACE_VERTICES + 1];
    Vector3D nn;

    VECTORSET(center, 0.0, 0.0, 0.0);
    for ( i = 0; i < n; i++ ) VECTORADD(center, *(v[i]->point), center);
    VECTORSCALEINVERSE((float) n, center, center);

    maxD = VECTORDIST(center, *(v[0]->point));
    max = 0;
    for ( i = 1; i < n; i++ ) {
        d = VECTORDIST(center, *(v[i]->point));
        if ( d > maxD ) {
            maxD = d;
            max = i;
        }
    }

    for ( i = 0; i < n; i++ ) {
        out[i] = false;
    }

    p1 = max;
    p0 = p1 - 1;
    if ( p0 < 0 ) {
        p0 = n - 1;
    }
    p2 = (p1 + 1) % n;
    VECTORTRIPLECROSSPRODUCT(*(v[p0]->point), *(v[p1]->point), *(v[p2]->point), *normal);
    VECTORNORMALIZE(*normal);
    index = vector3DDominantCoord(normal);

    for ( i = 0; i < n; i++ ) {
	    VECTORPROJECT(q[i], *(v[i]->point), index);
    }

    corners = n;
    p0 = -1;
    a = 0.0; // To make gcc -Wall not complain

    while ( corners >= 3 ) {
        start = p0;

        do {
            p0 = (p0 + 1) % n;
            while ( out[p0] ) {
                p0 = (p0 + 1) % n;
            }

            p1 = (p0 + 1) % n;
            while ( out[p1] ) {
                p1 = (p1 + 1) % n;
            }

            p2 = (p1 + 1) % n;
            while ( out[p2] ) {
                p2 = (p2 + 1) % n;
            }

            if ( p0 == start ) {
                break;
            }

            VECTORTRIPLECROSSPRODUCT(*(v[p0]->point), *(v[p1]->point), *(v[p2]->point), nn);
            a = VECTORNORM(nn);
            VECTORSCALEINVERSE((float)a, nn, nn);
            d = VECTORDIST(nn, *normal);

            good = true;
            if ( d <= 1.0 ) {
                for ( i = 0; i < n && good; i++ ) {
                    if ( out[i] || v[i] == v[p0] || v[i] == v[p1] || v[i] == v[p2] ) {
                        continue;
                    }

                    if ( pointInsideTriangle2D(&q[i], &q[p0], &q[p1], &q[p2])) {
                        good = false;
                    }

                    j = (i + 1) % n;
                    if ( out[j] || v[j] == v[p0] ) {
                        continue;
                    }

                    if ( segmentsIntersect2D(&q[p2], &q[p0], &q[i], &q[j])) {
                        good = false;
                    }
                }
            }
        } while ( d > 1.0 || !good );

        if ( p0 == start ) {
            doError("misbuilt polygonal face");
            return MGF_OK; // Don't stop parsing the input however
        }

        if ( std::fabs(a) > EPSILON ) {
            // Avoid degenerate faces
            Patch *face, *twin;
            face = newFace(v[p0], v[p1], v[p2], nullptr, normal);
            if ( !globalCurrentMaterial->sided ) {
                twin = newFace(backv[p2], backv[p1], backv[p0], nullptr, backnormal);
                face->twin = twin;
                twin->twin = face;
            }
        }

        out[p1] = true;
        corners--;
    }

    return MGF_OK;
}

static int
handleFaceEntity(int argc, char **argv) {
    Vertex *v[MAXIMUM_FACE_VERTICES + 1];
    Vertex *backV[MAXIMUM_FACE_VERTICES + 1];
    Vector3D normal;
    Vector3D backNormal;
    Patch *face;
    Patch *twin;
    int i;
    int errcode;

    if ( argc < 4 ) {
        doError("too few vertices in face");
        return MGF_OK; // Don't stop parsing the input
    }

    if ( argc - 1 > MAXIMUM_FACE_VERTICES ) {
        doWarning(
                "too many vertices in face. Recompile the program with larger MAXIMUM_FACE_VERTICES constant in readmgf.c");
        return MGF_OK; // No reason to stop parsing the input
    }

    if ( !globalInComplex ) {
        if ( materialChanged()) {
            if ( globalInSurface ) {
                surfaceDone();
            }
            newSurface();
            getCurrentMaterial();
        }
    }

    for ( i = 0; i < argc - 1; i++ ) {
        v[i] = getVertex(argv[i + 1]);
        if ( v[i] == nullptr ) {
            // This is however a reason to stop parsing the input
            return MGF_ERROR_UNDEFINED_REFERENCE;
        }
        backV[i] = nullptr;
        if ( !globalCurrentMaterial->sided )
            backV[i] = getBackFaceVertex(v[i], argv[i + 1]);
    }

    if ( !faceNormal(argc - 1, v, &normal)) {
        doWarning("degenerate face");
        return MGF_OK; // Just ignore the generated face
    }
    if ( !globalCurrentMaterial->sided ) VECTORSCALE(-1.0, normal, backNormal);

    errcode = MGF_OK;
    if ( argc == 4 ) {
        // Triangles
        face = newFace(v[0], v[1], v[2], nullptr, &normal);
        if ( !globalCurrentMaterial->sided ) {
            twin = newFace(backV[2], backV[1], backV[0], nullptr, &backNormal);
            face->twin = twin;
            twin->twin = face;
        }
    } else if ( argc == 5 ) {
        // Quadrilaterals
        if ( globalInComplex || faceIsConvex(argc - 1, v, &normal)) {
            face = newFace(v[0], v[1], v[2], v[3], &normal);
            if ( !globalCurrentMaterial->sided ) {
                twin = newFace(backV[3], backV[2], backV[1], backV[0], &backNormal);
                face->twin = twin;
                twin->twin = face;
            }
        } else {
            errcode = doComplexFace(argc - 1, v, &normal, backV, &backNormal);
        }
    } else {
        // More than 4 vertices
        errcode = doComplexFace(argc - 1, v, &normal, backV, &backNormal);
    }

    return errcode;
}

/**
Eliminates the holes by creating seems to the nearest vertex
on another contour. Creates an argument list for the face
without hole entity handling routine handleFaceEntity() and calls it
*/
static int
handleFaceWithHolesEntity(int argc, char **argv) {
    FVECT v[MAXIMUM_FACE_VERTICES + 1]; // v[i] = location of vertex argv[i]
    char *nargv[MAXIMUM_FACE_VERTICES + 1], // Arguments to be passed to the face
                                            // without hole entity handler
    copied[MAXIMUM_FACE_VERTICES + 1]; // copied[i] is 1 or 0 indicating if
                                       // the vertex argv[i] has been copied to new contour
    int newContour[MAXIMUM_FACE_VERTICES]; // newContour[i] will contain the i-th
                                           // vertex of the face with eliminated holes
    int i;
    int numberOfVerticesInNewContour;

    if ( argc - 1 > MAXIMUM_FACE_VERTICES ) {
        doWarning(
                "too many vertices in face. Recompile the program with larger MAXIMUM_FACE_VERTICES constant in readmgf.c");
        return MGF_OK; // No reason to stop parsing the input
    }

    // Get the location of the vertices: the location of the vertex
    // argv[i] is kept in v[i] (i=1 ... argc-1, and argv[i] not a contour
    // separator)
    for ( i = 1; i < argc; i++ ) {
        MgfVertexContext *vp;

        if ( *argv[i] == '-' ) {
            // Skip contour separators
            continue;
        }

        vp = getNamedVertex(argv[i]);
        if ( !vp ) {
            // Undefined vertex
            return MGF_ERROR_UNDEFINED_REFERENCE;
        }
        mgfTransformPoint(v[i], vp->p); // Transform with the current transform

        copied[i] = false; // Vertex not yet copied to nargv
    }

    // Copy the outer contour
    numberOfVerticesInNewContour = 0;
    for ( i = 1; i < argc && *argv[i] != '-'; i++ ) {
        newContour[numberOfVerticesInNewContour++] = i;
        copied[i] = true;
    }

    // Find next not yet copied vertex in argv (i++ should suffice, but
    // this way we can also skip multiple "-"s ...)
    for ( ; i < argc; i++ ) {
        if ( *argv[i] == '-' ) {
            // Skip contour separators
            continue;
        }
        if ( !copied[i] ) {
            // Not yet copied vertex encountered
            break;
        }
    }

    while ( i < argc ) {
        // First i vertex of a hole that is not yet eliminated
        int nearestCopied;
        int nearestOther;
        int first;
        int last;
        int num;
        int j;
        int k;
        double minimumDistance;

        // Find the not yet copied vertex that is nearest to the already
        // copied ones
        nearestCopied = nearestOther = 0;
        minimumDistance = HUGE;
        for ( j = i; j < argc; j++ ) {
            if ( *argv[j] == '-' || copied[j] ) {
                continue;
            }
            // Contour separator or already copied vertex

            for ( k = 0; k < numberOfVerticesInNewContour; k++ ) {
                double d = distanceSquared(&v[j], &v[newContour[k]]);
                if ( d < minimumDistance ) {
                    minimumDistance = d;
                    nearestCopied = k;
                    nearestOther = j;
                }
            }
        }

        // Find first vertex of this nearest contour
        for ( first = nearestOther; *argv[first] != '-'; first-- ) {
        }
        first++;

        // Find last vertex in this nearest contour
        for ( last = nearestOther; last < argc && *argv[last] != '-'; last++ ) {
        }
        last--;

        // Number of vertices in the nearest contour
        num = last - first + 1;

        // Create num+2 extra vertices in new contour
        if ( numberOfVerticesInNewContour + num + 2 > MAXIMUM_FACE_VERTICES ) {
            doWarning(
                    "too many vertices in face. Recompile the program with larger MAXIMUM_FACE_VERTICES constant in readmgf.c");
            return MGF_OK; // No reason to stop parsing the input
        }

        // Shift the elements in new contour starting at position nearestCopied
        // num+2 places further. Vertex new contour[nearestCopied] will be connected
        // to vertex nearestOther ... last, first ... nearestOther and
        // back to newContour[nearestCopied]
        for ( k = numberOfVerticesInNewContour - 1; k >= nearestCopied; k-- ) {
            newContour[k + num + 2] = newContour[k];
        }
        numberOfVerticesInNewContour += num + 2;

        // Insert the vertices of the nearest contour (closing the loop)
        k = nearestCopied + 1;
        for ( j = nearestOther; j <= last; j++ ) {
            newContour[k++] = j;
            copied[j] = true;
        }
        for ( j = first; j <= nearestOther; j++ ) {
            newContour[k++] = j;
            copied[j] = true;
        }

        // Find next not yet copied vertex in argv
        for ( ; i < argc; i++ ) {
            if ( *argv[i] == '-' ) {
                continue;
            }
            // Skip contour separators
            if ( !copied[i] ) {
                // Not yet copied vertex encountered
                break;
            }
        }
    }

    // Build an argument list for the new polygon without holes
    nargv[0] = (char *) "f";
    for ( i = 0; i < numberOfVerticesInNewContour; i++ ) {
        nargv[i + 1] = argv[newContour[i]];
    }

    // And handle the face without holes
    return handleFaceEntity(numberOfVerticesInNewContour + 1, nargv);
}

static int
handleSurfaceEntity(int argc, char **argv) {
    int errcode;

    if ( globalInComplex ) {
        // mgfEntitySphere calls mgfEntityCone
        return doDiscretize(argc, argv);
    } else {
        globalInComplex = true;
        if ( globalInSurface ) {
            surfaceDone();
        }
        newSurface();
        getCurrentMaterial();

        errcode = doDiscretize(argc, argv);

        surfaceDone();
        globalInComplex = false;

        return errcode;
    }
}

static int
handleObjectEntity(int argc, char **argv) {
    int i;

    if ( argc > 1 ) {
        // Beginning of a new object
        for ( i = 0; i < globalGeometryStackPtr - globalGeometryStack; i++ ) {
            fprintf(stderr, "\t");
        }
        fprintf(stderr, "%s ...\n", argv[1]);

        if ( globalInSurface ) {
            surfaceDone();
        }

        pushCurrentGeometryList();

        newSurface();
    } else {
        // End of object definition
        Geometry *theGeometry = nullptr;

        if ( globalInSurface ) {
            surfaceDone();
        }

        long listSize = 0;
        if ( globalCurrentGeometryList != nullptr ) {
            listSize += globalCurrentGeometryList->size();
        }

        if ( listSize > 0 ) {
            theGeometry = geomCreateCompound(new Compound(globalCurrentGeometryList));
        }

        popCurrentGeometryList();

        if ( theGeometry ) {
            globalCurrentGeometryList->add(0, theGeometry);
            GLOBAL_scene_geometries = globalCurrentGeometryList;
        }

        newSurface();
    }

    return handleObject2Entity(argc, argv);
}

static int
handleUnknownEntity(int argc, char **argv) {
    doWarning("unknown entity");

    return MGF_OK;
}

static void
initMgf() {
    GLOBAL_mgf_handleCallbacks[MG_E_FACE] = handleFaceEntity;
    GLOBAL_mgf_handleCallbacks[MG_E_FACEH] = handleFaceWithHolesEntity;
    GLOBAL_mgf_handleCallbacks[MG_E_VERTEX] = handleVertexEntity;
    GLOBAL_mgf_handleCallbacks[MG_E_POINT] = handleVertexEntity;
    GLOBAL_mgf_handleCallbacks[MG_E_NORMAL] = handleVertexEntity;
    GLOBAL_mgf_handleCallbacks[MG_E_COLOR] = handleColorEntity;
    GLOBAL_mgf_handleCallbacks[MG_E_CXY] = handleColorEntity;
    GLOBAL_mgf_handleCallbacks[MG_E_CMIX] = handleColorEntity;
    GLOBAL_mgf_handleCallbacks[MG_E_MATERIAL] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MG_E_ED] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MG_E_IR] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MG_E_RD] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MG_E_RS] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MG_E_SIDES] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MG_E_TD] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MG_E_TS] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MG_E_OBJECT] = handleObjectEntity;
    GLOBAL_mgf_handleCallbacks[MG_E_XF] = handleTransformationEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ERROR_SPHERE] = handleSurfaceEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ERROR_TORUS] = handleSurfaceEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ERROR_RING] = handleSurfaceEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ERROR_CYLINDER] = handleSurfaceEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ERROR_CONE] = handleSurfaceEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ERROR_PRISM] = handleSurfaceEntity;
    GLOBAL_mgf_unknownEntityHandleCallback = handleUnknownEntity;

    mgfAlternativeInit(GLOBAL_mgf_handleCallbacks);
}

/**
Reads in an mgf file. The result is that the global variables
GLOBAL_scene_world and GLOBAL_scene_materials are filled in.
*/
void
readMgf(char *filename) {
    MgfReaderContext mgfReaderContext{};
    int status{};

    mgfSetNrQuartCircDivs(GLOBAL_fileOptions_numberOfQuarterCircleDivisions);
    mgfSetIgnoreSidedness(GLOBAL_fileOptions_forceOneSidedSurfaces);
    mgfSetMonochrome(GLOBAL_fileOptions_monochrome);

    initMgf();

    globalPointsOctree = nullptr;
    globalNormalsOctree = nullptr;
    globalCurrentGeometryList = new java::ArrayList<Geometry *>();

    if ( GLOBAL_scene_materials == nullptr ) {
        GLOBAL_scene_materials = new java::ArrayList<Material *>();
    }
    globalCurrentMaterial = &GLOBAL_material_defaultMaterial;

    globalGeometryStackPtr = globalGeometryStack;

    globalInComplex = false;
    globalInSurface = false;

    newSurface();

    if ( filename[0] == '#' ) {
        status = mgfOpen(&mgfReaderContext, nullptr);
    } else {
        status = mgfOpen(&mgfReaderContext, filename);
    }
    if ( status ) {
        doError(GLOBAL_mgf_errors[status]);
    } else {
        while ( mgfReadNextLine() > 0 && !status ) {
            status = mgfParseCurrentLine();
            if ( status ) {
                doError(GLOBAL_mgf_errors[status]);
            }
        }
        mgfClose();
    }
    mgfClear();

    if ( globalInSurface ) {
        surfaceDone();
    }
    GLOBAL_scene_geometries = (globalCurrentGeometryList);

    if ( globalPointsOctree != nullptr) {
        free(globalPointsOctree);
    }
    if ( globalNormalsOctree != nullptr) {
        free(globalNormalsOctree);
    }
}

void
mgfFreeMemory() {
    printf("Freeing %ld geometries\n", globalCurrentGeometryList->size());
    long surfaces = 0;
    long patchSets = 0;
    for ( int i = 0; i < globalCurrentGeometryList->size(); i++ ) {
        if ( globalCurrentGeometryList->get(i)->surfaceData != nullptr ) {
            surfaces++;
        }
        if ( globalCurrentGeometryList->get(i)->patchSetData != nullptr ) {
            patchSets++;
        }
    }
    printf("  - Surfaces: %ld\n", surfaces);
    printf("  - Patch sets: %ld\n", patchSets);
    fflush(stdout);

    for ( int i = 0; i < globalCurrentGeometryList->size(); i++ ) {
        geomDestroy(globalCurrentGeometryList->get(i));
    }
    delete globalCurrentGeometryList;
    globalCurrentGeometryList = nullptr;
    GLOBAL_scene_geometries = nullptr;

    freeLists();
}

void freeLists() {
    if ( globalCurrentPointList != nullptr ) {
        delete globalCurrentPointList;
        globalCurrentPointList = nullptr;
    }

    if ( globalCurrentNormalList != nullptr ) {
        delete globalCurrentNormalList;
        globalCurrentNormalList = nullptr;
    }

    if ( globalCurrentVertexList != nullptr ) {
        delete globalCurrentVertexList;
        globalCurrentVertexList = nullptr;
    }

    if ( globalCurrentFaceList != nullptr ) {
        delete globalCurrentFaceList;
        globalCurrentFaceList = nullptr;
    }
}

#include "java/util/ArrayList.txx"
#include "common/linealAlgebra/vectorMacros.h"
#include "common/error.h"
#include "scene/scene.h"
#include "scene/splitbsdf.h"
#include "scene/phong.h"
#include "io/mgf/parser.h"
#include "io/mgf/vectoroctree.h"
#include "io/mgf/fileopts.h"
#include "io/mgf/readmgf.h"
#include "lookup.h"

// Objects 'o' contexts can be nested this deep
#define MAXIMUM_GEOMETRY_STACK_DEPTH 100

// No face can have more than this vertices
#define MAXIMUM_FACE_VERTICES 100

LUTAB GLOBAL_mgf_vertexLookUpTable = LU_SINIT(free, free);

static VectorOctreeNode *globalPointsOctree = nullptr;
static VectorOctreeNode *globalNormalsOctree = nullptr;

// Elements for surface currently being created
static java::ArrayList<Vector3D *> *globalCurrentPointList = nullptr;
static java::ArrayList<Vector3D *> *globalCurrentNormalList = nullptr;
static java::ArrayList<Vertex *> *globalCurrentVertexList = nullptr;
static java::ArrayList<Patch *> *globalCurrentFaceList = nullptr;
static java::ArrayList<Geometry *> *globalCurrentGeometryList = nullptr;
static Material *globalCurrentMaterial = nullptr;

// Geometry stack: used for building a hierarchical representation of the scene
static java::ArrayList<Geometry *> *globalGeometryStack[MAXIMUM_GEOMETRY_STACK_DEPTH];
static java::ArrayList<Geometry *> **globalGeometryStackPtr = nullptr;

static int globalInComplex = false; // True if reading a sphere, torus or other unsupported
static int globalInSurface = false; // True if busy creating a new surface
static bool globalAllSurfacesSided = false; // When set to true, all surfaces will be considered one-sided

void
doError(const char *errmsg) {
    logError(nullptr, (char *) "%s line %d: %s", GLOBAL_mgf_file->fileName, GLOBAL_mgf_file->lineNumber, errmsg);
}

void
doWarning(const char *errmsg) {
    logWarning(nullptr, (char *) "%s line %d: %s", GLOBAL_mgf_file->fileName, GLOBAL_mgf_file->lineNumber, errmsg);
}

static void
pushCurrentGeometryList() {
    if ( globalGeometryStackPtr - globalGeometryStack >= MAXIMUM_GEOMETRY_STACK_DEPTH ) {
        doError(
                "Objects are nested too deep for this program. Recompile with larger MAXIMUM_GEOMETRY_STACK_DEPTH constant in read mgf");
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
Returns squared distance between the two vectors
*/
static double
distanceSquared(VECTOR3Dd *v1, VECTOR3Dd *v2) {
    VECTOR3Dd d;

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
doDiscretize(int argc, char **argv, RadianceMethod *context) {
    int en = mgfEntity(argv[0]);

    switch ( en ) {
        case MGF_ENTITY_SPHERE:
            return mgfEntitySphere(argc, argv, context);
        case MGF_ENTITY_TORUS:
            return mgfEntityTorus(argc, argv, context);
        case MGF_ENTITY_CYLINDER:
            return mgfEntityCylinder(argc, argv, context);
        case MGF_ENTITY_RING:
            return mgfEntityRing(argc, argv, context);
        case MGF_ENTITY_CONE:
            return mgfEntityCone(argc, argv, context);
        case MGF_ENTITY_PRISM:
            return mgfEntityPrism(argc, argv, context);
        default:
            logFatal(4, "mgf.c: doDiscretize", "Unsupported geometry entity number %d", en);
    }

    return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE; // Definitely illegal when this point is reached
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
mgfSetIgnoreSidedness(bool yesno) {
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
        Geometry *newGeometry = new MeshSurface(
            globalCurrentMaterial,
            globalCurrentPointList,
            globalCurrentNormalList,
            nullptr, // null texture coordinate list
            globalCurrentVertexList,
            globalCurrentFaceList,
            MaterialColorFlags::NO_COLORS);
        globalCurrentGeometryList->add(0, newGeometry);
    }
    globalInSurface = false;
}

static Vector3D *
installPoint(float x, float y, float z) {
    Vector3D *coord = new Vector3D(x, y, z);
    globalCurrentPointList->add(0, coord);
    return coord;
}

static Vector3D *
installNormal(float x, float y, float z) {
    Vector3D *norm = new Vector3D(x, y, z);
    globalCurrentNormalList->add(0, norm);
    return norm;
}

static Vertex *
installVertex(Vector3D *coord, Vector3D *norm) {
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

    theVertex = (Vertex *) (vp->clientData);
    if ( !theVertex || vp->clock >= 1 || vp->xid != xf_xid(GLOBAL_mgf_xfContext) || is0Vector(vp->n)) {
        // New vertex, or updated vertex or same vertex, but other transform, or
        // vertex without normal: create a new Vertex
        VECTOR3Dd vert;
        VECTOR3Dd norm;
        Vector3D *theNormal;
        Vector3D *thePoint;

        mgfTransformPoint(vert, vp->p);
        thePoint = installPoint((float)vert[0], (float)vert[1], (float)vert[2]);
        if ( is0Vector(vp->n)) {
            theNormal = nullptr;
        } else {
            mgfTransformVector(norm, vp->n);
            theNormal = installNormal((float)norm[0], (float)norm[1], (float)norm[2]);
        }
        theVertex = installVertex(thePoint, theNormal);
        vp->clientData = (void *) theVertex;
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
getBackFaceVertex(Vertex *v) {
    Vertex *back = v->back;

    if ( !back ) {
        Vector3D *the_point, *the_normal;

        the_point = v->point;
        the_normal = v->normal;
        if ( the_normal ) {
            the_normal = installNormal(-the_normal->x, -the_normal->y, -the_normal->z);
        }

        back = v->back = installVertex(the_point, the_normal);
        back->back = v;
    }

    return back;
}

static Patch *
newFace(Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4, RadianceMethod *context) {
    Patch *theFace;
    int numberOfVertices = v4 ? 4 : 3;

    if ( v1 == nullptr || v2 == nullptr || v3 == nullptr ) {
        return nullptr;
    }

    if ( GLOBAL_mgf_xfContext && GLOBAL_mgf_xfContext->rev ) {
        theFace = new Patch(numberOfVertices, v3, v2, v1, v4, context);
    } else {
        theFace = new Patch(numberOfVertices, v1, v2, v3, v4, context);
    }

    globalCurrentFaceList->add(0, theFace);

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

    n.set(0, 0, 0);
    vectorSubtract(*(v[numberOfVertices - 1]->point), *(v[0]->point), cur);
    for ( i = 0; i < numberOfVertices; i++ ) {
        prev = cur;
        vectorSubtract(*(v[i]->point), *(v[0]->point), cur);
        n.x += (prev.y - cur.y) * (prev.z + cur.z);
        n.y += (prev.z - cur.z) * (prev.x + cur.x);
        n.z += (prev.x - cur.x) * (prev.y + cur.y);
    }
    norm = vectorNorm(n);

    if ( norm < EPSILON ) {
        // Degenerate normal --> degenerate polygon
        return nullptr;
    }
    vectorScaleInverse((float) norm, n, n);
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
    Vector2D v2d[MAXIMUM_FACE_VERTICES + 1];
    Vector2D p;
    Vector2D c;
    int i;
    int index;
    int sign;

    index = vector3DDominantCoord(normal);
    for ( i = 0; i < numberOfVertices; i++ ) {
        vectorProject(v2d[i], *(v[i]->point), index);
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
        if ( ((p.u * c.v > c.u * p.v) ? 1 : -1) != sign ) {
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
    int coLinear = false;

    // From Graphics Gems II, Mukesh Prasad, Intersection of Line Segments, p7
    du = std::fabs(p2->u - p1->u);
    dv = std::fabs(p2->v - p1->v);
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

        if ( std::fabs(r3) < EPSILON && std::fabs(r4) < EPSILON ) {
            coLinear = true;
        } else if ((r3 > -EPSILON && r4 > -EPSILON) || (r3 < EPSILON && r4 < EPSILON)) {
            return false;
        }
    }

    if ( !coLinear ) {
        du = std::fabs(p4->u - p3->u);
        dv = std::fabs(p4->v - p3->v);
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

            if ( std::fabs(r1) < EPSILON && std::fabs(r2) < EPSILON ) {
                coLinear = true;
            } else if ((r1 > -EPSILON && r2 > -EPSILON) || (r1 < EPSILON && r2 < EPSILON)) {
                return false;
            }
        }
    }

    if ( !coLinear ) {
        return true;
    }

    return false; // Co-linear segments never intersect: do as if they are always
		          // a bit apart from each other
}

/**
Handles concave faces and faces with > 4 vertices. This routine started as an
ANSI-C version of face2tri, but I changed it a lot to make it more robust.
Inspiration comes from Burger and Gillis, Interactive Computer Graphics and
the (indispensable) Graphics Gems books
*/
static void
doComplexFace(int n, Vertex **v, Vector3D *normal, Vertex **backv, RadianceMethod *context) {
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

    center.set(0.0, 0.0, 0.0);
    for ( i = 0; i < n; i++ ) {
        vectorAdd(center, *(v[i]->point), center);
    }
    vectorScaleInverse((float) n, center, center);

    maxD = vectorDist(center, *(v[0]->point));
    max = 0;
    for ( i = 1; i < n; i++ ) {
        d = vectorDist(center, *(v[i]->point));
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
    vectorTripleCrossProduct(*(v[p0]->point), *(v[p1]->point), *(v[p2]->point), *normal);
    vectorNormalize(*normal);
    index = vector3DDominantCoord(normal);

    for ( i = 0; i < n; i++ ) {
        vectorProject(q[i], *(v[i]->point), index);
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

            vectorTripleCrossProduct(*(v[p0]->point), *(v[p1]->point), *(v[p2]->point), nn);
            a = vectorNorm(nn);
            vectorScaleInverse((float) a, nn, nn);
            d = vectorDist(nn, *normal);

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
            return; // Don't stop parsing the input however
        }

        if ( std::fabs(a) > EPSILON ) {
            // Avoid degenerate faces
            Patch *face = newFace(v[p0], v[p1], v[p2], nullptr, context);
            if ( !globalCurrentMaterial->sided && face != nullptr ) {
                Patch *twin = newFace(backv[p2], backv[p1], backv[p0], nullptr, context);
                face->twin = twin;
                if ( twin != nullptr ) {
                    twin->twin = face;
                }
            }
        }

        out[p1] = true;
        corners--;
    }
}

static int
handleFaceEntity(int argc, char **argv, RadianceMethod *context) {
    Vertex *v[MAXIMUM_FACE_VERTICES + 1];
    Vertex *backV[MAXIMUM_FACE_VERTICES + 1];
    Vector3D normal;
    Vector3D backNormal;
    Patch *face;
    Patch *twin;
    int errcode;

    if ( argc < 4 ) {
        doError("too few vertices in face");
        return MGF_OK; // Don't stop parsing the input
    }

    if ( argc - 1 > MAXIMUM_FACE_VERTICES ) {
        doWarning(
                "too many vertices in face. Recompile the program with larger MAXIMUM_FACE_VERTICES constant in read mgf");
        return MGF_OK; // No reason to stop parsing the input
    }

    if ( !globalInComplex ) {
        if ( mgfMaterialChanged(globalCurrentMaterial) ) {
            if ( globalInSurface ) {
                surfaceDone();
            }
            newSurface();
            mgfGetCurrentMaterial(&globalCurrentMaterial, globalAllSurfacesSided);
        }
    }

    for ( int i = 0; i < argc - 1; i++ ) {
        v[i] = getVertex(argv[i + 1]);
        if ( v[i] == nullptr ) {
            // This is however a reason to stop parsing the input
            return MGF_ERROR_UNDEFINED_REFERENCE;
        }
        backV[i] = nullptr;
        if ( !globalCurrentMaterial->sided )
            backV[i] = getBackFaceVertex(v[i]);
    }

    if ( !faceNormal(argc - 1, v, &normal)) {
        doWarning("degenerate face");
        return MGF_OK; // Just ignore the generated face
    }
    if ( !globalCurrentMaterial->sided ) vectorScale(-1.0, normal, backNormal);

    errcode = MGF_OK;
    if ( argc == 4 ) {
        // Triangles
        face = newFace(v[0], v[1], v[2], nullptr, context);
        if ( !globalCurrentMaterial->sided && face != nullptr ) {
            twin = newFace(backV[2], backV[1], backV[0], nullptr, context);
            face->twin = twin;
            if ( twin != nullptr ) {
                twin->twin = face;
            }
        }
    } else if ( argc == 5 ) {
        // Quadrilaterals
        if ( globalInComplex || faceIsConvex(argc - 1, v, &normal)) {
            face = newFace(v[0], v[1], v[2], v[3], context);
            if ( !globalCurrentMaterial->sided && face != nullptr ) {
                twin = newFace(backV[3], backV[2], backV[1], backV[0], context);
                face->twin = twin;
                if ( twin != nullptr ) {
                    twin->twin = face;
                }
            }
        } else {
            doComplexFace(argc - 1, v, &normal, backV, context);
            errcode = MGF_OK;
        }
    } else {
        // More than 4 vertices
        doComplexFace(argc - 1, v, &normal, backV, context);
        errcode = MGF_OK;
    }

    return errcode;
}

/**
Eliminates the holes by creating seems to the nearest vertex
on another contour. Creates an argument list for the face
without hole entity handling routine handleFaceEntity() and calls it
*/
static int
handleFaceWithHolesEntity(int argc, char **argv, RadianceMethod *context) {
    VECTOR3Dd v[MAXIMUM_FACE_VERTICES + 1]; // v[i] = location of vertex argv[i]
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
                "too many vertices in face. Recompile the program with larger MAXIMUM_FACE_VERTICES constant in read mgf");
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
                    "too many vertices in face. Recompile the program with larger MAXIMUM_FACE_VERTICES constant in read mgf");
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
    return handleFaceEntity(numberOfVerticesInNewContour + 1, nargv, context);
}

static int
handleSurfaceEntity(int argc, char **argv, RadianceMethod *context) {
    int errcode;

    if ( globalInComplex ) {
        // mgfEntitySphere calls mgfEntityCone
        return doDiscretize(argc, argv, context);
    } else {
        globalInComplex = true;
        if ( globalInSurface ) {
            surfaceDone();
        }
        newSurface();
        mgfGetCurrentMaterial(&globalCurrentMaterial, globalAllSurfacesSided);

        errcode = doDiscretize(argc, argv, context);

        surfaceDone();
        globalInComplex = false;

        return errcode;
    }
}

static int
handleObjectEntity(int argc, char **argv, RadianceMethod * /*context*/) {
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
handleUnknownEntity(int /*argc*/, char ** /*argv*/) {
    doWarning("unknown entity");

    return MGF_OK;
}

/**
Handle a vertex entity
*/
int
handleVertexEntity(int ac, char **av, RadianceMethod * /*context*/)
{
    LUENT *lp;

    switch ( mgfEntity(av[0]) ) {
        case MGF_ENTITY_VERTEX:
            // get/set vertex context
            if ( ac > 4 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( ac == 1 ) {
                // Set unnamed vertex context
                GLOBAL_mgf_vertexContext = GLOBAL_mgf_defaultVertexContext;
                GLOBAL_mgf_currentVertex = &GLOBAL_mgf_vertexContext;
                GLOBAL_mgf_currentVertexName = nullptr;
                return MGF_OK;
            }
            if ( !isNameWords(av[1]) ) {
                return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }
            lp = lookUpFind(&GLOBAL_mgf_vertexLookUpTable, av[1]);
            // Lookup context
            if ( lp == nullptr ) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            GLOBAL_mgf_currentVertexName = lp->key;
            GLOBAL_mgf_currentVertex = (MgfVertexContext *) lp->data;
            if ( ac == 2 ) {
                // Re-establish previous context
                if ( GLOBAL_mgf_currentVertex == nullptr) {
                    return MGF_ERROR_UNDEFINED_REFERENCE;
                }
                return MGF_OK;
            }
            if ( av[2][0] != '=' || av[2][1] ) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            if ( GLOBAL_mgf_currentVertex == nullptr) {
                // Create new vertex context
                GLOBAL_mgf_currentVertexName = (char *) malloc(strlen(av[1]) + 1);
                if ( !GLOBAL_mgf_currentVertexName ) {
                    return MGF_ERROR_OUT_OF_MEMORY;
                }
                strcpy(GLOBAL_mgf_currentVertexName, av[1]);
                lp->key = GLOBAL_mgf_currentVertexName;
                GLOBAL_mgf_currentVertex = (MgfVertexContext *) malloc(sizeof(MgfVertexContext));
                if ( !GLOBAL_mgf_currentVertex ) {
                    return MGF_ERROR_OUT_OF_MEMORY;
                }
                lp->data = (char *) GLOBAL_mgf_currentVertex;
            }
            if ( ac == 3 ) {
                // Use default template
                *GLOBAL_mgf_currentVertex = GLOBAL_mgf_defaultVertexContext;
                return MGF_OK;
            }
            lp = lookUpFind(&GLOBAL_mgf_vertexLookUpTable, av[3]);
            // Lookup template
            if ( lp == nullptr) {
                return MGF_ERROR_OUT_OF_MEMORY;
            }
            if ( lp->data == nullptr) {
                return MGF_ERROR_UNDEFINED_REFERENCE;
            }
            *GLOBAL_mgf_currentVertex = *(MgfVertexContext *) lp->data;
            GLOBAL_mgf_currentVertex->clock++;
            return MGF_OK;
        case MGF_ENTITY_POINT:
            // Set point
            if ( ac != 4 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2]) || !isFloatWords(av[3])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentVertex->p[0] = strtod(av[1], nullptr);
            GLOBAL_mgf_currentVertex->p[1] = strtod(av[2], nullptr);
            GLOBAL_mgf_currentVertex->p[2] = strtod(av[3], nullptr);
            GLOBAL_mgf_currentVertex->clock++;
            return MGF_OK;
        case MGF_ENTITY_NORMAL:
            // Set normal
            if ( ac != 4 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( !isFloatWords(av[1]) || !isFloatWords(av[2]) || !isFloatWords(av[3])) {
                return MGF_ERROR_ARGUMENT_TYPE;
            }
            GLOBAL_mgf_currentVertex->n[0] = strtod(av[1], nullptr);
            GLOBAL_mgf_currentVertex->n[1] = strtod(av[2], nullptr);
            GLOBAL_mgf_currentVertex->n[2] = strtod(av[3], nullptr);
            normalize(GLOBAL_mgf_currentVertex->n);
            GLOBAL_mgf_currentVertex->clock++;
            return MGF_OK;
    }
    return MGF_ERROR_UNKNOWN_ENTITY;
}

static void
initMgf() {
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_FACE] = handleFaceEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_FACE_WITH_HOLES] = handleFaceWithHolesEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_VERTEX] = handleVertexEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_POINT] = handleVertexEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_NORMAL] = handleVertexEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_COLOR] = handleColorEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_CXY] = handleColorEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_C_MIX] = handleColorEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_MATERIAL] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_ED] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_IR] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_RD] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_RS] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_SIDES] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_TD] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_TS] = handleMaterialEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_OBJECT] = handleObjectEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_XF] = handleTransformationEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_SPHERE] = handleSurfaceEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_TORUS] = handleSurfaceEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_RING] = handleSurfaceEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_CYLINDER] = handleSurfaceEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_CONE] = handleSurfaceEntity;
    GLOBAL_mgf_handleCallbacks[MGF_ENTITY_PRISM] = handleSurfaceEntity;
    GLOBAL_mgf_unknownEntityHandleCallback = handleUnknownEntity;

    mgfAlternativeInit(GLOBAL_mgf_handleCallbacks);
}

static void
freeLists() {
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

/**
Reads in an mgf file. The result is that the global variables
GLOBAL_scene_world and GLOBAL_scene_materials are filled in.
*/
void
readMgf(
    char *filename,
    RadianceMethod *context,
    bool singleSided)
{
    mgfSetNrQuartCircDivs(GLOBAL_fileOptions_numberOfQuarterCircleDivisions);
    mgfSetIgnoreSidedness(singleSided);
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

    MgfReaderContext mgfReaderContext{};
    int status;
    if ( filename[0] == '#' ) {
        status = mgfOpen(&mgfReaderContext, nullptr);
    } else {
        status = mgfOpen(&mgfReaderContext, filename);
    }
    if ( status ) {
        doError(GLOBAL_mgf_errors[status]);
    } else {
        while ( mgfReadNextLine() > 0 && !status ) {
            status = mgfParseCurrentLine(context);
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
    GLOBAL_scene_geometries = globalCurrentGeometryList;

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
        if ( globalCurrentGeometryList->get(i)->className == SURFACE_MESH ) {
            surfaces++;
        }
        if ( globalCurrentGeometryList->get(i)->className == PATCH_SET ) {
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

    freeLists();
}

#include <cstring>

#include "common/error.h"
#include "material/cie.h"
#include "skin/Geometry.h"
#include "skin/Vertex.h"
#include "skin/Patch.h"
#include "skin/vertexlist.h"
#include "scene/scene.h"
#include "scene/splitbsdf.h"
#include "io/mgf/parser.h"
#include "shared/options.h"
#include "readmgf.h"
#include "skin/Compound.h"
#include "vectoroctree.h"
#include "scene/phong.h"
#include "app/fileopts.h"

static VECTOROCTREE *globalPoints; // global point octree
static VECTOROCTREE *globalNormals; // global normal octree

/* pointlist, normal list, vertex list ... of surface currently being created */
static Vector3DListNode *currentPointList;
static Vector3DListNode *currentNormalList;
static VERTEXLIST *currentVertexList;
static VERTEXLIST *autoVertexList;
static PatchSet *currentFaceList;
static GeometryListNode *currentGeomList;
static MATERIAL *currentMaterial;

#ifndef MAXGEOMSTACKDEPTH
    #define MAXGEOMSTACKDEPTH    100    /* objects ('o' contexts) can be nested this deep */
#endif

/* Geometry stack: used for building a hierarchical representation of the scene */
static GeometryListNode *geomStack[MAXGEOMSTACKDEPTH], **geomStackPtr;
static VERTEXLIST *autoVertexListStack[MAXGEOMSTACKDEPTH], **autoVertexListStackPtr;

#ifndef MAXFACEVERTICES
    #define MAXFACEVERTICES           100    /* no face can have more than this vertices */
#endif

static int incomplex = false; // true if reading a sphere, torus or other unsupported
static int insurface = false; // true if busy creating a new surface
static int all_surfaces_sided = false; // when set to true, all surfaces will be considered one-sided

/**
Sets the number of quarter circle divisions for discretizing cylinders, spheres, cones, etc.
*/
static void
MgfSetNrQuartCircDivs(int divs) {
    if ( divs <= 0 ) {
        Error(nullptr, "Number of quarter circle divisions (%d) should be positive", divs);
        return;
    }

    mg_nqcdivs = divs;
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
MgfSetIgnoreSidedness(int yesno) {
    all_surfaces_sided = yesno;
}

/**
If yesno is true, all materials will be converted to be monochrome.
*/
static void
MgfSetMonochrome(int yesno) {
    monochrome = yesno;
}

static void
do_error(const char *errmsg) {
    Error(nullptr, (char *) "%s line %d: %s", mg_file->fname, mg_file->lineno, errmsg);
}

static void
do_warning(const char *errmsg) {
    Warning(nullptr, (char *) "%s line %d: %s", mg_file->fname, mg_file->lineno, errmsg);
}

static void
NewSurface() {
    currentPointList = VectorListCreate();
    currentNormalList = VectorListCreate();
    currentVertexList = VertexListCreate();
    currentFaceList = PatchListCreate();
    insurface = true;
}

static void
surfaceDone() {
    Geometry *newGeometry;

    if ( currentFaceList ) {
        newGeometry = geomCreateSurface(
                surfaceCreate(currentMaterial, currentPointList, currentNormalList,
                              (Vector3DListNode *) nullptr,
                              currentVertexList, currentFaceList, NO_COLORS), &GLOBAL_skin_surfaceGeometryMethods);
        currentGeomList = GeomListAdd(currentGeomList, newGeometry);
    }
    insurface = false;
}

/**
This routine returns true if the current material has changed
*/
static int
MaterialChanged() {
    char *matname;

    matname = c_cmname;
    if ( !matname || *matname == '\0' ) {    /* this might cause strcmp to crash !! */
        matname = (char *) "unnamed";
    }

    /* is it another material than the one used for the previous face ?? If not, the
     * currentMaterial remains the same. */
    if ( strcmp(matname, currentMaterial->name) == 0 && c_cmaterial->clock == 0 ) {
        return false;
    }

    return true;
}

/* Translates mgf color into out color representation. */
static void
MgfGetColor(C_COLOR *cin, double intensity, COLOR *cout) {
    float xyz[3], rgb[3];

    c_ccvt(cin, C_CSXY);
    if ( cin->cy > EPSILON ) {
        xyz[0] = cin->cx / cin->cy * intensity;
        xyz[1] = 1. * intensity;
        xyz[2] = (1. - cin->cx - cin->cy) / cin->cy * intensity;
    } else {
        do_warning("invalid color specification (Y<=0) ... setting to black");
        xyz[0] = xyz[1] = xyz[2] = 0.;
    }

    if ( xyz[0] < 0. || xyz[1] < 0. || xyz[2] < 0. ) {
        do_warning("invalid color specification (negative CIE XYZ componenets) ... clipping to zero");
        if ( xyz[0] < 0. ) {
            xyz[0] = 0.;
        }
        if ( xyz[1] < 1. ) {
            xyz[1] = 0.;
        }
        if ( xyz[2] < 2. ) {
            xyz[2] = 0.;
        }
    }

    xyz_rgb(xyz, rgb);
    if ( clipgamut(rgb)) {
        do_warning("color desaturated during gamut clipping");
    }
    COLORSET(*cout, rgb[0], rgb[1], rgb[2]);
}

#define NUMSMPLS 3
#define SPEC_SAMPLES(col, rgb) \
  rgb[0] = col.spec[0]; rgb[1] = col.spec[1]; rgb[2] = col.spec[2];


float
ColorMax(COLOR col) {
    /* We should check every wavelength in the visible spectrum, but
     * as a first approximation, only the three RGB primary colors
     * are checked. */
    float samples[NUMSMPLS], mx;
    int i;

    SPEC_SAMPLES(col, samples);

    mx = -HUGE;
    for ( i = 0; i < NUMSMPLS; i++ ) {
        if ( samples[i] > mx ) {
            mx = samples[i];
        }
    }

    return mx;
}

#undef SPEC_SAMPLES
#undef NUMSMPLS

/* This routine checks whether the mgf material being used has changed. If it
 * changed, this routine converts to our representation of materials and
 * creates a new MATERIAL, which is added to the global material library.
 * The routine returns true if the material being used has changed. */
static int
GetCurrentMaterial() {
    COLOR Ed, Es, Rd, Td, Rs, Ts, A;
    float Ne, Nr, Nt, a;
    MATERIAL *thematerial;
    char *matname;

    matname = c_cmname;
    if ( !matname || *matname == '\0' ) {    /* this might cause strcmp to crash !! */
        matname = (char *) "unnamed";
    }

    /* is it another material than the one used for the previous face ?? If not, the
     * currentMaterial remains the same. */
    if ( strcmp(matname, currentMaterial->name) == 0 && c_cmaterial->clock == 0 ) {
        return false;
    }

    if ((thematerial = MaterialLookup(GLOBAL_scene_materials, matname))) {
        if ( c_cmaterial->clock == 0 ) {
            currentMaterial = thematerial;
            return true;
        }
    }

    /* new material, or a material that changed. Convert intensities and chromaticities
     * to our color model. */
    MgfGetColor(&c_cmaterial->ed_c, c_cmaterial->ed, &Ed);
    MgfGetColor(&c_cmaterial->rd_c, c_cmaterial->rd, &Rd);
    MgfGetColor(&c_cmaterial->td_c, c_cmaterial->td, &Td);
    MgfGetColor(&c_cmaterial->rs_c, c_cmaterial->rs, &Rs);
    MgfGetColor(&c_cmaterial->ts_c, c_cmaterial->ts, &Ts);

    /* check/correct range of reflectances and transmittances */
    COLORADD(Rd, Rs, A);
    if ((a = ColorMax(A)) > 1. - EPSILON ) {
        do_warning("invalid material specification: total reflectance shall be < 1");
        a = (1. - EPSILON) / a;
        COLORSCALE(a, Rd, Rd);
        COLORSCALE(a, Rs, Rs);
    }

    COLORADD(Td, Ts, A);
    if ((a = ColorMax(A)) > 1. - EPSILON ) {
        do_warning("invalid material specification: total transmittance shall be < 1");
        a = (1. - EPSILON) / a;
        COLORSCALE(a, Td, Td);
        COLORSCALE(a, Ts, Ts);
    }

    /* convert lumen/m^2 to W/m^2 */
    COLORSCALE((1. / WHITE_EFFICACY), Ed, Ed);

    COLORCLEAR(Es);
    Ne = 0.;

    /* specular power = (0.6/roughness)^2 (see mgf docs) */
    if ( c_cmaterial->rs_a != 0.0 ) {
        Nr = 0.6 / c_cmaterial->rs_a;
        Nr *= Nr;
    } else {
        Nr = 0.0;
    }

    if ( c_cmaterial->ts_a != 0.0 ) {
        Nt = 0.6 / c_cmaterial->ts_a;
        Nt *= Nt;
    } else {
        Nt = 0.0;
    }

    if ( monochrome ) {
        COLORSETMONOCHROME(Ed, ColorGray(Ed));
        COLORSETMONOCHROME(Es, ColorGray(Es));
        COLORSETMONOCHROME(Rd, ColorGray(Rd));
        COLORSETMONOCHROME(Rs, ColorGray(Rs));
        COLORSETMONOCHROME(Td, ColorGray(Td));
        COLORSETMONOCHROME(Ts, ColorGray(Ts));
    }

    thematerial = MaterialCreate(matname,
                                 (COLORNULL(Ed) && COLORNULL(Es)) ? (EDF *) nullptr : EdfCreate(
                                         PhongEdfCreate(&Ed, &Es, Ne), &PhongEdfMethods),
                                 BsdfCreate(SplitBSDFCreate(
                                         (COLORNULL(Rd) && COLORNULL(Rs)) ? (BRDF *) nullptr : BrdfCreate(
                                                 PhongBrdfCreate(&Rd, &Rs, Nr), &PhongBrdfMethods),
                                         (COLORNULL(Td) && COLORNULL(Ts)) ? (BTDF *) nullptr : BtdfCreate(
                                                 PhongBtdfCreate(&Td, &Ts, Nt, c_cmaterial->nr, c_cmaterial->ni),
                                                 &PhongBtdfMethods), (TEXTURE *) nullptr), &SplitBsdfMethods),
                                 all_surfaces_sided ? 1 : c_cmaterial->sided);

    GLOBAL_scene_materials = MaterialListAdd(GLOBAL_scene_materials, thematerial);
    currentMaterial = thematerial;

    /* reset the clock value so we will be aware of possible changes in future */
    c_cmaterial->clock = 0;

    return true;
}

static Vector3D *
InstallPoint(float x, float y, float z) {
    Vector3D *coord = VectorCreate(x, y, z);
    currentPointList = VectorListAdd(currentPointList, coord);
    return coord;
}

static Vector3D *
InstallNormal(float x, float y, float z) {
    Vector3D *norm = VectorCreate(x, y, z);
    currentNormalList = VectorListAdd(currentNormalList, norm);
    return norm;
}

static VERTEX *
InstallVertex(Vector3D *coord, Vector3D *norm, char *name) {
    VERTEX *v = VertexCreate(coord, norm, (Vector3D *) nullptr, PatchListCreate());
    currentVertexList = VertexListAdd(currentVertexList, v);
    return v;
}

static VERTEX *
GetVertex(char *name) {
    C_VERTEX *vp;
    VERTEX *thevertex;

    if ((vp = c_getvert(name)) == nullptr ) {
        return (VERTEX *) nullptr;
    }

    thevertex = (VERTEX *) (vp->client_data);
    if ( !thevertex || vp->clock >= 1 || vp->xid != xf_xid(xf_context) || is0vect(vp->n)) {
        /* new vertex, or updated vertex or same vertex, but other transform, or
         * vertex without normal: create a new VERTEX. */
        FVECT vert, norm;
        Vector3D *thenormal;
        Vector3D *thepoint;

        xf_xfmpoint(vert, vp->p);
        thepoint = InstallPoint(vert[0], vert[1], vert[2]);
        if ( is0vect(vp->n)) {
            thenormal = (Vector3D *) nullptr;
        } else {
            xf_xfmvect(norm, vp->n);
            thenormal = InstallNormal(norm[0], norm[1], norm[2]);
        }
        thevertex = InstallVertex(thepoint, thenormal, name);
        vp->client_data = (void *) thevertex;
        vp->xid = xf_xid(xf_context);
    }
    vp->clock = 0;

    return thevertex;
}

/* Create a vertex with given name, but with reversed normal as
 * the given vertex. For back-faces of two-sided surfaces. */
static VERTEX *
GetBackFaceVertex(VERTEX *v, char *name) {
    VERTEX *back = v->back;

    if ( !back ) {
        Vector3D *the_point, *the_normal;

        the_point = v->point;
        the_normal = v->normal;
        if ( the_normal ) {
            the_normal = InstallNormal(-the_normal->x, -the_normal->y, -the_normal->z);
        }

        back = v->back = InstallVertex(the_point, the_normal, name);
        back->back = v;
    }

    return back;
}

static PATCH *
NewFace(VERTEX *v1, VERTEX *v2, VERTEX *v3, VERTEX *v4, Vector3D *normal) {
    PATCH *theface;

    if ( xf_context && xf_context->rev ) {
        theface = PatchCreate(v4 ? 4 : 3, v3, v2, v1, v4);
    } else {
        theface = PatchCreate(v4 ? 4 : 3, v1, v2, v3, v4);
    }

    if ( theface ) {
        currentFaceList = PatchListAdd(currentFaceList, theface);
    }

    return theface;
}

/* computes the normal to the patch plane */
static Vector3D *
FaceNormal(int nrvertices, VERTEX **v, Vector3D *normal) {
    double norm;
    Vector3Dd prev, cur;
    Vector3D n;
    int i;

    VECTORSET(n, 0, 0, 0);
    VECTORSUBTRACT(*(v[nrvertices - 1]->point), *(v[0]->point), cur);
    for ( i = 0; i < nrvertices; i++ ) {
        prev = cur;
        VECTORSUBTRACT(*(v[i]->point), *(v[0]->point), cur);
        n.x += (prev.y - cur.y) * (prev.z + cur.z);
        n.y += (prev.z - cur.z) * (prev.x + cur.x);
        n.z += (prev.x - cur.x) * (prev.y + cur.y);
    }
    norm = VECTORNORM(n);

    if ( norm < EPSILON ) {
        /* Degenerate normal --> degenerate polygon */
        return (Vector3D *) nullptr;
    }
    VECTORSCALEINVERSE(norm, n, n);
    *normal = n;

    return normal;
}

/* Tests whether the polygon is convex or concave. This is accomplished by projecting
 * onto the coordinate plane "most parallel" to the polygon and checking the signs
 * the the cross produtc of succeeding edges: the signs are all equal for a
 * convex polygon. */
static int
FaceIsConvex(int nrvertices, VERTEX **v, Vector3D *normal) {
    Vector2Dd v2d[MAXFACEVERTICES + 1], p, c;
    int i, index, sign;

    index = VectorDominantCoord(normal);
    for ( i = 0; i < nrvertices; i++ ) {
	VECTORPROJECT(v2d[i], *(v[i]->point), index);
    }

    p.u = v2d[3].u - v2d[2].u;
    p.v = v2d[3].v - v2d[2].v;
    c.u = v2d[0].u - v2d[3].u;
    c.v = v2d[0].v - v2d[3].v;
    sign = (p.u * c.v > c.u * p.v) ? 1 : -1;

    for ( i = 1; i < nrvertices; i++ ) {
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

/* returns true if the 2D point p is inside the 2D triangle p1-p2-p3. */
static int
PointInsideTriangle2D(Vector2D *p, Vector2D *p1, Vector2D *p2, Vector2D *p3) {
    double u0, v0, u1, v1, u2, v2, a, b;

    /* from Graphics Gems I, Didier Badouel, An Efficient Ray-Polygon Intersection, p390 */
    u0 = p->u - p1->u;
    v0 = p->v - p1->v;
    u1 = p2->u - p1->u;
    v1 = p2->v - p1->v;
    u2 = p3->u - p1->u;
    v2 = p3->v - p1->v;

    a = 10.;
    b = 10.;    /* values large enough so the result would be false */
    if ( fabs(u1) < EPSILON ) {
        if ( fabs(u2) > EPSILON && fabs(v1) > EPSILON ) {
            b = u0 / u2;
            if ( b < EPSILON || b > 1. - EPSILON ) {
                return false;
            } else {
                a = (v0 - b * v2) / v1;
            }
        }
    } else {
        b = v2 * u1 - u2 * v1;
        if ( fabs(b) > EPSILON ) {
            b = (v0 * u1 - u0 * v1) / b;
            if ( b < EPSILON || b > 1. - EPSILON ) {
                return false;
            } else {
                a = (u0 - b * u2) / u1;
            }
        }
    }

    return (a >= EPSILON && a <= 1. - EPSILON && (a + b) <= 1. - EPSILON);
}

/* returns true if the 2D segments p1-p2 and p3-p4 intersect */
static int
SegmentsIntersect2D(Vector2D *p1, Vector2D *p2, Vector2D *p3, Vector2D *p4) {
    double a, b, c, du, dv, r1, r2, r3, r4;
    int colinear = false;

    /* from Graphics Gems II, Mukesh Prasad, Intersection of Line Segments, p7 */
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

    return false;    /* colinear segments never intersect: do as if they are always
		 * a little bit apart from each other */
}

/* handles concave faces and faces with >4 vertices. This routine started as an
 * ANSI-C version of mgflib/face2tri.C, but I changed it a lot to make it more robust. 
 * Inspiration comes from Burger and Gillis, Interactive Computer Graphics and
 * the (indispensable) Graphics Gems books. */
static int
do_complex_face(int n, VERTEX **v, Vector3D *normal, VERTEX **backv, Vector3D *backnormal) {
    int i, j, max, p0, p1, p2, corners, start, good, index;
    double maxd, d, a;
    Vector3D center;
    char out[MAXFACEVERTICES + 1];
    Vector2D q[MAXFACEVERTICES + 1];
    Vector3D nn;

    VECTORSET(center, 0., 0., 0.);
    for ( i = 0; i < n; i++ ) VECTORADD(center, *(v[i]->point), center);
    VECTORSCALEINVERSE((float) n, center, center);

    maxd = VECTORDIST(center, *(v[0]->point));
    max = 0;
    for ( i = 1; i < n; i++ ) {
        d = VECTORDIST(center, *(v[i]->point));
        if ( d > maxd ) {
            maxd = d;
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
    index = VectorDominantCoord(normal);

    for ( i = 0; i < n; i++ ) {
	VECTORPROJECT(q[i], *(v[i]->point), index);
    }

    corners = n;
    p0 = -1;
    a = 0.;    /* to make gcc -Wall not complain */

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
            VECTORSCALEINVERSE(a, nn, nn);
            d = VECTORDIST(nn, *normal);

            good = true;
            if ( d <= 1.0 ) {
                for ( i = 0; i < n && good; i++ ) {
                    if ( out[i] || v[i] == v[p0] || v[i] == v[p1] || v[i] == v[p2] ) {
                        continue;
                    }

                    if ( PointInsideTriangle2D(&q[i], &q[p0], &q[p1], &q[p2])) {
                        good = false;
                    }

                    j = (i + 1) % n;
                    if ( out[j] || v[j] == v[p0] ) {
                        continue;
                    }

                    if ( SegmentsIntersect2D(&q[p2], &q[p0], &q[i], &q[j])) {
                        good = false;
                    }
                }
            }
        } while ( d > 1.0 || !good );

        if ( p0 == start ) {
            do_error("misbuilt polygonal face");
            return MG_OK;    /* don't stop parsing the input however. */
        }

        if ( fabs(a) > EPSILON ) {    /* avoid degenerate faces */
            PATCH *face, *twin;
            face = NewFace(v[p0], v[p1], v[p2], (VERTEX *) nullptr, normal);
            if ( !currentMaterial->sided ) {
                twin = NewFace(backv[p2], backv[p1], backv[p0], (VERTEX *) nullptr, backnormal);
                face->twin = twin;
                twin->twin = face;
            }
        }

        out[p1] = true;
        corners--;
    }

    return MG_OK;
}

static int
do_face(int argc, char **argv) {
    VERTEX *v[MAXFACEVERTICES + 1], *backv[MAXFACEVERTICES + 1];
    Vector3D normal, backnormal;
    PATCH *face, *twin;
    int i, errcode;

    if ( argc < 4 ) {
        do_error("too few vertices in face");
        return MG_OK;    /* don't stop parsing the input */
    }

    if ( argc - 1 > MAXFACEVERTICES ) {
        do_warning(
                "too many vertices in face. Recompile the program with larger MAXFACEVERTICES constant in readmgf.c");
        return MG_OK;    /* no reason to stop parsing the input */
    }

    if ( !incomplex ) {
        if ( MaterialChanged()) {
            if ( insurface ) {
                surfaceDone();
            }
            NewSurface();
            GetCurrentMaterial();
        }
    }

    for ( i = 0; i < argc - 1; i++ ) {
        if ((v[i] = GetVertex(argv[i + 1])) == (VERTEX *) nullptr ) {
            return MG_EUNDEF;
        }    /* this is however a reason to stop parsing the input */
        backv[i] = (VERTEX *) nullptr;
        if ( !currentMaterial->sided )
            backv[i] = GetBackFaceVertex(v[i], argv[i + 1]);
    }

    if ( !FaceNormal(argc - 1, v, &normal)) {
        do_warning("degenerate face");
        return MG_OK;    /* just ignore the generate face */
    }
    if ( !currentMaterial->sided ) VECTORSCALE(-1., normal, backnormal);

    errcode = MG_OK;
    if ( argc == 4 ) {        /* triangles */
        face = NewFace(v[0], v[1], v[2], (VERTEX *) nullptr, &normal);
        if ( !currentMaterial->sided ) {
            twin = NewFace(backv[2], backv[1], backv[0], (VERTEX *) nullptr, &backnormal);
            face->twin = twin;
            twin->twin = face;
        }
    } else if ( argc == 5 ) {    /* quadrilaterals */
        if ( incomplex || FaceIsConvex(argc - 1, v, &normal)) {
            face = NewFace(v[0], v[1], v[2], v[3], &normal);
            if ( !currentMaterial->sided ) {
                twin = NewFace(backv[3], backv[2], backv[1], backv[0], &backnormal);
                face->twin = twin;
                twin->twin = face;
            }
        } else {
            errcode = do_complex_face(argc - 1, v, &normal, backv, &backnormal);
        }
    } else {            /* more than 4 vertices */
        errcode = do_complex_face(argc - 1, v, &normal, backv, &backnormal);
    }

    return errcode;
}

/* returns squared distance between the two FVECTs */
static double
FVECT_DistanceSquared(FVECT *v1, FVECT *v2) {
    FVECT d;

    d[0] = (*v2)[0] - (*v1)[0];
    d[1] = (*v2)[1] - (*v1)[1];
    d[2] = (*v2)[2] - (*v1)[2];
    return (d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
}

/* Eliminates the holes by creating seems to the nearest vertex
 * on another contour. Creates an argument list for the face
 * without hole entity handling routine do_face() and calls it. */
static int
do_face_with_holes(int argc, char **argv) {
    FVECT v[MAXFACEVERTICES + 1];      /* v[i] = location of vertex argv[i] */
    char *nargv[MAXFACEVERTICES + 1], /* arguments to be passed to the face
				   * without hole entity handler */
    copied[MAXFACEVERTICES + 1]; /* copied[i] is 1 or 0 indicating whether
				   * or not the vertex argv[i] has been 
				   * copied to newcontour */
    int newcontour[MAXFACEVERTICES];/* newcontour[i] will contain the i-th
				   * vertex of the face with eliminated 
				   * holes */
    int i, ncopied;          /* ncopied is the number of vertices in
				   * newcontour */

    if ( argc - 1 > MAXFACEVERTICES ) {
        do_warning(
                "too many vertices in face. Recompile the program with larger MAXFACEVERTICES constant in readmgf.c");
        return MG_OK;    /* no reason to stop parsing the input */
    }

    /* Get the location of the vertices: the location of the vertex
     * argv[i] is kept in v[i] (i=1 ... argc-1, and argv[i] not a contour
     * separator) */
    for ( i = 1; i < argc; i++ ) {
        C_VERTEX *vp;

        if ( *argv[i] == '-' ) {    /* skip contour separators */
            continue;
        }

        vp = c_getvert(argv[i]);
        if ( !vp ) {            /* undefined vertex. */
            return MG_EUNDEF;
        }
        xf_xfmpoint(v[i], vp->p);    /* transform with the current transform */

        copied[i] = false;        /* vertex not yet copied to nargv */
    }

    /* copy the outer contour */
    ncopied = 0;
    for ( i = 1; i < argc && *argv[i] != '-'; i++ ) {
        newcontour[ncopied++] = i;
        copied[i] = true;
    }

    /* find next not yet copied vertex in argv (i++ should suffice, but
     * this way we can also skip multiple "-"s ...) */
    for ( ; i < argc; i++ ) {
        if ( *argv[i] == '-' ) {
            continue;
        }        /* skip contour separators */
        if ( !copied[i] )
            break;        /* not yet copied vertex encountered */
    }

    while ( i < argc ) {
        /* i is the first vertex of a hole that is not yet eliminated. */
        int nearestcopied, nearestother, first, last, num, j, k;
        double mindist;

        /* Find the not yet copied vertex that is nearest to the already
         * copied ones. */
        nearestcopied = nearestother = 0;
        mindist = HUGE;
        for ( j = i; j < argc; j++ ) {
            if ( *argv[j] == '-' || copied[j] ) {
                continue;
            }    /* contour separator or already copied vertex */
            for ( k = 0; k < ncopied; k++ ) {
                double d = FVECT_DistanceSquared(&v[j], &v[newcontour[k]]);
                if ( d < mindist ) {
                    mindist = d;
                    nearestcopied = k;
                    nearestother = j;
                }
            }
        }

        /* find first vertex of this nearest contour */
        for ( first = nearestother; *argv[first] != '-'; first-- ) {
        }
        first++;

        /* find last vertex in this nearest contour */
        for ( last = nearestother; last < argc && *argv[last] != '-'; last++ ) {
        }
        last--;

        /* number of vertices in nearest contour */
        num = last - first + 1;

        /* create num+2 extra vertices in newcontour. */
        if ( ncopied + num + 2 > MAXFACEVERTICES ) {
            do_warning(
                    "too many vertices in face. Recompile the program with larger MAXFACEVERTICES constant in readmgf.c");
            return MG_OK;    /* no reason to stop parsing the input */
        }

        /* shift the elements in newcontour starting at position nearestcopied
         * num+2 places further. Vertex newcontour[nearestcopied] will be connected
         * to vertex nearestother ... last, first ... nearestother and
         * than back to newcontour[nearestcopied]. */
        for ( k = ncopied - 1; k >= nearestcopied; k-- ) {
            newcontour[k + num + 2] = newcontour[k];
        }
        ncopied += num + 2;

        /* insert the vertices of the nearest contour (closing the loop) */
        k = nearestcopied + 1;
        for ( j = nearestother; j <= last; j++ ) {
            newcontour[k++] = j;
            copied[j] = true;
        }
        for ( j = first; j <= nearestother; j++ ) {
            newcontour[k++] = j;
            copied[j] = true;
        }

        /* find next not yet copied vertex in argv */
        for ( ; i < argc; i++ ) {
            if ( *argv[i] == '-' ) {
                continue;
            }    /* skip contour separators */
            if ( !copied[i] )
                break;        /* not yet copied vertex encountered */
        }
    }

    /* build an argument list for the new polygon without holes */
    nargv[0] = (char *) "f";
    for ( i = 0; i < ncopied; i++ ) {
        nargv[i + 1] = argv[newcontour[i]];
    }

    /* and handle the face without holes */
    return do_face(ncopied + 1, nargv);
}

/* the mgf parser already contains some good routines for discretizing spheres, ...
 * into polygons. In the official release of the parser library, these routines
 * are internal (declared static in parse.c and no reference to them in parser.h).
 * The parser was changed so we can call them in order not to have to duplicate
 * the code. */
static int
do_discretize(int argc, char **argv) {
    int en = mg_entity(argv[0]);

    switch ( en ) {
        case MG_E_SPH:
            return e_sph(argc, argv);
        case MG_E_TORUS:
            return e_torus(argc, argv);
        case MG_E_CYL:
            return e_cyl(argc, argv);
        case MG_E_RING:
            return e_ring(argc, argv);
        case MG_E_CONE:
            return e_cone(argc, argv);
        case MG_E_PRISM:
            return e_prism(argc, argv);
        default:
            Fatal(4, "mgf.c: do_discretize", "Unsupported geometry entity number %d", en);
    }

    return MG_EILL;    /* definitely illegal when this point is reached */
}

static int
do_surface(int argc, char **argv) {
    int errcode;

    if ( incomplex ) { /* e_sph calls e_cone ... */
        return do_discretize(argc, argv);
    } else {
        incomplex = true;
        if ( insurface ) {
            surfaceDone();
        }
        NewSurface();
        GetCurrentMaterial();

        errcode = do_discretize(argc, argv);

        surfaceDone();
        incomplex = false;

        return errcode;
    }
}

static void
PushCurrentGeomList() {
    if ( geomStackPtr - geomStack >= MAXGEOMSTACKDEPTH ) {
        do_error(
                "Objects are nested too deep for this program. Recompile with larger MAXGEOMSTACKDEPTH constant in readmgf.c");
        return;
    } else {
        *geomStackPtr = currentGeomList;
        geomStackPtr++;
        currentGeomList = GeomListCreate();

        *autoVertexListStackPtr = autoVertexList;
        autoVertexListStackPtr++;
        autoVertexList = VertexListCreate();
    }
}

static void
popCurrentGeomList() {
    if ( geomStackPtr <= geomStack ) {
        do_error("Object stack underflow ... unbalanced 'o' contexts?");
        currentGeomList = GeomListCreate();
        return;
    } else {
        geomStackPtr--;
        currentGeomList = *geomStackPtr;

        VertexListDestroy(autoVertexList);
        autoVertexListStackPtr--;
        autoVertexList = *autoVertexListStackPtr;
    }
}

static int
do_object(int argc, char **argv) {
    int i;

    if ( argc > 1 ) {    /* beginning of a new object */
        for ( i = 0; i < geomStackPtr - geomStack; i++ ) {
            fprintf(stderr, "\t");
        }
        fprintf(stderr, "%s ...\n", argv[1]);

        if ( insurface ) {
            surfaceDone();
        }

        PushCurrentGeomList();

        NewSurface();
    } else {
        // End of object definition
        Geometry *theGeometry = nullptr;

        if ( insurface ) {
            surfaceDone();
        }

        if ( GeomListCount(currentGeomList) > 0 ) {
            theGeometry = geomCreateCompound(compoundCreate(currentGeomList), CompoundMethods());
        }

        popCurrentGeomList();

        if ( theGeometry ) {
            currentGeomList = GeomListAdd(currentGeomList, theGeometry);
        }

        NewSurface();
    }

    return obj_handler(argc, argv);
}

static int unknown_count;    /* unknown entity count */

static int
do_unknown(int argc, char **argv) {
    do_warning("unknown entity");
    unknown_count++;

    return MG_OK;
}

static void
InitMgf() {
    mg_ehand[MG_E_FACE] = do_face;
    mg_ehand[MG_E_FACEH] = do_face_with_holes;

    mg_ehand[MG_E_VERTEX] = c_hvertex;
    mg_ehand[MG_E_POINT] = c_hvertex;
    mg_ehand[MG_E_NORMAL] = c_hvertex;

    mg_ehand[MG_E_COLOR] = c_hcolor;
    mg_ehand[MG_E_CXY] = c_hcolor;
    mg_ehand[MG_E_CMIX] = c_hcolor;
    /* we don't use spectra.... let the mgf parser library convert to tristimulus
     * values itself
        mg_ehand[MG_E_CSPEC] = c_hcolor;
        mg_ehand[MG_E_CCT] = c_hcolor;
    */

    mg_ehand[MG_E_MATERIAL] = c_hmaterial;
    mg_ehand[MG_E_ED] = c_hmaterial;
    mg_ehand[MG_E_IR] = c_hmaterial;
    mg_ehand[MG_E_RD] = c_hmaterial;
    mg_ehand[MG_E_RS] = c_hmaterial;
    mg_ehand[MG_E_SIDES] = c_hmaterial;
    mg_ehand[MG_E_TD] = c_hmaterial;
    mg_ehand[MG_E_TS] = c_hmaterial;

    mg_ehand[MG_E_OBJECT] = do_object;

    mg_ehand[MG_E_XF] = xf_handler;

    mg_ehand[MG_E_SPH] = do_surface;
    mg_ehand[MG_E_TORUS] = do_surface;
    mg_ehand[MG_E_RING] = do_surface;
    mg_ehand[MG_E_CYL] = do_surface;
    mg_ehand[MG_E_CONE] = do_surface;
    mg_ehand[MG_E_PRISM] = do_surface;

    unknown_count = 0;
    mg_uhand = do_unknown;

    mg_init();
}

/**
Reads in an mgf file. The result is that the global variables
GLOBAL_scene_world and GLOBAL_scene_materials are filled in.
*/
void
ReadMgf(char *filename) {
    MG_FCTXT fctxt;
    int err;

    MgfSetNrQuartCircDivs(nqcdivs);
    MgfSetIgnoreSidedness(force_onesided_surfaces);
    MgfSetMonochrome(monochrome);

    InitMgf();

    globalPoints = ((VECTOROCTREE *)OctreeCreate());
    globalNormals = ((VECTOROCTREE *)OctreeCreate());
    currentGeomList = GeomListCreate();

    GLOBAL_scene_materials = MaterialListCreate();
    currentMaterial = &defaultMaterial;

    geomStackPtr = geomStack;
    autoVertexListStackPtr = autoVertexListStack;
    autoVertexList = VertexListCreate();

    incomplex = false;
    insurface = false;

    NewSurface();

    if ( filename[0] == '#' ) {
        err = mg_open(&fctxt, nullptr);
    } else {
        err = mg_open(&fctxt, filename);
    }
    if ( err ) {
        do_error(mg_err[err]);
    } else {
        while ( mg_read() > 0 && !err ) {
            err = mg_parse();
            if ( err ) {
                do_error(mg_err[err]);
            }
        }
        mg_close();
    }
    mg_clear();

    if ( insurface ) {
        surfaceDone();
    }
    GLOBAL_scene_world = currentGeomList;

    VertexListDestroy(autoVertexList);
    if (globalPoints != nullptr) {
        free(globalPoints);
    }
    if (globalNormals != nullptr) {
        free(globalNormals);
    }
}


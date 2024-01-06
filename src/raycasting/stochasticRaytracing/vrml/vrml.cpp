/* vrml.c: saves "illuminated" model in the VRML'97 file */

#include <cstring>

#include "common/error.h"
#include "skin/Geometry.h"
#include "scene/scene.h"
#include "shared/writevrml.h"
#include "shared/render.h"
#include "shared/defaults.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "vrml.h"

#define FACES_PER_SET 1000  /* keeps all arrays smaller than 30k els */
static int vid, nwrit, matidx;
static int nrcoords, nrcolors, nrcoordindices, pass;
static FILE *vrmlfp;

/* iterator adapted in order to handle only a subset of the elements
 * at a time, taking into account 'pass' and FACES_PER_SET */
static void (*elemfunc)(ELEMENT *);

static int leaf_element_count;

static void count_and_call(ELEMENT *elem) {
    if ( leaf_element_count >= pass * FACES_PER_SET &&
         leaf_element_count < (pass + 1) * FACES_PER_SET ) {
        elemfunc(elem);
    }
    leaf_element_count++;
}

static void GeomIterateLeafElements(Geometry *geom, void (*func)(ELEMENT *)) {
    PatchSet *patches = geomPatchList(geom);
    elemfunc = func;
    leaf_element_count = 0;
    ForAllPatches(P, patches)
                {
                    McrForAllLeafElements(TOPLEVEL_ELEMENT(P), count_and_call);
                }
    EndForAll;
}

static void ResetVertexId(VERTEX *v) {
    v->tmp = -1;
}

static void TriangleResetVertexIds(VERTEX *v1, VERTEX *v2, VERTEX *v3) {
    ResetVertexId(v1);
    ResetVertexId(v2);
    ResetVertexId(v3);
}

static void QuadResetVertexIds(VERTEX *v1, VERTEX *v2, VERTEX *v3, VERTEX *v4) {
    ResetVertexId(v1);
    ResetVertexId(v2);
    ResetVertexId(v3);
    ResetVertexId(v4);
}

/* with T-vertex elimination */
static void ResetVertexIds(ELEMENT *elem) {
    ElementTVertexElimination(elem, TriangleResetVertexIds, QuadResetVertexIds);
}

static void WriteVertexCoord(VERTEX *v) {
    if ( v->tmp == -1 ) {
        /* not yet written */
        if ( nwrit > 0 ) {
            fprintf(vrmlfp, ", ");
        }
        nwrit++;
        if ( nwrit % 4 == 0 ) {
            fprintf(vrmlfp, "\n\t  ");
        }
        fprintf(vrmlfp, "%g %g %g", v->point->x, v->point->y, v->point->z);
        v->tmp = vid;
        vid++;
    }
}

static void TriangleWriteVertexCoords(VERTEX *v1, VERTEX *v2, VERTEX *v3) {
    WriteVertexCoord(v1);
    WriteVertexCoord(v2);
    WriteVertexCoord(v3);
}

static void QuadWriteVertexCoords(VERTEX *v1, VERTEX *v2, VERTEX *v3, VERTEX *v4) {
    WriteVertexCoord(v1);
    WriteVertexCoord(v2);
    WriteVertexCoord(v3);
    WriteVertexCoord(v4);
}

static void WriteVertexCoords(ELEMENT *elem) {
    ElementTVertexElimination(elem, TriangleWriteVertexCoords, QuadWriteVertexCoords);
}

static void WritePrimitiveCoords(Geometry *geom) {
    GeomIterateLeafElements(geom, ResetVertexIds);

    vid = nwrit = 0;
    fprintf(vrmlfp, "\tcoord Coordinate {\n\t  point [ ");
    GeomIterateLeafElements(geom, WriteVertexCoords);
    fprintf(vrmlfp, " ] ");
    fprintf(vrmlfp, "\n\t}\n");

    nrcoords = nwrit;
}

static void WriteVertexColor(VERTEX *v) {
    if ( v->tmp == -1 ) {
        /* not yet written */
        if ( nwrit > 0 ) {
            fprintf(vrmlfp, ", ");
        }
        nwrit++;
        if ( nwrit % 4 == 0 ) {
            fprintf(vrmlfp, "\n\t  ");
        }
        fprintf(vrmlfp, "%.3g %.3g %.3g", v->color.r, v->color.g, v->color.b);
        v->tmp = vid;
        vid++;
    }
}

static void TriangleWriteVertexColors(VERTEX *v1, VERTEX *v2, VERTEX *v3) {
    WriteVertexColor(v1);
    WriteVertexColor(v2);
    WriteVertexColor(v3);
}

static void QuadWriteVertexColors(VERTEX *v1, VERTEX *v2, VERTEX *v3, VERTEX *v4) {
    WriteVertexColor(v1);
    WriteVertexColor(v2);
    WriteVertexColor(v3);
    WriteVertexColor(v4);
}

static void ElementWriteVertexColors(ELEMENT *elem) {
    ElementTVertexElimination(elem, TriangleWriteVertexColors, QuadWriteVertexColors);
}

static void WritePrimitiveColors(Geometry *geom) {
    GeomIterateLeafElements(geom, ResetVertexIds);

    vid = nwrit = 0;
    fprintf(vrmlfp, "\tcolor Color {\n\t  color [ ");
    GeomIterateLeafElements(geom, ElementWriteVertexColors);
    fprintf(vrmlfp, " ] ");
    fprintf(vrmlfp, "\n\t}\n");

    nrcolors = nwrit;
}

static void WriteCoordIndex(int index) {
    nwrit++;
    if ( nwrit % 20 == 0 ) {
        fprintf(vrmlfp, "\n\t  ");
    }
    fprintf(vrmlfp, "%d ", index);
}

static void TriangleWriteVertexCoordIndices(VERTEX *v1, VERTEX *v2, VERTEX *v3) {
    WriteCoordIndex(v1->tmp);
    WriteCoordIndex(v2->tmp);
    WriteCoordIndex(v3->tmp);
    WriteCoordIndex(-1);
}

static void QuadWriteVertexCoordIndices(VERTEX *v1, VERTEX *v2, VERTEX *v3, VERTEX *v4) {
    WriteCoordIndex(v1->tmp);
    WriteCoordIndex(v2->tmp);
    WriteCoordIndex(v3->tmp);
    WriteCoordIndex(v4->tmp);
    WriteCoordIndex(-1);
}

static void ElementWriteCoordIndices(ELEMENT *elem) {
    ElementTVertexElimination(elem, TriangleWriteVertexCoordIndices, QuadWriteVertexCoordIndices);
}

static void WritePrimitiveCoordIndices(Geometry *geom) {
    nwrit = 0;
    fprintf(vrmlfp, "\tcoordIndex [ ");
    GeomIterateLeafElements(geom, ElementWriteCoordIndices);
    fprintf(vrmlfp, " ]\n");

    nrcoordindices = nwrit;
}

extern void
McrWriteVRMLHeader(FILE *fp);

void
McrWriteVRMLHeader(FILE *fp) {
    Matrix4x4 model_xf;
    Vector3D model_rotaxis;
    float model_rotangle;

    fprintf(fp, "#VRML V2.0 utf8\n\n");

    fprintf(fp,
            "WorldInfo {\n  title \"%s\"\n  info [ \"Created with RenderPark (%s) using Monte Carlo radiosty\" ]\n}\n\n",
            "Some nice model",
            RPKHOME);

    fprintf(fp, "NavigationInfo {\n type \"WALK\"\n headlight FALSE\n}\n\n");

    model_xf = VRMLModelTransform(&model_rotaxis, &model_rotangle);
    WriteVRMLViewPoints(fp, model_xf);

    fprintf(fp, "Transform {\n  rotation %g %g %g %g\n  children [\n",
            model_rotaxis.x, model_rotaxis.y, model_rotaxis.z, model_rotangle);
}

void McrWriteVRMLTrailer(FILE *fp) {
    fprintf(fp, "  ]\n}\n\n");
}

static unsigned char IdFirstChar[256], IdRestChar[256];

static void initidtranstabs() {
    int i;
    const char *IdSpecialChars = "!$%&()*/:;<=>?@^_`|~";
    for ( i = 0; i < 256; i++ ) {
        IdFirstChar[i] = '_';
    }
    for ( i = 'a'; i <= 'z'; i++ ) {
        IdFirstChar[i] = i;
    }
    for ( i = 'A'; i <= 'Z'; i++ ) {
        IdFirstChar[i] = i;
    }
    for ( i = 0; i < strlen(IdSpecialChars); i++ ) {
        IdFirstChar[(int) IdSpecialChars[i]] = IdSpecialChars[i];
    }
    for ( i = 0; i < 256; i++ ) {
        IdRestChar[i] = IdFirstChar[i];
    }
    IdRestChar['+'] = '+';
    IdRestChar['-'] = '-';
    for ( i = '0'; i <= '9'; i++ ) {
        IdRestChar[i] = i;
    }
}

static const char *make_valid_vrml_id(const char *id) {
    int idlen, i, n;
    static char buf[101];
    if ( !id || id[0] == '\0' ) {
        return "";
    }
    idlen = strlen(id);
    if ( idlen > 100 ) {
        Warning("make_valid_vrml_id", "id '%s' is being truncated to %d characters",
                id, 100);
    }
    n = idlen > 100 ? 100 : idlen;  /* minimum of both */
    buf[0] = IdFirstChar[(int) id[0]];
    for ( i = 1; i < n; i++ ) {
        buf[i] = IdRestChar[(int) id[i]];
    }
    buf[n] = '\0';
    return buf;
}

static void WriteMaterial(Geometry *geom) {
    MeshSurface *surf = GeomGetSurface(geom);
    MATERIAL *mat = surf->material;
    PATCH *first_patch = (surf->faces) ? surf->faces->patch : (PATCH *) nullptr;
    HITREC hit;
    COLOR Rd, Rs;
    RGB rd, rs;
    float specularity;

    if ( !first_patch || !mat || !mat->bsdf ) {
        return;
    }

    if ( mat->radiance_data != nullptr) {
        /* has been written before */
        fprintf(vrmlfp, "      appearance Appearance {\n");
        fprintf(vrmlfp, "\tmaterial USE %s\n", make_valid_vrml_id((char *) mat->radiance_data));
        fprintf(vrmlfp, "      }\n");
        return;
    }

    InitHit(&hit, first_patch, (Geometry *) nullptr, &first_patch->midpoint, &first_patch->normal, mat, 0.);
    Rd = BsdfScatteredPower(mat->bsdf, &hit, &first_patch->normal, BRDF_DIFFUSE_COMPONENT);
    convertColorToRGB(Rd, &rd);
    Rs = BsdfScatteredPower(mat->bsdf, &hit, &first_patch->normal, BRDF_GLOSSY_COMPONENT | BRDF_SPECULAR_COMPONENT);
    convertColorToRGB(Rs, &rs);
    specularity = 128.;

    fprintf(vrmlfp, "      appearance Appearance {\n");
    fprintf(vrmlfp, "\tmaterial DEF %s Material {\n", make_valid_vrml_id(mat->name));
    fprintf(vrmlfp, "\t  ambientIntensity 0.\n");
    if ( mat->edf ) {
        fprintf(vrmlfp, "\t  emissiveColor %.3g %.3g %.3g\n", rd.r, rd.g, rd.b);
    } else {
        fprintf(vrmlfp, "\t  emissiveColor %.3g %.3g %.3g\n", 0., 0., 0.);
    }
    fprintf(vrmlfp, "\t  diffuseColor %.3g %.3g %.3g \n", rd.r, rd.g, rd.b);
    fprintf(vrmlfp, "\t  specularColor %.3g %.3g %.3g \n", rs.r, rs.g, rs.b);
    fprintf(vrmlfp, "\t  shininess %g\n", specularity / 128. > 1. ? 1. : specularity / 128.);
    fprintf(vrmlfp, "\t  transparency 0.\n");
    fprintf(vrmlfp, "\t}\n");
    fprintf(vrmlfp, "      }\n");

    mat->radiance_data = (void *)mat->name;
}

static void BeginWritePrimitive(Geometry *geom) {
    static int wgiv = false;
    fprintf(vrmlfp, "    Shape {\n");
    if ( GeomIsSurface(geom)) {
        WriteMaterial(geom);
    }
    fprintf(vrmlfp, "      geometry IndexedFaceSet {\n");
    if ( GeomIsSurface(geom)) {
        fprintf(vrmlfp, "\tsolid %s\n", GeomGetSurface(geom)->material->sided ? "TRUE" : "FALSE");
    }
    if ( !renderopts.smooth_shading && !wgiv ) {
        Warning(nullptr, "I assume you want a smooth shaded model ...");
        wgiv = true;
    }
    fprintf(vrmlfp, "\tcolorPerVertex %s\n", "TRUE");
}

static const char *PrimitiveMatName(Geometry *geom) {
    MeshSurface *surf = GeomGetSurface(geom);
    if ( !surf ) {
        return "unknown (not a surface)";
    } else {
        return make_valid_vrml_id(surf->material->name);
    }
}

static void EndWritePrimitive(Geometry *geom) {
    fprintf(vrmlfp, "      }\n"); /* end IndexedFaceSet */
    fprintf(vrmlfp, "    },\n");  /* end Shape */

    fprintf(stderr, "Shape material %s, pass %d, %d coords, %d colors, %d coordindices\n",
            PrimitiveMatName(geom), pass, nrcoords, nrcolors, nrcoordindices);
}

static void WritePrimitivePass(Geometry *geom) {
    BeginWritePrimitive(geom);
    WritePrimitiveCoords(geom);
    WritePrimitiveColors(geom);
    WritePrimitiveCoordIndices(geom);
    EndWritePrimitive(geom);
}

static void WritePrimitive(Geometry *geom) {
    pass = 0;
    WritePrimitivePass(geom);
    if ( leaf_element_count > FACES_PER_SET ) {
        /* large set, additional passes needed */
        for ( pass = 1; pass <= leaf_element_count / FACES_PER_SET; pass++ ) {
            WritePrimitivePass(geom);
        } /* write next batch of leaf elements */
    }
}

void IteratePrimitiveGeoms(GeometryListNode *list, void (*func)(Geometry *)) {
    ForAllGeoms(geom, list)
                {
                    if ( geomIsAggregate(geom)) {
                        IteratePrimitiveGeoms(geomPrimList(geom), func);
                    } else {
                        func(geom);
                    }
                }
    EndForAll;
}

static void ResetMaterialData() {
    ForAllMaterials(mat, GLOBAL_scene_materials)
                {
                    mat->radiance_data = (void *) nullptr;
                }
    EndForAll;
}

void McrWriteVRML(FILE *fp) {
    initidtranstabs();

    matidx = 0;
    ResetMaterialData();

    McrWriteVRMLHeader(fp);

    vrmlfp = fp;
    IteratePrimitiveGeoms(GLOBAL_scene_world, WritePrimitive);
    McrWriteVRMLTrailer(fp);

    ResetMaterialData();
}

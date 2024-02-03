/* vrml.c: saves "illuminated" model in the VRML'97 file */

#include <cstring>

#include "common/error.h"
#include "skin/Geometry.h"
#include "scene/scene.h"
#include "io/writevrml.h"
#include "render/render.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "raycasting/stochasticRaytracing/vrml/vrml.h"

#define FACES_PER_SET 1000  /* keeps all arrays smaller than 30k els */
static int vid, nwrit, matidx;
static int nrcoords, nrcolors, nrcoordindices, pass;
static FILE *vrmlfp;

/* iterator adapted in order to handle only a subset of the elements
 * at a time, taking into account 'pass' and FACES_PER_SET */
static void (*elemfunc)(StochasticRadiosityElement *);

static int leaf_element_count;

static void
countAndCall(StochasticRadiosityElement *elem) {
    if ( leaf_element_count >= pass * FACES_PER_SET &&
         leaf_element_count < (pass + 1) * FACES_PER_SET ) {
        elemfunc(elem);
    }
    leaf_element_count++;
}

static void
geometryIterateLeafElements(Geometry *geom, void (*func)(StochasticRadiosityElement *)) {
    java::ArrayList<Patch *> *patchList = geomPatchArrayList(geom);
    elemfunc = func;
    leaf_element_count = 0;
    for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
        monteCarloRadiosityForAllLeafElements(topLevelGalerkinElement(patchList->get(i)), countAndCall);
    }
}

static void
resetVertexId(Vertex *v) {
    v->tmp = -1;
}

static void
triangleResetVertexIds(Vertex *v1, Vertex *v2, Vertex *v3) {
    resetVertexId(v1);
    resetVertexId(v2);
    resetVertexId(v3);
}

static void
quadResetVertexIds(Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4) {
    resetVertexId(v1);
    resetVertexId(v2);
    resetVertexId(v3);
    resetVertexId(v4);
}

/**
With T-vertex elimination
*/
static void
resetVertexIds(StochasticRadiosityElement *elem) {
    elementTVertexElimination(elem, triangleResetVertexIds, quadResetVertexIds);
}

static void
writeVertexCoord(Vertex *v) {
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

static void
triangleWriteVertexCoords(Vertex *v1, Vertex *v2, Vertex *v3) {
    writeVertexCoord(v1);
    writeVertexCoord(v2);
    writeVertexCoord(v3);
}

static void
quadWriteVertexCoords(Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4) {
    writeVertexCoord(v1);
    writeVertexCoord(v2);
    writeVertexCoord(v3);
    writeVertexCoord(v4);
}

static void
writeVertexCoords(StochasticRadiosityElement *elem) {
    elementTVertexElimination(elem, triangleWriteVertexCoords, quadWriteVertexCoords);
}

static void
writePrimitiveCoords(Geometry *geom) {
    geometryIterateLeafElements(geom, resetVertexIds);

    vid = nwrit = 0;
    fprintf(vrmlfp, "\tcoord Coordinate {\n\t  point [ ");
    geometryIterateLeafElements(geom, writeVertexCoords);
    fprintf(vrmlfp, " ] ");
    fprintf(vrmlfp, "\n\t}\n");

    nrcoords = nwrit;
}

static void
writeVertexColor(Vertex *v) {
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

static void
triangleWriteVertexColors(Vertex *v1, Vertex *v2, Vertex *v3) {
    writeVertexColor(v1);
    writeVertexColor(v2);
    writeVertexColor(v3);
}

static void
quadWriteVertexColors(Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4) {
    writeVertexColor(v1);
    writeVertexColor(v2);
    writeVertexColor(v3);
    writeVertexColor(v4);
}

static void
elementWriteVertexColors(StochasticRadiosityElement *elem) {
    elementTVertexElimination(elem, triangleWriteVertexColors, quadWriteVertexColors);
}

static void
writePrimitiveColors(Geometry *geom) {
    geometryIterateLeafElements(geom, resetVertexIds);

    vid = nwrit = 0;
    fprintf(vrmlfp, "\tcolor Color {\n\t  color [ ");
    geometryIterateLeafElements(geom, elementWriteVertexColors);
    fprintf(vrmlfp, " ] ");
    fprintf(vrmlfp, "\n\t}\n");

    nrcolors = nwrit;
}

static void
writeCoordIndex(int index) {
    nwrit++;
    if ( nwrit % 20 == 0 ) {
        fprintf(vrmlfp, "\n\t  ");
    }
    fprintf(vrmlfp, "%d ", index);
}

static void
triangleWriteVertexCoordIndices(Vertex *v1, Vertex *v2, Vertex *v3) {
    writeCoordIndex(v1->tmp);
    writeCoordIndex(v2->tmp);
    writeCoordIndex(v3->tmp);
    writeCoordIndex(-1);
}

static void
quadWriteVertexCoordIndices(Vertex *v1, Vertex *v2, Vertex *v3, Vertex *v4) {
    writeCoordIndex(v1->tmp);
    writeCoordIndex(v2->tmp);
    writeCoordIndex(v3->tmp);
    writeCoordIndex(v4->tmp);
    writeCoordIndex(-1);
}

static void
elementWriteCoordIndices(StochasticRadiosityElement *elem) {
    elementTVertexElimination(elem, triangleWriteVertexCoordIndices, quadWriteVertexCoordIndices);
}

static void
writePrimitiveCoordIndices(Geometry *geom) {
    nwrit = 0;
    fprintf(vrmlfp, "\tcoordIndex [ ");
    geometryIterateLeafElements(geom, elementWriteCoordIndices);
    fprintf(vrmlfp, " ]\n");

    nrcoordindices = nwrit;
}

void
mcrWriteVrmlHeader(FILE *fp) {
    Matrix4x4 model_xf;
    Vector3D model_rotaxis;
    float model_rotangle;

    fprintf(fp, "#VRML V2.0 utf8\n\n");

    fprintf(fp,
            "WorldInfo {\n  rpk\n  info [ \"Created with RenderPark (%s) using Monte Carlo radiosty\" ]\n}\n\n",
            "Some nice model");

    fprintf(fp, "NavigationInfo {\n type \"WALK\"\n headlight FALSE\n}\n\n");

    model_xf = transformModelVRML(&model_rotaxis, &model_rotangle);
    writeVRMLViewPoints(fp, model_xf);

    fprintf(fp, "Transform {\n  rotation %g %g %g %g\n  children [\n",
            model_rotaxis.x, model_rotaxis.y, model_rotaxis.z, model_rotangle);
}

void
mcrWriteVrmlTrailer(FILE *fp) {
    fprintf(fp, "  ]\n}\n\n");
}

static unsigned char IdFirstChar[256], IdRestChar[256];

static void
initIdTransTabs() {
    unsigned int i;
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
    IdRestChar[(int)'+'] = '+';
    IdRestChar[(int)'-'] = '-';
    for ( i = '0'; i <= '9'; i++ ) {
        IdRestChar[i] = i;
    }
}

static const char *
makeValidVrmlId(const char *id) {
    int idlen, i, n;
    static char buf[101];
    if ( !id || id[0] == '\0' ) {
        return "";
    }
    idlen = strlen(id);
    if ( idlen > 100 ) {
        logWarning("makeValidVrmlId", "id '%s' is being truncated to %d characters",
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

static void
writeMaterial(Geometry *geom) {
    MeshSurface *surf = geomGetSurface(geom);
    Material *mat = surf->material;
    Patch *firstPatch;
    RayHit hit;
    COLOR Rd;
    COLOR Rs;
    RGB rd;
    RGB rs;
    float specularity;

    if ( surf->faces != nullptr && surf->faces->size() > 0 ) {
        firstPatch = surf->faces->get(0);
    } else {
        firstPatch = nullptr;
    }

    if ( !firstPatch || !mat || !mat->bsdf ) {
        return;
    }

    if ( mat->radiance_data != nullptr) {
        // Has been written before
        fprintf(vrmlfp, "      appearance Appearance {\n");
        fprintf(vrmlfp, "\tmaterial USE %s\n", makeValidVrmlId((char *) mat->radiance_data));
        fprintf(vrmlfp, "      }\n");
        return;
    }

    hitInit(&hit, firstPatch, (Geometry *) nullptr, &firstPatch->midpoint, &firstPatch->normal, mat, 0.);
    Rd = bsdfScatteredPower(mat->bsdf, &hit, &firstPatch->normal, BRDF_DIFFUSE_COMPONENT);
    convertColorToRGB(Rd, &rd);
    Rs = bsdfScatteredPower(mat->bsdf, &hit, &firstPatch->normal, BRDF_GLOSSY_COMPONENT | BRDF_SPECULAR_COMPONENT);
    convertColorToRGB(Rs, &rs);
    specularity = 128.;

    fprintf(vrmlfp, "      appearance Appearance {\n");
    fprintf(vrmlfp, "\tmaterial DEF %s Material {\n", makeValidVrmlId(mat->name));
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

    mat->radiance_data = mat->name;
}

static void
beginWritePrimitive(Geometry *geom) {
    static int wgiv = false;
    fprintf(vrmlfp, "    Shape {\n");
    if ( geomIsSurface(geom)) {
        writeMaterial(geom);
    }
    fprintf(vrmlfp, "      geometry IndexedFaceSet {\n");
    if ( geomIsSurface(geom)) {
        fprintf(vrmlfp, "\tsolid %s\n", geomGetSurface(geom)->material->sided ? "TRUE" : "FALSE");
    }
    if ( !GLOBAL_render_renderOptions.smoothShading && !wgiv ) {
        logWarning(nullptr, "I assume you want a smooth shaded model ...");
        wgiv = true;
    }
    fprintf(vrmlfp, "\tcolorPerVertex %s\n", "TRUE");
}

static const char *
primitiveMatName(Geometry *geom) {
    MeshSurface *surf = geomGetSurface(geom);
    if ( !surf ) {
        return "unknown (not a surface)";
    } else {
        return makeValidVrmlId(surf->material->name);
    }
}

static void
endWritePrimitive(Geometry *geom) {
    fprintf(vrmlfp, "      }\n"); /* end IndexedFaceSet */
    fprintf(vrmlfp, "    },\n");  /* end Shape */

    fprintf(stderr, "Shape material %s, pass %d, %d coords, %d colors, %d coordindices\n",
            primitiveMatName(geom), pass, nrcoords, nrcolors, nrcoordindices);
}

static void
writePrimitivePass(Geometry *geom) {
    beginWritePrimitive(geom);
    writePrimitiveCoords(geom);
    writePrimitiveColors(geom);
    writePrimitiveCoordIndices(geom);
    endWritePrimitive(geom);
}

static void
writePrimitive(Geometry *geom) {
    pass = 0;
    writePrimitivePass(geom);
    if ( leaf_element_count > FACES_PER_SET ) {
        /* large set, additional passes needed */
        for ( pass = 1; pass <= leaf_element_count / FACES_PER_SET; pass++ ) {
            writePrimitivePass(geom);
        } /* write next batch of leaf elements */
    }
}

void
iteratePrimitiveGeoms(GeometryListNode *list, void (*func)(Geometry *)) {
    GeometryListNode *listStart = list;
    if ( listStart != nullptr ) {
        for ( GeometryListNode *window = listStart; window != nullptr; window = window->next ) {
            Geometry *geom = window->geometry;
            if ( geomIsAggregate(geom)) {
                iteratePrimitiveGeoms(geomPrimList(geom), func);
            } else {
                func(geom);
            }
        }
    }
}

static void
resetMaterialData() {
    for (int i = 0; GLOBAL_scene_materials != nullptr && i < GLOBAL_scene_materials->size(); i++) {
        GLOBAL_scene_materials->get(i)->radiance_data = nullptr;
    }
}

void
mcrWriteVrml(FILE *fp) {
    initIdTransTabs();

    matidx = 0;
    resetMaterialData();

    mcrWriteVrmlHeader(fp);

    vrmlfp = fp;
    iteratePrimitiveGeoms(GLOBAL_scene_world, writePrimitive);
    mcrWriteVrmlTrailer(fp);

    resetMaterialData();
}

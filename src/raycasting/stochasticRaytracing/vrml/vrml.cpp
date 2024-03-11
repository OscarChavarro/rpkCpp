/**
Saves "illuminated" model in the VRML'97 file
*/

#include <cstring>

#include "common/error.h"
#include "common/options.h"
#include "skin/Geometry.h"
#include "scene/scene.h"
#include "io/writevrml.h"
#include "render/render.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "raycasting/stochasticRaytracing/vrml/vrml.h"

// Keeps all arrays smaller than 30k els
#define FACES_PER_SET 1000
static int globalVid;
static int global_numberOfWrites;
static int globalNumberOfCoords;
static int globalNumberOfColors;
static int globalNumberOfCoordinateIndices;
static int globalPass;
static int globalLeafElementCount;
static FILE *globalVrmlFp;

// Iterator adapted in order to handle only a subset of the elements
// at a time, taking into account 'pass' and FACES_PER_SET
static void (*elemFunction)(StochasticRadiosityElement *);

static void
countAndCall(Element *element) {
    StochasticRadiosityElement *stochasticRadiosityElement = (StochasticRadiosityElement *)element;
    if ( globalLeafElementCount >= globalPass * FACES_PER_SET &&
         globalLeafElementCount < (globalPass + 1) * FACES_PER_SET ) {
        elemFunction(stochasticRadiosityElement);
    }
    globalLeafElementCount++;
}

static void
geometryIterateLeafElements(Geometry *geom, void (*func)(StochasticRadiosityElement *)) {
    java::ArrayList<Patch *> *patchList = geomPatchArrayListReference(geom);
    elemFunction = func;
    globalLeafElementCount = 0;
    for ( int i = 0; patchList != nullptr && i < patchList->size(); i++ ) {
        if ( topLevelGalerkinElement(patchList->get(i)) != nullptr ) {
            topLevelGalerkinElement(patchList->get(i))->traverseClusterLeafElements(countAndCall);
        }
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
    stochasticRadiosityElementTVertexElimination(elem, triangleResetVertexIds, quadResetVertexIds);
}

static void
writeVertexCoord(Vertex *v) {
    if ( v->tmp == -1 ) {
        /* not yet written */
        if ( global_numberOfWrites > 0 ) {
            fprintf(globalVrmlFp, ", ");
        }
        global_numberOfWrites++;
        if ( global_numberOfWrites % 4 == 0 ) {
            fprintf(globalVrmlFp, "\n\t  ");
        }
        fprintf(globalVrmlFp, "%g %g %g", v->point->x, v->point->y, v->point->z);
        v->tmp = globalVid;
        globalVid++;
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
    stochasticRadiosityElementTVertexElimination(elem, triangleWriteVertexCoords, quadWriteVertexCoords);
}

static void
writePrimitiveCoords(Geometry *geom) {
    geometryIterateLeafElements(geom, resetVertexIds);

    globalVid = global_numberOfWrites = 0;
    fprintf(globalVrmlFp, "\tcoord Coordinate {\n\t  point [ ");
    geometryIterateLeafElements(geom, writeVertexCoords);
    fprintf(globalVrmlFp, " ] ");
    fprintf(globalVrmlFp, "\n\t}\n");

    globalNumberOfCoords = global_numberOfWrites;
}

static void
writeVertexColor(Vertex *v) {
    if ( v->tmp == -1 ) {
        /* not yet written */
        if ( global_numberOfWrites > 0 ) {
            fprintf(globalVrmlFp, ", ");
        }
        global_numberOfWrites++;
        if ( global_numberOfWrites % 4 == 0 ) {
            fprintf(globalVrmlFp, "\n\t  ");
        }
        fprintf(globalVrmlFp, "%.3g %.3g %.3g", v->color.r, v->color.g, v->color.b);
        v->tmp = globalVid;
        globalVid++;
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
    stochasticRadiosityElementTVertexElimination(elem, triangleWriteVertexColors, quadWriteVertexColors);
}

static void
writePrimitiveColors(Geometry *geom) {
    geometryIterateLeafElements(geom, resetVertexIds);

    globalVid = global_numberOfWrites = 0;
    fprintf(globalVrmlFp, "\tcolor Color {\n\t  color [ ");
    geometryIterateLeafElements(geom, elementWriteVertexColors);
    fprintf(globalVrmlFp, " ] ");
    fprintf(globalVrmlFp, "\n\t}\n");

    globalNumberOfColors = global_numberOfWrites;
}

static void
writeCoordIndex(int index) {
    global_numberOfWrites++;
    if ( global_numberOfWrites % 20 == 0 ) {
        fprintf(globalVrmlFp, "\n\t  ");
    }
    fprintf(globalVrmlFp, "%d ", index);
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
    stochasticRadiosityElementTVertexElimination(elem, triangleWriteVertexCoordIndices, quadWriteVertexCoordIndices);
}

static void
writePrimitiveCoordIndices(Geometry *geom) {
    global_numberOfWrites = 0;
    fprintf(globalVrmlFp, "\tcoordIndex [ ");
    geometryIterateLeafElements(geom, elementWriteCoordIndices);
    fprintf(globalVrmlFp, " ]\n");

    globalNumberOfCoordinateIndices = global_numberOfWrites;
}

void
mcrWriteVrmlHeader(FILE *fp) {
    Matrix4x4 modelTransform{};
    Vector3D modelRotationAxis;
    float modelRotationAngle;

    fprintf(fp, "#VRML V2.0 utf8\n\n");

    fprintf(fp,
            "WorldInfo {\n  title \"%s\"\n  info [ \"Created with RenderPark (%s) using Monte Carlo radiosity\" ]\n}\n\n",
            "Some nice model",
            RPKHOME);

    fprintf(fp, "NavigationInfo {\n type \"WALK\"\n headlight FALSE\n}\n\n");

    modelTransform = transformModelVRML(&modelRotationAxis, &modelRotationAngle);
    writeVRMLViewPoints(fp, modelTransform);

    fprintf(fp, "Transform {\n  rotation %g %g %g %g\n  children [\n",
            modelRotationAxis.x, modelRotationAxis.y, modelRotationAxis.z, modelRotationAngle);
}

void
mcrWriteVrmlTrailer(FILE *fp) {
    fprintf(fp, "  ]\n}\n\n");
}

static unsigned char idFirstChar[256], idRestChar[256];

static void
initIdTransTabs() {
    unsigned int i;
    const char *IdSpecialChars = "!$%&()*/:;<=>?@^_`|~";
    for ( i = 0; i < 256; i++ ) {
        idFirstChar[i] = '_';
    }
    for ( i = 'a'; i <= 'z'; i++ ) {
        idFirstChar[i] = i;
    }
    for ( i = 'A'; i <= 'Z'; i++ ) {
        idFirstChar[i] = i;
    }
    for ( i = 0; i < strlen(IdSpecialChars); i++ ) {
        idFirstChar[(int)(unsigned char)IdSpecialChars[i]] = IdSpecialChars[i];
    }
    for ( i = 0; i < 256; i++ ) {
        idRestChar[i] = idFirstChar[i];
    }
    idRestChar[(int)'+'] = '+';
    idRestChar[(int)'-'] = '-';
    for ( i = '0'; i <= '9'; i++ ) {
        idRestChar[i] = i;
    }
}

static const char *
makeValidVrmlId(const char *id) {
    int idLen;
    int i;
    int n;
    static char buf[101];
    if ( !id || id[0] == '\0' ) {
        return "";
    }
    idLen = (int)strlen(id);
    if ( idLen > 100 ) {
        logWarning("makeValidVrmlId", "id '%s' is being truncated to %d characters",
                   id, 100);
    }
    n = idLen > 100 ? 100 : idLen;  /* minimum of both */
    buf[0] = (char)idFirstChar[(int)(unsigned char)id[0]];
    for ( i = 1; i < n; i++ ) {
        buf[i] = (char)idRestChar[(int)(unsigned char)id[i]];
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
    RGB rd{};
    RGB rs{};
    float specularLevel;

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
        fprintf(globalVrmlFp, "      appearance Appearance {\n");
        fprintf(globalVrmlFp, "\tmaterial USE %s\n", makeValidVrmlId((char *) mat->radiance_data));
        fprintf(globalVrmlFp, "      }\n");
        return;
    }

    hitInit(&hit, firstPatch, nullptr, &firstPatch->midpoint, &firstPatch->normal, mat, 0.0);
    Rd = bsdfScatteredPower(mat->bsdf, &hit, &firstPatch->normal, BRDF_DIFFUSE_COMPONENT);
    convertColorToRGB(Rd, &rd);
    Rs = bsdfScatteredPower(mat->bsdf, &hit, &firstPatch->normal, BRDF_GLOSSY_COMPONENT | BRDF_SPECULAR_COMPONENT);
    convertColorToRGB(Rs, &rs);
    specularLevel = 128.0;

    fprintf(globalVrmlFp, "      appearance Appearance {\n");
    fprintf(globalVrmlFp, "\tmaterial DEF %s Material {\n", makeValidVrmlId(mat->name));
    fprintf(globalVrmlFp, "\t  ambientIntensity 0.\n");
    if ( mat->edf ) {
        fprintf(globalVrmlFp, "\t  emissionColor %.3g %.3g %.3g\n", rd.r, rd.g, rd.b);
    } else {
        fprintf(globalVrmlFp, "\t  emissionColor %.3g %.3g %.3g\n", 0.0, 0.0, 0.0);
    }
    fprintf(globalVrmlFp, "\t  diffuseColor %.3g %.3g %.3g \n", rd.r, rd.g, rd.b);
    fprintf(globalVrmlFp, "\t  specularColor %.3g %.3g %.3g \n", rs.r, rs.g, rs.b);
    fprintf(globalVrmlFp, "\t  shininess %g\n", specularLevel / 128.0 > 1.0 ? 1.0 : specularLevel / 128.0);
    fprintf(globalVrmlFp, "\t  transparency 0.\n");
    fprintf(globalVrmlFp, "\t}\n");
    fprintf(globalVrmlFp, "      }\n");

    mat->radiance_data = mat->name;
}

static void
beginWritePrimitive(Geometry *geom) {
    static bool flag = false;
    fprintf(globalVrmlFp, "    Shape {\n");
    if ( geomIsSurface(geom) ) {
        writeMaterial(geom);
    }
    fprintf(globalVrmlFp, "      geometry IndexedFaceSet {\n");
    if ( geomIsSurface(geom) ) {
        fprintf(globalVrmlFp, "\tsolid %s\n", geomGetSurface(geom)->material->sided ? "TRUE" : "FALSE");
    }
    if ( !GLOBAL_render_renderOptions.smoothShading && !flag ) {
        logWarning(nullptr, "I assume you want a smooth shaded model ...");
        flag = true;
    }
    fprintf(globalVrmlFp, "\tcolorPerVertex %s\n", "TRUE");
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
    fprintf(globalVrmlFp, "      }\n"); /* end IndexedFaceSet */
    fprintf(globalVrmlFp, "    },\n");  /* end Shape */

    fprintf(stderr, "Shape material %s, pass %d, %d coords, %d colors, %d coord indices\n",
            primitiveMatName(geom), globalPass, globalNumberOfCoords, globalNumberOfColors, globalNumberOfCoordinateIndices);
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
    globalPass = 0;
    writePrimitivePass(geom);
    if ( globalLeafElementCount > FACES_PER_SET ) {
        // Large set, additional passes needed
        for ( globalPass = 1; globalPass <= globalLeafElementCount / FACES_PER_SET; globalPass++ ) {
            // Write next batch of leaf elements
            writePrimitivePass(geom);
        }
    }
}

static void
iteratePrimitiveGeoms(java::ArrayList<Geometry *> *geometryList, void (*functionCallback)(Geometry *)) {
    for ( int i = 0; geometryList!= nullptr && i < geometryList->size(); i++ ) {
        Geometry *geometry = geometryList->get(i);

        if ( geomIsAggregate(geometry) ) {
            java::ArrayList<Geometry *> * tmpList = geomPrimListCopy(geometry);
            iteratePrimitiveGeoms(tmpList, functionCallback);
            delete tmpList;
        } else {
            functionCallback(geometry);
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
    resetMaterialData();
    mcrWriteVrmlHeader(fp);

    globalVrmlFp = fp;
    java::ArrayList<Geometry *> *allGeometries = GLOBAL_scene_geometries;
    iteratePrimitiveGeoms(allGeometries, writePrimitive);
    delete allGeometries;
    mcrWriteVrmlTrailer(fp);

    resetMaterialData();
}

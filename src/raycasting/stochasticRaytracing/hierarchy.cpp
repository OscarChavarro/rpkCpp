#include "java/util/ArrayList.txx"
#include "material/statistics.h"
#include "scene/scene.h"
#include "shared/options.h"
#include "skin/Vertex.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"

ELEM_HIER_STATE GLOBAL_stochasticRaytracing_hierarchy;

void
elementHierarchyDefaults() {
    GLOBAL_stochasticRaytracing_hierarchy.epsilon = DEFAULT_EH_EPSILON;
    GLOBAL_stochasticRaytracing_hierarchy.minarea = DEFAULT_EH_MINAREA;
    GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing = DEFAULT_EH_HIERARCHICAL_MESHING;
    GLOBAL_stochasticRaytracing_hierarchy.clustering = DEFAULT_EH_CLUSTERING;
    GLOBAL_stochasticRaytracing_hierarchy.tvertex_elimination = DEFAULT_EH_TVERTEX_ELIMINATION;
    GLOBAL_stochasticRaytracing_hierarchy.oracle = powerOracle;
    GLOBAL_stochasticRaytracing_hierarchy.nr_elements = 0;
    GLOBAL_stochasticRaytracing_hierarchy.nr_clusters = 0;
}

void
elementHierarchyInit() {
    /* Max link power is importance-dependent, the initial value is however the
     * same, just in case of importance-driven computations the threshold
     * will be multiplied by the importance value. (jp) */

    GLOBAL_stochasticRaytracing_hierarchy.maxlinkpow = GLOBAL_stochasticRaytracing_hierarchy.epsilon *
                                                       colorMaximumComponent(GLOBAL_statistics_maxSelfEmittedPower);

    // These lists hold vertices created during hierarchical refinement
    GLOBAL_stochasticRaytracing_hierarchy.coords = VectorListCreate();
    GLOBAL_stochasticRaytracing_hierarchy.normals = VectorListCreate();
    GLOBAL_stochasticRaytracing_hierarchy.texCoords = VectorListCreate();
    GLOBAL_stochasticRaytracing_hierarchy.vertices = new java::ArrayList<Vertex *>();
    GLOBAL_stochasticRaytracing_hierarchy.topcluster = monteCarloRadiosityCreateClusterHierarchy(
            GLOBAL_scene_clusteredWorldGeom);
}

void
elementHierarchyTerminate() {
    // Destroy clusters
    monteCarloRadiosityDestroyClusterHierarchy(GLOBAL_stochasticRaytracing_hierarchy.topcluster);
    GLOBAL_stochasticRaytracing_hierarchy.topcluster = (StochasticRadiosityElement *) nullptr;

    // Destroy surface elements
    PatchSet *listStart = (PatchSet *)(GLOBAL_scene_patches);
    if ( listStart != nullptr ) {
        PatchSet *patchWindow;
        for ( patchWindow = listStart; patchWindow; patchWindow = patchWindow->next ) {
            Patch *p = (Patch *) (patchWindow->patch);
            // need to be destroyed before destroying the automatically created vertices
            monteCarloRadiosityDestroyToplevelSurfaceElement(TOPLEVEL_ELEMENT(p));
            p->radianceData = nullptr; // prevents destroying a 2nd time later
        }
    }

    // Delete vertices
    java::ArrayList<Vertex *> *vertices = GLOBAL_stochasticRaytracing_hierarchy.vertices;
    if ( vertices != nullptr ) {
        for ( int i = 0; i < vertices->size(); i++ ) {
            vertexDestroy(vertices->get(i));
        }
        delete vertices;
    }

    // Delete positions
    Vector3DListNode *vectorWindow = GLOBAL_stochasticRaytracing_hierarchy.coords;
    Vector3D *position;
    while ( vectorWindow != nullptr ) {
        position = vectorWindow->vector;
        vectorWindow = vectorWindow->next;
        vector3DDestroy(position);
    }

    vectorWindow = GLOBAL_stochasticRaytracing_hierarchy.coords;
    while ( vectorWindow != nullptr ) {
        Vector3DListNode *positionNode = vectorWindow->next;
        free(vectorWindow);
        vectorWindow = positionNode;
    }
    GLOBAL_stochasticRaytracing_hierarchy.coords = nullptr;

    // Delete normals
    Vector3DListNode *normalWindow = GLOBAL_stochasticRaytracing_hierarchy.normals;
    Vector3D *normal;
    while ( normalWindow != nullptr ) {
        normal = normalWindow->vector;
        normalWindow = normalWindow->next;
        vector3DDestroy(normal);
    }

    normalWindow = GLOBAL_stochasticRaytracing_hierarchy.normals;
    while ( normalWindow != nullptr ) {
        Vector3DListNode *normalNode = normalWindow->next;
        free(normalWindow);
        normalWindow = normalNode;
    }
    GLOBAL_stochasticRaytracing_hierarchy.normals = nullptr;

    // Delete texture coordinates
    Vector3DListNode *texCoordWindow = GLOBAL_stochasticRaytracing_hierarchy.texCoords;
    Vector3D *texCoord;
    while ( texCoordWindow != nullptr ) {
        texCoord = texCoordWindow->vector;
        texCoordWindow = texCoordWindow->next;
        vector3DDestroy(texCoord);
    }

    texCoordWindow = GLOBAL_stochasticRaytracing_hierarchy.texCoords;
    while ( texCoordWindow != nullptr ) {
        Vector3DListNode *texCoordNode = texCoordWindow->next;
        free(texCoordWindow);
        texCoordWindow = texCoordNode;
    }
    GLOBAL_stochasticRaytracing_hierarchy.texCoords = nullptr;
}

#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "java/util/ArrayList.txx"
#include "skin/Vertex.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"

static const float DEFAULT_EH_EPSILON = 5e-4f;
static const float DEFAULT_EH_MINIMUM_AREA = 1e-6f;
static const bool DEFAULT_EH_HIERARCHICAL_MESHING = true;
static const bool DEFAULT_EH_T_VERTEX_ELIMINATION = true;
static const HierarchyClusteringMode DEFAULT_EH_CLUSTERING = HierarchyClusteringMode::ORIENTED_CLUSTERING;

ElementHierarchyState GLOBAL_stochasticRaytracing_hierarchy;

void
elementHierarchyDefaults() {
    GLOBAL_stochasticRaytracing_hierarchy.epsilon = DEFAULT_EH_EPSILON;
    GLOBAL_stochasticRaytracing_hierarchy.minimumArea = DEFAULT_EH_MINIMUM_AREA;
    GLOBAL_stochasticRaytracing_hierarchy.do_h_meshing = DEFAULT_EH_HIERARCHICAL_MESHING;
    GLOBAL_stochasticRaytracing_hierarchy.clustering = DEFAULT_EH_CLUSTERING;
    GLOBAL_stochasticRaytracing_hierarchy.tvertex_elimination = DEFAULT_EH_T_VERTEX_ELIMINATION;
    GLOBAL_stochasticRaytracing_hierarchy.oracle = powerOracle;
    GLOBAL_stochasticRaytracing_hierarchy.nr_elements = 0;
    GLOBAL_stochasticRaytracing_hierarchy.nr_clusters = 0;
}

void
elementHierarchyInit(Geometry *clusteredWorldGeometry) {
    // These lists hold vertices created during hierarchical refinement
    GLOBAL_stochasticRaytracing_hierarchy.coords = new java::ArrayList<Vector3D *>();
    GLOBAL_stochasticRaytracing_hierarchy.normals = new java::ArrayList<Vector3D *>();
    GLOBAL_stochasticRaytracing_hierarchy.texCoords = new java::ArrayList<Vector3D *>();
    GLOBAL_stochasticRaytracing_hierarchy.vertices = new java::ArrayList<Vertex *>();
    GLOBAL_stochasticRaytracing_hierarchy.topCluster =
        stochasticRadiosityElementCreateFromGeometry(clusteredWorldGeometry);
}

void
elementHierarchyTerminate(const java::ArrayList<Patch *> *scenePatches) {
    // Destroy clusters
    stochasticRadiosityElementDestroyClusterHierarchy(GLOBAL_stochasticRaytracing_hierarchy.topCluster);
    GLOBAL_stochasticRaytracing_hierarchy.topCluster = nullptr;

    // Destroy surface elements
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        // Need to be destroyed before destroying the automatically created vertices
        stochasticRadiosityElementDestroy(topLevelStochasticRadiosityElement(patch));
        patch->radianceData = nullptr; // Prevents destroying a 2nd time later
    }

    // Delete vertices
    java::ArrayList<Vertex *> *vertices = GLOBAL_stochasticRaytracing_hierarchy.vertices;
    if ( vertices != nullptr ) {
        for ( int i = 0; i < vertices->size(); i++ ) {
            delete vertices->get(i);
        }
        delete vertices;
    }

    // Delete positions
    for ( int i = 0;
        GLOBAL_stochasticRaytracing_hierarchy.coords != nullptr &&
        i < GLOBAL_stochasticRaytracing_hierarchy.coords->size();
        i++) {
        delete GLOBAL_stochasticRaytracing_hierarchy.coords->get(i);
    }

    delete GLOBAL_stochasticRaytracing_hierarchy.coords;
    GLOBAL_stochasticRaytracing_hierarchy.coords = nullptr;

    // Delete normals
    for ( int i = 0;
          GLOBAL_stochasticRaytracing_hierarchy.normals != nullptr &&
          i < GLOBAL_stochasticRaytracing_hierarchy.normals->size();
          i++ ) {
        delete GLOBAL_stochasticRaytracing_hierarchy.normals->get(i);
    }

    delete GLOBAL_stochasticRaytracing_hierarchy.normals;
    GLOBAL_stochasticRaytracing_hierarchy.normals = nullptr;

    // Delete texture coordinates
    for ( int i = 0;
          GLOBAL_stochasticRaytracing_hierarchy.texCoords != nullptr &&
          i < GLOBAL_stochasticRaytracing_hierarchy.texCoords->size();
          i++ ) {
        Vector3D *texCoord = GLOBAL_stochasticRaytracing_hierarchy.texCoords->get(i);
        delete texCoord;
    }

    delete GLOBAL_stochasticRaytracing_hierarchy.texCoords;
    GLOBAL_stochasticRaytracing_hierarchy.texCoords = nullptr;
}

#endif

#include "material/statistics.h"
#include "scene/scene.h"
#include "skin/Vertex.h"
#include "skin/vertexlist.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"

ELEM_HIER_STATE hierarchy;

/* ========================================================================

                                                                    METHODS 

   ======================================================================== */
void ElementHierarchyDefaults() {
    hierarchy.epsilon = DEFAULT_EH_EPSILON;
    hierarchy.minarea = DEFAULT_EH_MINAREA;
    hierarchy.do_h_meshing = DEFAULT_EH_HIERARCHICAL_MESHING;
    hierarchy.clustering = DEFAULT_EH_CLUSTERING;
    hierarchy.tvertex_elimination = DEFAULT_EH_TVERTEX_ELIMINATION;
    hierarchy.oracle = PowerOracle;

    hierarchy.nr_elements = 0;
    hierarchy.nr_clusters = 0;
}

void ElementHierarchyInit() {
    /* Max link power is importance-depndent, the inital value is however the
     * same, just in case of importance-driven computations the threshold
     * will be multiplied by the importance value. (jp) */

    hierarchy.maxlinkpow = hierarchy.epsilon *
            colorMaximumComponent(GLOBAL_statistics_maxSelfEmittedPower);

    /* These lists hold vertices created during hierarchical refinement. */

    hierarchy.coords = VectorListCreate();
    hierarchy.normals = VectorListCreate();
    hierarchy.texCoords = VectorListCreate();
    hierarchy.vertices = VertexListCreate();

    hierarchy.topcluster = McrCreateClusterHierarchy(GLOBAL_scene_clusteredWorldGeom);
}

void ElementHierarchyTerminate() {
    /* destroy clusters */
    McrDestroyClusterHierarchy(hierarchy.topcluster);
    hierarchy.topcluster = (ELEMENT *) nullptr;

    /* destroy surface elements */
    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    /* need to be destroyed before destroying the automatically created
                     * vertices */
                    McrDestroyToplevelSurfaceElement(TOPLEVEL_ELEMENT(P));
                    P->radiance_data = (void *) nullptr; /* prevents destroying a 2nd time later */
                }
    EndForAll;

    VertexListIterate(hierarchy.vertices, VertexDestroy);
    VertexListDestroy(hierarchy.vertices);
    hierarchy.vertices = VertexListCreate();
    VectorListIterate(hierarchy.coords, VectorDestroy);
    VectorListDestroy(hierarchy.coords);
    hierarchy.coords = VectorListCreate();
    VectorListIterate(hierarchy.normals, VectorDestroy);
    VectorListDestroy(hierarchy.normals);
    hierarchy.normals = VectorListCreate();
    VectorListIterate(hierarchy.texCoords, VectorDestroy);
    VectorListDestroy(hierarchy.texCoords);
    hierarchy.texCoords = VectorListCreate();
}

#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED
#include "material/RendererConfiguration.h"
#include "java/util/ArrayList.txx"
#include "common/logging/Logger.h"
#include "raycasting/stochasticRaytracing/McradP.h"
#include "raycasting/stochasticRaytracing/Hierarchy.h"

ElementHierarchyState::ElementHierarchyState():
    epsilon(),
    do_h_meshing(),
    minimumArea(),
    nr_elements(),
    nr_clusters(),
    tvertex_elimination(),
    clustering(),
    oracle(),
    topCluster(),
    coords(),
    normals(),
    texCoords(),
    vertices()
{
}

void
ElementHierarchyState::setActiveState(ElementHierarchyState &state) {
    activeStatePtr() = &state;
}

ElementHierarchyState &
ElementHierarchyState::activeState() {
    ElementHierarchyState *state = activeStatePtr();
    if ( state == NULL ) {
        Logger::fatal(-1, "ElementHierarchyState::activeState", "Element hierarchy state was not initialized");
    }
    return *state;
}

ElementHierarchyState *&
ElementHierarchyState::activeStatePtr() {
    static ElementHierarchyState *activeState = NULL;
    return activeState;
}

void
Hierarchy::elementHierarchyDefaults() {
    ElementHierarchyState::activeState().epsilon = DEFAULT_EH_EPSILON;
    ElementHierarchyState::activeState().minimumArea = DEFAULT_EH_MINIMUM_AREA;
    ElementHierarchyState::activeState().do_h_meshing = DEFAULT_EH_HIERARCHICAL_MESHING;
    ElementHierarchyState::activeState().clustering = DEFAULT_EH_CLUSTERING;
    ElementHierarchyState::activeState().tvertex_elimination = DEFAULT_EH_T_VERTEX_ELIMINATION;
    ElementHierarchyState::activeState().oracle = Hierarchy::powerOracle;
    ElementHierarchyState::activeState().nr_elements = 0;
    ElementHierarchyState::activeState().nr_clusters = 0;
}

void
Hierarchy::elementHierarchyInit(Geometry *clusteredWorldGeometry) {
    // These lists hold vertices created during hierarchical refinement
    ElementHierarchyState::activeState().coords = new ArrayList<Vector3D *>();
    ElementHierarchyState::activeState().normals = new ArrayList<Vector3D *>();
    ElementHierarchyState::activeState().texCoords = new ArrayList<Vector3D *>();
    ElementHierarchyState::activeState().vertices = new ArrayList<Vertex *>();
    ElementHierarchyState::activeState().topCluster =
        StochasticRadiosityElement::stchsRadElemCreateFromGeom(clusteredWorldGeometry);
}

void
Hierarchy::elementHierarchyTerminate(const ArrayList<Patch *> *scenePatches) {
    // Destroy clusters
    StochasticRadiosityElement::stchsRadElemDestroyClustHier(ElementHierarchyState::activeState().topCluster);
    ElementHierarchyState::activeState().topCluster = NULL;

    // Destroy surface elements
    for ( int i = 0; scenePatches != NULL && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        // Need to be destroyed before destroying the automatically created vertices
        StochasticRadiosityElement::stchsRadElemDestroy(McradP::topLvlStochRadElem(patch));
        patch->radianceData = NULL; // Prevents destroying a 2nd time later
    }

    // Delete vertices
    ArrayList<Vertex *> *vertices = ElementHierarchyState::activeState().vertices;
    if ( vertices != NULL ) {
        for ( int i = 0; i < vertices->size(); i++ ) {
            delete vertices->get(i);
        }
        delete vertices;
    }

    // Delete positions
    for ( int i = 0;
        ElementHierarchyState::activeState().coords != NULL &&
        i < ElementHierarchyState::activeState().coords->size();
        i++) {
        delete ElementHierarchyState::activeState().coords->get(i);
    }

    delete ElementHierarchyState::activeState().coords;
    ElementHierarchyState::activeState().coords = NULL;

    // Delete normals
    for ( int i = 0;
          ElementHierarchyState::activeState().normals != NULL &&
          i < ElementHierarchyState::activeState().normals->size();
          i++ ) {
        delete ElementHierarchyState::activeState().normals->get(i);
    }

    delete ElementHierarchyState::activeState().normals;
    ElementHierarchyState::activeState().normals = NULL;

    // Delete texture coordinates
    for ( int i = 0;
          ElementHierarchyState::activeState().texCoords != NULL &&
          i < ElementHierarchyState::activeState().texCoords->size();
          i++ ) {
        Vector3D *texCoord = ElementHierarchyState::activeState().texCoords->get(i);
        delete texCoord;
    }

    delete ElementHierarchyState::activeState().texCoords;
    ElementHierarchyState::activeState().texCoords = NULL;
}

#endif

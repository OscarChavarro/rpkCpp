#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED
#include "common/RenderOptions.h"
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
    if ( state == nullptr ) {
        Logger::fatal(-1, "ElementHierarchyState::activeState", "Element hierarchy state was not initialized");
    }
    return *state;
}

ElementHierarchyState *&
ElementHierarchyState::activeStatePtr() {
    static ElementHierarchyState *activeState = nullptr;
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
    ElementHierarchyState::activeState().coords = new java::ArrayList<Vector3D *>();
    ElementHierarchyState::activeState().normals = new java::ArrayList<Vector3D *>();
    ElementHierarchyState::activeState().texCoords = new java::ArrayList<Vector3D *>();
    ElementHierarchyState::activeState().vertices = new java::ArrayList<Vertex *>();
    ElementHierarchyState::activeState().topCluster =
        StochasticRadiosityElement::stochasticRadiosityElementCreateFromGeometry(clusteredWorldGeometry);
}

void
Hierarchy::elementHierarchyTerminate(const java::ArrayList<Patch *> *scenePatches) {
    // Destroy clusters
    StochasticRadiosityElement::stochasticRadiosityElementDestroyClusterHierarchy(ElementHierarchyState::activeState().topCluster);
    ElementHierarchyState::activeState().topCluster = nullptr;

    // Destroy surface elements
    for ( int i = 0; scenePatches != nullptr && i < scenePatches->size(); i++ ) {
        Patch *patch = scenePatches->get(i);
        // Need to be destroyed before destroying the automatically created vertices
        StochasticRadiosityElement::stochasticRadiosityElementDestroy(McradP::topLevelStochasticRadiosityElement(patch));
        patch->setRadianceData(nullptr); // Prevents destroying a 2nd time later
    }

    // Delete vertices
    java::ArrayList<Vertex *> *vertices = ElementHierarchyState::activeState().vertices;
    if ( vertices != nullptr ) {
        for ( int i = 0; i < vertices->size(); i++ ) {
            delete vertices->get(i);
        }
        delete vertices;
    }

    // Delete positions
    for ( int i = 0;
        ElementHierarchyState::activeState().coords != nullptr &&
        i < ElementHierarchyState::activeState().coords->size();
        i++) {
        delete ElementHierarchyState::activeState().coords->get(i);
    }

    delete ElementHierarchyState::activeState().coords;
    ElementHierarchyState::activeState().coords = nullptr;

    // Delete normals
    for ( int i = 0;
          ElementHierarchyState::activeState().normals != nullptr &&
          i < ElementHierarchyState::activeState().normals->size();
          i++ ) {
        delete ElementHierarchyState::activeState().normals->get(i);
    }

    delete ElementHierarchyState::activeState().normals;
    ElementHierarchyState::activeState().normals = nullptr;

    // Delete texture coordinates
    for ( int i = 0;
          ElementHierarchyState::activeState().texCoords != nullptr &&
          i < ElementHierarchyState::activeState().texCoords->size();
          i++ ) {
        Vector3D *texCoord = ElementHierarchyState::activeState().texCoords->get(i);
        delete texCoord;
    }

    delete ElementHierarchyState::activeState().texCoords;
    ElementHierarchyState::activeState().texCoords = nullptr;
}

#endif

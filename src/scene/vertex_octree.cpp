/* vertex_octree.c: Octrees containing BREP_VERTEX structs for fast 
 *		    vertex lookup. */

#include "BREP/BREP_SOLID.h"
#include "BREP/BREP_VERTEX_OCTREE.h"
#include "BREP/brep_log.h"
#include "scene/vertex_octree.h"

/* a user specified routine to compare the client data of two vertices */
static BREP_COMPARE_FUNC brep_vertex_clientdata_compare = (BREP_COMPARE_FUNC) nullptr,
        brep_vertex_location_compare = (BREP_COMPARE_FUNC) nullptr;

/* Set a routine to compare the BREP_VERTEX client data for two vertices.
 * BrepSetVertexCompareRoutine() returns the previously installed compare 
 * routine so it can be restored when necessary. */
BREP_COMPARE_FUNC BrepSetVertexCompareRoutine(BREP_COMPARE_FUNC routine) {
    BREP_COMPARE_FUNC oldroutine = brep_vertex_clientdata_compare;
    brep_vertex_clientdata_compare = routine;
    return oldroutine;
}

/* Set a routine to compare the location of two BREP_VERTEXes.
 * BrepSetVertexCompareLocationRoutine() returns the previously installed compare 
 * routine so it can be restored when necessary. */
BREP_COMPARE_FUNC BrepSetVertexCompareLocationRoutine(BREP_COMPARE_FUNC routine) {
    BREP_COMPARE_FUNC oldroutine = brep_vertex_location_compare;
    brep_vertex_location_compare = routine;
    return oldroutine;
}

/* compares two vertices. Calls a user installed routine to compare the
 * client data of two vertices. The routine to be used is specified with
 * BrepSetVertexCompareRoutine() in brep.h */
int BrepVertexCompare(BREP_VERTEX *v1, BREP_VERTEX *v2) {
    if ( !brep_vertex_clientdata_compare ) {
        BrepFatal(v1->client_data, "BrepVertexCompare", "no user specified vertex compare routine!");
    }

    return brep_vertex_clientdata_compare(v1->client_data, v2->client_data);
}

/* Calls a user installed routine to compare the location 
 * of two vertices. The routine to be used is specified with
 * BrepSetVertexCompareLocationRoutine() in brep.h */
int BrepVertexCompareLocation(BREP_VERTEX *v1, BREP_VERTEX *v2) {
    if ( !brep_vertex_location_compare ) {
        BrepFatal(v1->client_data, "BrepVertexCompareLocation",
                  "no user specified routine for comparing the location of two vertices");
    }

    return brep_vertex_location_compare(v1->client_data, v2->client_data);
}

/* Looks up a vertex in the vertex octree, return nullptr if not found */
BREP_VERTEX *BrepFindVertex(void *vertex_data, BREP_VERTEX_OCTREE *vertices) {
    BREP_VERTEX vert;
    vert.client_data = vertex_data;

    return BrepVertexOctreeFind(vertices, &vert);
}

/* attaches an existing BREP_VERTEX to the vertex octree */
void BrepAttachVertex(BREP_VERTEX *vertex, BREP_VERTEX_OCTREE **vertices) {
    *vertices = BrepVertexOctreeAdd(*vertices, vertex);
}

/* remove a BREP_VERTEX from the vertex octree */
void BrepReleaseVertex(BREP_VERTEX *vertex, BREP_VERTEX_OCTREE **vertices) {
    BrepError(vertex->client_data, "BrepReleaseVertex", "not yet implemented");
}

/* Creates a vertex and installs it in the vertex octree. */
BREP_VERTEX *BrepInstallVertex(void *vertex_data, BREP_VERTEX_OCTREE **vertices) {
    BREP_VERTEX *vert;

    vert = BrepCreateVertex(vertex_data);
    *vertices = BrepVertexOctreeAdd(*vertices, vert);

    return vert;
}

/* creates a new vertex octree: currently only returns a nullptr pointer */
BREP_VERTEX_OCTREE *BrepCreateVertexOctree() {
    return BrepVertexOctreeCreate();
}

/* Destroys a vertex octree, does not destroy the vertices referenced in
 * the octree. */
void BrepDestroyVertexOctree(BREP_VERTEX_OCTREE *vertices) {
    BrepVertexOctreeDestroy(vertices);
}

/* calls func for each BREP_VERTEX in the octree */
void BrepIterateVertices(BREP_VERTEX_OCTREE *vertices, void (*func)(BREP_VERTEX *)) {
    BrepVertexOctreeIterate(vertices, func);
}

/* destroys all the vertices and the vertex octree, same as
 * BrepIterateVertices(vertices, BrepDestroyVertex) followed by
 * BrepDestroyvertexOctree(). */
void BrepDestroyVertices(BREP_VERTEX_OCTREE *vertices) {
    BrepIterateVertices(vertices, BrepDestroyVertex);
    BrepDestroyVertexOctree(vertices);
}

/* Looks up the first vertex in the vertex octree at the location as specified in the 
 * given vertex data. There may be multiple vertices at the same location (e.g. having 
 * a different normal and/or a different name). These vertices should normally be
 * stored as a suboctree of the octree containing all vertices. A pointer to the
 * top of this suboctree is returned if there are vertices at the given location.
 * nullptr is returned if there are no vertices at the given location. */
BREP_VERTEX_OCTREE *BrepFindVertexAtLocation(void *vertex_data, BREP_VERTEX_OCTREE *vertices) {
    BREP_VERTEX vert;
    vert.client_data = vertex_data;

    return (BREP_VERTEX_OCTREE *) OctreeFindSubtree((OCTREE *) vertices, (void *) &vert,
                                                    (int (*)(void *, void *)) BrepVertexCompareLocation);
}

/* Iterators over all vertices in the given vertex octree that are at the same
 * location as specified in the vertex data. */
void BrepIterateVerticesAtLocation(void *vertex_data, BREP_VERTEX_OCTREE *vertices,
                                   void (*routine)(BREP_VERTEX *vertex)) {
    BREP_VERTEX_OCTREE *vertoct;

    vertoct = BrepFindVertexAtLocation(vertex_data, vertices);
    OctreeIterate((OCTREE *) vertoct, (void (*)(void *)) routine);
}

/* 1 extra parameter */
void BrepIterateVerticesAtLocation1A(void *vertex_data, BREP_VERTEX_OCTREE *vertices,
                                     void (*routine)(BREP_VERTEX *vertex, void *parm),
                                     void *parm) {
    BREP_VERTEX_OCTREE *vertoct;

    vertoct = BrepFindVertexAtLocation(vertex_data, vertices);
    OctreeIterate1A((OCTREE *) vertoct, (void (*)(void *, void *)) routine, parm);
}

/* 2 extra parameters */
void BrepIterateVerticesAtLocation2A(void *vertex_data, BREP_VERTEX_OCTREE *vertices,
                                     void (*routine)(BREP_VERTEX *vertex, void *parm1, void *parm2),
                                     void *parm1, void *parm2) {
    BREP_VERTEX_OCTREE *vertoct;

    vertoct = BrepFindVertexAtLocation(vertex_data, vertices);
    OctreeIterate2A((OCTREE *) vertoct, (void (*)(void *, void *, void *)) routine, parm1, parm2);
}

static void BrepIterateWingsWithVertices(BREP_VERTEX *v2, BREP_VERTEX_OCTREE *v1octree,
                                         void (*func)(BREP_WING *)) {
    OctreeIterate2A((OCTREE *) v1octree, (void (*)(void *, void *, void *)) BrepIterateWingsWithVertex, (void *) v2,
                    (void *) func);
}

/* Iterator over all edge-wings between vertices at the locations specified by 
 * v1data and v2data. */
void BrepIterateWingsBetweenLocations(void *v1data, void *v2data,
                                      BREP_VERTEX_OCTREE *vertices,
                                      void (*func)(BREP_WING *)) {
    BREP_VERTEX_OCTREE *v1octree, *v2octree;

    v1octree = BrepFindVertexAtLocation(v1data, vertices);
    if ( !v1octree ) {
        return;
    }

    v2octree = BrepFindVertexAtLocation(v2data, vertices);
    if ( !v2octree ) {
        return;
    }

    OctreeIterate2A((OCTREE *) v2octree, (void (*)(void *, void *, void *)) BrepIterateWingsWithVertices,
                    (void *) v1octree, (void *) func);
}

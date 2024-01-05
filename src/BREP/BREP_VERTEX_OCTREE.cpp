/**
Octrees containing BREP_VERTEX structs for fast vertex lookup
*/
#include "BREP/BREP_VERTEX_OCTREE.h"

// A user specified routine to compare the client data of two vertices
static BREP_COMPARE_FUNC globalBrepVertexClientDataCompare = nullptr;
static BREP_COMPARE_FUNC globalBrepVertexLocationCompare = nullptr;

/**
Set a routine to compare the BREP_VERTEX client data for two vertices.
brepSetVertexCompareRoutine() returns the previously installed compare
routine so it can be restored when necessary
*/
BREP_COMPARE_FUNC
brepSetVertexCompareLocationRoutine(BREP_COMPARE_FUNC routine) {
    BREP_COMPARE_FUNC oldRoutine = globalBrepVertexLocationCompare;
    globalBrepVertexLocationCompare = routine;
    return oldRoutine;
}

/**
Set a routine to compare the location of two BREP vertices.
brepSetVertexCompareLocationRoutine() returns the previously installed compare
routine so it can be restored when necessary
*/
BREP_COMPARE_FUNC
brepSetVertexCompareRoutine(BREP_COMPARE_FUNC routine) {
    BREP_COMPARE_FUNC oldRoutine = globalBrepVertexClientDataCompare;
    globalBrepVertexClientDataCompare = routine;
    return oldRoutine;
}

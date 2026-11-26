#include "java/lang/String.h"

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

// The descriptions of the nodes, with the same texts as the Java `toString`
// methods (used by the debugging traces of the operators)

java::String _PolyhedralBoundedSolidEdge::toString() const
{
    java::String msg = "Edge id " + java::String::valueOf(id) + ". Half1: ";
    if ( leftHalf == nullptr ) {
        msg += "null. ";
    }
    else {
        msg += "vertex " + java::String::valueOf(leftHalf->startingVertex->id);
    }

    msg += " / Half2: ";

    if ( rightHalf == nullptr ) {
        msg += "null. ";
    }
    else {
        msg += "vertex " + java::String::valueOf(rightHalf->startingVertex->id);
    }
    return java::String(msg.c_str());
}

java::String _PolyhedralBoundedSolidFace::toString() const
{
    java::String msg = "Face id [" + java::String::valueOf(id) + "], " +
        java::String::valueOf(boundariesList.size()) + " loops.";
    return java::String(msg.c_str());
}

java::String _PolyhedralBoundedSolidHalfEdge::toString()
{
    java::String msg = "HalfEdge id " + java::String::valueOf(id) + ". ";

    msg += "From vertex [" + java::String::valueOf(startingVertex->id) + "] ";
    msg += "to vertex [" + java::String::valueOf(next()->startingVertex->id) + "]. ";

    msg += "Parent face [" + java::String::valueOf(parentLoop->parentFace->id) + "]. ";
    if ( parentEdge == nullptr ) {
        msg += "<without parent edge>. ";
    }
    else {
        msg += "Parent edge " + java::String::valueOf(parentEdge->id) + " ";
        if ( this == parentEdge->leftHalf ) {
            msg += "(left)";
        }
        else if ( this == parentEdge->rightHalf ) {
            msg += "(right)";
        }
        else {
            msg += "(INCONSISTENT!)";
        }
        msg += ". ";
    }
    msg += "Next halfedge: " + java::String::valueOf(next()->id) + ".";
    return java::String(msg.c_str());
}

java::String _PolyhedralBoundedSolidLoop::toString() const
{
    java::String msg = "Loop, parent face " + java::String::valueOf(parentFace->id);
    return java::String(msg.c_str());
}

java::String _PolyhedralBoundedSolidVertex::toString() const
{
    java::String* p = position.toString();
    java::String msg = "vertex id " + java::String::valueOf(id) + ". Position " +
        java::String(p->c_str()) + ". ";
    delete p;
    if ( emanatingHalfEdge == nullptr ) {
        msg += "No associated half-edge.";
    }
    else {
        msg += "H.E. " + java::String::valueOf(emanatingHalfEdge->id);
    }
    return java::String(msg.c_str());
}

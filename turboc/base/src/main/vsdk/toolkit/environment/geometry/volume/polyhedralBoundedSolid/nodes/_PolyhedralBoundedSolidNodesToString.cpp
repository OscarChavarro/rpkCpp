#include <stddef.h>
#include "java/lang/String.h"

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
#include "vsdk/toolkit/common/Cpp98Support.h"

// The descriptions of the nodes, with the same texts as the Java `toString`
// methods (used by the debugging traces of the operators)

String _PolyhedralBoundedSolidEdge::toString() const
{
    String msg = "Edge id " + Cpp98Support::toString(id) + ". Half1: ";
    if ( leftHalf == NULL ) {
        msg += "null. ";
    }
    else {
        msg += "vertex " + Cpp98Support::toString(leftHalf->startingVertex->id);
    }

    msg += " / Half2: ";

    if ( rightHalf == NULL ) {
        msg += "null. ";
    }
    else {
        msg += "vertex " + Cpp98Support::toString(rightHalf->startingVertex->id);
    }
    return String(msg.c_str());
}

String _PolyhedralBoundedSolidFace::toString() const
{
    String msg = "Face id [" + Cpp98Support::toString(id) + "], " +
        Cpp98Support::toString(boundariesList.size()) + " loops.";
    return String(msg.c_str());
}

String _PolyhedralBoundedSolidHalfEdge::toString()
{
    String msg = "HalfEdge id " + Cpp98Support::toString(id) + ". ";

    msg += "From vertex [" + Cpp98Support::toString(startingVertex->id) + "] ";
    msg += "to vertex [" + Cpp98Support::toString(next()->startingVertex->id) + "]. ";

    msg += "Parent face [" + Cpp98Support::toString(parentLoop->parentFace->id) + "]. ";
    if ( parentEdge == NULL ) {
        msg += "<without parent edge>. ";
    }
    else {
        msg += "Parent edge " + Cpp98Support::toString(parentEdge->id) + " ";
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
    msg += "Next halfedge: " + Cpp98Support::toString(next()->id) + ".";
    return String(msg.c_str());
}

String _PolyhedralBoundedSolidLoop::toString() const
{
    String msg = "Loop, parent face " + Cpp98Support::toString(parentFace->id);
    return String(msg.c_str());
}

String _PolyhedralBoundedSolidVertex::toString() const
{
    String* p = position.toString();
    String msg = "vertex id " + Cpp98Support::toString(id) + ". Position " +
        String(p->c_str()) + ". ";
    delete p;
    if ( emanatingHalfEdge == NULL ) {
        msg += "No associated half-edge.";
    }
    else {
        msg += "H.E. " + Cpp98Support::toString(emanatingHalfEdge->id);
    }
    return String(msg.c_str());
}

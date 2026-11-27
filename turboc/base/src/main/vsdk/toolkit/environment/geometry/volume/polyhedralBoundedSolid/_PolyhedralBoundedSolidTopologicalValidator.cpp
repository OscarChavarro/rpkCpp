
#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.txx"
#include "java/util/Collections.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/Cpp98Support.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidTopologicalValidator.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

static const char* const CLASS_NAME = "_PolyhedralBoundedSolidTopologicalValidator";

BOOLEAN _PolyhedralBoundedSolidTopologicalValidator::validateTopologicalIntegrity(
    PolyhedralBoundedSolid* solid)
{
    long int i;
    long int j;
    long int k;
    _PolyhedralBoundedSolidEdge* e;
    _PolyhedralBoundedSolidHalfEdge* h1;
    _PolyhedralBoundedSolidHalfEdge* h2;
    _PolyhedralBoundedSolidFace* f;
    _PolyhedralBoundedSolidLoop* l;

    for ( i = 0; i < solid->getEdgesList().size(); i++ ) {
        e = solid->getEdgesList().get(i);
        h1 = e->rightHalf;
        h2 = e->leftHalf;
        if ( h1 == 0 || h2 == 0 ) {
            Logger::reportMessage(CLASS_NAME, Logger::WARNING,
                "validateTopologicalIntegrity", "Edge with null halfedge!");
            return FALSE;
        }
        if ( h1->parentLoop->parentFace->parentSolid !=
             h2->parentLoop->parentFace->parentSolid ) {
            Logger::reportMessage(CLASS_NAME, Logger::WARNING,
                "validateTopologicalIntegrity",
                "Edge belonging to two different solids!");
            return FALSE;
        }
    }

    ArrayList<int> edgeCount(Collections<int>::nCopies(solid->getEdgesList().size(), 0));

    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        f = solid->getPolygonsList().get(i);
        for ( j = 0; j < f->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidHalfEdge* he;
            _PolyhedralBoundedSolidHalfEdge* heStart;

            l = f->boundariesList.get(j);
            he = l->boundaryStartHalfEdge;
            if ( he == 0 ) {
                Logger::reportMessage(CLASS_NAME, Logger::WARNING,
                    "validateTopologicalIntegrity",
                    String("Loop without starting halfedge\n") +
                    "Offending solid:\n" + solid->toString());
                return FALSE;
            }
            heStart = he;
            do {
                he = he->next();
                if ( he == 0 ) {
                    Logger::reportMessage(CLASS_NAME, Logger::WARNING,
                        "validateTopologicalIntegrity", "Not closed loop!");
                    return FALSE;
                }
                for ( k = 0; k < (long int)edgeCount.size(); k++ ) {
                    if ( he->parentEdge == solid->getEdgesList().get(k) ) {
                        edgeCount[k]++;
                        break;
                    }
                }
            } while ( he != heStart );
        }
    }

    for ( i = 0; i < (long int)edgeCount.size(); i++ ) {
        if ( edgeCount[i] != 2 ) {
            Logger::reportMessage(CLASS_NAME, Logger::WARNING,
                "validateTopologicalIntegrity",
                "Edges with different halfedges than 2!");
            return FALSE;
        }
    }
    return TRUE;
}

BOOLEAN _PolyhedralBoundedSolidTopologicalValidator::validateBackPointers(
    PolyhedralBoundedSolid* solid, String* msg)
{
    BOOLEAN ok = TRUE;
    long int i;
    long int j;
    long int k;
    String discarded;

    if ( msg == 0 ) {
        msg = &discarded;
    }
    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        for ( j = 0; j < face->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(j);
            if ( loop->boundaryStartHalfEdge == 0 ||
                 !loop->halfEdgesList.contains(
                     loop->boundaryStartHalfEdge) ) {
                *msg += "  - Face [";
                *msg += Cpp98Support::toString(face->id).c_str();
                *msg += "] has a loop whose start half-edge is not";
                *msg += " a member of the loop.\n";
                ok = FALSE;
            }
            for ( k = 0; k < loop->halfEdgesList.size(); k++ ) {
                if ( loop->halfEdgesList.get(k)->parentLoop != loop ) {
                    *msg += "  - Face [";
                    *msg += Cpp98Support::toString(face->id).c_str();
                    *msg += "] has a half-edge whose parent loop";
                    *msg += " is another loop.\n";
                    ok = FALSE;
                    break;
                }
            }
        }
    }

    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        _PolyhedralBoundedSolidVertex* v = solid->getVerticesList().get(i);
        _PolyhedralBoundedSolidHalfEdge* he = v->emanatingHalfEdge;
        if ( he == 0 ) {
            *msg += "  - Vertex [";
            *msg += Cpp98Support::toString(v->id).c_str();
            *msg += "] has no emanating half-edge.\n";
            ok = FALSE;
        }
        else if ( he->startingVertex != v ) {
            *msg += "  - Vertex [";
            *msg += Cpp98Support::toString(v->id).c_str();
            *msg += "] emanating half-edge starts at another";
            *msg += " vertex.\n";
            ok = FALSE;
        }
        else if ( he->parentLoop == 0 ||
                  !he->parentLoop->halfEdgesList.contains(he) ) {
            *msg += "  - Vertex [";
            *msg += Cpp98Support::toString(v->id).c_str();
            *msg += "] emanating half-edge is not a member of its";
            *msg += " loop.\n";
            ok = FALSE;
        }
    }
    return ok;
}

#include "java/lang/String.h"

#include "java/util/ArrayList.txx"
#include "java/util/Collections.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidTopologicalValidator.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

namespace {
const char* const CLASS_NAME = "_PolyhedralBoundedSolidTopologicalValidator";
}

bool _PolyhedralBoundedSolidTopologicalValidator::validateTopologicalIntegrity(
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
            return false;
        }
        if ( h1->parentLoop->parentFace->parentSolid !=
             h2->parentLoop->parentFace->parentSolid ) {
            Logger::reportMessage(CLASS_NAME, Logger::WARNING,
                "validateTopologicalIntegrity",
                "Edge belonging to two different solids!");
            return false;
        }
    }

    java::ArrayList<int> edgeCount = java::Collections::nCopies<int>(solid->getEdgesList().size(), 0);

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
                    java::String("Loop without starting halfedge\n") +
                    "Offending solid:\n" + solid->toString());
                return false;
            }
            heStart = he;
            do {
                he = he->next();
                if ( he == 0 ) {
                    Logger::reportMessage(CLASS_NAME, Logger::WARNING,
                        "validateTopologicalIntegrity", "Not closed loop!");
                    return false;
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
            return false;
        }
    }
    return true;
}

/**
Back-pointer invariants of the half-edge structure ([MANT1988] Sec.
10.2; Fable5.2Analysis.md Stage B step 3):
- every loop's `boundaryStartHalfEdge` is a member of the loop, and every
  member half-edge names that loop as `parentLoop`;
- every vertex has a non-null emanating half-edge that starts at the
  vertex and is a member of its loop (for the lone vertex of an empty
  loop, that is the loop's edge-less half-edge).
Validation never repairs them: see
`PolyhedralBoundedSolidRepair::remakeEmanatingHalfEdgeReferences`.
@return true when all hold; violations are appended to `msg`.
*/
bool _PolyhedralBoundedSolidTopologicalValidator::validateBackPointers(
    PolyhedralBoundedSolid* solid,
    java::String* msg)
{
    bool ok = true;
    int i;
    int j;
    int k;

    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        for ( j = 0; j < face->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(j);
            if ( loop->boundaryStartHalfEdge == 0 ||
                 !loop->halfEdgesList.contains(
                     loop->boundaryStartHalfEdge) ) {
                *msg += java::String("  - Face [") +
                    java::String::valueOf(face->id) +
                    "] has a loop whose start half-edge is not" +
                    " a member of the loop.\n";
                ok = false;
            }
            for ( k = 0; k < loop->halfEdgesList.size(); k++ ) {
                if ( loop->halfEdgesList.get(k)->parentLoop != loop ) {
                    *msg += java::String("  - Face [") +
                        java::String::valueOf(face->id) +
                        "] has a half-edge whose parent loop" +
                        " is another loop.\n";
                    ok = false;
                    break;
                }
            }
        }
    }

    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        _PolyhedralBoundedSolidVertex* v = solid->getVerticesList().get(i);
        _PolyhedralBoundedSolidHalfEdge* he = v->emanatingHalfEdge;
        if ( he == 0 ) {
            *msg += java::String("  - Vertex [") +
                java::String::valueOf(v->id) +
                "] has no emanating half-edge.\n";
            ok = false;
        }
        else if ( he->startingVertex != v ) {
            *msg += java::String("  - Vertex [") +
                java::String::valueOf(v->id) +
                "] emanating half-edge starts at another" +
                " vertex.\n";
            ok = false;
        }
        else if ( he->parentLoop == 0 ||
                  !he->parentLoop->halfEdgesList.contains(he) ) {
            *msg += java::String("  - Vertex [") +
                java::String::valueOf(v->id) +
                "] emanating half-edge is not a member of its" +
                " loop.\n";
            ok = false;
        }
    }
    return ok;
}

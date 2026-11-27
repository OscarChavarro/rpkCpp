#include <stddef.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/TreeMap.h"
#include "java/util/TreeSet.h"
#include "java/lang/String.h"

#include "java/util/ArrayList.txx"
#include "java/util/Collections.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidTopologySummary.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
#include "vsdk/toolkit/common/Cpp98Support.h"
#include "java/lang/IllegalArgumentException.h"


static _PolyhedralBoundedSolidFace* faceOf(_PolyhedralBoundedSolidHalfEdge* halfEdge)
{
    if ( halfEdge == NULL || halfEdge->parentLoop == NULL ) {
        return NULL;
    }
    return halfEdge->parentLoop->parentFace;
}

static void collectVertex(_PolyhedralBoundedSolidHalfEdge* halfEdge,
                   TreeSet<_PolyhedralBoundedSolidVertex*>& vertices)
{
    if ( halfEdge != NULL && halfEdge->startingVertex != NULL ) {
        vertices.add(halfEdge->startingVertex);
    }
}

static void collectFaceVertices(_PolyhedralBoundedSolidFace* face,
                         TreeSet<_PolyhedralBoundedSolidVertex*>& vertices)
{
    long i;
    long j;
    for ( i = 0; i < face->boundariesList.size(); i++ ) {
        for ( j = 0;
              j < face->boundariesList.get(i)->halfEdgesList.size();
              j++ ) {
            collectVertex(
                face->boundariesList.get(i)->halfEdgesList.get(j),
                vertices);
        }
    }
}

static int floorMod(int value, int modulus)
{
    int result = value % modulus;
    return result < 0 ? result + modulus : result;
}

static const char* booleanName(BOOLEAN value)
{
    return value ? "true" : "false";
}

class DisjointSet {
private:
    ArrayList<int> parent;
    ArrayList<unsigned char> rank;

public:
    explicit DisjointSet(int size)
        : parent(Collections<int>::nCopies(size, 0)),
          rank(Collections<unsigned char>::nCopies(size, 0))
    {
        for ( int i = 0; i < size; i++ ) {
            parent[i] = i;
        }
    }

    int find(int value)
    {
        if ( parent[value] != value ) {
            parent[value] = find(parent[value]);
        }
        return parent[value];
    }

    void unite(int first, int second)
    {
        int firstRoot = find(first);
        int secondRoot = find(second);
        if ( firstRoot == secondRoot ) {
            return;
        }
        if ( rank[firstRoot] < rank[secondRoot] ) {
            parent[firstRoot] = secondRoot;
        }
        else if ( rank[firstRoot] > rank[secondRoot] ) {
            parent[secondRoot] = firstRoot;
        }
        else {
            parent[secondRoot] = firstRoot;
            rank[firstRoot]++;
        }
    }
};


//= Shell =================================================================

PolyhedralBoundedSolidTopologySummary::Shell::Shell()
    : faceCount(0), edgeCount(0), vertexCount(0),
      adjustedEulerCharacteristic(0), closed(FALSE),
      closedOrientableEulerCompatible(FALSE)
{
}

PolyhedralBoundedSolidTopologySummary::Shell::Shell(int faceCount,
    int edgeCount, int vertexCount, int adjustedEulerCharacteristic,
    BOOLEAN closed)
    : faceCount(faceCount), edgeCount(edgeCount), vertexCount(vertexCount),
      adjustedEulerCharacteristic(adjustedEulerCharacteristic),
      closed(closed),
      closedOrientableEulerCompatible(closed &&
          adjustedEulerCharacteristic <= 2 &&
          floorMod(adjustedEulerCharacteristic, 2) == 0)
{
}

int PolyhedralBoundedSolidTopologySummary::Shell::getFaceCount() const { return faceCount; }
int PolyhedralBoundedSolidTopologySummary::Shell::getEdgeCount() const { return edgeCount; }
int PolyhedralBoundedSolidTopologySummary::Shell::getVertexCount() const { return vertexCount; }
int PolyhedralBoundedSolidTopologySummary::Shell::getAdjustedEulerCharacteristic() const { return adjustedEulerCharacteristic; }
BOOLEAN PolyhedralBoundedSolidTopologySummary::Shell::isClosed() const { return closed; }
BOOLEAN PolyhedralBoundedSolidTopologySummary::Shell::isClosedOrientableEulerCompatible() const { return closedOrientableEulerCompatible; }

String PolyhedralBoundedSolidTopologySummary::Shell::toString() const
{
    String msg = "Shell{faces=" + Cpp98Support::toString(faceCount) +
        ", edges=" + Cpp98Support::toString(edgeCount) +
        ", vertices=" + Cpp98Support::toString(vertexCount) +
        ", adjustedEuler=" + Cpp98Support::toString(adjustedEulerCharacteristic) +
        ", closed=" + booleanName(closed) +
        ", closedOrientableEulerCompatible=" +
        booleanName(closedOrientableEulerCompatible) + "}";
    return String(msg.c_str());
}

//= Summary ===============================================================

PolyhedralBoundedSolidTopologySummary::PolyhedralBoundedSolidTopologySummary(
    PolyhedralBoundedSolid* solid)
{
    faceCount = (int)solid->getPolygonsList().size();
    edgeCount = (int)solid->getEdgesList().size();
    vertexCount = (int)solid->getVerticesList().size();

    TreeMap<_PolyhedralBoundedSolidFace*, int> faceIndexes;
    int i;
    for ( i = 0; i < faceCount; i++ ) {
        faceIndexes.put(solid->getPolygonsList().get(i), i);
    }

    DisjointSet components(faceCount);
    int invalidAdjacencies = 0;
    for ( i = 0; i < edgeCount; i++ ) {
        _PolyhedralBoundedSolidEdge* edge = solid->getEdgesList().get(i);
        const int* leftIndex = faceIndexes.get(faceOf(edge->leftHalf));
        const int* rightIndex = faceIndexes.get(faceOf(edge->rightHalf));
        if ( leftIndex == NULL || rightIndex == NULL ) {
            invalidAdjacencies++;
            continue;
        }
        components.unite(*leftIndex, *rightIndex);
    }
    invalidEdgeAdjacencyCount = invalidAdjacencies;

    // Ordered by component root, as the Java `TreeMap`
    TreeMap<int, ArrayList<_PolyhedralBoundedSolidFace*> > componentFaces;
    int reachedFaces = 0;
    for ( i = 0; i < faceCount; i++ ) {
        int root = components.find(i);
        componentFaces.putIfAbsent(root,
            ArrayList<_PolyhedralBoundedSolidFace*>()).add(
                solid->getPolygonsList().get(i));
        reachedFaces++;
    }
    int componentFaceTotal = 0;
    long c;
    for ( c = 0; c < componentFaces.size(); c++ ) {
        componentFaceTotal += (int)componentFaces.getValue(c).size();
    }
    everyFaceReachedExactlyOnce =
        (int)faceIndexes.size() == faceCount &&
        reachedFaces == faceCount && componentFaceTotal == faceCount;

    int totalAdjustedFaceTerm = 0;
    for ( c = 0; c < componentFaces.size(); c++ ) {
        const ArrayList<_PolyhedralBoundedSolidFace*>& faces =
            componentFaces.getValue(c);
        TreeSet<_PolyhedralBoundedSolidFace*> faceSet;
        long f;
        for ( f = 0; f < faces.size(); f++ ) {
            faceSet.add(faces[f]);
        }
        TreeSet<_PolyhedralBoundedSolidEdge*> shellEdges;
        TreeSet<_PolyhedralBoundedSolidVertex*> shellVertices;
        int adjustedFaceTerm = 0;
        BOOLEAN closed = TRUE;

        for ( f = 0; f < faces.size(); f++ ) {
            adjustedFaceTerm += 2 - (int)faces[f]->boundariesList.size();
            collectFaceVertices(faces[f], shellVertices);
        }
        totalAdjustedFaceTerm += adjustedFaceTerm;

        for ( i = 0; i < (int)solid->getEdgesList().size(); i++ ) {
            _PolyhedralBoundedSolidEdge* edge = solid->getEdgesList().get(i);
            _PolyhedralBoundedSolidFace* leftFace = faceOf(edge->leftHalf);
            _PolyhedralBoundedSolidFace* rightFace = faceOf(edge->rightHalf);
            BOOLEAN touchesShell = faceSet.contains(leftFace) ||
                faceSet.contains(rightFace);
            if ( !touchesShell ) {
                continue;
            }
            shellEdges.add(edge);
            if ( leftFace == NULL || rightFace == NULL ||
                 !faceSet.contains(leftFace) ||
                 !faceSet.contains(rightFace) ) {
                closed = FALSE;
            }
            collectVertex(edge->leftHalf, shellVertices);
            collectVertex(edge->rightHalf, shellVertices);
        }

        int chi = (int)shellVertices.size() - (int)shellEdges.size() +
            adjustedFaceTerm;
        shells.add(Shell((int)faces.size(), (int)shellEdges.size(),
            (int)shellVertices.size(), chi, closed));
    }
    adjustedEulerCharacteristic =
        vertexCount - edgeCount + totalAdjustedFaceTerm;
}

PolyhedralBoundedSolidTopologySummary PolyhedralBoundedSolidTopologySummary::from(
    PolyhedralBoundedSolid* solid)
{
    if ( solid == NULL ) {
        throw IllegalArgumentException("solid must not be null");
    }
    return PolyhedralBoundedSolidTopologySummary(solid);
}

int PolyhedralBoundedSolidTopologySummary::getFaceCount() const { return faceCount; }
int PolyhedralBoundedSolidTopologySummary::getEdgeCount() const { return edgeCount; }
int PolyhedralBoundedSolidTopologySummary::getVertexCount() const { return vertexCount; }
int PolyhedralBoundedSolidTopologySummary::getShellCount() const { return (int)shells.size(); }
int PolyhedralBoundedSolidTopologySummary::getAdjustedEulerCharacteristic() const { return adjustedEulerCharacteristic; }
BOOLEAN PolyhedralBoundedSolidTopologySummary::isEveryFaceReachedExactlyOnce() const { return everyFaceReachedExactlyOnce; }
int PolyhedralBoundedSolidTopologySummary::getInvalidEdgeAdjacencyCount() const { return invalidEdgeAdjacencyCount; }

const ArrayList<PolyhedralBoundedSolidTopologySummary::Shell>&
PolyhedralBoundedSolidTopologySummary::getShells() const
{
    return shells;
}

BOOLEAN PolyhedralBoundedSolidTopologySummary::hasUniversalContradiction() const
{
    if ( !everyFaceReachedExactlyOnce || invalidEdgeAdjacencyCount > 0 ) {
        return TRUE;
    }
    for ( long i = 0; i < shells.size(); i++ ) {
        if ( !shells[i].isClosedOrientableEulerCompatible() ) {
            return TRUE;
        }
    }
    return FALSE;
}

String PolyhedralBoundedSolidTopologySummary::toString() const
{
    String perShell = "[";
    for ( long i = 0; i < shells.size(); i++ ) {
        if ( i > 0 ) {
            perShell += ", ";
        }
        perShell += shells[i].toString().c_str();
    }
    perShell += "]";
    String msg = "TopologySummary{faces=" + Cpp98Support::toString(faceCount) +
        ", edges=" + Cpp98Support::toString(edgeCount) +
        ", vertices=" + Cpp98Support::toString(vertexCount) +
        ", shells=" + Cpp98Support::toString(shells.size()) +
        ", adjustedEuler=" + Cpp98Support::toString(adjustedEulerCharacteristic) +
        ", everyFaceReachedExactlyOnce=" +
        booleanName(everyFaceReachedExactlyOnce) +
        ", invalidEdgeAdjacencies=" + Cpp98Support::toString(invalidEdgeAdjacencyCount) +
        ", perShell=" + perShell + "}";
    return String(msg.c_str());
}

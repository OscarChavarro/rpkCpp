#include "java/lang/IllegalArgumentException.h"
#include "java/lang/String.h"
#include "java/util/Collections.h"
#include "java/util/TreeMap.txx"
#include "java/util/TreeSet.txx"

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidTopologySummary.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

namespace {

_PolyhedralBoundedSolidFace* faceOf(_PolyhedralBoundedSolidHalfEdge* halfEdge)
{
    if ( halfEdge == nullptr || halfEdge->parentLoop == nullptr ) {
        return nullptr;
    }
    return halfEdge->parentLoop->parentFace;
}

void collectVertex(_PolyhedralBoundedSolidHalfEdge* halfEdge,
                   java::TreeSet<_PolyhedralBoundedSolidVertex*>& vertices)
{
    if ( halfEdge != nullptr && halfEdge->startingVertex != nullptr ) {
        vertices.add(halfEdge->startingVertex);
    }
}

void collectFaceVertices(_PolyhedralBoundedSolidFace* face,
                         java::TreeSet<_PolyhedralBoundedSolidVertex*>& vertices)
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

int floorMod(int value, int modulus)
{
    int result = value % modulus;
    return result < 0 ? result + modulus : result;
}

const char* booleanName(bool value)
{
    return value ? "true" : "false";
}

class DisjointSet {
private:
    java::ArrayList<int> parent;
    java::ArrayList<unsigned char> rank;

public:
    explicit DisjointSet(int size)
        : parent(java::Collections::nCopies<int>(size, 0)),
          rank(java::Collections::nCopies<unsigned char>(size, 0))
    {
        for ( int i = 0; i < size; i++ ) {
            parent[(size_t)i] = i;
        }
    }

    int find(int value)
    {
        if ( parent[(size_t)value] != value ) {
            parent[(size_t)value] = find(parent[(size_t)value]);
        }
        return parent[(size_t)value];
    }

    void unite(int first, int second)
    {
        int firstRoot = find(first);
        int secondRoot = find(second);
        if ( firstRoot == secondRoot ) {
            return;
        }
        if ( rank[(size_t)firstRoot] < rank[(size_t)secondRoot] ) {
            parent[(size_t)firstRoot] = secondRoot;
        }
        else if ( rank[(size_t)firstRoot] > rank[(size_t)secondRoot] ) {
            parent[(size_t)secondRoot] = firstRoot;
        }
        else {
            parent[(size_t)secondRoot] = firstRoot;
            rank[(size_t)firstRoot]++;
        }
    }
};

}

//= Shell =================================================================

PolyhedralBoundedSolidTopologySummary::Shell::Shell(int faceCount,
    int edgeCount, int vertexCount, int adjustedEulerCharacteristic,
    bool closed)
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
bool PolyhedralBoundedSolidTopologySummary::Shell::isClosed() const { return closed; }
bool PolyhedralBoundedSolidTopologySummary::Shell::isClosedOrientableEulerCompatible() const { return closedOrientableEulerCompatible; }

java::String PolyhedralBoundedSolidTopologySummary::Shell::toString() const
{
    java::String msg = "Shell{faces=" + java::String::valueOf(faceCount) +
        ", edges=" + java::String::valueOf(edgeCount) +
        ", vertices=" + java::String::valueOf(vertexCount) +
        ", adjustedEuler=" + java::String::valueOf(adjustedEulerCharacteristic) +
        ", closed=" + booleanName(closed) +
        ", closedOrientableEulerCompatible=" +
        booleanName(closedOrientableEulerCompatible) + "}";
    return java::String(msg.c_str());
}

//= Summary ===============================================================

PolyhedralBoundedSolidTopologySummary::PolyhedralBoundedSolidTopologySummary(
    PolyhedralBoundedSolid* solid)
{
    faceCount = (int)solid->getPolygonsList().size();
    edgeCount = (int)solid->getEdgesList().size();
    vertexCount = (int)solid->getVerticesList().size();

    java::TreeMap<_PolyhedralBoundedSolidFace*, int> faceIndexes;
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
        if ( leftIndex == nullptr || rightIndex == nullptr ) {
            invalidAdjacencies++;
            continue;
        }
        components.unite(*leftIndex, *rightIndex);
    }
    invalidEdgeAdjacencyCount = invalidAdjacencies;

    // Ordered by component root, as the Java `TreeMap`
    java::TreeMap<int, java::ArrayList<_PolyhedralBoundedSolidFace*> > componentFaces;
    int reachedFaces = 0;
    for ( i = 0; i < faceCount; i++ ) {
        int root = components.find(i);
        componentFaces.putIfAbsent(root,
            java::ArrayList<_PolyhedralBoundedSolidFace*>()).add(
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
        const java::ArrayList<_PolyhedralBoundedSolidFace*>& faces =
            componentFaces.getValue(c);
        java::TreeSet<_PolyhedralBoundedSolidFace*> faceSet;
        for ( long f = 0; f < faces.size(); f++ ) {
            faceSet.add(faces[f]);
        }
        java::TreeSet<_PolyhedralBoundedSolidEdge*> shellEdges;
        java::TreeSet<_PolyhedralBoundedSolidVertex*> shellVertices;
        int adjustedFaceTerm = 0;
        bool closed = true;

        for ( size_t f = 0; f < faces.size(); f++ ) {
            adjustedFaceTerm += 2 - (int)faces[f]->boundariesList.size();
            collectFaceVertices(faces[f], shellVertices);
        }
        totalAdjustedFaceTerm += adjustedFaceTerm;

        for ( i = 0; i < (int)solid->getEdgesList().size(); i++ ) {
            _PolyhedralBoundedSolidEdge* edge = solid->getEdgesList().get(i);
            _PolyhedralBoundedSolidFace* leftFace = faceOf(edge->leftHalf);
            _PolyhedralBoundedSolidFace* rightFace = faceOf(edge->rightHalf);
            bool touchesShell = faceSet.contains(leftFace) ||
                faceSet.contains(rightFace);
            if ( !touchesShell ) {
                continue;
            }
            shellEdges.add(edge);
            if ( leftFace == nullptr || rightFace == nullptr ||
                 !faceSet.contains(leftFace) ||
                 !faceSet.contains(rightFace) ) {
                closed = false;
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
    if ( solid == nullptr ) {
        throw java::IllegalArgumentException("solid must not be null");
    }
    return PolyhedralBoundedSolidTopologySummary(solid);
}

int PolyhedralBoundedSolidTopologySummary::getFaceCount() const { return faceCount; }
int PolyhedralBoundedSolidTopologySummary::getEdgeCount() const { return edgeCount; }
int PolyhedralBoundedSolidTopologySummary::getVertexCount() const { return vertexCount; }
int PolyhedralBoundedSolidTopologySummary::getShellCount() const { return (int)shells.size(); }
int PolyhedralBoundedSolidTopologySummary::getAdjustedEulerCharacteristic() const { return adjustedEulerCharacteristic; }
bool PolyhedralBoundedSolidTopologySummary::isEveryFaceReachedExactlyOnce() const { return everyFaceReachedExactlyOnce; }
int PolyhedralBoundedSolidTopologySummary::getInvalidEdgeAdjacencyCount() const { return invalidEdgeAdjacencyCount; }

const java::ArrayList<PolyhedralBoundedSolidTopologySummary::Shell>&
PolyhedralBoundedSolidTopologySummary::getShells() const
{
    return shells;
}

bool PolyhedralBoundedSolidTopologySummary::hasUniversalContradiction() const
{
    if ( !everyFaceReachedExactlyOnce || invalidEdgeAdjacencyCount > 0 ) {
        return true;
    }
    for ( size_t i = 0; i < shells.size(); i++ ) {
        if ( !shells[i].isClosedOrientableEulerCompatible() ) {
            return true;
        }
    }
    return false;
}

java::String PolyhedralBoundedSolidTopologySummary::toString() const
{
    java::String perShell = "[";
    for ( size_t i = 0; i < shells.size(); i++ ) {
        if ( i > 0 ) {
            perShell += ", ";
        }
        perShell += shells[i].toString().c_str();
    }
    perShell += "]";
    java::String msg = "TopologySummary{faces=" + java::String::valueOf(faceCount) +
        ", edges=" + java::String::valueOf(edgeCount) +
        ", vertices=" + java::String::valueOf(vertexCount) +
        ", shells=" + java::String::valueOf(shells.size()) +
        ", adjustedEuler=" + java::String::valueOf(adjustedEulerCharacteristic) +
        ", everyFaceReachedExactlyOnce=" +
        booleanName(everyFaceReachedExactlyOnce) +
        ", invalidEdgeAdjacencies=" + java::String::valueOf(invalidEdgeAdjacencyCount) +
        ", perShell=" + perShell + "}";
    return java::String(msg.c_str());
}

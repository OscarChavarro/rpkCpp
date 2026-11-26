#ifndef __POLYHEDRAL_BOUNDED_SOLID_TOPOLOGY_SUMMARY__
#define __POLYHEDRAL_BOUNDED_SOLID_TOPOLOGY_SUMMARY__

#include "java/lang/IllegalArgumentException.h"
#include "java/util/ArrayList.h"

#include "java/lang/String.h"

class PolyhedralBoundedSolid;

/**
Immutable connected-shell and adjusted-Euler summary for a polyhedral B-Rep.

A face with inner boundaries contributes `2 - boundaryLoopCount` instead of
one to the Euler face term. Therefore the reported characteristic is
`V - E + sum(2 - boundaryLoopCount(face))`.

C++ counterpart of Java's `PolyhedralBoundedSolidTopologySummary`.
*/
class PolyhedralBoundedSolidTopologySummary {
public:
    /**
    Topology values for one face-connected shell.
    */
    class Shell {
    private:
        int faceCount;
        int edgeCount;
        int vertexCount;
        int adjustedEulerCharacteristic;
        bool closed;
        bool closedOrientableEulerCompatible;

    public:
        // Default constructible, as the elements of `java::ArrayList`
        Shell() : faceCount(0), edgeCount(0), vertexCount(0),
                  adjustedEulerCharacteristic(0), closed(false),
                  closedOrientableEulerCompatible(false) {}
        Shell(int faceCount, int edgeCount, int vertexCount,
              int adjustedEulerCharacteristic, bool closed);

        int getFaceCount() const;
        int getEdgeCount() const;
        int getVertexCount() const;
        int getAdjustedEulerCharacteristic() const;
        bool isClosed() const;
        bool isClosedOrientableEulerCompatible() const;
        java::String toString() const;
    };

    /**
    @param solid solid to summarize (must not be null; Java throws
    `IllegalArgumentException`, here `java::IllegalArgumentException`)
    */
    static PolyhedralBoundedSolidTopologySummary from(PolyhedralBoundedSolid* solid);

    int getFaceCount() const;
    int getEdgeCount() const;
    int getVertexCount() const;
    int getShellCount() const;
    int getAdjustedEulerCharacteristic() const;
    bool isEveryFaceReachedExactlyOnce() const;
    int getInvalidEdgeAdjacencyCount() const;
    const java::ArrayList<Shell>& getShells() const;
    bool hasUniversalContradiction() const;
    java::String toString() const;

private:
    int faceCount;
    int edgeCount;
    int vertexCount;
    int adjustedEulerCharacteristic;
    bool everyFaceReachedExactlyOnce;
    int invalidEdgeAdjacencyCount;
    java::ArrayList<Shell> shells;

    explicit PolyhedralBoundedSolidTopologySummary(PolyhedralBoundedSolid* solid);
};

#endif

#ifndef __PBS_TOPOLOGY_SUMMARY__
#define __PBS_TOPOLOGY_SUMMARY__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.h"

#include "java/lang/String.h"
#include "java/lang/IllegalArgumentException.h"

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
        BOOLEAN closed;
        BOOLEAN closedOrientableEulerCompatible;

    public:
        /**
        Empty shell, needed by the storage of `ArrayList`
        */
        Shell();

        Shell(int faceCount, int edgeCount, int vertexCount,
              int adjustedEulerCharacteristic, BOOLEAN closed);

        int getFaceCount() const;
        int getEdgeCount() const;
        int getVertexCount() const;
        int getAdjustedEulerCharacteristic() const;
        BOOLEAN isClosed() const;
        BOOLEAN isClosedOrientableEulerCompatible() const;
        String toString() const;
    };

    /**
    @param solid solid to summarize (must not be null; throws
    `IllegalArgumentException`, as Java)
    */
    static PolyhedralBoundedSolidTopologySummary from(PolyhedralBoundedSolid* solid);

    int getFaceCount() const;
    int getEdgeCount() const;
    int getVertexCount() const;
    int getShellCount() const;
    int getAdjustedEulerCharacteristic() const;
    BOOLEAN isEveryFaceReachedExactlyOnce() const;
    int getInvalidEdgeAdjacencyCount() const;
    const ArrayList<Shell>& getShells() const;
    BOOLEAN hasUniversalContradiction() const;
    String toString() const;

private:
    int faceCount;
    int edgeCount;
    int vertexCount;
    int adjustedEulerCharacteristic;
    BOOLEAN everyFaceReachedExactlyOnce;
    int invalidEdgeAdjacencyCount;
    ArrayList<Shell> shells;

    explicit PolyhedralBoundedSolidTopologySummary(PolyhedralBoundedSolid* solid);
};

#endif

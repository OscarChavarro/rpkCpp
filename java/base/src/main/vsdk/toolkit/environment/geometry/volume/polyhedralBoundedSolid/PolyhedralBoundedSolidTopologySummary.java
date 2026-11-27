package vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid;

import java.util.ArrayList;
import java.util.Collections;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidEdge;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidFace;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidHalfEdge;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidVertex;

/**
Immutable connected-shell and adjusted-Euler summary for a polyhedral B-Rep.

<p>A face with inner boundaries contributes {@code 2 - boundaryLoopCount}
instead of one to the Euler face term. Therefore the reported characteristic
is {@code V - E + sum(2 - boundaryLoopCount(face))}.</p>
 */
public final class PolyhedralBoundedSolidTopologySummary
{
    /**
    Topology values for one face-connected shell.
     */
    public static final class Shell
    {
        private final int faceCount;
        private final int edgeCount;
        private final int vertexCount;
        private final int adjustedEulerCharacteristic;
        private final boolean closed;
        private final boolean closedOrientableEulerCompatible;

        private Shell(int faceCount, int edgeCount, int vertexCount,
            int adjustedEulerCharacteristic, boolean closed)
        {
            this.faceCount = faceCount;
            this.edgeCount = edgeCount;
            this.vertexCount = vertexCount;
            this.adjustedEulerCharacteristic =
                adjustedEulerCharacteristic;
            this.closed = closed;
            this.closedOrientableEulerCompatible = closed &&
                adjustedEulerCharacteristic <= 2 &&
                Math.floorMod(adjustedEulerCharacteristic, 2) == 0;
        }

        public int getFaceCount()
        {
            return faceCount;
        }

        public int getEdgeCount()
        {
            return edgeCount;
        }

        public int getVertexCount()
        {
            return vertexCount;
        }

        public int getAdjustedEulerCharacteristic()
        {
            return adjustedEulerCharacteristic;
        }

        public boolean isClosed()
        {
            return closed;
        }

        public boolean isClosedOrientableEulerCompatible()
        {
            return closedOrientableEulerCompatible;
        }

        @Override
        public String toString()
        {
            return "Shell{faces=" + faceCount +
                ", edges=" + edgeCount +
                ", vertices=" + vertexCount +
                ", adjustedEuler=" + adjustedEulerCharacteristic +
                ", closed=" + closed +
                ", closedOrientableEulerCompatible=" +
                closedOrientableEulerCompatible + "}";
        }
    }

    private final int faceCount;
    private final int edgeCount;
    private final int vertexCount;
    private final int adjustedEulerCharacteristic;
    private final boolean everyFaceReachedExactlyOnce;
    private final int invalidEdgeAdjacencyCount;
    private final List<Shell> shells;

    private PolyhedralBoundedSolidTopologySummary(
        PolyhedralBoundedSolid solid)
    {
        faceCount = solid.getPolygonsList().size();
        edgeCount = solid.getEdgesList().size();
        vertexCount = solid.getVerticesList().size();

        IdentityHashMap<_PolyhedralBoundedSolidFace, Integer> faceIndexes =
            new IdentityHashMap<_PolyhedralBoundedSolidFace, Integer>();
        int i;
        for ( i = 0; i < faceCount; i++ ) {
            faceIndexes.put(solid.getPolygonsList().get(i),
                Integer.valueOf(i));
        }

        DisjointSet components = new DisjointSet(faceCount);
        int invalidAdjacencies = 0;
        for ( i = 0; i < edgeCount; i++ ) {
            _PolyhedralBoundedSolidEdge edge = solid.getEdgesList().get(i);
            _PolyhedralBoundedSolidFace leftFace = faceOf(edge.leftHalf);
            _PolyhedralBoundedSolidFace rightFace = faceOf(edge.rightHalf);
            Integer leftIndex = faceIndexes.get(leftFace);
            Integer rightIndex = faceIndexes.get(rightFace);
            if ( leftIndex == null || rightIndex == null ) {
                invalidAdjacencies++;
                continue;
            }
            components.union(leftIndex.intValue(), rightIndex.intValue());
        }
        invalidEdgeAdjacencyCount = invalidAdjacencies;

        Map<Integer, List<_PolyhedralBoundedSolidFace>> componentFaces =
            new java.util.TreeMap<Integer,
                List<_PolyhedralBoundedSolidFace>>();
        int reachedFaces = 0;
        for ( i = 0; i < faceCount; i++ ) {
            int root = components.find(i);
            List<_PolyhedralBoundedSolidFace> faces =
                componentFaces.get(Integer.valueOf(root));
            if ( faces == null ) {
                faces = new ArrayList<_PolyhedralBoundedSolidFace>();
                componentFaces.put(Integer.valueOf(root), faces);
            }
            faces.add(solid.getPolygonsList().get(i));
            reachedFaces++;
        }
        everyFaceReachedExactlyOnce =
            faceIndexes.size() == faceCount &&
            reachedFaces == faceCount && componentFaces.values().stream()
                .mapToInt(List::size).sum() == faceCount;

        ArrayList<Shell> computedShells = new ArrayList<Shell>();
        int totalAdjustedFaceTerm = 0;
        for ( List<_PolyhedralBoundedSolidFace> faces :
              componentFaces.values() ) {
            Set<_PolyhedralBoundedSolidFace> faceSet =
                Collections.newSetFromMap(
                    new IdentityHashMap<_PolyhedralBoundedSolidFace,
                        Boolean>());
            faceSet.addAll(faces);
            Set<_PolyhedralBoundedSolidEdge> shellEdges =
                Collections.newSetFromMap(
                    new IdentityHashMap<_PolyhedralBoundedSolidEdge,
                        Boolean>());
            Set<_PolyhedralBoundedSolidVertex> shellVertices =
                Collections.newSetFromMap(
                    new IdentityHashMap<_PolyhedralBoundedSolidVertex,
                        Boolean>());
            int adjustedFaceTerm = 0;
            boolean closed = true;

            for ( _PolyhedralBoundedSolidFace face : faces ) {
                adjustedFaceTerm += 2 - face.boundariesList.size();
                collectFaceVertices(face, shellVertices);
            }
            totalAdjustedFaceTerm += adjustedFaceTerm;

            for ( i = 0; i < solid.getEdgesList().size(); i++ ) {
                _PolyhedralBoundedSolidEdge edge =
                    solid.getEdgesList().get(i);
                _PolyhedralBoundedSolidFace leftFace =
                    faceOf(edge.leftHalf);
                _PolyhedralBoundedSolidFace rightFace =
                    faceOf(edge.rightHalf);
                boolean touchesShell = faceSet.contains(leftFace) ||
                    faceSet.contains(rightFace);
                if ( !touchesShell ) {
                    continue;
                }
                shellEdges.add(edge);
                if ( leftFace == null || rightFace == null ||
                     !faceSet.contains(leftFace) ||
                     !faceSet.contains(rightFace) ) {
                    closed = false;
                }
                collectVertex(edge.leftHalf, shellVertices);
                collectVertex(edge.rightHalf, shellVertices);
            }

            int chi = shellVertices.size() - shellEdges.size() +
                adjustedFaceTerm;
            computedShells.add(new Shell(faces.size(), shellEdges.size(),
                shellVertices.size(), chi, closed));
        }
        shells = Collections.unmodifiableList(computedShells);
        adjustedEulerCharacteristic =
            vertexCount - edgeCount + totalAdjustedFaceTerm;
    }

    public static PolyhedralBoundedSolidTopologySummary from(
        PolyhedralBoundedSolid solid)
    {
        if ( solid == null ) {
            throw new IllegalArgumentException("solid must not be null");
        }
        return new PolyhedralBoundedSolidTopologySummary(solid);
    }

    public int getFaceCount()
    {
        return faceCount;
    }

    public int getEdgeCount()
    {
        return edgeCount;
    }

    public int getVertexCount()
    {
        return vertexCount;
    }

    public int getShellCount()
    {
        return shells.size();
    }

    public int getAdjustedEulerCharacteristic()
    {
        return adjustedEulerCharacteristic;
    }

    public boolean isEveryFaceReachedExactlyOnce()
    {
        return everyFaceReachedExactlyOnce;
    }

    public int getInvalidEdgeAdjacencyCount()
    {
        return invalidEdgeAdjacencyCount;
    }

    public List<Shell> getShells()
    {
        return shells;
    }

    public boolean hasUniversalContradiction()
    {
        if ( !everyFaceReachedExactlyOnce ||
             invalidEdgeAdjacencyCount > 0 ) {
            return true;
        }
        for ( Shell shell : shells ) {
            if ( !shell.isClosedOrientableEulerCompatible() ) {
                return true;
            }
        }
        return false;
    }

    @Override
    public String toString()
    {
        return "TopologySummary{faces=" + faceCount +
            ", edges=" + edgeCount +
            ", vertices=" + vertexCount +
            ", shells=" + shells.size() +
            ", adjustedEuler=" + adjustedEulerCharacteristic +
            ", everyFaceReachedExactlyOnce=" +
            everyFaceReachedExactlyOnce +
            ", invalidEdgeAdjacencies=" + invalidEdgeAdjacencyCount +
            ", perShell=" + shells + "}";
    }

    private static _PolyhedralBoundedSolidFace faceOf(
        _PolyhedralBoundedSolidHalfEdge halfEdge)
    {
        if ( halfEdge == null || halfEdge.parentLoop == null ) {
            return null;
        }
        return halfEdge.parentLoop.parentFace;
    }

    private static void collectVertex(
        _PolyhedralBoundedSolidHalfEdge halfEdge,
        Set<_PolyhedralBoundedSolidVertex> vertices)
    {
        if ( halfEdge != null && halfEdge.startingVertex != null ) {
            vertices.add(halfEdge.startingVertex);
        }
    }

    private static void collectFaceVertices(
        _PolyhedralBoundedSolidFace face,
        Set<_PolyhedralBoundedSolidVertex> vertices)
    {
        int i;
        int j;
        for ( i = 0; i < face.boundariesList.size(); i++ ) {
            for ( j = 0;
                  j < face.boundariesList.get(i).halfEdgesList.size();
                  j++ ) {
                collectVertex(
                    face.boundariesList.get(i).halfEdgesList.get(j),
                    vertices);
            }
        }
    }

    private static final class DisjointSet
    {
        private final int[] parent;
        private final byte[] rank;

        private DisjointSet(int size)
        {
            parent = new int[size];
            rank = new byte[size];
            int i;
            for ( i = 0; i < size; i++ ) {
                parent[i] = i;
            }
        }

        private int find(int value)
        {
            if ( parent[value] != value ) {
                parent[value] = find(parent[value]);
            }
            return parent[value];
        }

        private void union(int first, int second)
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
    }
}

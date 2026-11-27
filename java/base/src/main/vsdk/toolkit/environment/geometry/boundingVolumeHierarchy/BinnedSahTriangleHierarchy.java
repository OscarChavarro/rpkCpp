//= References:                                                             =
//= [WALD2007] Wald, Ingo. "On fast Construction of SAH-based Bounding      =
//=     Volume Hierarchies". Proceedings of the 2007 IEEE Symposium on      =
//=     Interactive Ray Tracing, 2007, pp. 33-40.                           =

package vsdk.toolkit.environment.geometry.boundingVolumeHierarchy;

import java.util.ArrayList;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.statistics.RaytraceStatistics;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.environment.geometry.element.Triangle;

/**
`TriangleMeshAccelerationMethod.WALD2007_BINNED_SAH` for the triangles of
one mesh (the bottom level): a `BinnedSahHierarchy` ([WALD2007].3) over the
padded triangle boxes.

Exactness: the leaves keep a copy of their triangle vertices in leaf order
(as the triangle buffer of the shader, for memory locality), the leaf test
is `Triangle.intersectRay` over the same values, a node is skipped only
when its entry is strictly beyond the closest hit, and ties go to the
lowest triangle index: the result is the brute force one.
*/
public class BinnedSahTriangleHierarchy extends TriangleAccelerationStructure {
    private final BinnedSahHierarchy hierarchy;
    private final ArrayList<Double> leafVertices;

    public BinnedSahTriangleHierarchy()
    {
        hierarchy = new BinnedSahHierarchy();
        leafVertices = new ArrayList<>();
    }

    /** Deep copy (cpp implicit copy constructor) */
    public BinnedSahTriangleHierarchy(BinnedSahTriangleHierarchy other)
    {
        super(other);
        hierarchy = new BinnedSahHierarchy(other.hierarchy);
        leafVertices = new ArrayList<>(other.leafVertices);
    }

    @Override
    public TriangleMeshAccelerationMethod getMethod()
    {
        return TriangleMeshAccelerationMethod.WALD2007_BINNED_SAH;
    }

    @Override
    public void build(double[] positions, int[] indexes)
    {
        final int primitiveCount = indexes != null ? indexes.length / 3 : 0;
        recordBuiltMesh(positions, indexes);

        // [WALD2007].3.2 Initial setup: the box of every triangle, computed once.
        ArrayList<Double> boxes = new ArrayList<>(6 * (primitiveCount > 0 ? primitiveCount : 1));
        double[] lo = new double[3];
        double[] hi = new double[3];
        for ( int i = 0; i < primitiveCount; i++ ) {
            triangleBox(positions, indexes, i, lo, hi);
            for ( int a = 0; a < 3; a++ ) {
                boxes.add(lo[a]);
            }
            for ( int a = 0; a < 3; a++ ) {
                boxes.add(hi[a]);
            }
        }
        hierarchy.build(boxes);

        // Triangle vertices in leaf order, as the triangle buffer of the shader.
        final ArrayList<Integer> order = hierarchy.getPrimitiveOrder();
        leafVertices.clear();
        leafVertices.ensureCapacity(9 * (primitiveCount > 0 ? primitiveCount : 1));
        for ( int i = 0; i < primitiveCount; i++ ) {
            final int triangle = order.get(i);
            for ( int k = 0; k < 3; k++ ) {
                final int vertex = indexes[triangle * 3 + k];
                leafVertices.add(positions[vertex * 3]);
                leafVertices.add(positions[vertex * 3 + 1]);
                leafVertices.add(positions[vertex * 3 + 2]);
            }
        }
    }

    @Override
    public boolean firstHit(Ray ray, double[] positions, int[] indexes,
        double[] outT, double[] outU, double[] outV, int[] outTriangle)
    {
        final ArrayList<BinnedSahHierarchyNode> nodes = hierarchy.getNodes();
        if ( builtTriangleCount <= 0 || nodes.size() == 0 ) {
            return false;
        }
        final BinnedSahHierarchy.RaySlab slab = new BinnedSahHierarchy.RaySlab(ray);
        final ArrayList<Integer> orderArray = hierarchy.getPrimitiveOrder();

        // Same initial bound and tie rule as the brute force loop.
        double bestT = 1e308;
        double bestU = 0;
        double bestV = 0;
        int best = -1;

        final double rootDistance = slab.entry(nodes.get(0), bestT);
        if ( rootDistance < 0.0 ) {
            RaytraceStatistics.recordPrimitiveLevelTraversal(0, 0);
            return false;
        }
        // Counted locally, reported once: no shared write per node.
        long nodeVisits = 0;
        long triangleTests = 0;
        // Traversal of casTraverseBlas (computeAccelerationStructure.glsl),
        // specialized for triangles: no virtual call per leaf entry.
        int[] nodeStack = new int[BinnedSahHierarchy.STACK_SIZE];
        double[] distanceStack = new double[BinnedSahHierarchy.STACK_SIZE];
        int sp = 0;
        nodeStack[sp] = 0;
        distanceStack[sp] = rootDistance;
        sp++;

        double[] t = new double[1];
        double[] u = new double[1];
        double[] v = new double[1];
        while ( sp > 0 ) {
            sp--;
            // Strictly farther than the closest hit: nothing there can win, not
            // even a tie (which needs t == bestT).
            if ( distanceStack[sp] > bestT ) {
                continue;
            }
            final BinnedSahHierarchyNode node = nodes.get(nodeStack[sp]);
            nodeVisits++;
            if ( node.count > 0 ) {
                triangleTests += node.count;
                for ( int i = node.leftOrFirst; i < node.leftOrFirst + node.count; i++ ) {
                    final int p = 9 * i;
                    final Vector3Dd v0 = new Vector3Dd(leafVertices.get(p), leafVertices.get(p + 1), leafVertices.get(p + 2));
                    final Vector3Dd v1 = new Vector3Dd(leafVertices.get(p + 3), leafVertices.get(p + 4), leafVertices.get(p + 5));
                    final Vector3Dd v2 = new Vector3Dd(leafVertices.get(p + 6), leafVertices.get(p + 7), leafVertices.get(p + 8));
                    if ( !Triangle.intersectRay(ray, v0, v1, v2, t, u, v) ) {
                        continue;
                    }
                    final int triangle = orderArray.get(i);
                    if ( t[0] < bestT || (t[0] == bestT && best >= 0 && triangle < best) ) {
                        bestT = t[0];
                        bestU = u[0];
                        bestV = v[0];
                        best = triangle;
                    }
                }
                continue;
            }
            int left = node.leftOrFirst;
            int right = left + 1;
            double dl = slab.entry(nodes.get(left), bestT);
            double dr = slab.entry(nodes.get(right), bestT);
            // Push the farther child first, so the nearer one is visited first.
            if ( dl >= 0.0 && dr >= 0.0 && dl < dr ) {
                final int swapNode = left;
                left = right;
                right = swapNode;
                final double swapDistance = dl;
                dl = dr;
                dr = swapDistance;
            }
            if ( sp + 2 > BinnedSahHierarchy.STACK_SIZE ) {
                // Unreachable with MAXIMUM_DEPTH < STACK_SIZE; never give a
                // wrong answer if it happens.
                RaytraceStatistics.recordPrimitiveLevelTraversal(nodeVisits, triangleTests);
                return firstHitBruteForce(ray, positions, indexes, outT, outU, outV, outTriangle);
            }
            if ( dl >= 0.0 ) {
                nodeStack[sp] = left;
                distanceStack[sp] = dl;
                sp++;
            }
            if ( dr >= 0.0 ) {
                nodeStack[sp] = right;
                distanceStack[sp] = dr;
                sp++;
            }
        }
        RaytraceStatistics.recordPrimitiveLevelTraversal(nodeVisits, triangleTests);
        if ( best < 0 ) {
            return false;
        }
        outT[0] = bestT;
        outU[0] = bestU;
        outV[0] = bestV;
        outTriangle[0] = best;
        return true;
    }

    @Override
    public TriangleAccelerationStructure copy()
    {
        return new BinnedSahTriangleHierarchy(this);
    }

    @Override
    public long getNodeCount()
    {
        return hierarchy.getNodeCount();
    }

    @Override
    public int getDepth()
    {
        return hierarchy.getDepth();
    }

    public BinnedSahHierarchy getHierarchy()
    {
        return hierarchy;
    }
}

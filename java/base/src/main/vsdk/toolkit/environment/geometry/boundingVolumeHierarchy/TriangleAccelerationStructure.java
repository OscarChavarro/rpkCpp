package vsdk.toolkit.environment.geometry.boundingVolumeHierarchy;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.statistics.RaytraceStatistics;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.environment.geometry.element.Triangle;

/**
Ray acceleration structure over the triangles of an indexed triangle mesh
(`TriangleMesh` arrays: three coordinates per vertex, three vertex indexes
per triangle). Every implementation answers exactly what the brute force
loop answers (`firstHitBruteForce`): the closest hit of
`Triangle.intersectRay`, ties resolved by the lowest triangle index, with
the same `t`, `u`, `v` values.

Built once, single threaded, before tracing; queries are read only and safe
from several threads. The structure is a snapshot of the mesh: it must be
rebuilt when the vertices or triangles change (`TriangleMesh` releases it
from its own mutators and checks the counts before using it).

Output parameters (`outT`, `outU`, `outV`, `outTriangle`, `t`, `u`, `v`) are
one-element holders, as in `Triangle.intersectRay`.
*/
public abstract class TriangleAccelerationStructure {
    // Relative and absolute padding of the triangle boxes: Moller-Trumbore can
    // accept points a few ulps outside the exact triangle, the slab test must
    // not reject them.
    private static final double RELATIVE_BOX_PADDING = 1e-9;
    private static final double ABSOLUTE_BOX_PADDING = 1e-12;

    protected int builtVertexCount;
    protected int builtTriangleCount;
    protected final double[] vertexBounds = new double[6];

    protected TriangleAccelerationStructure()
    {
        builtVertexCount = -1;
        builtTriangleCount = -1;
        for ( int a = 0; a < 3; a++ ) {
            vertexBounds[a] = 1e308;
            vertexBounds[a + 3] = -1e308;
        }
    }

    /** Copy of the base state, for the subclasses' `copy` (cpp implicit copy
    constructor) */
    protected TriangleAccelerationStructure(TriangleAccelerationStructure other)
    {
        builtVertexCount = other.builtVertexCount;
        builtTriangleCount = other.builtTriangleCount;
        for ( int i = 0; i < 6; i++ ) {
            vertexBounds[i] = other.vertexBounds[i];
        }
    }

    /** Stores the sizes and the vertex bounds; every `build` calls it */
    protected void recordBuiltMesh(double[] positions, int[] indexes)
    {
        builtVertexCount = positions != null ? positions.length / 3 : 0;
        builtTriangleCount = indexes != null ? indexes.length / 3 : 0;
        // Same operations and order as TriangleMesh.calculateMinMaxPositions,
        // so the cached bounds are bit identical to getMinMax().
        double minX = Double.MAX_VALUE;
        double minY = Double.MAX_VALUE;
        double minZ = Double.MAX_VALUE;
        double maxX = -Double.MAX_VALUE;
        double maxY = -Double.MAX_VALUE;
        double maxZ = -Double.MAX_VALUE;
        for ( int i = 0; i < builtVertexCount; i++ ) {
            final double x = positions[3*i+0];
            final double y = positions[3*i+1];
            final double z = positions[3*i+2];
            if ( x < minX ) minX = x;
            if ( y < minY ) minY = y;
            if ( z < minZ ) minZ = z;
            if ( x > maxX ) maxX = x;
            if ( y > maxY ) maxY = y;
            if ( z > maxZ ) maxZ = z;
        }
        vertexBounds[0] = minX; vertexBounds[1] = minY; vertexBounds[2] = minZ;
        vertexBounds[3] = maxX; vertexBounds[4] = maxY; vertexBounds[5] = maxZ;
    }

    public abstract TriangleMeshAccelerationMethod getMethod();

    public abstract void build(double[] positions, int[] indexes);

    /**
    Closest hit of the ray against the triangles given to `build` (the same
    arrays must be passed). Returns false on a miss.
    */
    public abstract boolean firstHit(Ray ray, double[] positions, int[] indexes,
        double[] outT, double[] outU, double[] outV, int[] outTriangle);

    /** New copy */
    public abstract TriangleAccelerationStructure copy();

    /** Number of nodes, for reports */
    public abstract long getNodeCount();

    /** Depth of the hierarchy (a single leaf has depth 1), for reports */
    public abstract int getDepth();

    /** True when built for meshes of these sizes */
    public boolean matches(int vertexCount, int triangleCount)
    {
        return builtVertexCount == vertexCount && builtTriangleCount == triangleCount;
    }

    /**
    Minimum and maximum vertex coordinates when built (x, y, z, X, Y, Z),
    computed exactly as `TriangleMesh.getMinMax`, which walks every vertex
    on each call: callers testing a ray against the mesh bounds use this
    copy instead.
    */
    public void getVertexBounds(double[] outMinMax)
    {
        for ( int i = 0; i < 6; i++ ) {
            outMinMax[i] = vertexBounds[i];
        }
    }

    private static double absoluteValue(double x)
    {
        return x < 0 ? -x : x;
    }

    /** Enlarges `[lo, hi]` a little, so that rounding never hides a hit of
    `Triangle.intersectRay` inside it */
    public static void padBox(double[] lo, double[] hi)
    {
        for ( int a = 0; a < 3; a++ ) {
            final double magnitude = absoluteValue(lo[a]) > absoluteValue(hi[a]) ?
                absoluteValue(lo[a]) : absoluteValue(hi[a]);
            final double pad = magnitude * RELATIVE_BOX_PADDING + ABSOLUTE_BOX_PADDING;
            lo[a] -= pad;
            hi[a] += pad;
        }
    }

    /** Box of one triangle, padded with `padBox` */
    public static void triangleBox(double[] positions, int[] indexes,
        int triangle, double[] lo, double[] hi)
    {
        for ( int a = 0; a < 3; a++ ) {
            lo[a] = 1e30;
            hi[a] = -1e30;
        }
        for ( int k = 0; k < 3; k++ ) {
            final int vertex = indexes[triangle * 3 + k];
            for ( int a = 0; a < 3; a++ ) {
                final double c = positions[vertex * 3 + a];
                lo[a] = c < lo[a] ? c : lo[a];
                hi[a] = c > hi[a] ? c : hi[a];
            }
        }
        padBox(lo, hi);
    }

    /** `Triangle.intersectRay` over one triangle of the arrays */
    public static boolean testTriangle(Ray ray, double[] positions,
        int[] indexes, int triangle, double[] t, double[] u, double[] v)
    {
        final int i0 = indexes[triangle * 3];
        final int i1 = indexes[triangle * 3 + 1];
        final int i2 = indexes[triangle * 3 + 2];
        final Vector3Dd v0 = new Vector3Dd(positions[i0 * 3], positions[i0 * 3 + 1], positions[i0 * 3 + 2]);
        final Vector3Dd v1 = new Vector3Dd(positions[i1 * 3], positions[i1 * 3 + 1], positions[i1 * 3 + 2]);
        final Vector3Dd v2 = new Vector3Dd(positions[i2 * 3], positions[i2 * 3 + 1], positions[i2 * 3 + 2]);
        return Triangle.intersectRay(ray, v0, v1, v2, t, u, v);
    }

    /** The reference result: every triangle is tested */
    public static boolean firstHitBruteForce(Ray ray, double[] positions,
        int[] indexes, double[] outT, double[] outU, double[] outV,
        int[] outTriangle)
    {
        final int triangleCount = indexes != null ? indexes.length / 3 : 0;
        // No nodes: every triangle is a primitive test.
        RaytraceStatistics.recordPrimitiveLevelTraversal(0, triangleCount);
        double bestT = 1e308;
        double bestU = 0;
        double bestV = 0;
        int best = -1;
        double[] t = new double[1];
        double[] u = new double[1];
        double[] v = new double[1];
        for ( int i = 0; i < triangleCount; i++ ) {
            if ( testTriangle(ray, positions, indexes, i, t, u, v) && t[0] < bestT ) {
                bestT = t[0];
                bestU = u[0];
                bestV = v[0];
                best = i;
            }
        }
        if ( best < 0 ) {
            return false;
        }
        outT[0] = bestT;
        outU[0] = bestU;
        outV[0] = bestV;
        outTriangle[0] = best;
        return true;
    }

    /** New, unbuilt structure for the method; null for `BRUTE_FORCE` */
    public static TriangleAccelerationStructure create(TriangleMeshAccelerationMethod method)
    {
        switch ( method ) {
          case WALD2007_BINNED_SAH:
            return new BinnedSahTriangleHierarchy();
          case VOXEL_GRID:
            return new GridTriangleHierarchy();
          case BRUTE_FORCE:
            return null;
        }
        return null;
    }
}

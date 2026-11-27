//= References:                                                             =
//= [SNYD1987] Snyder, John M. and Barr, Alan H. "Ray Tracing Complex       =
//=     Models Containing Surface Tessellations". SIGGRAPH '87, Computer    =
//=     Graphics 21(4), 1987, pp. 119-128.                                  =
//=     PDF: doc/references/[SNYD1987]_*.pdf                                =

package vsdk.toolkit.environment.geometry.boundingVolumeHierarchy;

import java.util.ArrayList;

import vsdk.toolkit.common.statistics.RaytraceStatistics;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.environment.geometry.spatialIndex.GridSpatialIndex;
import vsdk.toolkit.environment.geometry.spatialIndex.PrimitiveRayTest;
import vsdk.toolkit.environment.geometry.spatialIndex.SpatialGroupTree;
import vsdk.toolkit.environment.geometry.spatialIndex.SpatialIndexWorkspace;
import vsdk.toolkit.environment.geometry.spatialIndex.SpatialQueryMode;
import vsdk.toolkit.environment.geometry.spatialIndex.SpatialQueryResult;

/**
`TriangleMeshAccelerationMethod.VOXEL_GRID` for the triangles of one mesh:
a `GridSpatialIndex` ([SNYD1987], RenderPark's voxel grid) over the padded
triangle boxes, all of them members of one group, so the mesh gets one
uniform grid of floor(n^(1/3) + 1) cells along each axis.

Exactness: the leaf test is `Triangle.intersectRay` over the mesh arrays,
as the brute force loop; a hit found in an earlier cell may lie beyond it
([SNYD1987] Figure 11),
but the walk goes on until it enters a cell beyond the closest hit, and the
cell holding the closest hit point lists every triangle through that point,
so the closest hit and its ties are always tested. Ties go to the lowest
triangle index: the result is the brute force one.

The query uses a workspace without mailbox, so `firstHit` shares nothing
and is safe from several threads; a triangle listed in several cells the
ray crosses is tested again in each (the repeated test gives the same hit,
which the tie rule does not take twice).
*/
public class GridTriangleHierarchy extends TriangleAccelerationStructure {
    /** Padded triangle boxes, six values per triangle, kept for `copy` */
    private final ArrayList<Double> triangleBoxes;
    private GridSpatialIndex index;

    /**
    The exact test of the grid query: `Triangle.intersectRay` on one triangle,
    accepted when strictly closer than the hit so far, or at the same parameter
    with a lower triangle index (the brute force tie rule). Keeps the face
    coordinates of the accepted hit.
    */
    private static final class GridTriangleRayTest implements PrimitiveRayTest {
        private final Ray ray;
        private final double[] positions;
        private final int[] indexes;
        private final double[] rayParameter = new double[1];
        private final double[] u = new double[1];
        private final double[] v = new double[1];

        int best;
        double bestU;
        double bestV;

        GridTriangleRayTest(Ray inRay, double[] inPositions, int[] inIndexes)
        {
            ray = inRay;
            positions = inPositions;
            indexes = inIndexes;
            best = -1;
            bestU = 0;
            bestV = 0;
        }

        @Override
        public boolean test(int primitive, double minimumT, double maximumT, double[] hitT)
        {
            if ( !TriangleAccelerationStructure.testTriangle(ray, positions, indexes, primitive, rayParameter, u, v) ) {
                return false;
            }
            if ( rayParameter[0] < minimumT || rayParameter[0] > maximumT ) {
                return false;
            }
            if ( rayParameter[0] == maximumT && !(best >= 0 && primitive < best) ) {
                return false;
            }
            best = primitive;
            bestU = u[0];
            bestV = v[0];
            hitT[0] = rayParameter[0];
            return true;
        }
    }

    public GridTriangleHierarchy()
    {
        triangleBoxes = new ArrayList<>();
        index = null;
    }

    /** Copy: the grid is built again from the kept boxes (cpp copy
    constructor) */
    public GridTriangleHierarchy(GridTriangleHierarchy other)
    {
        super(other);
        triangleBoxes = new ArrayList<>(other.triangleBoxes);
        index = null;
        buildIndex();
    }

    @Override
    public TriangleMeshAccelerationMethod getMethod()
    {
        return TriangleMeshAccelerationMethod.VOXEL_GRID;
    }

    private void buildIndex()
    {
        index = null;
        final int triangleCount = triangleBoxes.size() / 6;
        if ( triangleCount == 0 ) {
            return;
        }
        double[] rootBox = {1e308, 1e308, 1e308, -1e308, -1e308, -1e308};
        for ( int i = 0; i < triangleCount; i++ ) {
            for ( int a = 0; a < 3; a++ ) {
                final double lo = triangleBoxes.get(6 * i + a);
                final double hi = triangleBoxes.get(6 * i + 3 + a);
                rootBox[a] = lo < rootBox[a] ? lo : rootBox[a];
                rootBox[a + 3] = hi > rootBox[a + 3] ? hi : rootBox[a + 3];
            }
        }
        SpatialGroupTree tree = new SpatialGroupTree(triangleBoxes, rootBox);
        for ( int i = 0; i < triangleCount; i++ ) {
            tree.addPrimitive(0, i);
        }
        index = new GridSpatialIndex(tree);
    }

    @Override
    public void build(double[] positions, int[] indexes)
    {
        recordBuiltMesh(positions, indexes);
        final int triangleCount = indexes != null ? indexes.length / 3 : 0;
        triangleBoxes.clear();
        double[] lo = new double[3];
        double[] hi = new double[3];
        for ( int i = 0; i < triangleCount; i++ ) {
            triangleBox(positions, indexes, i, lo, hi);
            for ( int a = 0; a < 3; a++ ) {
                triangleBoxes.add(lo[a]);
            }
            for ( int a = 0; a < 3; a++ ) {
                triangleBoxes.add(hi[a]);
            }
        }
        buildIndex();
    }

    @Override
    public boolean firstHit(Ray ray, double[] positions, int[] indexes,
        double[] outT, double[] outU, double[] outV, int[] outTriangle)
    {
        if ( builtTriangleCount <= 0 || index == null ) {
            return false;
        }
        GridTriangleRayTest test = new GridTriangleRayTest(ray, positions, indexes);
        // No mailbox: nothing to allocate per query (see the class comment).
        SpatialIndexWorkspace workspace = new SpatialIndexWorkspace(0);
        // Same initial bound as the brute force loop; Triangle.intersectRay
        // accepts only parameters above VSDK.EPSILON, so 0 loses no hit.
        // queryWith: the generic form of the query, as cpp's template
        final SpatialQueryResult result = index.queryWith(ray, 0.0, 1e308,
            SpatialQueryMode.CLOSEST_HIT, test, workspace);
        RaytraceStatistics.recordPrimitiveLevelTraversal(workspace.getNodeVisits(),
            workspace.getPrimitiveTests());
        if ( !result.hasHit() ) {
            return false;
        }
        outT[0] = result.getParameter();
        outU[0] = test.bestU;
        outV[0] = test.bestV;
        outTriangle[0] = result.getPrimitive();
        return true;
    }

    @Override
    public TriangleAccelerationStructure copy()
    {
        return new GridTriangleHierarchy(this);
    }

    /** Number of grids (one for a mesh) */
    @Override
    public long getNodeCount()
    {
        return index == null ? 0 : index.getNumberOfGrids();
    }

    @Override
    public int getDepth()
    {
        return index == null ? 0 : 1;
    }

    /** Cells along each axis of the grid; 0 when empty */
    public int getResolution()
    {
        return index == null ? 0 : index.getResolution(0);
    }
}

package vsdk.toolkit.environment.geometry.surface;
import java.io.Serial;

import java.util.ArrayList;

import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.element.RayHit;
import vsdk.toolkit.environment.geometry.boundingVolumeHierarchy.BinnedSahHierarchy;
import vsdk.toolkit.environment.geometry.boundingVolumeHierarchy.TriangleAccelerationStructure;
import vsdk.toolkit.environment.geometry.boundingVolumeHierarchy.TriangleMeshAccelerationMethod;

public class TriangleMeshGroup extends Surface {
    @Serial private static final long serialVersionUID = 20060502L;

    private ArrayList<TriangleMesh> meshes;

    private final ThreadLocal<int[]> intersectionInformation =
        ThreadLocal.withInitial(() -> new int[] {-1, -1});

    // Top level ray acceleration over the meshes (the TLAS role of the
    // Vulkan raytracer), see prepareAccelerationStructures. Valid while
    // preparedMeshCount == meshes.size(); -1 when there is none.
    private BinnedSahHierarchy meshHierarchy;
    private long preparedMeshCount;
    private final double[] preparedBounds = new double[6];

    public TriangleMeshGroup() {
        meshes = new ArrayList<TriangleMesh> ();
        meshHierarchy = new BinnedSahHierarchy();
        preparedMeshCount = -1;
    }

    public TriangleMeshGroup(ArrayList<TriangleMesh> meshes) {
        this.meshes = meshes;
        meshHierarchy = new BinnedSahHierarchy();
        preparedMeshCount = -1;
    }

    public TriangleMeshGroup(TriangleMeshGroup group) {
        this.meshes = group.getMeshes();
        meshHierarchy = new BinnedSahHierarchy(group.meshHierarchy);
        preparedMeshCount = group.preparedMeshCount;
        for ( int i = 0; i < 6; i++ ) preparedBounds[i] = group.preparedBounds[i];
    }

    public ArrayList<TriangleMesh> getMeshes() {
        return this.meshes;
    }

    /**
    `TriangleMesh.prepareAccelerationStructure` on every mesh and, unless
    the method is `BRUTE_FORCE`, a `BinnedSahHierarchy` ([WALD2007]) over the
    mesh boxes, so a ray visits the meshes nearest first instead of all of
    them. Single threaded and idempotent: a second call with the same method
    and unchanged meshes only reads. The hit does not depend on the method.
    `setMeshes` / `addMesh` drop the top level; changes made to the meshes
    through `getMeshes` / `getMeshAt` require calling this method again.
    @param method acceleration method to use
    */
    public void prepareAccelerationStructures(TriangleMeshAccelerationMethod method)
    {
        boolean changed = false;
        for ( int i = 0; i < meshes.size(); i++ ) {
            final TriangleAccelerationStructure before = meshes.get(i).getAccelerationStructure();
            meshes.get(i).prepareAccelerationStructure(method);
            if ( meshes.get(i).getAccelerationStructure() != before ) changed = true;
        }
        if ( method == TriangleMeshAccelerationMethod.BRUTE_FORCE ) {
            if ( preparedMeshCount != -1 ) preparedMeshCount = -1;
            return;
        }
        // Nothing written when already prepared: safe from tracing threads.
        if ( !changed && preparedMeshCount == meshes.size() ) return;
        buildMeshHierarchy();
    }

    private void buildMeshHierarchy()
    {
        preparedMeshCount = -1;
        double[] bounds = new double[6];
        if ( !getPreparedMinMax(bounds) ) return;  // some mesh has no structure
        // [WALD2007] top level: one padded box per mesh, from its cached bounds.
        ArrayList<Double> boxes = new ArrayList<>(6 * (meshes.size() > 0 ? meshes.size() : 1));
        for ( int i = 0; i < meshes.size(); i++ ) {
            double[] mm = new double[6];
            meshes.get(i).getPreparedMinMax(mm);
            double[] lo = {mm[0], mm[1], mm[2]};
            double[] hi = {mm[3], mm[4], mm[5]};
            TriangleAccelerationStructure.padBox(lo, hi);
            for ( int k = 0; k < 3; k++ ) boxes.add(lo[k]);
            for ( int k = 0; k < 3; k++ ) boxes.add(hi[k]);
        }
        meshHierarchy.build(boxes);
        for ( int k = 0; k < 6; k++ ) preparedBounds[k] = bounds[k];
        preparedMeshCount = meshes.size();
    }

    /**
    @return top level hierarchy over the meshes, or null when not prepared
    */
    public BinnedSahHierarchy getMeshHierarchy()
    {
        return preparedMeshCount == meshes.size() ? meshHierarchy : null;
    }

    /**
    `getMinMax` from the prepared structures, when every mesh has one.
    @param outMinMax six values array to fill
    @return true when `outMinMax` was written
    */
    public boolean getPreparedMinMax(double[] outMinMax)
    {
        if ( preparedMeshCount >= 0 && preparedMeshCount == meshes.size() ) {
            for ( int k = 0; k < 6; k++ ) outMinMax[k] = preparedBounds[k];
            return true;
        }
        // Same combination as computeMinMaxPositions, from cached mesh bounds.
        if ( meshes.size() == 0 ) return false;
        double minX = 0, minY = 0, minZ = 0, maxX = 0, maxY = 0, maxZ = 0;
        double[] mm = new double[6];
        for ( int i = 0; i < meshes.size(); i++ ) {
            if ( !meshes.get(i).getPreparedMinMax(mm) ) return false;
            if ( i == 0 ) {
                minX = mm[0]; minY = mm[1]; minZ = mm[2];
                maxX = mm[3]; maxY = mm[4]; maxZ = mm[5];
            }
            if ( mm[0] < minX ) minX = mm[0];
            if ( mm[1] < minY ) minY = mm[1];
            if ( mm[2] < minZ ) minZ = mm[2];
            if ( mm[3] > maxX ) maxX = mm[3];
            if ( mm[4] > maxY ) maxY = mm[4];
            if ( mm[5] > maxZ ) maxZ = mm[5];
        }
        outMinMax[0] = minX; outMinMax[1] = minY; outMinMax[2] = minZ;
        outMinMax[3] = maxX; outMinMax[4] = maxY; outMinMax[5] = maxZ;
        return true;
    }

    public void setMeshes(ArrayList<TriangleMesh> meshes) {
        this.meshes = meshes;
        preparedMeshCount = -1;
    }

    public void addMesh(TriangleMesh mesh) {
        this.meshes.add(mesh);
        preparedMeshCount = -1;
    }

    public TriangleMesh getMeshAt(int index) {
        return this.meshes.get(index);
    }

    /**
    @return a new 6 valued double array containing the coordinates of a min-max
    bounding box for current geometry.
    */
    @Override
    public double[] getMinMax() {
        return computeMinMaxPositions();
    }

    /**
    Kept for source and binary compatibility. MinMax values are calculated
    without storing state by getMinMax().
    Note that for each position in this array:
        0 - min (x)
        1 - min (y)
        2 - min (z)
        3 - max (x)
        4 - max (y)
        5 - max (z)
    */
    public void calculateMinMaxPositions() {
        computeMinMaxPositions();
    }

    private double[] computeMinMaxPositions() {
        double[] minMax = new double[6];

        boolean first = true;

        double minX = Double.MAX_VALUE, minY = Double.MAX_VALUE,
            minZ = Double.MAX_VALUE;
        double maxX = Double.MIN_VALUE, maxY = Double.MIN_VALUE,
            maxZ = Double.MIN_VALUE;

        for (int i = 0; i < meshes.size(); i++) {
            double[] minmax_mesh = meshes.get(i).getMinMax();
            double x = minmax_mesh[0];
            double y = minmax_mesh[1];
            double z = minmax_mesh[2];
            double X = minmax_mesh[3];
            double Y = minmax_mesh[4];
            double Z = minmax_mesh[5];

            if (first) {
                minX = x;
                maxX = X;
                minY = y;
                maxY = Y;
                minZ = z;
                maxZ = Z;
                first = false;
            }

            if (x < minX) {
                minX = x;
            }
            if (y < minY) {
                minY = y;
            }
            if (z < minZ) {
                minZ = z;
            }
            if (X > maxX) {
                maxX = X;
            }
            if (Y > maxY) {
                maxY = Y;
            }
            if (Z > maxZ) {
                maxZ = Z;
            }
        }
        minMax[0] = minX;
        minMax[1] = minY;
        minMax[2] = minZ;
        minMax[3] = maxX;
        minMax[4] = maxY;
        minMax[5] = maxZ;
        return minMax;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doIntersectionFirstHit.

    @param inOut_Ray
    @return true if given ray intersects current TriangleMeshGroup
    */
    public Ray doIntersectionFirstHit(Ray inOut_Ray) {
        RayHit hit = new RayHit(RayHit.DETAIL_NONE, true);
        if ( doIntersectionFirstHit(inOut_Ray, hit) ) {
            return hit.getRay();
        }
        return null;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doExtraInformation.
    @param inRay
    @param inT
    @param outData
    */
    public void
    doExtraInformation(
        Ray inRay, 
        double inT,
        RayHit outData) {
        if ( outData == null ) {
            return;
        }
        doIntersectionFirstHit(inRay.withT(inT), outData);
    }

    private void setIntersectionIndexes(int mesh, int triangle)
    {
        // Legacy last-hit state written by every ray: skip writes that change
        // nothing.
        int[] info = intersectionInformation.get();
        if ( info[0] != mesh ) info[0] = mesh;
        if ( info[1] != triangle ) info[1] = triangle;
    }

    @Override
    public boolean doIntersectionFirstHit(Ray inRay, RayHit outHit)
    {
        _TriangleMeshGroupClosestHit closest =
            new _TriangleMeshGroupClosestHit(meshes, inRay);
        double[] minT = {1e308};
        boolean traversed = false;
        if ( preparedMeshCount >= 0 && preparedMeshCount == meshes.size() ) {
            traversed = meshHierarchy.traverseClosest(inRay, closest, minT);
            if ( !traversed ) closest.reset();  // stack overflow: linear loop below
        }
        if ( !traversed ) {
            minT[0] = 1e308;
            for ( int i = 0; i < meshes.size(); i++ ) closest.visit(i, minT);
        }

        if ( !closest.hasBestHit ) {
            setIntersectionIndexes(-1, -1);
            return false;
        }

        setIntersectionIndexes(closest.bestMesh, closest.bestTriangle);
        RayHit bestHit = closest.bestHit;

        if ( outHit != null ) {
            outHit.clone(bestHit);
        }
        return true;
    }

    public int[] doIntersectionInformation()
    {
        int[] info = intersectionInformation.get();
        return new int[] {info[0], info[1]};
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doContainmentTest.
    \todo  Check efficiency for this implementation. Note that for the
    special application of volume rendering generation, it is better
    to provide another method, to add voxels after a path following
    over the line.
    @return INSIDE, OUTSIDE or LIMIT constant value
    */
    @Override
    public int doContainmentTest(Vector3Dd p, double distanceTolerance)
    {
        TriangleMesh mesh;
        int status;
        int i;

        // Chain of responsability behavior design pattern with TriangleMesh
        for ( i= 0; i< meshes.size(); i++ ) {
            mesh = meshes.get(i);
            status = mesh.doContainmentTest(p, distanceTolerance);
            if ( status != OUTSIDE ) {
                return status;
            }
        }
        return OUTSIDE;
    }

    /**
    Provides an object to text report convertion, optimized for human
    readability and debugging. Do not use for serialization or persistence
    purposes.
    @return human readable report from current mesh group
    */
    @Override
    public String toString() {
        return "TriangleMeshGroup < #Mesh: " + this.meshes.size() + " >";
    }

    public TriangleMeshGroup exportToTriangleMeshGroup()
    {
        return this;
    }

}

//=       back facing triangles                                             =
//=       interpolation                                                     =
//= References:                                                             =
//= [DACH2006] Dachille, Frank IX. Kaufman, Arie. "Incremental Triangle     =
//=     Voxelization", Center for Visual Computing and Department of        =
//=     Computer Science, State University of New York at Stony Brook, 2000 =

package vsdk.toolkit.environment.geometry.surface;
import java.io.Serial;

// Java basic classes
import java.util.ArrayList;

// VitralSDK classes
import vsdk.toolkit.common.dataStructures.ArrayListOfInts;
import vsdk.toolkit.common.dataStructures.ArrayListOfDoubles;
import vsdk.toolkit.environment.geometry.element.Triangle;
import vsdk.toolkit.environment.geometry.boundingVolumeHierarchy.TriangleAccelerationStructure;
import vsdk.toolkit.environment.geometry.boundingVolumeHierarchy.TriangleMeshAccelerationMethod;
import vsdk.toolkit.environment.geometry.element.Intersection;
import vsdk.toolkit.environment.geometry.Geometry;
import vsdk.toolkit.environment.geometry.element.GeometryBounds;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.environment.geometry.element.RayBoundary;
import vsdk.toolkit.environment.geometry.element.RayCrossing;
import vsdk.toolkit.environment.geometry.element.RayCrossingKind;
import vsdk.toolkit.environment.geometry.element.RayCrossingQuery;
import vsdk.toolkit.environment.geometry.element.RayCrossingSink;
import vsdk.toolkit.environment.geometry.element.RayCrossingSupport;
import vsdk.toolkit.environment.geometry.element.RayFirstHitStatus;
import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.environment.geometry.element.Vertex;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.element.RayHit;
import vsdk.toolkit.media.Image;
import vsdk.toolkit.environment.material.SimpleMaterial;

/**
This class represents a "basic" triangle mesh. Its model is based in a set
of vertexes and triangles (the edges are not store explicitly, and there
can not be an edge not forming part of a triangle).

This basic triangle mesh model can be associated with one and only one
material (this could change in future), but can have multiple textures,
and each texture can be mapped to a different set of triangles.

As every model class or `Entity` in VSDK, this class only can represent
(store in memory) the mesh model. It doesn't provide persistence or rendering
functionality, as this could be found at `io` and `render` packages.
Nevertheles, this class will be highly coupled with both of those, so
making any change here will impact highly that code.

This class does not ensure nor impose data integrity, and this will be the 
sole responsability of the cooperating utilities and applications.

\todo  Document more this class (include samples and data structure diagrams)
\todo  Generalize the material usage model, to conform similarly to current
      texture usage (i.e. allow multiple materials per mesh)
\todo  Make sure this is always using good names with complete words on it
      (rename methods and attributes)
\todo  Extend the model to allow dangling edges
*/
public class TriangleMesh extends Surface implements Cloneable {

//= Class attributes ========================================================

    @SuppressWarnings("FieldNameHidesFieldInSuperclass")

    @Serial private static final long serialVersionUID = 20080512L;

    // Basic mesh data model
    private String name = "default";

    /**
     * Whether raster renderers must draw both windings of this surface.
     * MIT .ray scenes use two-sided triangle intersection, so their imported
     * meshes set this flag to keep the GPU result consistent with raytracing.
     */
    private boolean doubleSided;

    // Consecutive triads of x/y/z point coordinates
    private double[] vertexPositions;

    // Consecutive triads of nx/ny/nz vertex normal coordinates
    private double[] vertexNormals;

    // Consecutive triads of nx/ny/nz vertex binormal coordinates (can be null)
    private double[] vertexBinormals;

    // Consecutive triads of nx/ny/nz vertex tangent coordinates (can be null)
    private double[] vertexTangents;

    // Consecutive triads of r/g/b vertex color components (can be null)
    private double[] vertexColors;

    // Consecutive values of boolean. If true, then vertex is selected.
    private boolean[] vertexSelections;

    // Consecutive pairs of u/v vertex texture coordinates (can be null)
    private double[] vertexUvs;

    private ArrayList<ArrayList<Integer>> incidentTrianglesPerVertexArray;

    /// Consecutive triads of p0/p1/p2 indices into the vertex arrays
    private int[] triangleIndexes;
    /// Consecutive triads of nx/ny/nz for normal vectors, one per triangle
    private double[] triangleNormals;

    // Auxiliary components for data model
    private SimpleMaterial[] materials;
    private Image[] textures;

    /**
    textureRanges is a 2D array which contents mappings between the
    `triangles` and `textures` sets. Each pair 
    <textureRanges[i][0], textureRanges[i][1]> means that the triangles
    from textureRanges[i-1][0] to textureRanges[i][0] (from triangle 0 
    when i = 0), are associated with the texture textureRanges[i][1]-1
    (or no texture for unspecified range or when textureRanges[i][1]
    contains a value out of textures array bounds or a value of 0).
    */
    private int[][] textureRanges;

    /**
    materialRanges is a 2D array which contents mappings between the
    `triangles` and `materials` sets. Each pair 
    <materialRanges[i][0], materialRanges[i][1]> means that the triangles
    from materialRanges[i-1][0] to materialRanges[i][0] (from triangle 0 
    when i = 0), are associated with the material materialRanges[i][1]
    (or no material for unspecified range or when materialRanges[i][1]
    contains a value out of materials array bounds).
    */
    private int[][] materialRanges;

    // Auxiliary data structures for storage of parcial results and 
    // preprocessing
    private final TriangleMeshIntersectionIndex intersectionTriangleIndex =
        new TriangleMeshIntersectionIndex();
    // Optional ray acceleration structure, see prepareAccelerationStructure
    private TriangleAccelerationStructure accelerationStructure;

//= Basic class management methods ==========================================

    public TriangleMesh() {
        doubleSided = false;
        vertexPositions = null;
        vertexNormals = null;
        vertexBinormals = null;
        vertexTangents = null;
        vertexColors = null;
        vertexUvs = null;
        vertexSelections = null;
        triangleIndexes = null;
        triangleNormals = null;
        incidentTrianglesPerVertexArray = null;
    }

    public boolean isDoubleSided() {
        return doubleSided;
    }

    public void setDoubleSided(boolean doubleSided) {
        this.doubleSided = doubleSided;
    }

    private double[] cloneDoubleArray(double in[])
    {
        double[] out = null;

        if ( in != null ) {
            out = new double[in.length];
            int i;
            for ( i = 0; i < in.length; i++ ) {
                out[i] = in[i];
            }
        }

        return out;
    }

    private boolean[] cloneBooleanArray(boolean in[])
    {
        boolean[] out = null;

        if ( in != null ) {
            out = new boolean[in.length];
            int i;
            for ( i = 0; i < in.length; i++ ) {
                out[i] = in[i];
            }
        }

        return out;
    }

    private int[] cloneIntArray(int in[])
    {
        int[] out = null;

        if ( in != null ) {
            out = new int[in.length];
            int i;
            for ( i = 0; i < in.length; i++ ) {
                out[i] = in[i];
            }
        }

        return out;
    }

    /**
    \todo  copy full structure!
    @return a new TriangleMesh containing an exact copy of the original
    @throws java.lang.CloneNotSupportedException
    */
    @Override
    public TriangleMesh clone() throws CloneNotSupportedException
    {
        super.clone();
        TriangleMesh other = new TriangleMesh();

        other.name = this.name;
        other.doubleSided = this.doubleSided;
        other.vertexPositions = cloneDoubleArray(this.vertexPositions);
        other.vertexPositions = cloneDoubleArray(this.vertexPositions);
        other.vertexNormals = cloneDoubleArray(this.vertexNormals);
        other.vertexBinormals = cloneDoubleArray(this.vertexBinormals);
        other.vertexTangents = cloneDoubleArray(this.vertexTangents);
        other.vertexColors = cloneDoubleArray(this.vertexColors);
        other.vertexUvs = cloneDoubleArray(this.vertexUvs);
        other.triangleNormals = cloneDoubleArray(this.triangleNormals);

        other. vertexSelections = cloneBooleanArray(this.vertexSelections);
        other.triangleIndexes = cloneIntArray(this.triangleIndexes);
        other.accelerationStructure = this.accelerationStructure != null ?
            this.accelerationStructure.copy() : null;

        // Pending!
        other.incidentTrianglesPerVertexArray = null;
        other.materials = null;
        other.textures = null;
        other.textureRanges = null;
        other.materialRanges = null;
         
/*
        ArrayList<ArrayList<Integer>> incidentTrianglesPerVertexArray;
        SimpleMaterial[] materials;
        Image[] textures;
        int[][] textureRanges;
        int[][] materialRanges;
*/
        return other;
    }

    /** Mathematical copy; scene-owned materials and textures are not copied. */
    @Override
    public Geometry copyGeometry()
    {
        // cpp copy-constructs and clears materials, textures and both range
        // tables; Java clone() already leaves those unset.
        try {
            return clone();
        }
        catch ( CloneNotSupportedException e ) {
            return null;
        }
    }

    public double[] getVertexPositionsView() { return vertexPositions; }
    public int[] getTriangleIndexesView() { return triangleIndexes; }
    public double[] getTriangleNormalsView() { return triangleNormals; }

    private static int lengthOf(double[] a) { return a == null ? 0 : a.length; }
    private static int lengthOf(int[] a) { return a == null ? 0 : a.length; }

    @Override
    public void getBounds(GeometryBounds result)
    {
        if (lengthOf(triangleIndexes) % 3 != 0 || lengthOf(vertexPositions) % 3 != 0) {
            result.setIndeterminate(); return;
        }
        if (getNumTriangles() == 0) { result.setEmpty(); return; }
        Vector3Dd minimum = new Vector3Dd(), maximum = new Vector3Dd();
        boolean first = true;
        for (int i = 0; i < triangleIndexes.length; ++i) {
            final int index = triangleIndexes[i];
            if (index < 0 || index >= getNumVertices()) { result.setIndeterminate(); return; }
            final double x = vertexPositions[index*3], y = vertexPositions[index*3+1];
            final double z = vertexPositions[index*3+2];
            if (!Double.isFinite(x) || !Double.isFinite(y) || !Double.isFinite(z)) {
                result.setIndeterminate(); return;
            }
            if (first) { minimum = maximum = new Vector3Dd(x,y,z); first = false; }
            else {
                minimum = new Vector3Dd(Math.min(minimum.x(),x), Math.min(minimum.y(),y), Math.min(minimum.z(),z));
                maximum = new Vector3Dd(Math.max(maximum.x(),x), Math.max(maximum.y(),y), Math.max(maximum.z(),z));
            }
        }
        if (first) result.setEmpty();
        else result.setFinite(minimum, maximum);
    }

    /** `points` receives p0, p1, p2 (cpp output references). */
    private boolean getTrianglePoints(int index, Vector3Dd[] points)
    {
        if (index < 0 || index >= getNumTriangles() || lengthOf(vertexPositions) % 3 != 0 ||
            lengthOf(triangleIndexes) % 3 != 0) return false;
        final int a = triangleIndexes[index*3], b = triangleIndexes[index*3+1], c = triangleIndexes[index*3+2];
        if (a < 0 || b < 0 || c < 0 || a >= getNumVertices() || b >= getNumVertices() || c >= getNumVertices())
            return false;
        points[0] = new Vector3Dd(vertexPositions[a*3], vertexPositions[a*3+1], vertexPositions[a*3+2]);
        points[1] = new Vector3Dd(vertexPositions[b*3], vertexPositions[b*3+1], vertexPositions[b*3+2]);
        points[2] = new Vector3Dd(vertexPositions[c*3], vertexPositions[c*3+1], vertexPositions[c*3+2]);
        return true;
    }

    private void fillTypedDetail(Ray ray, double t, int face, double u, double v,
        Vector3Dd p0, Vector3Dd p1, Vector3Dd p2,
        boolean normalReversed, RayHit hit)
    {
        if (hit == null || !hit.needsAnySurfaceData()) return;
        hit.beginDetailEvaluation();
        if (hit.needsPoint()) {
            hit.point = ray.getOrigin().add(ray.getDirection().multiply(t));
            hit.markDetailAvailable(RayHit.DETAIL_POINT);
        }
        if (hit.needsNormal()) {
            final Vector3Dd normal = p1.subtract(p0).crossProduct(p2.subtract(p0));
            final double squared = normal.dotProduct(normal);
            if (Double.isFinite(squared) && squared > 0) {
                hit.normal = squared < VSDK.EPSILON ? normal.multiply(1.0/Math.sqrt(squared)) : normal.normalized();
                if (normalReversed) hit.normal = hit.normal.multiply(-1);
                hit.markDetailAvailable(RayHit.DETAIL_NORMAL);
            }
        }
        if (hit.needsTextureCoordinates() && lengthOf(vertexUvs) >= (long)getNumVertices()*2) {
            final int a = triangleIndexes[face*3], b = triangleIndexes[face*3+1], c = triangleIndexes[face*3+2];
            final double w = 1.0-u-v;
            final double texU = w*vertexUvs[a*2] + u*vertexUvs[b*2] + v*vertexUvs[c*2];
            final double texV = w*vertexUvs[a*2+1] + u*vertexUvs[b*2+1] + v*vertexUvs[c*2+1];
            if (Double.isFinite(texU) && Double.isFinite(texV)) {
                hit.u = texU; hit.v = texV; hit.markDetailAvailable(RayHit.DETAIL_UV);
            }
        }
    }

    @Override
    public RayFirstHitStatus queryFirstHit(Ray ray, RayCrossingQuery query,
        RayHit hit, RayBoundary boundary)
    {
        if (hit != null) hit.clear();
        if (boundary != null) boundary.assign(new RayBoundary());
        if (lengthOf(triangleIndexes) % 3 != 0 || lengthOf(vertexPositions) % 3 != 0)
            return RayFirstHitStatus.Indeterminate;
        double[] t = {0}, u = {0}, v = {0};
        if (getNumTriangles() == 0) {
            final Vector3Dd zero = new Vector3Dd();
            return Triangle.queryIntersection(ray, zero, zero, zero, query, t, u, v);
        }
        int best = -1;
        double bestT = 0, bestU = 0, bestV = 0;
        Vector3Dd bestP0 = new Vector3Dd(), bestP1 = new Vector3Dd(), bestP2 = new Vector3Dd();
        Vector3Dd[] p = new Vector3Dd[3];
        for (int i = 0; i < getNumTriangles(); ++i) {
            if (!getTrianglePoints(i, p)) return RayFirstHitStatus.Indeterminate;
            final RayFirstHitStatus status = Triangle.queryIntersection(ray, p[0], p[1], p[2], query, t, u, v);
            if (status != RayFirstHitStatus.Hit && status != RayFirstHitStatus.Miss) return status;
            if (status == RayFirstHitStatus.Hit && (best < 0 || t[0] < bestT)) {
                best = i; bestT = t[0]; bestU = u[0]; bestV = v[0]; bestP0 = p[0]; bestP1 = p[1]; bestP2 = p[2];
            }
        }
        if (best < 0) return RayFirstHitStatus.Miss;
        final Vector3Dd point = ray.getOrigin().add(ray.getDirection().multiply(bestT));
        if (!Double.isFinite(point.x()) || !Double.isFinite(point.y()) ||
            !Double.isFinite(point.z())) return RayFirstHitStatus.Indeterminate;
        if (boundary != null) boundary.assign(new RayBoundary(bestT, this, false, best));
        if (hit != null) {
            hit.setHitDistance(bestT);
            if (hit.shouldStoreRay()) hit.setRay(ray.withT(bestT));
            fillTypedDetail(ray, bestT, best, bestU, bestV, bestP0, bestP1, bestP2, false, hit);
        }
        return RayFirstHitStatus.Hit;
    }

    @Override
    public RayFirstHitStatus querySurfaceDetail(Ray ray, RayBoundary boundary,
        RayHit hit)
    {
        if (hit != null) hit.clear();
        if (hit == null || boundary.geometry != this || boundary.surfaceIndex < 0 ||
            boundary.surfaceIndex >= getNumTriangles() || !Double.isFinite(boundary.t))
            return RayFirstHitStatus.InvalidInput;
        if (boundary.getInstanceCount() != 0) return RayFirstHitStatus.Unsupported;
        final int face = (int)boundary.surfaceIndex;
        Vector3Dd[] p = new Vector3Dd[3];
        if (!getTrianglePoints(face, p)) return RayFirstHitStatus.Indeterminate;
        double[] t = {0}, u = {0}, v = {0};
        final RayFirstHitStatus status = Triangle.queryIntersection(ray, p[0], p[1], p[2],
            new RayCrossingQuery(boundary.t, boundary.t), t, u, v);
        if (status != RayFirstHitStatus.Hit) return status;
        final Vector3Dd point = ray.getOrigin().add(ray.getDirection().multiply(t[0]));
        if (!Double.isFinite(point.x()) || !Double.isFinite(point.y()) ||
            !Double.isFinite(point.z())) return RayFirstHitStatus.Indeterminate;
        hit.setHitDistance(t[0]);
        if (hit.shouldStoreRay()) hit.setRay(ray.withT(t[0]));
        fillTypedDetail(ray, t[0], face, u[0], v[0], p[0], p[1], p[2], boundary.normalReversed, hit);
        return RayFirstHitStatus.Hit;
    }

    @Override
    public RayCrossingSupport emitRayCrossings(Ray ray, RayCrossingQuery query,
        RayCrossingSink sink)
    {
        if (lengthOf(triangleIndexes) % 3 != 0 || lengthOf(vertexPositions) % 3 != 0)
            return RayCrossingSupport.Indeterminate;
        ArrayList<Double> depths = new ArrayList<>();
        ArrayList<Integer> faces = new ArrayList<>();
        double[] t = {0}, u = {0}, v = {0};
        if (getNumTriangles() == 0) {
            final Vector3Dd zero = new Vector3Dd();
            final RayFirstHitStatus status = Triangle.queryIntersection(ray, zero, zero, zero, query, t, u, v);
            if (status == RayFirstHitStatus.Unsupported) return RayCrossingSupport.Unsupported;
            if (status == RayFirstHitStatus.InvalidInput) return RayCrossingSupport.InvalidInput;
            if (status == RayFirstHitStatus.Indeterminate) return RayCrossingSupport.Indeterminate;
            return RayCrossingSupport.Supported;
        }
        Vector3Dd[] p = new Vector3Dd[3];
        for (int i = 0; i < getNumTriangles(); ++i) {
            if (!getTrianglePoints(i, p)) return RayCrossingSupport.Indeterminate;
            final RayFirstHitStatus status = Triangle.queryIntersection(ray, p[0], p[1], p[2], query, t, u, v);
            if (status == RayFirstHitStatus.Unsupported) return RayCrossingSupport.Unsupported;
            if (status == RayFirstHitStatus.InvalidInput) return RayCrossingSupport.InvalidInput;
            if (status == RayFirstHitStatus.Indeterminate) return RayCrossingSupport.Indeterminate;
            if (status == RayFirstHitStatus.Hit) {
                final Vector3Dd point = ray.getOrigin().add(ray.getDirection().multiply(t[0]));
                if (!Double.isFinite(point.x()) || !Double.isFinite(point.y()) ||
                    !Double.isFinite(point.z())) return RayCrossingSupport.Indeterminate;
                int at = 0;
                while (at < depths.size() && depths.get(at) <= t[0]) ++at;
                if (at == depths.size()) { depths.add(t[0]); faces.add(i); }
                else { depths.add(at, t[0]); faces.add(at, i); }
            }
        }
        for (int i = 0; i < depths.size(); ++i) {
            RayCrossing event = new RayCrossing(depths.get(i), this, RayCrossingKind.Unclassified);
            event.surfaceIndex = faces.get(i);
            sink.crossing(event);
        }
        return RayCrossingSupport.Supported;
    }

    public String getName() {
        return this.name;
    }

    public void getVertexAt(int i, Vertex vertex) {
        vertex.position = vertex.position.withX(vertexPositions[3*i]);
        vertex.position = vertex.position.withY(vertexPositions[3*i+1]);
        vertex.position = vertex.position.withZ(vertexPositions[3*i+2]);
        if ( vertexNormals != null ) {
            vertex.normal = vertex.normal.withX(vertexNormals[3*i]);
            vertex.normal = vertex.normal.withY(vertexNormals[3*i+1]);
            vertex.normal = vertex.normal.withZ(vertexNormals[3*i+2]);
        }
        if ( vertexBinormals != null ) {
            vertex.binormal = vertex.binormal.withX(vertexBinormals[3*i]);
            vertex.binormal = vertex.binormal.withY(vertexBinormals[3*i+1]);
            vertex.binormal = vertex.binormal.withZ(vertexBinormals[3*i+2]);
        }
        if ( vertexTangents != null ) {
            vertex.tangent = vertex.tangent.withX(vertexTangents[3*i]);
            vertex.tangent = vertex.tangent.withY(vertexTangents[3*i+1]);
            vertex.tangent = vertex.tangent.withZ(vertexTangents[3*i+2]);
        }
        if ( vertexUvs != null ) {
            vertex.u = vertexUvs[2*i];
            vertex.v = vertexUvs[2*i+1];
        }
    }

    public SimpleMaterial[] getMaterials() {
        return this.materials;
    }

    public Image[] getTextures() {
        return this.textures;
    }

    public Image getTextureAt(int index) {
        return this.textures[index];
    }

    public void setName(String name) {
        this.name = name;
    }

    public void initVertexPositionsArray(int n)
    {
        accelerationStructure = null;
        vertexPositions = new double[n*3];
    }

    public void initVertexNormalsArray()
    {
        int n = vertexPositions.length / 3;
        if ( vertexNormals == null ||
             vertexNormals.length != vertexPositions.length ) {
            vertexNormals = new double[n*3];
        }
    }

    public void initVertexBinormalsArray()
    {
        int n = vertexPositions.length / 3;
        if ( vertexBinormals == null ||
             vertexBinormals.length != vertexPositions.length ) {
            vertexBinormals = new double[n*3];
        }
    }

    public void initVertexTangentsArray()
    {
        int n = vertexPositions.length / 3;
        if ( vertexTangents == null ||
             vertexTangents.length != vertexPositions.length ) {
            vertexTangents = new double[n*3];
        }
    }

    public void initVertexColorsArray()
    {
        int n = vertexPositions.length / 3;
        if ( vertexColors == null ||
             vertexColors.length != vertexPositions.length ) {
            vertexColors = new double[n*3];
        }
    }

    public void initVertexUvsArray()
    {
        int n = vertexPositions.length / 3;
        if ( vertexUvs == null ||
             vertexUvs.length/2 != vertexPositions.length/3 ) {
            vertexUvs = new double[n*2];
        }
    }

    public void initIncidentTrianglesPerVertexArray() {
        incidentTrianglesPerVertexArray = new ArrayList<ArrayList<Integer>>();
    }

    public void detachColors()
    {
        vertexColors = null;
    }

    public void detachNormals()
    {
        vertexNormals = null;
    }

    public void detachUvs()
    {
        vertexUvs = null;
    }

    /**
    This method provides a clear structured form of defining the mesh vertexes,
    but it is inefficient. Its use is discouraged for applications manipulating
    big meshes.
    @param vertexes
    @param withNormals
    @param withBinormals
    @param withTangents
    @param withUvs
    */
    public void setVertexes(Vertex[] vertexes,
                            boolean withNormals, boolean withBinormals,
                            boolean withTangents, boolean withUvs) {
        int n, i;

        n = vertexes.length;

        initVertexPositionsArray(n);
        if ( withNormals ) {
            initVertexNormalsArray();
        }
        if ( withBinormals ) {
            initVertexBinormalsArray();
        }
        if ( withTangents ) {
            initVertexTangentsArray();
        }
        if ( withUvs ) {
            initVertexUvsArray();
        }

        for ( i = 0; i < n; i++ ) {
            vertexPositions[3*i] = vertexes[i].position.x();
            vertexPositions[3*i+1] = vertexes[i].position.y();
            vertexPositions[3*i+2] = vertexes[i].position.z();
            if ( vertexNormals != null ) {
                vertexNormals[3*i] = vertexes[i].normal.x();
                vertexNormals[3*i+1] = vertexes[i].normal.y();
                vertexNormals[3*i+2] = vertexes[i].normal.z();
            }
            if ( vertexBinormals != null ) {
                vertexBinormals[3*i] = vertexes[i].binormal.x();
                vertexBinormals[3*i+1] = vertexes[i].binormal.y();
                vertexBinormals[3*i+2] = vertexes[i].binormal.z();
            }
            if ( vertexTangents != null ) {
                vertexTangents[3*i] = vertexes[i].tangent.x();
                vertexTangents[3*i+1] = vertexes[i].tangent.y();
                vertexTangents[3*i+2] = vertexes[i].tangent.z();
            }
            if ( vertexUvs != null ) {
                vertexUvs[2*i] = vertexes[i].u;
                vertexUvs[2*i+1] = vertexes[i].v;
            }
        }
    }

    public void initTriangleArrays(int n)
    {
        accelerationStructure = null;
        triangleIndexes = new int[n*3];
        triangleNormals = new double [n*3];
    }

    /**
    This method provides a clear structured form of defining the mesh triangles,
    but it is inefficient. Its use is discouraged for applications manipulating
    big meshes.
    @param triangles
    */
    public void setTriangles(Triangle[] triangles) {
        int n, i;

        n = triangles.length;

        initTriangleArrays(n);

        for ( i = 0; i < n; i++ ) {
            triangleIndexes[3*i] = triangles[i].getPoint0();
            triangleIndexes[3*i+1] = triangles[i].getPoint1();
            triangleIndexes[3*i+2] = triangles[i].getPoint2();
            if ( triangleNormals != null ) {
                triangleNormals[3*i] = triangles[i].getNormal().x();
                triangleNormals[3*i+1] = triangles[i].getNormal().y();
                triangleNormals[3*i+2] = triangles[i].getNormal().z();
            }
        }
    }

    public void setTextures(Image[] textures) {
        this.textures = textures;
    }

    public void setMaterials(SimpleMaterial[] materials) {
        this.materials = materials;
    }

    /**
    Given a vertex structure and an `i` position, this method copies
    the information from the structure in to the i-th vertex arrays position.
    PRE: 0 <= i < vertexPositions.length/3
    @param i
    @param vertex
    */
    public void setVertexAt(int i, Vertex vertex) {
        accelerationStructure = null;
        vertexPositions[3*i] = vertex.position.x();
        vertexPositions[3*i+1] = vertex.position.y();
        vertexPositions[3*i+2] = vertex.position.z();
        vertexNormals[3*i] = vertex.normal.x();
        vertexNormals[3*i+1] = vertex.normal.y();
        vertexNormals[3*i+2] = vertex.normal.z();
        vertexBinormals[3*i] = vertex.binormal.x();
        vertexBinormals[3*i+1] = vertex.binormal.y();
        vertexBinormals[3*i+2] = vertex.binormal.z();
        vertexTangents[3*i] = vertex.tangent.x();
        vertexTangents[3*i+1] = vertex.tangent.y();
        vertexTangents[3*i+2] = vertex.tangent.z();
        vertexUvs[2*i] = vertex.u;
        vertexUvs[2*i+1] = vertex.v;
    }

    /**
    Given a triangle structure and an `i` position, this method copies
    the information from the structure in to the i-th triangle arrays position.
    PRE: 0 <= i < vertexPositions.length/3
    @param i
    @param triangle
    */
    public void setTriangleAt(int i, Triangle triangle) {
        accelerationStructure = null;
        triangleIndexes[3*i] = triangle.getPoint0();
        triangleIndexes[3*i+1] = triangle.getPoint1();
        triangleIndexes[3*i+2] = triangle.getPoint2();
        if ( triangleNormals != null ) {
            triangleNormals[3*i] = triangle.getNormal().x();
            triangleNormals[3*i+1] = triangle.getNormal().y();
            triangleNormals[3*i+2] = triangle.getNormal().z();
        }
    }

    public int getNumVertices()
    {
        if ( vertexPositions == null ) return 0;
        return vertexPositions.length/3;
    }

    public int getNumTriangles()
    {
        if ( triangleIndexes == null ) return 0;
        return triangleIndexes.length/3;
    }

    public boolean[] getVertexSelections()
    {
        if ( vertexSelections == null ||
             vertexSelections.length != (vertexPositions.length/3) ) {
            vertexSelections = new boolean[vertexPositions.length/3];
            int i;
            for ( i = 0; i < vertexSelections.length; i++ ) {
                vertexSelections[i] = false;
            }
        }
        return vertexSelections;
    }

    public double[] getVertexPositions()
    {
        return vertexPositions;
    }

    public double[] getVertexNormals()
    {
        return vertexNormals;
    }

    public double[] getVertexBinormals()
    {
        return vertexBinormals;
    }

    public double[] getVertexTangents()
    {
        return vertexTangents;
    }

    public double[] getVertexColors()
    {
        return vertexColors;
    }

    public double[] getVertexUvs()
    {
        return vertexUvs;
    }

    public int[] getTriangleIndexes()
    {
        return triangleIndexes;
    }

    public double[] getTriangleNormals()
    {
        return triangleNormals;
    }

    public void setTextureAt(int index, Image image) {
        this.textures[index] = image;
    }

//= Methods for managing textureRanges ======================================

    public int[][] getTextureRanges() {
        return textureRanges;
    }

    /**
    Note this always returns an array with two (2) integers: the first one
    is an index to `triangles` array, the second one is an index to the
    `textures` array.
    @param spanRange
    @return an integer array for textures ranges indexes
    */
    public int[] getTextureRangeAt(int spanRange) {
        return textureRanges[spanRange];
    }

    public void setTextureRanges(int ranges[][]) {
        textureRanges = ranges;
    }

//= Methods for managing materialRanges =====================================

    public int[][] getMaterialRanges() {
        return materialRanges;
    }

    /**
    Note this always returns an array with two (2) integers: the first one
    is an index to `triangles` array, the second one is an index to the
    `materials` array.
    @param spanRange
    @return a integer array with material ranges indexes
    */
    public int[] getMaterialRangeAt(int spanRange) {
        return materialRanges[spanRange];
    }

    public void setMaterialRanges(int ranges[][]) {
        materialRanges = ranges;
    }

//= Fundamental geometry operations methods =================================

    public void calculateNormals() {
        Vector3Dd tn;
        int i, j;

        Vector3Dd v1 = new Vector3Dd();
        Vector3Dd v2 = new Vector3Dd();
        Vector3Dd v3 = new Vector3Dd();

        for ( i = 0; i < getNumTriangles(); i++ ) {
            v1 = new Vector3Dd(vertexPositions[3*triangleIndexes[3*i+0]+0], vertexPositions[3*triangleIndexes[3*i+0]+1], vertexPositions[3*triangleIndexes[3*i+0]+2]);
            v2 = new Vector3Dd(vertexPositions[3*triangleIndexes[3*i+1]+0], vertexPositions[3*triangleIndexes[3*i+1]+1], vertexPositions[3*triangleIndexes[3*i+1]+2]);
            v3 = new Vector3Dd(vertexPositions[3*triangleIndexes[3*i+2]+0], vertexPositions[3*triangleIndexes[3*i+2]+1], vertexPositions[3*triangleIndexes[3*i+2]+2]);

            double ax = v2.x() - v1.x();
            double ay = v2.y() - v1.y();
            double az = v2.z() - v1.z();

            double bx = v3.x() - v2.x();
            double by = v3.y() - v2.y();
            double bz = v3.z() - v2.z();

            Vector3Dd a = new Vector3Dd(ax, ay, az);
            Vector3Dd b = new Vector3Dd(bx, by, bz);

            a = a.normalized();
            b = b.normalized();

            tn = a.crossProduct(b);
            tn = tn.normalized();
            triangleNormals[3*i] = tn.x();
            triangleNormals[3*i+1] = tn.y();
            triangleNormals[3*i+2] = tn.z();
        }

        initIncidentTrianglesPerVertexArray();
        for ( i = 0; i < getNumVertices(); i++ ) {
            incidentTrianglesPerVertexArray.add(new ArrayList<Integer>());
        }

        for ( i = 0; i < getNumTriangles(); i++ ) {
            incidentTrianglesPerVertexArray.get(triangleIndexes[3*i+0]).add(i);
            incidentTrianglesPerVertexArray.get(triangleIndexes[3*i+1]).add(i);
            incidentTrianglesPerVertexArray.get(triangleIndexes[3*i+2]).add(i);
        }

        initVertexNormalsArray();

        for ( i = 0; i < getNumVertices(); i++ ) {
            double nx = 0.0;
            double ny = 0.0;
            double nz = 0.0;
            for ( j = 0; j < incidentTrianglesPerVertexArray.get(i).size(); j++ ) {
                int ii;
                ii = incidentTrianglesPerVertexArray.get(i).get(j);
                nx += triangleNormals[3*ii+0];
                ny += triangleNormals[3*ii+1];
                nz += triangleNormals[3*ii+2];
            }
            Vector3Dd n = new Vector3Dd(nx, ny, nz).normalized();
            vertexNormals[3*i] = n.x();
            vertexNormals[3*i+1] = n.y();
            vertexNormals[3*i+2] = n.z();
        }
    }

    /**
    In the triangle meshes the vector normal directions must be consistent with
    the triangle normal, which direction depends on triangle order (clockwise
    or counterclockwise).

    This method checks for inverted normals, and invert them to make it
    consistent with triangle orientation order.

    Note that this methods recalculate triangle surface normals.
    */
    public void reorientateNormals()
    {
        int i;
        Vector3Dd a, b, c;
        Vector3Dd u, v, n, an, bn, cn;

        a = new Vector3Dd();
        b = new Vector3Dd();
        c = new Vector3Dd();
        an = new Vector3Dd();
        bn = new Vector3Dd();
        cn = new Vector3Dd();
        for ( i = 0; i < getNumTriangles(); i++ ) {

            a = new Vector3Dd(vertexPositions[3*triangleIndexes[3*i]+0], vertexPositions[3*triangleIndexes[3*i]+1], vertexPositions[3*triangleIndexes[3*i]+2]);
            b = new Vector3Dd(vertexPositions[3*triangleIndexes[3*i+1]+0], vertexPositions[3*triangleIndexes[3*i+1]+1], vertexPositions[3*triangleIndexes[3*i+1]+2]);
            c = new Vector3Dd(vertexPositions[3*triangleIndexes[3*i+2]+0], vertexPositions[3*triangleIndexes[3*i+2]+1], vertexPositions[3*triangleIndexes[3*i+2]+2]);

            an = new Vector3Dd(vertexNormals[3*triangleIndexes[3*i]+0], vertexNormals[3*triangleIndexes[3*i]+1], vertexNormals[3*triangleIndexes[3*i]+2]);
            bn = new Vector3Dd(vertexNormals[3*triangleIndexes[3*i+1]+0], vertexNormals[3*triangleIndexes[3*i+1]+1], vertexNormals[3*triangleIndexes[3*i+1]+2]);
            cn = new Vector3Dd(vertexNormals[3*triangleIndexes[3*i+2]+0], vertexNormals[3*triangleIndexes[3*i+2]+1], vertexNormals[3*triangleIndexes[3*i+2]+2]);

            u = b.subtract(a);
            v = c.subtract(a);
            n = u.crossProduct(v);
            n = n.normalized();
            if ( triangleNormals != null ) {
                triangleNormals[3*i+0] = n.x();
                triangleNormals[3*i+1] = n.y();
                triangleNormals[3*i+2] = n.z();
            }
            if ( n.dotProduct(an) < 0 ) {
                an = an.multiply(-1);
                vertexNormals[3*triangleIndexes[3*i]+0] = an.x();
                vertexNormals[3*triangleIndexes[3*i]+1] = an.y();
                vertexNormals[3*triangleIndexes[3*i]+2] = an.z();
            }
            if ( n.dotProduct(bn) < 0 ) {
                bn = bn.multiply(-1);
                vertexNormals[3*triangleIndexes[3*i+1]+0] = bn.x();
                vertexNormals[3*triangleIndexes[3*i+1]+1] = bn.y();
                vertexNormals[3*triangleIndexes[3*i+1]+2] = bn.z();
            }
            if ( n.dotProduct(cn) < 0 ) {
                cn = cn.multiply(-1);
                vertexNormals[3*triangleIndexes[3*i+2]+0] = cn.x();
                vertexNormals[3*triangleIndexes[3*i+2]+1] = cn.y();
                vertexNormals[3*triangleIndexes[3*i+2]+2] = cn.z();
            }
        }
    }

    /** Needed for supplying the Geometry.getMinMax operation */
    private double[] calculateMinMaxPositions() {
        double[] minMax = new double[6];

        double minX = Double.MAX_VALUE;
        double minY = Double.MAX_VALUE;
        double minZ = Double.MAX_VALUE;
        double maxX = -Double.MAX_VALUE;
        double maxY = -Double.MAX_VALUE;
        double maxZ = -Double.MAX_VALUE;
        int i;

        for ( i = 0; i < getNumVertices(); i++ ) {
            double x = vertexPositions[3*i+0];
            double y = vertexPositions[3*i+1];
            double z = vertexPositions[3*i+2];

            if ( x < minX ) minX = x;
            if ( y < minY ) minY = y;
            if ( z < minZ ) minZ = z;
            if ( x > maxX ) maxX = x;
            if ( y > maxY ) maxY = y;
            if ( z > maxZ ) maxZ = z;
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
    This method is supposed to be a friend of TriangleMesh related objects.
    The method is used to query the last intersected triangle, after a
    positive called to the doIntersectionFirstHit method.
    @return selected triangle
    */
    public int doIntersectionInformation() {
        return intersectionTriangleIndex.get();
    }

    /**
    Makes `doIntersectionFirstHit` use the given method: builds its
    acceleration structure (single threaded) unless one of the same method
    is already built for the current sizes; `BRUTE_FORCE` releases it. Call
    it before tracing from several threads: afterwards, calls with the same
    method only read. The result of the intersection does not depend on the
    method.

    The structure is a snapshot: the mutators of this class release it, but
    writes through the array getters (`getVertexPositions`,
    `getTriangleIndexes`) do not, so call `releaseAccelerationStructure` or
    this method again after them.
    @param method acceleration method to use
    */
    public void prepareAccelerationStructure(TriangleMeshAccelerationMethod method)
    {
        if ( method == TriangleMeshAccelerationMethod.BRUTE_FORCE ) {
            if ( accelerationStructure != null ) accelerationStructure = null;
            return;
        }
        final TriangleAccelerationStructure current = accelerationStructure;
        if ( current != null && current.getMethod() == method &&
             current.matches(getNumVertices(), getNumTriangles()) ) return;
        TriangleAccelerationStructure structure = TriangleAccelerationStructure.create(method);
        if ( structure != null ) structure.build(vertexPositions, triangleIndexes);
        accelerationStructure = structure;
    }

    public void releaseAccelerationStructure()
    {
        accelerationStructure = null;
    }

    /**
    @return current structure, null when the brute force loop is used
    */
    public TriangleAccelerationStructure getAccelerationStructure()
    {
        return accelerationStructure;
    }

    /**
    When a structure built for the current sizes exists: writes what
    `getMinMax` would return, without walking the vertices, and returns true.
    @param outMinMax six values array to fill
    @return true when `outMinMax` was written
    */
    public boolean getPreparedMinMax(double[] outMinMax)
    {
        final TriangleAccelerationStructure structure = accelerationStructure;
        if ( structure == null || !structure.matches(getNumVertices(), getNumTriangles()) ) return false;
        structure.getVertexBounds(outMinMax);
        return true;
    }

    /**
    Provides an object to text report convertion, optimized for human
    readability and debugging. Do not use for serialization or persistence
    purposes.
    @return human readable representation of current TriangleMesh information
    */
    @Override
    public String toString() {
        String msg;
        int i;
        double mm[];

        mm = getMinMax();
        Vector3Dd p;

        msg = "- TriangleMesh ------------------------------------------------------------\n";
        msg += "  - Number of triangles:" + getNumTriangles() + "\n";
        msg += "  - Number of vertexes:" + getNumVertices() + "\n";
        p = new Vector3Dd(mm[0], mm[1], mm[2]);
        msg += "  - MINMAX: " + p;
        p = new Vector3Dd(mm[3], mm[4], mm[5]);
        msg += " - " + p + "\n";
        if ( materials == null ) {
            msg += "  - No materials available!\n";
          }
          else {
            msg += "  - " + materials.length + " materials\n";
        }

        if ( materialRanges == null ) {
            msg += "  - No material ranges association table available!\n";
        }
        else {
            msg += "  - " + materialRanges.length + " material spans:\n";
            for ( i = 0; i < materialRanges.length; i++ ) {
                msg += "    . " + materialRanges[i][0] + " -> " + materialRanges[i][1] + "\n";
            }
        }

        if ( textures == null ) {
            msg += "  - No textures available!\n";
          }
          else {
            msg += "  - " + textures.length + " textures\n";
        }

        if ( textureRanges == null ) {
            msg += "  - No texture ranges association table available!\n";
        }
        else {
            msg += "  - " + textureRanges.length + " texture spans:\n";
            for ( i = 0; i < textureRanges.length; i++ ) {
                msg += "    . " + textureRanges[i][0] + " -> " + textureRanges[i][1] + "\n";
            }
        }

        msg += "---------------------------------------------------------------------------\n";
        return msg;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doIntersectionFirstHit.
    @param inOut_Ray
    @return true if given ray intersects current TriangleMesh
    */
    public Ray
    doIntersectionFirstHit(Ray inOut_Ray) {
        RayHit hit = new RayHit(RayHit.DETAIL_NONE, true);
        if ( doIntersectionFirstHit(inOut_Ray, hit) ) {
            return hit.getRay();
        }
        return null;
    }

    @Override
    public boolean doIntersectionFirstHit(Ray inRay, RayHit outHit)
    {
        return doIntersectionInternal(inRay, outHit, null);
    }

    boolean doIntersectionFirstHit(Ray inRay, RayHit outHit, int[] outTriangleIndex)
    {
        return doIntersectionInternal(inRay, outHit, outTriangleIndex);
    }

    private boolean doIntersectionInternal(
        Ray inRay,
        RayHit outHit,
        int[] outTriangleIndex)
    {
        Ray ray = new Ray(inRay);
        double[] bestT = {1e308};
        int[] bestTri = {-1};
        double[] bestU = {0};
        double[] bestV = {0};
        boolean found;

        final TriangleAccelerationStructure structure = accelerationStructure;
        if ( structure != null && structure.matches(getNumVertices(), getNumTriangles()) ) {
            found = structure.firstHit(ray, vertexPositions, triangleIndexes,
                bestT, bestU, bestV, bestTri);
        }
        else {
            found = TriangleAccelerationStructure.firstHitBruteForce(ray,
                vertexPositions, triangleIndexes, bestT, bestU, bestV, bestTri);
        }
        final int bestTriangle = bestTri[0];
        Intersection bestHit = null;
        if ( found ) {
            // Same point and geometric normal of
            // Triangle.doIntersectionWithTriangle, used before by this loop.
            Vector3Dd v0 = new Vector3Dd(
                vertexPositions[3*triangleIndexes[3*bestTriangle+0]+0],
                vertexPositions[3*triangleIndexes[3*bestTriangle+0]+1],
                vertexPositions[3*triangleIndexes[3*bestTriangle+0]+2]
            );
            Vector3Dd v1 = new Vector3Dd(
                vertexPositions[3*triangleIndexes[3*bestTriangle+1]+0],
                vertexPositions[3*triangleIndexes[3*bestTriangle+1]+1],
                vertexPositions[3*triangleIndexes[3*bestTriangle+1]+2]
            );
            Vector3Dd v2 = new Vector3Dd(
                vertexPositions[3*triangleIndexes[3*bestTriangle+2]+0],
                vertexPositions[3*triangleIndexes[3*bestTriangle+2]+1],
                vertexPositions[3*triangleIndexes[3*bestTriangle+2]+2]
            );
            bestHit = new Intersection(
                bestT[0],
                ray.getOrigin().add(ray.getDirection().multiply(bestT[0])),
                v1.subtract(v0).crossProduct(v2.subtract(v0)).normalized()
            );
        }

        if ( bestHit == null ) {
            intersectionTriangleIndex.set(-1);
            if ( outTriangleIndex != null && outTriangleIndex.length > 0 ) {
                outTriangleIndex[0] = -1;
            }
            return false;
        }
        intersectionTriangleIndex.set(bestTriangle);
        if ( outTriangleIndex != null && outTriangleIndex.length > 0 ) {
            outTriangleIndex[0] = bestTriangle;
        }

        if ( outHit != null ) {
            outHit.setRay(inRay.withT(bestHit.getT()));
            outHit.point = new Vector3Dd(bestHit.getPoint());
            outHit.normal = new Vector3Dd(bestHit.getNormal()).normalized();
            outHit.tangent = new Vector3Dd();
            outHit.u = 0;
            outHit.v = 0;

            interpolateTriangleData(bestTriangle, outHit, inRay.getDirection());
            fillMaterialAndTexture(bestTriangle, outHit);
        }
        return true;
    }

    private void interpolateTriangleData(int triangleIndex, RayHit outHit, Vector3Dd rayDirection)
    {
        if ( vertexNormals == null || vertexUvs == null ) {
            if ( outHit.normal.dotProduct(rayDirection) >= 0 ) {
                outHit.normal = outHit.normal.multiply(-1);
            }
            return;
        }

        Vector3Dd p0 = new Vector3Dd(
            vertexPositions[3*triangleIndexes[3*triangleIndex+0]+0],
            vertexPositions[3*triangleIndexes[3*triangleIndex+0]+1],
            vertexPositions[3*triangleIndexes[3*triangleIndex+0]+2]);
        Vector3Dd p1 = new Vector3Dd(
            vertexPositions[3*triangleIndexes[3*triangleIndex+1]+0],
            vertexPositions[3*triangleIndexes[3*triangleIndex+1]+1],
            vertexPositions[3*triangleIndexes[3*triangleIndex+1]+2]);
        Vector3Dd p2 = new Vector3Dd(
            vertexPositions[3*triangleIndexes[3*triangleIndex+2]+0],
            vertexPositions[3*triangleIndexes[3*triangleIndex+2]+1],
            vertexPositions[3*triangleIndexes[3*triangleIndex+2]+2]);

        double A = p0.x() - p2.x();
        double B = p1.x() - p2.x();
        double C = p2.x() - outHit.point.x();
        double D = p0.y() - p2.y();
        double E = p1.y() - p2.y();
        double F = p2.y() - outHit.point.y();
        double G = p0.z() - p2.z();
        double H = p1.z() - p2.z();
        double I = p2.z() - outHit.point.z();

        double lambda0 = (B*(F+I)-C*(E+H))/(A*(E+H)-B*(D+G));
        double lambda1 = (A*(F+I)-C*(D+G))/(B*(D+G)-A*(E+H));
        double lambda2 = 1-lambda0-lambda1;

        Vector3Dd n0 = new Vector3Dd(
            vertexNormals[3*triangleIndexes[3*triangleIndex+0]+0],
            vertexNormals[3*triangleIndexes[3*triangleIndex+0]+1],
            vertexNormals[3*triangleIndexes[3*triangleIndex+0]+2]);
        Vector3Dd n1 = new Vector3Dd(
            vertexNormals[3*triangleIndexes[3*triangleIndex+1]+0],
            vertexNormals[3*triangleIndexes[3*triangleIndex+1]+1],
            vertexNormals[3*triangleIndexes[3*triangleIndex+1]+2]);
        Vector3Dd n2 = new Vector3Dd(
            vertexNormals[3*triangleIndexes[3*triangleIndex+2]+0],
            vertexNormals[3*triangleIndexes[3*triangleIndex+2]+1],
            vertexNormals[3*triangleIndexes[3*triangleIndex+2]+2]);
        outHit.normal = n0.multiply(lambda0)
            .add(n1.multiply(lambda1)
            .add(n2.multiply(lambda2)))
            .normalized();

        double u0 = vertexUvs[2*triangleIndexes[3*triangleIndex+0]+0];
        double v0 = vertexUvs[2*triangleIndexes[3*triangleIndex+0]+1];
        double u1 = vertexUvs[2*triangleIndexes[3*triangleIndex+1]+0];
        double v1 = vertexUvs[2*triangleIndexes[3*triangleIndex+1]+1];
        double u2 = vertexUvs[2*triangleIndexes[3*triangleIndex+2]+0];
        double v2 = vertexUvs[2*triangleIndexes[3*triangleIndex+2]+1];
        outHit.u = u0*lambda0 + u1*lambda1 + u2*lambda2;
        outHit.v = v0*lambda0 + v1*lambda1 + v2*lambda2;

        if ( outHit.normal.dotProduct(rayDirection) >= 0 ) {
            outHit.normal = outHit.normal.multiply(-1);
        }
    }

    private void fillMaterialAndTexture(int triangleIndex, RayHit outHit)
    {
        if ( materials != null ) {
            outHit.material = materials[0];
        }
        if ( materialRanges != null ) {
            for ( int i = 0; i < materialRanges.length-1 ; i++ ) {
                if ( triangleIndex >= materialRanges[i][0] &&
                     triangleIndex < materialRanges[i+1][0] ) {
                    outHit.material = materials[materialRanges[i+1][1]];
                    break;
                }
            }
        }

        outHit.texture = null;
        if ( textureRanges != null ) {
            for ( int i = 0; i < textureRanges.length-1 ; i++ ) {
                if ( triangleIndex >= textureRanges[i][0] &&
                     triangleIndex < textureRanges[i+1][0] ) {
                    outHit.texture = textures[textureRanges[i+1][1]-1];
                    break;
                }
            }
        }
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doExtraInformation.
    @param inT
    */
    public void
    doExtraInformation(Ray inRay, double inT,
                                   RayHit outData) {
        if ( outData == null ) {
            return;
        }
        Ray queryRay = inRay.withT(inT);
        doIntersectionFirstHit(queryRay, outData);
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
        int i;
        int status;
        Vector3Dd p0, p1, p2;

        p0 = new Vector3Dd();
        p1 = new Vector3Dd();
        p2 = new Vector3Dd();

        for ( i = 0; i < getNumTriangles(); i++ ) {
            p0 = new Vector3Dd(vertexPositions[3*triangleIndexes[3*i+0]+0], vertexPositions[3*triangleIndexes[3*i+0]+1], vertexPositions[3*triangleIndexes[3*i+0]+2]);
            p1 = new Vector3Dd(vertexPositions[3*triangleIndexes[3*i+1]+0], vertexPositions[3*triangleIndexes[3*i+1]+1], vertexPositions[3*triangleIndexes[3*i+1]+2]);
            p2 = new Vector3Dd(vertexPositions[3*triangleIndexes[3*i+2]+0], vertexPositions[3*triangleIndexes[3*i+2]+1], vertexPositions[3*triangleIndexes[3*i+2]+2]);
            status = Triangle.containmentTest(
                p0, p1, p2, p, distanceTolerance);
            if ( status != OUTSIDE ) {
                return LIMIT;
            }
        }

        return OUTSIDE;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.getMinMax.
    @return a new 6 valued double array containing the coordinates of a min-max
    bounding box for current geometry.
    */
    @Override
    public double[] getMinMax() {
        return calculateMinMaxPositions();
    }

    public TriangleMeshGroup exportToTriangleMeshGroup()
    {
        TriangleMeshGroup group = new TriangleMeshGroup();
        group.addMesh(this);
        return group;
    }

    /**
    This method does the following:
    1. Detect all triangles with "incorrect" vertex indices and removes them.
    2. Detect all vertices without incident triangles.
    3. Reacomodate mesh representation arrays to fit the new situation. This
       can imply that some triangle indices gets updated.
    */
    public void compact()
    {
        accelerationStructure = null;
        vertexNormals = null;

        //- Count triangle incidences on vertices -------------------------
        boolean count[];
        int i;
        int a, b, c;
        int n;

        n = vertexPositions.length/3;
        count = new boolean[n];

        for ( i = 0; i < n; i++ ) {
            count[i] = false;
        }

        for ( i = 0; i < triangleIndexes.length; i++ ) {
            a = triangleIndexes[i];
            if ( a >= 0 && a < n ) {
                count[a] = true;
            }
        }

        //- Calculate new index map ---------------------------------------
        int j = 0;
        int map[];

        map = new int[n];
        for ( i = 0; i < n; i++ ) {
            if ( count[i] ) {
                map[i] = j;
                j++;
            }
            else {
                map[i] = -1;
            }
        }

        //- Calculate new vertex arrays -----------------------------------
        double oldVertexPositions[] = vertexPositions;
        double oldVertexNormals[] = vertexNormals;

        initVertexPositionsArray(j);
        if ( oldVertexNormals != null ) {
            initVertexNormalsArray();
        }

        j = 0;
        for ( i = 0; i < n; i++ ) {
            if ( count[i] ) {
                vertexPositions[3*j+0] = oldVertexPositions[3*i+0];
                vertexPositions[3*j+1] = oldVertexPositions[3*i+1];
                vertexPositions[3*j+2] = oldVertexPositions[3*i+2];
                if ( oldVertexNormals != null ) {
                    vertexNormals[3*j+0] = oldVertexNormals[3*i+0];
                    vertexNormals[3*j+1] = oldVertexNormals[3*i+1];
                    vertexNormals[3*j+2] = oldVertexNormals[3*i+2];
                }
                j++;
            }
        }

        //- Calculate new triangle indices arrays -------------------------
        j = 0;
        int oldTriangleIndices[] = triangleIndexes;

        for ( i = 0; i < oldTriangleIndices.length/3; i++ ) {
            a = oldTriangleIndices[3*i+0];
            b = oldTriangleIndices[3*i+1];
            c = oldTriangleIndices[3*i+2];
            if ( a < 0 || a >= n || b < 0 || b >= n || c < 0 || c >= n ) {
                // This triangle is not copied
            }
            else {
                j++;
            }
        }

        initTriangleArrays(j);
        j = 0;
        for ( i = 0; i < oldTriangleIndices.length/3; i++ ) {
            a = oldTriangleIndices[3*i+0];
            b = oldTriangleIndices[3*i+1];
            c = oldTriangleIndices[3*i+2];
            if ( a < 0 || a >= n || b < 0 || b >= n || c < 0 || c >= n ) {
                // This triangle is not copied
            }
            else {
                triangleIndexes[3*j+0] = map[oldTriangleIndices[3*i+0]];
                triangleIndexes[3*j+1] = map[oldTriangleIndices[3*i+1]];
                triangleIndexes[3*j+2] = map[oldTriangleIndices[3*i+2]];
                j++;
            }
        }

    }

    /**
    This method removes all currently selected vertices, and its
    corresponding incident triangles.

    Note that this method also removes "dangling" vertices, selected or not.
    */
    public void removeSelectedVertices()
    {
        accelerationStructure = null;
        //-----------------------------------------------------------------
        if ( vertexSelections == null ) {
            return;
        }

        vertexNormals = null;
        vertexBinormals = null;
        vertexTangents = null;

        //-----------------------------------------------------------------
        int i, a, b, c, n;
        n = vertexPositions.length/3;
        for ( i = 0; i < triangleIndexes.length/3; i++ ) {
            a = triangleIndexes[3*i+0];
            b = triangleIndexes[3*i+1];
            c = triangleIndexes[3*i+2];
            if ( a < 0 || a >= n || b < 0 || b >= n || c < 0 || c >= n ||
                 vertexSelections[a] || vertexSelections[b] ||
                 vertexSelections[c] ) {
                triangleIndexes[3*i+0] = -1;
                triangleIndexes[3*i+1] = -1;
                triangleIndexes[3*i+2] = -1;
            }
        }

        //-----------------------------------------------------------------
        compact();

/*
        int i;
        for ( i = 0; i < vertexPositions.length/3; i++ ) {
            if ( vertexSelections[i] ) {
                vertexPositions[3*i] += 0.01;
            }
        }
*/
        //-----------------------------------------------------------------
        vertexSelections = null;

        calculateNormals();
    }

    private void appendVertices(ArrayListOfDoubles ev)
    {
        int i;
        double newVertexPositions[] = new double[vertexPositions.length + ev.size()];

        for ( i = 0; i < vertexPositions.length; i++ ) {
            newVertexPositions[i] = vertexPositions[i];
        }
        for ( i = 0; i < ev.size(); i++ ) {
            newVertexPositions[i+vertexPositions.length] = ev.get(i);
        }

        vertexPositions = newVertexPositions;
    }

    private void appendTriangles(ArrayListOfInts et)
    {
        int i;
        int newTriangleIndices[] = new int[triangleIndexes.length + et.size()];

        for ( i = 0; i < triangleIndexes.length; i++ ) {
            newTriangleIndices[i] = triangleIndexes[i];
        }

        for ( i = 0; i < et.size(); i++ ) {
            newTriangleIndices[i+triangleIndexes.length] = et.get(i);
        }

        triangleIndexes = newTriangleIndices;
    }

    private void
    simpleTriangleCut(InfinitePlane p,
                      ArrayListOfDoubles extraVertices,
                      ArrayListOfInts extraTriangles,
                      int nv, int i,
                      Vector3Dd p1, Vector3Dd p2, Vector3Dd p3)
    {
        Vector3Dd a, b, ma = null, mb = null;
        RayHit gia = new RayHit();
        RayHit gib = new RayHit();
        a = (p2.subtract(p1));
        b = (p3.subtract(p1));
        a = a.normalized();
        b = b.normalized();

        Ray ra = new Ray(p1, a);
        Ray rb = new Ray(p1, b);

        Ray hitA = p.doIntersectionWithNegative(ra);
        if ( hitA != null ) {
            if ( p.doIntersectionFirstHit(hitA, gia) ) {
                ma = gia.point;
            }
        }
        Ray hitB = p.doIntersectionWithNegative(rb);
        if ( hitB != null ) {
            if ( p.doIntersectionFirstHit(hitB, gib) ) {
                mb = gib.point;
            }
        }

        extraTriangles.add(i);

        extraTriangles.add(((extraVertices.size()/3 + nv)));

        if ( ma == null || mb == null /*|| extraVertices == null*/ ) {
            Logger.reportMessage(this, VSDK.WARNING, "simpleTriangleCut",
            "extraVertices is null!");
            return;
	    }

        extraVertices.add(ma.x());
        extraVertices.add(ma.y());
        extraVertices.add(ma.z());

        extraTriangles.add(((extraVertices.size()/3 + nv)));

        extraVertices.add(mb.x());
        extraVertices.add(mb.y());
        extraVertices.add(mb.z());

        triangleIndexes[3*i+0] = -1;
        triangleIndexes[3*i+1] = -1;
        triangleIndexes[3*i+2] = -1;
    }

    private void
    halfTriangleCut(InfinitePlane p,
                    ArrayListOfDoubles extraVertices,
                    ArrayListOfInts extraTriangles, 
                    int nv, int i, int j,
                    Vector3Dd p1, Vector3Dd p2, Vector3Dd p3)
    {
        Vector3Dd a, b, ma = null;
        RayHit gia = new RayHit();
        RayHit gib = new RayHit();
        a = (p2.subtract(p3));
        a = a.normalized();

        Ray ra = new Ray(p2, a);

        Ray hitA = p.doIntersectionWithNegative(ra);
        if ( hitA != null ) {
            if ( p.doIntersectionFirstHit(hitA, gia) ) {
                ma = gia.point;
            }
        }

        extraTriangles.add(i);

        extraTriangles.add(((extraVertices.size()/3 + nv)));

        if ( ma == null /*|| extraVertices == null*/ ) {
            Logger.reportMessage(this, VSDK.WARNING, "simpleTriangleCut",
            "extraVertices is null!");
            return;
    	}

        extraVertices.add(ma.x());
        extraVertices.add(ma.y());
        extraVertices.add(ma.z());

        extraTriangles.add(j);

        triangleIndexes[3*i+0] = -1;
        triangleIndexes[3*i+1] = -1;
        triangleIndexes[3*i+2] = -1;
    }

    private void
    doubleTriangleCut(InfinitePlane p,
                      ArrayListOfDoubles extraVertices,
                      ArrayListOfInts extraTriangles, 
                      int nv, int i, int j,
                      Vector3Dd p1, Vector3Dd p2, Vector3Dd p3)
    {
        Vector3Dd a, b, ma = null, mb = null;
        RayHit gia = new RayHit();
        RayHit gib = new RayHit();
        a = (p1.subtract(p3));
        b = (p2.subtract(p3));
        a = a.normalized();
        b = b.normalized();

        Ray ra = new Ray(p3, a);
        Ray rb = new Ray(p3, b);

        Ray hitA = p.doIntersectionWithNegative(ra);
        if ( hitA != null ) {
            if ( p.doIntersectionFirstHit(hitA, gia) ) {
                ma = gia.point;
            }
        }
        Ray hitB = p.doIntersectionWithNegative(rb);
        if ( hitB != null ) {
            if ( p.doIntersectionFirstHit(hitB, gib) ) {
                mb = gib.point;
            }
        }

        //-----------------------------------------------------------------
        extraTriangles.add(3*triangleIndexes[3*i+0]+0);

        extraTriangles.add(((extraVertices.size()/3 + nv)));

        if ( ma == null || mb == null /*|| extraVertices == null*/ ) {
            Logger.reportMessage(this, VSDK.WARNING, "simpleTriangleCut",
            "extraVertices is null!");
            return;
   	    }

        extraVertices.add(ma.x());
        extraVertices.add(ma.y());
        extraVertices.add(ma.z());

        extraTriangles.add(((extraVertices.size()/3 + nv)));

        extraTriangles.add(i); // for next tri.
        extraTriangles.add(((extraVertices.size()/3 + nv))); 
        extraTriangles.add(j);

        extraVertices.add(mb.x());
        extraVertices.add(mb.y());
        extraVertices.add(mb.z());

        //-----------------------------------------------------------------
        triangleIndexes[3*i+0] = -1;
        triangleIndexes[3*i+1] = -1;
        triangleIndexes[3*i+2] = -1;
    }

    /**
    Given current mesh and a plane p, this method modifies the current mesh,
    leaving on it only the triangles which lies on the INSIDE part of the
    plane `p`. For triangles cutting the plane, new triangles are generated.
    @param p
    */
    public void
    slice(InfinitePlane p)
    {
        int i;
        Vector3Dd p1, p2, p3;
        p1 = new Vector3Dd();
        p2 = new Vector3Dd();
        p3 = new Vector3Dd();
        int t1, t2, t3;

        ArrayListOfInts extraTriangles = new ArrayListOfInts(10);
        ArrayListOfDoubles extraVertices = new ArrayListOfDoubles(10);

        int nv = vertexPositions.length/3;

        for ( i = 0; i < triangleIndexes.length/3; i++ ) {

            p1 = new Vector3Dd(vertexPositions[3*triangleIndexes[3*i+0]+0], vertexPositions[3*triangleIndexes[3*i+0]+1], vertexPositions[3*triangleIndexes[3*i+0]+2]);

            p2 = new Vector3Dd(vertexPositions[3*triangleIndexes[3*i+1]+0], vertexPositions[3*triangleIndexes[3*i+1]+1], vertexPositions[3*triangleIndexes[3*i+1]+2]);

            p3 = new Vector3Dd(vertexPositions[3*triangleIndexes[3*i+2]+0], vertexPositions[3*triangleIndexes[3*i+2]+1], vertexPositions[3*triangleIndexes[3*i+2]+2]);

            t1 = p.doContainmentTestHalfSpace(p1, VSDK.EPSILON);
            t2 = p.doContainmentTestHalfSpace(p2, VSDK.EPSILON);
            t3 = p.doContainmentTestHalfSpace(p3, VSDK.EPSILON);

            if ( (t1 == OUTSIDE && t2 == OUTSIDE && t3 == OUTSIDE) ||
                 (t1 == LIMIT && t2 == OUTSIDE && t3 == OUTSIDE) ||
                 (t1 == OUTSIDE && t2 == LIMIT && t3 == OUTSIDE) ||
                 (t1 == OUTSIDE && t2 == OUTSIDE && t3 == LIMIT) ||
                 (t1 == LIMIT && t2 == LIMIT && t3 == OUTSIDE) ||
                 (t1 == LIMIT && t2 == OUTSIDE && t3 == LIMIT) ||
                 (t1 == OUTSIDE && t2 == LIMIT && t3 == LIMIT) 
               ) {
                triangleIndexes[3*i+0] = -1;
                triangleIndexes[3*i+1] = -1;
                triangleIndexes[3*i+2] = -1;
            }
            else if ( t1 == LIMIT && t2 == LIMIT && t3 == LIMIT ) {
                Vector3Dd a, b, n;
                a = p2.subtract(p1);
                b = p3.subtract(p1);
                n = a.crossProduct(b);
                if ( n.dotProduct(p.getNormal()) > 0 ) {
                    triangleIndexes[3*i+0] = -1;
                    triangleIndexes[3*i+1] = -1;
                    triangleIndexes[3*i+2] = -1;
                }
            }
            else if (
                 (t1 == LIMIT && t2 == INSIDE && t3 == INSIDE) ||
                 (t1 == INSIDE && t2 == LIMIT && t3 == INSIDE) ||
                 (t1 == INSIDE && t2 == INSIDE && t3 == LIMIT) ||
                 (t1 == LIMIT && t2 == LIMIT && t3 == INSIDE) ||
                 (t1 == LIMIT && t2 == INSIDE && t3 == LIMIT) ||
                 (t1 == INSIDE && t2 == LIMIT && t3 == LIMIT) ||
                 (t1 == INSIDE && t2 == INSIDE && t3 == INSIDE)
                     )
            {
                // Untested?
            }
            else {
                if ( t1 == INSIDE && t2 == OUTSIDE && t3 == OUTSIDE ) {
                    simpleTriangleCut(p, extraVertices, extraTriangles, nv, 3*triangleIndexes[3*i+0]+0, p1, p2, p3);
                }
                else if ( t2 == INSIDE && t1 == OUTSIDE && t3 == OUTSIDE ) {
                    simpleTriangleCut(p, extraVertices, extraTriangles, nv, 3*triangleIndexes[3*i+0]+0, p2, p1, p3);
                }
                else if ( t3 == INSIDE && t1 == OUTSIDE && t2 == OUTSIDE ) {
                    simpleTriangleCut(p, extraVertices, extraTriangles, nv, 3*triangleIndexes[3*i+0]+0, p3, p1, p2);
                }
                else if ( t1 == INSIDE && t2 == INSIDE && t3 == OUTSIDE ) {
                    doubleTriangleCut(p, extraVertices, extraTriangles, nv, 3*triangleIndexes[3*i+0]+0, 3*triangleIndexes[3*i+0]+1, p1, p2, p3);
                }
                else if ( t1 == INSIDE && t3 == INSIDE && t2 == OUTSIDE ) {
                    doubleTriangleCut(p, extraVertices, extraTriangles, nv, 3*triangleIndexes[3*i+0]+0, 3*triangleIndexes[3*i+0]+2, p1, p3, p2);
                }
                else if ( t2 == INSIDE && t3 == INSIDE && t1 == OUTSIDE ) {
                    doubleTriangleCut(p, extraVertices, extraTriangles, nv, 3*triangleIndexes[3*i+0]+1, 3*triangleIndexes[3*i+0]+2, p2, p3, p1);
                }


                else if ( t1 == INSIDE && t2 == LIMIT && t3 == OUTSIDE ) {
                    halfTriangleCut(p, extraVertices, extraTriangles, nv, 3*triangleIndexes[3*i+0]+0, 3*triangleIndexes[3*i+0]+1, p2, p1, p3);
                }
                else if ( t2 == INSIDE && t1 == LIMIT && t3 == OUTSIDE ) {
                    halfTriangleCut(p, extraVertices, extraTriangles, nv, 3*triangleIndexes[3*i+0]+0, 3*triangleIndexes[3*i+0]+0, p1, p2, p3);
                }
                else if ( t2 == INSIDE && t3 == LIMIT && t1 == OUTSIDE ) {
                    halfTriangleCut(p, extraVertices, extraTriangles, nv, 3*triangleIndexes[3*i+0]+0, 3*triangleIndexes[3*i+0]+2, p3, p2, p1);
                }
                else if ( t3 == INSIDE && t2 == LIMIT && t1 == OUTSIDE ) {
                    System.out.println("MyCase4");
                    halfTriangleCut(p, extraVertices, extraTriangles, nv, 3*triangleIndexes[3*i+0]+0, 3*triangleIndexes[3*i+0]+1, p2, p3, p1);
                }
                else if ( t3 == INSIDE && t1 == LIMIT && t2 == OUTSIDE ) {
                    halfTriangleCut(p, extraVertices, extraTriangles, nv, 3*triangleIndexes[3*i+0]+0, 3*triangleIndexes[3*i+0]+0, p1, p3, p2);
                }
                else if ( t1 == INSIDE && t3 == LIMIT && t2 == OUTSIDE ) {
                    halfTriangleCut(p, extraVertices, extraTriangles, nv, 3*triangleIndexes[3*i+0]+0, 3*triangleIndexes[3*i+0]+2, p3, p1, p2);
                }
                else {
                    System.out.println("Unhandled case: ");
                    System.out.println("  - P1: " + t1);
                    System.out.println("  - P2: " + t2);
                    System.out.println("  - P3: " + t3);
                }
            }
        }

        //-----------------------------------------------------------------
        appendVertices(extraVertices);
        appendTriangles(extraTriangles);

        //-----------------------------------------------------------------
        compact();

        //-----------------------------------------------------------------
        calculateNormals();
    }
}

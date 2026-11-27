//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =
//= [.wMANT2008] Mantyla Martti. "Personal Home Page", <<shar>> archive     =
//=     containing the C programs from [MANT1988]. Available at             =
//=     http://www.cs.hut.fi/~mam . Last visited April 12 / 2008.           =

package vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid;

// Java classes
import java.io.Serial;
import java.util.ArrayList;
import java.util.List;

// Vitral classes
import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.environment.geometry.element.Vertex2D;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.dataStructures.CircularArrayList;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.environment.geometry.Geometry;
import vsdk.toolkit.environment.geometry.element.RayHit;
import vsdk.toolkit.environment.geometry.surface.InfinitePlane;
import vsdk.toolkit.environment.geometry.volume.Solid;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidFace;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidLoop;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidHalfEdge;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidEdge;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidVertex;
import vsdk.toolkit.processing.ComputationalGeometry;
import vsdk.toolkit.environment.geometry.surface.polygon.Polygon2D;

/**
This class encapsulates a polyhedral boundary representation for 2-manifold
solids, as presented in [MANT1988].

As noted in [MANT1988].6.2.1., a "polyhedral model" is a boundary model
that has only planar faces. So, the name of this class `PolyhedralBoundedSolid`
implies that its faces should be planar. However, some intermediate steps
in complex algorithms such as the splitter and the set operators, permits
the use of "special" non-planar faces for "gluing".  Check [MANT1988] book
for more details on that.

As noted in [MANT1988].10.2.1, current implementation of the
`PolyhedralBoundedSolid` class uses a five-level hierarchical data
structure, consisting of:
  - PolyhedralBoundedSolid
  - _PolyhedralBoundedSolidFace
  - _PolyhedralBoundedSolidLoop
  - _PolyhedralBoundedSolidHalfEdge (and _PolyhedralBoundedSolidEdge)
  - _PolyhedralBoundedSolidVertex
Current class forms the root element (Facade) that gives access to faces,
edges and vertices of the model through aggregations in
CircularArrayList's.

Note that this is a quite complex data-structure. Its implementation follows
the strategies outlined on book [MANT1988]. For the sake of clarity, it
was decided to keep most of its internal datastructures public, breaking
so the encapsulation concept. Note that if internal data structures are
made private and accessing get/set methods are provided for them, then
the complexity of algorithms using current data-structure should become
unmanageable, both in terms of code verbosity and bad performance (time
complexity) due to extra calls to a lot of simple methods.
*/
public class PolyhedralBoundedSolid extends Solid {
    @Serial private static final long serialVersionUID = 20061118L;

    public static final int PLUS = 1;
    public static final int MINUS = 0;
    private static final String SOLID_WITH_MESSAGE = "Solid with ";

    //= Main boundary representation solid data structure =============
    private CircularArrayList<_PolyhedralBoundedSolidFace> polygonsList;
    private CircularArrayList<_PolyhedralBoundedSolidEdge> edgesList;
    private CircularArrayList<_PolyhedralBoundedSolidVertex> verticesList;
    private int maxVertexId;
    private int maxFaceId;
    private boolean modelIsValid;

    //=================================================================
    public PolyhedralBoundedSolid()
    {
        polygonsList = new CircularArrayList<>();
        edgesList = new CircularArrayList<>();
        verticesList = new CircularArrayList<>();
        maxVertexId = -1;
        maxFaceId = -1;
        modelIsValid = false;
    }

    //= SUPPORT MACROS FOR BASIC DATA-STRUCTURE MANIPULATION ==========

    /**
    Find the face identified with `id`. Returns null if face not found,
    or current founded face otherwise.
    Build based over function `fface` in program [MANT1988].11.9.
    @param id face id to search.
    @return matching face, or null when not found.
    */
    public _PolyhedralBoundedSolidFace
    findFace(int id)
    {
        int i;
        _PolyhedralBoundedSolidFace face;

        for ( i = 0; i < polygonsList.size(); i++ ) {
            face = polygonsList.get(i);
            if ( face.id == id ) {
                return face;
            }
        }
        return null;
    }

    /**
    Finds the vertex identified by `id`.
    @param id vertex id to search.
    @return matching vertex, or null when not found.
    */
    public _PolyhedralBoundedSolidVertex
    findVertex(int id)
    {
        int i;
        _PolyhedralBoundedSolidVertex v;

        for ( i = 0; i < verticesList.size(); i++ ) {
            v = verticesList.get(i);
            if ( v.id == id ) {
                return v;
            }
        }
        return null;
    }

    /**
    Returns the list of faces stored in this solid.
    @return internal face list.
    */
    public CircularArrayList<_PolyhedralBoundedSolidFace>
    getPolygonsList()
    {
        return polygonsList;
    }

    /**
    Replaces the internal face list.
    @param polygonsList new face list.
    */
    public void setPolygonsList(
        CircularArrayList<_PolyhedralBoundedSolidFace> polygonsList)
    {
        this.polygonsList = polygonsList;
    }

    /**
    Returns the list of edges stored in this solid.
    @return internal edge list.
    */
    public CircularArrayList<_PolyhedralBoundedSolidEdge>
    getEdgesList()
    {
        return edgesList;
    }

    /**
    Replaces the internal edge list.
    @param edgesList new edge list.
    */
    public void setEdgesList(
        CircularArrayList<_PolyhedralBoundedSolidEdge> edgesList)
    {
        this.edgesList = edgesList;
    }

    /**
    Returns the list of vertices stored in this solid.
    @return internal vertex list.
    */
    public CircularArrayList<_PolyhedralBoundedSolidVertex>
    getVerticesList()
    {
        return verticesList;
    }

    /**
    Replaces the internal vertex list.
    @param verticesList new vertex list.
    */
    public void setVerticesList(
        CircularArrayList<_PolyhedralBoundedSolidVertex> verticesList)
    {
        this.verticesList = verticesList;
    }

    //=================================================================

    /**
    This method gives access to the higher vertex id used in current solid
    model. This method is useful for higher level modeling operations, as
    noted in section [MANT1988].12.2. Current method (and method getMaxFaceId)
    is build after the function `getmaxnames` of program [MANT1988].12.1.
    @return the maximum id used in vertices set
    */
    public int getMaxVertexId()
    {
        return maxVertexId;
    }

    public void setMaxVertexId(int maxVertexId)
    {
        this.maxVertexId = maxVertexId;
    }

    /**
    This method gives access to the higher face id used in current solid
    model. This method is useful for higher level modeling operations, as
    noted in section [MANT1988].12.2. Current method (and method
    getMaxVertexId) is build after the function `getmaxnames` of program
    [MANT1988].12.1.
    @return the maximum id used on faces set
    */
    public int getMaxFaceId()
    {
        return maxFaceId;
    }

    public void setMaxFaceId(int maxFaceId)
    {
        this.maxFaceId = maxFaceId;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doIntersectionFirstHit.
    @param inOutRay input ray to test.
    @return hit ray with updated `t` when intersection exists, or null otherwise.
    */
    public Ray doIntersectionFirstHit(Ray inOutRay) {
        RayHit hit = new RayHit(RayHit.DETAIL_NONE, true);
        if ( doIntersectionFirstHit(inOutRay, hit) ) {
            return hit.getRay();
        }
        return null;
    }

    @Override
    public boolean doIntersectionFirstHit(Ray inRay, RayHit outHit)
    {
        int i;
        double minT; // Shortest distance founded so far
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext =
            PolyhedralBoundedSolidNumericPolicy.forSolid(this);

        // Initialization values for search algorithm
        minT = Double.MAX_VALUE;
        RayHit bestInfo = null;
        Vector3Dd p;
        int pos;

        for ( i = 0; i < polygonsList.size(); i++ ) {
            Ray ray = new Ray(inRay);
            _PolyhedralBoundedSolidFace face = polygonsList.get(i);
            InfinitePlane containingPlane = face.getContainingPlane();
            if ( containingPlane == null ) {
                continue;
            }
            RayHit planeHit = new RayHit();
            if ( containingPlane.doIntersectionFirstHit(ray, planeHit) ) {
                Ray hit = planeHit.getRay();
                if ( hit.getT() < minT ) {
                    hit = hit.withDirection(hit.getDirection().normalized());
                    p = hit.getOrigin().add(hit.getDirection().multiply(hit.getT()));
                    pos = testPointInsideForRayIntersection(
                        face, p, numericContext.bigEpsilon());
                    if ( pos == Geometry.INSIDE || pos == Geometry.LIMIT ) {
                        minT = hit.getT();
                        bestInfo = new RayHit(planeHit);
                    }
                }
            }
        }

        if ( bestInfo == null ) {
            return false;
        }
        if ( outHit != null ) {
            outHit.clone(bestInfo);
            outHit.setRay(inRay.withT(minT));
        }
        return true;
    }

    private static Vector3Dd dropCoordinate(Vector3Dd in, int coordinate)
    {
        return switch (coordinate) {
            case 1 -> new Vector3Dd(in.y(), in.z(), 0);
            case 2 -> new Vector3Dd(in.x(), in.z(), 0);
            default -> new Vector3Dd(in.x(), in.y(), 0);
        };
    }

    private static int dominantCoordinateForFace(_PolyhedralBoundedSolidFace face)
    {
        Vector3Dd n = face.getContainingPlane().getNormal();

        if ( Math.abs(n.x()) >= Math.abs(n.y()) &&
             Math.abs(n.x()) >= Math.abs(n.z()) ) {
            return 1;
        }
        if ( Math.abs(n.y()) >= Math.abs(n.x()) &&
             Math.abs(n.y()) >= Math.abs(n.z()) ) {
            return 2;
        }
        return 3;
    }

    private static int testPointInsideForRayIntersection(
        _PolyhedralBoundedSolidFace face,
        Vector3Dd point,
        double tolerance)
    {
        int dominantCoordinate;
        int insideLoopCount;
        int i;
        Vector3Dd projectedPoint;
        Vertex2D projectedPoint2D;

        if ( face == null || face.getContainingPlane() == null ) {
            return Geometry.OUTSIDE;
        }

        dominantCoordinate = dominantCoordinateForFace(face);
        projectedPoint = dropCoordinate(point, dominantCoordinate);
        projectedPoint2D = new Vertex2D(projectedPoint.x(), projectedPoint.y());
        insideLoopCount = 0;

        for ( i = 0; i < face.boundariesList.size(); i++ ) {
            _PolyhedralBoundedSolidLoop loop = face.boundariesList.get(i);
            _PolyhedralBoundedSolidHalfEdge he = loop.boundaryStartHalfEdge;
            _PolyhedralBoundedSolidHalfEdge start;
            List<Vertex2D> projectedLoopVertices;
            byte loopStatus;

            if ( he == null ) {
                return Geometry.OUTSIDE;
            }
            start = he;
            projectedLoopVertices = new ArrayList<>();

            do {
                if ( Vector3Dd.distance(point, he.startingVertex.position)
                     < 2 * tolerance ) {
                    return Geometry.LIMIT;
                }
                if ( ComputationalGeometry.lineSegmentContainmentTest(
                         he.startingVertex.position,
                         he.next().startingVertex.position,
                         point, tolerance) == Geometry.LIMIT ) {
                    return Geometry.LIMIT;
                }

                projectedPoint = dropCoordinate(he.startingVertex.position,
                    dominantCoordinate);
                projectedLoopVertices.add(
                    new Vertex2D(projectedPoint.x(), projectedPoint.y()));
                he = he.next();
            } while ( he != start );

            loopStatus = Polygon2D.isPointInsideContour(
                projectedPoint2D, projectedLoopVertices);
            if ( loopStatus == 0 ) {
                return Geometry.LIMIT;
            }
            if ( loopStatus > 0 ) {
                insideLoopCount++;
            }
        }

        return ((insideLoopCount % 2) == 1) ?
            Geometry.INSIDE : Geometry.OUTSIDE;
    }

    /**
    Fills `outData` with intersection details for the provided ray and distance.
    @param inRay input ray.
    @param inT ray parameter used as candidate intersection distance.
    @param outData output hit information container.
    */
    @Override
    public void doExtraInformation(Ray inRay, double inT, 
                                  RayHit outData) {
        if ( outData == null ) {
            return;
        }
        doIntersectionFirstHit(inRay.withT(inT), outData);
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

        for ( i = 0; i < verticesList.size(); i++ ) {
            _PolyhedralBoundedSolidVertex v;
            v = verticesList.get(i);
            double x = v.position.x();
            double y = v.position.y();
            double z = v.position.z();

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
    Check the general interface contract in superclass method
    Geometry.getMinMax.
    @return six-value array with min/max coordinates: `[minX, minY, minZ, maxX, maxY, maxZ]`.
    */
    @Override
    public double[] getMinMax() {
        return calculateMinMaxPositions();
    }

    /**
    Returns true if the model was validated using
    `PolyhedralBoundedSolidValidationEngine.validateIntermediate` or
    `PolyhedralBoundedSolidValidationEngine.validateStrict`, and validation
    succeeded after the latest geometrical or topological operation.
    @return true when the cached validation flag is valid; false otherwise.
    */
    public boolean isValid()
    {
        return modelIsValid;
    }

    void setValidationState(boolean flag)
    {
        modelIsValid = flag;
    }

    /**
    Given `this` and `other` solids, this method erases the `other` solid
    while appending its parts to current one as a new shell. This method
    follows section [MANT1988].12.4.1 and program [MANT1988].12.8.
    @param other solid whose topology is moved into this solid.
    */
    public void merge(PolyhedralBoundedSolid other)
    {
        //-----------------------------------------------------------------
        int offsetFacesId = getMaxFaceId();
        int offsetVertexId = getMaxVertexId();
        _PolyhedralBoundedSolidFace f;
        _PolyhedralBoundedSolidVertex v;

        //-----------------------------------------------------------------
        while ( other.getPolygonsList().size() > 0 ) {
            f = other.getPolygonsList().get(0);
            f.id += offsetFacesId;
            // The merged face now belongs to this solid: Euler operators
            // (i.e. `lkfmrh` when gluing) check `parentSolid`
            f.parentSolid = this;
            if ( f.id > maxFaceId ) maxFaceId = f.id;
            polygonsList.add(f);
            other.getPolygonsList().remove(0);
        }
        while ( other.getEdgesList().size() > 0 ) {
            edgesList.add(other.getEdgesList().get(0));
            other.getEdgesList().remove(0);
        }
        while ( other.getVerticesList().size() > 0 ) {
            v = other.getVerticesList().get(0);
            v.id += offsetVertexId;
            if ( v.id > maxVertexId ) maxVertexId = v.id;
            verticesList.add(v);
            other.getVerticesList().remove(0);
        }
    }

    //= TEXTUAL QUERY OPERATIONS ======================================

    private String intPreSpaces(int val, int fieldSize)
    {
        String cad;
        int remain;
        StringBuilder sb;

        cad = VSDK.formatNumberWithinZeroes(val, 1);
        remain = fieldSize - cad.length();
        sb = new StringBuilder(Math.max(fieldSize, cad.length()));

        for ( ; remain > 0; remain-- ) {
            sb.append(" ");
        }
        sb.append(cad);
        return sb.toString();
    }

    /**
    Check the general interface contract in superclass method
    Geometry.computeQuantitativeInvisibility.

    This is not well understood for cases of intersection with face limits
    (vertices and edges). In some cases, computation of quantitative
    invisibility seems to be failing.
    @param origin ray origin.
    @param p target point.
    @return  the number of front facing surface elements (with
    respect to `origin`) between the `origin` point and the `p` point
    */
    /// Transient per-frame caches for repeated visibility (quantitative
    /// invisibility) queries. The Appel hidden-line renderer issues tens of
    /// thousands of QI samples against a STATIC solid in a single frame; without
    /// these, every sample recomputed each face's containing plane (a Newell fit
    /// + a face-scale estimate) for every one of the four jittered rays — tens of
    /// millions of redundant plane fits. `beginVisibilityQueries` snapshots the
    /// face planes and tolerance once; `endVisibilityQueries` drops the snapshot.
    /// While active the solid MUST NOT be modified.
    private transient InfinitePlane[] queryPlaneCache;
    /// Per-face axis-aligned bounding box (6 doubles: minX,minY,minZ,maxX,maxY,
    /// maxZ) used to cull faces a query ray cannot hit before the plane test.
    private transient double[] queryFaceAabb;
    private transient PolyhedralBoundedSolidNumericPolicy.ToleranceContext
        queryNumericContext;

    /**
    Snapshots per-face containing planes and the tolerance context for a batch of
    visibility queries on this (unchanged) solid. Call once before issuing many
    computeQuantitativeInvisibility queries, then endVisibilityQueries when done.
    */
    public void beginVisibilityQueries()
    {
        queryNumericContext = PolyhedralBoundedSolidNumericPolicy.forSolid(this);
        int faceCount = polygonsList.size();
        queryPlaneCache = new InfinitePlane[faceCount];
        queryFaceAabb = new double[faceCount * 6];
        for ( int i = 0; i < faceCount; i++ ) {
            _PolyhedralBoundedSolidFace face = polygonsList.get(i);
            queryPlaneCache[i] = face.getContainingPlane();
            computeFaceAabb(face, i);
        }
    }

    private void computeFaceAabb(_PolyhedralBoundedSolidFace face, int index)
    {
        double minX = Double.MAX_VALUE;
        double minY = Double.MAX_VALUE;
        double minZ = Double.MAX_VALUE;
        double maxX = -Double.MAX_VALUE;
        double maxY = -Double.MAX_VALUE;
        double maxZ = -Double.MAX_VALUE;
        for ( int b = 0; b < face.boundariesList.size(); b++ ) {
            _PolyhedralBoundedSolidLoop loop = face.boundariesList.get(b);
            if ( loop == null || loop.boundaryStartHalfEdge == null ) {
                continue;
            }
            _PolyhedralBoundedSolidHalfEdge he = loop.boundaryStartHalfEdge;
            _PolyhedralBoundedSolidHalfEdge start = he;
            do {
                if ( he.startingVertex != null ) {
                    Vector3Dd q = he.startingVertex.position;
                    if ( q.x() < minX ) minX = q.x();
                    if ( q.y() < minY ) minY = q.y();
                    if ( q.z() < minZ ) minZ = q.z();
                    if ( q.x() > maxX ) maxX = q.x();
                    if ( q.y() > maxY ) maxY = q.y();
                    if ( q.z() > maxZ ) maxZ = q.z();
                }
                he = he.next();
            } while ( he != null && he != start );
        }
        int o = index * 6;
        queryFaceAabb[o] = minX;
        queryFaceAabb[o + 1] = minY;
        queryFaceAabb[o + 2] = minZ;
        queryFaceAabb[o + 3] = maxX;
        queryFaceAabb[o + 4] = maxY;
        queryFaceAabb[o + 5] = maxZ;
    }

    /**
    Slab test: does the segment from `origin` along unit `direction` for length
    `maxT` reach the cached AABB of face `faceIndex`? A cheap, allocation-free
    reject for faces a query ray cannot hit. Conservative (a small epsilon pad),
    so it never culls a face the ray actually crosses.
    */
    private boolean rayReachesFaceAabb(Vector3Dd origin, double dirX, double dirY,
        double dirZ, double maxT, int faceIndex, double pad)
    {
        int o = faceIndex * 6;
        double tMin = 0.0;
        double tMax = maxT;
        // X slab
        if ( Math.abs(dirX) < 1.0e-12 ) {
            if ( origin.x() < queryFaceAabb[o] - pad ||
                 origin.x() > queryFaceAabb[o + 3] + pad ) {
                return false;
            }
        }
        else {
            double t1 = (queryFaceAabb[o] - pad - origin.x()) / dirX;
            double t2 = (queryFaceAabb[o + 3] + pad - origin.x()) / dirX;
            if ( t1 > t2 ) { double tmp = t1; t1 = t2; t2 = tmp; }
            if ( t1 > tMin ) tMin = t1;
            if ( t2 < tMax ) tMax = t2;
            if ( tMin > tMax ) return false;
        }
        // Y slab
        if ( Math.abs(dirY) < 1.0e-12 ) {
            if ( origin.y() < queryFaceAabb[o + 1] - pad ||
                 origin.y() > queryFaceAabb[o + 4] + pad ) {
                return false;
            }
        }
        else {
            double t1 = (queryFaceAabb[o + 1] - pad - origin.y()) / dirY;
            double t2 = (queryFaceAabb[o + 4] + pad - origin.y()) / dirY;
            if ( t1 > t2 ) { double tmp = t1; t1 = t2; t2 = tmp; }
            if ( t1 > tMin ) tMin = t1;
            if ( t2 < tMax ) tMax = t2;
            if ( tMin > tMax ) return false;
        }
        // Z slab
        if ( Math.abs(dirZ) < 1.0e-12 ) {
            if ( origin.z() < queryFaceAabb[o + 2] - pad ||
                 origin.z() > queryFaceAabb[o + 5] + pad ) {
                return false;
            }
        }
        else {
            double t1 = (queryFaceAabb[o + 2] - pad - origin.z()) / dirZ;
            double t2 = (queryFaceAabb[o + 5] + pad - origin.z()) / dirZ;
            if ( t1 > t2 ) { double tmp = t1; t1 = t2; t2 = tmp; }
            if ( t1 > tMin ) tMin = t1;
            if ( t2 < tMax ) tMax = t2;
            if ( tMin > tMax ) return false;
        }
        return true;
    }

    /** Releases the visibility-query snapshot taken by beginVisibilityQueries. */
    public void endVisibilityQueries()
    {
        queryPlaneCache = null;
        queryFaceAabb = null;
        queryNumericContext = null;
    }

    // Package-private query accessors so PolyhedralBoundedSolidPredicates can
    // reuse the per-frame snapshot (plane cache + face AABBs + tolerance) taken
    // by beginVisibilityQueries, falling back to on-the-fly computation when no
    // snapshot is active.

    InfinitePlane cachedFacePlane(int faceIndex)
    {
        if ( queryPlaneCache != null && faceIndex < queryPlaneCache.length ) {
            return queryPlaneCache[faceIndex];
        }
        return polygonsList.get(faceIndex).getContainingPlane();
    }

    /** Tolerance context for visibility queries: the cached one if a snapshot is
        active, otherwise computed for this solid. */
    PolyhedralBoundedSolidNumericPolicy.ToleranceContext queryToleranceContext()
    {
        return queryNumericContext != null ? queryNumericContext :
            PolyhedralBoundedSolidNumericPolicy.forSolid(this);
    }

    /** AABB reject for a query ray against face `faceIndex`. Returns true (do not
        cull) when no AABB snapshot is active. */
    boolean queryRayReachesFace(Vector3Dd origin, double dirX, double dirY,
        double dirZ, double maxT, int faceIndex, double pad)
    {
        if ( queryFaceAabb == null ) {
            return true;
        }
        return rayReachesFaceAabb(origin, dirX, dirY, dirZ, maxT, faceIndex, pad);
    }

    /** AABB reject for a query POINT against face `faceIndex` (is the point in
        the face's padded box?). Returns true (do not cull) when no snapshot is
        active. Used to skip the on-surface test for far faces. */
    boolean queryPointNearFace(Vector3Dd point, int faceIndex, double pad)
    {
        if ( queryFaceAabb == null ) {
            return true;
        }
        int o = faceIndex * 6;
        return point.x() >= queryFaceAabb[o] - pad &&
               point.x() <= queryFaceAabb[o + 3] + pad &&
               point.y() >= queryFaceAabb[o + 1] - pad &&
               point.y() <= queryFaceAabb[o + 4] + pad &&
               point.z() >= queryFaceAabb[o + 2] - pad &&
               point.z() <= queryFaceAabb[o + 5] + pad;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doContainmentTest. Delegated to the robust predicate layer
    (`PolyhedralBoundedSolidPredicates.classifyPoint`): a point on a face is
    `LIMIT`, any other is classified by a ray-cast parity test.
    @param p point to classify, in the space of the solid
    @param distanceTolerance distance to the faces under which the point is
    classified as `LIMIT`
    @return INSIDE, OUTSIDE or LIMIT constant value
    */
    @Override
    public int doContainmentTest(Vector3Dd p, double distanceTolerance)
    {
        if ( p == null ) {
            Logger.reportMessage(this, VSDK.WARNING, "doContainmentTest",
                "Null point given: it is classified as OUTSIDE.");
            return OUTSIDE;
        }
        return PolyhedralBoundedSolidPredicates.classifyPoint(this, p,
            distanceTolerance);
    }

    @Override
    public int computeQuantitativeInvisibility(Vector3Dd origin, Vector3Dd p)
    {
        // Delegated to the robust predicate layer: QI = the number of times the
        // (straight, pulled-back) line of sight enters this solid's interior
        // before reaching the surface point `p`, computed by a boundary-crossing
        // count whose interval classification uses the robust point-in-solid
        // test (PolyhedralBoundedSolidPredicates). This replaces the earlier
        // perpendicular-jitter majority vote, which fabricated phantom occluders
        // at axis-aligned silhouette corners and missed occluders entered through
        // an edge. The per-frame plane/AABB snapshot taken by
        // beginVisibilityQueries is reused by the predicates for speed.
        return PolyhedralBoundedSolidPredicates.quantitativeInvisibility(this,
            origin, p);
    }

    /**
    Utility routine used to compare floating values inside the boundary
    representation winged edge data structure, following procedure `comp`
    from program [MANT1988].13.2.
    @param a first value.
    @param b second value.
    @param tolerance comparison tolerance.
    @return 0 if two numbers are nearly equal, 1 if a > b, -1 if a < b
    */
    public static int compareValue(double a, double b, double tolerance)
    {
        double delta;

        delta = Math.abs(a - b);
        if ( delta < tolerance ) {
            return 0;
        }
        else if ( a > b ) {
            return 1;
        }
        return -1;
    }

    /**
    This method get current solid in an "inverted" (geometrical sense) solid.
    Works on half edge data structure by inverting the order of each loop.
    This is an answer to problem [MANT1988].15.6.

    Current implementation does NOT correct face normals. Explicit model
    validation is encouraged after the application of this method.
    */
    public void revert()
    {
        int i;

        for ( i = 0; i < polygonsList.size(); i++ ) {
            polygonsList.get(i).revert();
        }
    }

    @Override
    public String toString()
    {
        StringBuilder msg = new StringBuilder();
        int i;
        int j;

        msg.append("= POLYHEDRAL BOUNDED SOLID STRUCTURE ==========================================\n");
        msg.append(SOLID_WITH_MESSAGE).append(verticesList.size()).append(" vertices:\n");
        for ( i = 0; i < verticesList.size(); i++ ) {
            _PolyhedralBoundedSolidVertex v;
            v = verticesList.get(i);
            msg.append("  - ").append(v).append("\n");
        }

        msg.append(SOLID_WITH_MESSAGE).append(edgesList.size()).append(" edges:\n");
        for ( i = 0; i < edgesList.size(); i++ ) {
            _PolyhedralBoundedSolidEdge e = edgesList.get(i);
            msg.append("  - ").append(e).append("\n");
        }
        msg.append(SOLID_WITH_MESSAGE).append(polygonsList.size()).append(" faces:\n");

        for ( i = 0; i < polygonsList.size(); i++ ) {
            _PolyhedralBoundedSolidFace face = polygonsList.get(i);
            msg.append("  - ").append(face).append("\n");
            for ( j = 0; j < face.boundariesList.size(); j++ ) {
                _PolyhedralBoundedSolidLoop loop;
                _PolyhedralBoundedSolidHalfEdge he;
                _PolyhedralBoundedSolidHalfEdge heStart;

                msg.append("    . Loop ").append(j).append(", with half-edges: \n");
                loop = face.boundariesList.get(j);


                msg.append("      | HeID  | StartVertex | End Vertex | nccw He | pccw He | parentEdge | mirror He | neighbor face\n");
                msg.append("      +-------+-------------+------------+---------+---------+------------+-----------+-------------+\n");

                he = loop.boundaryStartHalfEdge;
                if ( he == null ) {
                    msg.append("<Loop without starting half-edge!>\n");
                    continue;
                }
                heStart = he;
                do {
                    he = he.next();
                    if ( he == null ) {
                        // Loop is not closed!
                        msg.append("      |  - (not closed loop)\n");
                        break;
                    }

                    msg.append("      | ")
                        .append(intPreSpaces(he.id, 4))
                        .append((he == loop.boundaryStartHalfEdge) ? "*" : " ")
                        .append(" | ")
                        .append(intPreSpaces(he.startingVertex.id, 11))
                        .append(" | ")
                        .append(intPreSpaces(he.next().startingVertex.id, 10))
                        .append(" | ")
                        .append(intPreSpaces(he.next().id, 7))
                        .append(" | ")
                        .append(intPreSpaces(he.previous().id, 7))
                        .append(" | ");
                    msg.append((he.parentEdge!=null)?
                        intPreSpaces(he.parentEdge.id, 10):"    <null>");
                    msg.append(" | ");
                    if ( he.mirrorHalfEdge() != null ) {
                        msg.append(intPreSpaces(he.mirrorHalfEdge().id, 9))
                            .append(" | ")
                            .append(intPreSpaces(he.mirrorHalfEdge().parentLoop.parentFace.id, 11))
                            .append(" | ");
                    }
                    else {
                        msg.append(" No Mirror Half Edge!   | ");
                    }

                    msg.append("\n");

                } while( he != heStart );
            }
        }
        msg.append("= END OF POLYHEDRAL BOUNDED SOLID STRUCTURE ===================================\n");
        return msg.toString();
    }

    @Override
    public PolyhedralBoundedSolid exportToPolyhedralBoundedSolid()
    {
        return this;
    }
}

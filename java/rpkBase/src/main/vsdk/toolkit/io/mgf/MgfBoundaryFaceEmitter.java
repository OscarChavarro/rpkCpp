package vsdk.toolkit.io.mgf;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolid;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidFace;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidHalfEdge;
import vsdk.toolkit.io.context.EntityTypeContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.context.ReaderContext;

/**
Turns the faces of a VITRAL boundary representation, built in the local
frame of a primitive, into MGF faces: for each face, its vertices are
defined (`v`, `p` and, if any, `n` entities, with the values written as the
MGF tessellators always did) and a `f` entity references them. So the faces
go through the same path as the faces of the file: the current transform,
the current material and its sidedness (two sided materials get twin
patches), and the grouping of the primitive in one surface.

Every face gets its own vertices, as with the classic tessellation, where
each `v` entity redefined in it gave a new vertex.

The local frame is given by an origin and three orthonormal axes u, v, w
(right handed), all in MGF coordinates: the local point (x, y, z) is
origin + x * u + y * v + z * w. A reversed emitter turns the surfaces
inside out (MGF primitives with negative radii): it reverses the order of
the vertices of the faces and the vertex normals.
*/
public final class MgfBoundaryFaceEmitter {
    private final Vector3Dd origin;
    private final Vector3Dd u;
    private final Vector3Dd v;
    private final Vector3Dd w;
    private final boolean reversed;

    public MgfBoundaryFaceEmitter(Vector3Dd origin, Vector3Dd u, Vector3Dd v, Vector3Dd w, boolean reversed) {
        this.origin = origin;
        this.u = u;
        this.v = v;
        this.w = w;
        this.reversed = reversed;
    }

    public Vector3Dd toMgfPoint(Vector3Dd localPoint) {
        return new Vector3Dd(
            origin.x() + localPoint.x() * u.x() + localPoint.y() * v.x() + localPoint.z() * w.x(),
            origin.y() + localPoint.x() * u.y() + localPoint.y() * v.y() + localPoint.z() * w.y(),
            origin.z() + localPoint.x() * u.z() + localPoint.y() * v.z() + localPoint.z() * w.z());
    }

    public Vector3Dd toMgfDirection(Vector3Dd localDirection) {
        return new Vector3Dd(
            localDirection.x() * u.x() + localDirection.y() * v.x() + localDirection.z() * w.x(),
            localDirection.x() * u.y() + localDirection.y() * v.y() + localDirection.z() * w.y(),
            localDirection.x() * u.z() + localDirection.y() * v.z() + localDirection.z() * w.z());
    }

    private static String formatted(double value) {
        StringBuilder text = new StringBuilder(MgfTessellationMath.MGF_PV_SIZE);
        MgfTessellationMath.formatFloat(text, MgfTessellationMath.MGF_PV_SIZE, value);
        return text.toString();
    }

    /**
    Emits a face.
    @param numberOfVertices number of vertices of the face
    @param localPoints its vertices in the local frame, counterclockwise seen
    from the outside of the (not reversed) surface
    @param mgfNormals vertex normals in MGF coordinates (oriented as the not
    reversed surface; a null vector for none), or null for no normals
    @return the MGF error code
    */
    public int emitFace(int numberOfVertices, Vector3Dd[] localPoints, Vector3Dd[] mgfNormals, ParseRuntimeContext context) {
        if ( numberOfVertices < 3 || numberOfVertices > ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT - 2 ) {
            return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }

        String[] faceEntity = new String[numberOfVertices + 1];
        faceEntity[0] = context.entityNames[EntityTypeContext.FACE];
        for ( int i = 0; i < numberOfVertices; i++ ) {
            // The vertices of a reversed face are taken backwards
            int k = reversed ? numberOfVertices - 1 - i : i;
            String name = "_bv" + i;
            String[] vertexEntity = {context.entityNames[EntityTypeContext.VERTEX], name, "="};
            int errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.VERTEX, 3, vertexEntity, context);
            if ( errorCode != ParseErrorContext.MGF_OK ) {
                return errorCode;
            }

            Vector3Dd point = toMgfPoint(localPoints[k]);
            String[] pointEntity = {
                context.entityNames[EntityTypeContext.MGF_POINT],
                formatted(point.x()),
                formatted(point.y()),
                formatted(point.z())
            };
            errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_POINT, 4, pointEntity, context);
            if ( errorCode != ParseErrorContext.MGF_OK ) {
                return errorCode;
            }

            if ( mgfNormals != null && !MgfTessellationMath.isNull(mgfNormals[k], 0.0) ) {
                double sign = reversed ? -1.0 : 1.0;
                String[] normalEntity = {
                    context.entityNames[EntityTypeContext.MGF_NORMAL],
                    formatted(sign * mgfNormals[k].x()),
                    formatted(sign * mgfNormals[k].y()),
                    formatted(sign * mgfNormals[k].z())
                };
                errorCode = MgfEntityControl.mgfHandle(EntityTypeContext.MGF_NORMAL, 4, normalEntity, context);
                if ( errorCode != ParseErrorContext.MGF_OK ) {
                    return errorCode;
                }
            }
            faceEntity[i + 1] = name;
        }
        return MgfEntityControl.mgfHandle(EntityTypeContext.FACE, numberOfVertices + 1, faceEntity, context);
    }

    /**
    Collects the vertices of the outer loop of face number `faceIndex` of
    `solid` (in the list of faces), in their order.
    @return the number of vertices (at most `maximumVertices`), or -1 if the
    face has holes or more vertices
    */
    public static int faceVertices(PolyhedralBoundedSolid solid, int faceIndex, Vector3Dd[] localPoints, int maximumVertices) {
        _PolyhedralBoundedSolidFace face = solid.getPolygonsList().get(faceIndex);
        if ( face.boundariesList.size() != 1 ) {
            return -1;
        }
        _PolyhedralBoundedSolidHalfEdge start = face.boundariesList.get(0).boundaryStartHalfEdge;
        _PolyhedralBoundedSolidHalfEdge he = start;
        int count = 0;
        do {
            if ( count >= maximumVertices ) {
                return -1;
            }
            localPoints[count++] = he.startingVertex.position;
            he = he.next();
        } while ( he != start );
        return count;
    }

    /**
    @return the normal (Newell's method, not normalized) of a polygon
    */
    public static Vector3Dd polygonNormal(int numberOfVertices, Vector3Dd[] points) {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        for ( int i = 0; i < numberOfVertices; i++ ) {
            Vector3Dd a = points[i];
            Vector3Dd b = points[(i + 1) % numberOfVertices];
            x += (a.y() - b.y()) * (a.z() + b.z());
            y += (a.z() - b.z()) * (a.x() + b.x());
            z += (a.x() - b.x()) * (a.y() + b.y());
        }
        return new Vector3Dd(x, y, z);
    }

    /**
    Emits the faces of `solid` selected by `model`, with the vertex normals
    it gives.
    @return the MGF error code
    */
    public int emitSolid(PolyhedralBoundedSolid solid, MgfSurfaceNormalModel model, ParseRuntimeContext context) {
        Vector3Dd[] points = new Vector3Dd[ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT];
        Vector3Dd[] normals = new Vector3Dd[ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT];

        for ( int i = 0; i < solid.getPolygonsList().size(); i++ ) {
            int numberOfVertices = faceVertices(solid, i, points, ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT - 2);
            if ( numberOfVertices < 3 ) {
                return ParseErrorContext.MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
            }

            Vector3Dd centroid = new Vector3Dd(0.0, 0.0, 0.0);
            for ( int k = 0; k < numberOfVertices; k++ ) {
                centroid = centroid.add(points[k]);
            }
            centroid = centroid.multiply(1.0 / numberOfVertices);
            if ( !model.selectsFace(centroid, polygonNormal(numberOfVertices, points)) ) {
                continue;
            }

            Vector3Dd[] faceNormals = null;
            if ( model.hasVertexNormals() ) {
                for ( int k = 0; k < numberOfVertices; k++ ) {
                    normals[k] = toMgfDirection(model.vertexNormal(points[k]));
                }
                faceNormals = normals;
            }
            int errorCode = emitFace(numberOfVertices, points, faceNormals, context);
            if ( errorCode != ParseErrorContext.MGF_OK ) {
                return errorCode;
            }
        }
        return ParseErrorContext.MGF_OK;
    }
}

#ifndef __MGF_BOUNDARY_FACE_EMITTER__
#define __MGF_BOUNDARY_FACE_EMITTER__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"
#include "vsdk/toolkit/io/mgf/MgfSurfaceNormalModel.h"

class PolyhedralBoundedSolid;

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
class MgfBoundaryFaceEmitter final {
  private:
    Vector3Dd origin;
    Vector3Dd u;
    Vector3Dd v;
    Vector3Dd w;
    bool reversed;

  public:
    MgfBoundaryFaceEmitter(const Vector3Dd &origin, const Vector3Dd &u, const Vector3Dd &v, const Vector3Dd &w, bool reversed);

    Vector3Dd toMgfPoint(const Vector3Dd &localPoint) const;
    Vector3Dd toMgfDirection(const Vector3Dd &localDirection) const;

    /**
    Emits a face.
    @param numberOfVertices number of vertices of the face
    @param localPoints its vertices in the local frame, counterclockwise seen
    from the outside of the (not reversed) surface
    @param mgfNormals vertex normals in MGF coordinates (oriented as the not
    reversed surface; a null vector for none), or null for no normals
    @return the MGF error code
    */
    int emitFace(int numberOfVertices, const Vector3Dd *localPoints, const Vector3Dd *mgfNormals, ParseRuntimeContext *context) const;

    /**
    Emits the faces of `solid` selected by `model`, with the vertex normals
    it gives.
    @return the MGF error code
    */
    int emitSolid(PolyhedralBoundedSolid *solid, const MgfSurfaceNormalModel &model, ParseRuntimeContext *context) const;

    /**
    Collects the vertices of the outer loop of face number `faceIndex` of
    `solid` (in the list of faces), in their order.
    @return the number of vertices (at most `maximumVertices`), or -1 if the
    face has holes or more vertices
    */
    static int faceVertices(PolyhedralBoundedSolid *solid, long faceIndex, Vector3Dd *localPoints, int maximumVertices);

    /**
    @return the normal (Newell's method, not normalized) of a polygon
    */
    static Vector3Dd polygonNormal(int numberOfVertices, const Vector3Dd *points);
};

#endif

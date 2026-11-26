#ifndef __MGF_SURFACE_NORMAL_MODEL__
#define __MGF_SURFACE_NORMAL_MODEL__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

/**
Vertex normals of the patches of a MGF primitive, in the local frame of the
VITRAL primitive that tessellates it, and the choice of the faces of its
boundary representation that become patches.

The normals are the ones the classic MGF tessellation gave (the one of the
MGF parser library, where spheres and tori were expanded into cones whose
end normals were "warped"): the exact normal of the curved surface at each
vertex for spheres and tori, the normal of the cone for cones and cylinders
(constant along each generatrix, and the axis at the apex), and none for
planar primitives.

Open primitives (cones, cylinders and rings) are exported by VITRAL as
laminas: two coincident sheets of faces with opposite orientations. Only
the faces of the sheet oriented as the MGF surface become patches.
*/
class MgfSurfaceNormalModel final {
  public:
    enum Kind {
        /// Closed planar solid: every face, no vertex normals
        FLAT_SOLID,
        /// Lamina on the plane z = 0: the faces looking to +Z, no normals
        FLAT_LAMINA,
        /// Sphere centered at the origin: every face, radial normals
        SPHERE,
        /// Lamina of a cone (or cylinder) along +Z, from radius
        /// `bottomRadius` at z = 0 to radius `topRadius` at z = `height`:
        /// the faces looking away from the axis
        CONE_LAMINA,
        /// Torus around the Z axis, of major radius `majorRadius`
        TORUS
    };

  private:
    Kind kind;
    double bottomRadius;
    double topRadius;
    double height;
    double majorRadius;

    MgfSurfaceNormalModel(Kind kind, double bottomRadius, double topRadius, double height, double majorRadius);

  public:
    static MgfSurfaceNormalModel flatSolid();
    static MgfSurfaceNormalModel flatLamina();
    static MgfSurfaceNormalModel sphere();
    static MgfSurfaceNormalModel coneLamina(double bottomRadius, double topRadius, double height);
    static MgfSurfaceNormalModel torus(double majorRadius);

    Kind getKind() const { return kind; }

    /**
    @return true if the patches get vertex normals
    */
    bool hasVertexNormals() const;

    /**
    @param localPoint a vertex of the tessellation, in the local frame
    @return the (outwards, not necessarily unit) normal of the surface at the
    vertex, in the local frame
    */
    Vector3Dd vertexNormal(const Vector3Dd &localPoint) const;

    /**
    @param localCentroid centroid of a face of the boundary representation
    @param localFaceNormal normal of the face (following its loop)
    @return true if the face becomes a patch
    */
    bool selectsFace(const Vector3Dd &localCentroid, const Vector3Dd &localFaceNormal) const;
};

#endif

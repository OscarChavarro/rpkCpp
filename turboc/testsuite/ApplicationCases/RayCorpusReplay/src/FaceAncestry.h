#ifndef __FACE_ANCESTRY__
#define __FACE_ANCESTRY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/elements/RayExclusionSet.h"
#include "vsdk/toolkit/scene/Scene.h"

/**
The geometries of the scene's cluster hierarchy by identifier, and for every
face the chain of geometries that contain it (its leaf geometry, then each
enclosing cluster up to the root). The legacy queries skip an excluded
geometry with everything below it; a face-level index applies the same rule
by rejecting a face when one of its ancestors is excluded.

Built once from the scene; read-only afterwards, shared by every worker.
*/
class FaceAncestry {
  private:
    ArrayList<const Geometry *> geometryById;
    ArrayList<int> parentById; // -1 at the root
    ArrayList<int> leafByFace; // identifier of the geometry holding each face, -1 if none

    void visit(const Geometry *geometry, int parentId, const Scene &scene);

    // Not copyable
    FaceAncestry(const FaceAncestry &);
    FaceAncestry &operator=(const FaceAncestry &);

  public:
    explicit FaceAncestry(const Scene &scene);

    /** The geometry with this identifier, or null when the hierarchy has none */
    const Geometry *getGeometry(long id) const;

    /** True when one of the geometries containing the face is excluded */
    bool excludesFace(int faceIndex, const RayExclusionSet &exclusions) const;
};

#endif

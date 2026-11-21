#ifndef FACE_ANCESTRY__
#define FACE_ANCESTRY__

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
    java::ArrayList<const RpkGeometry *> geometryById;
    java::ArrayList<int> parentById; // -1 at the root
    java::ArrayList<int> leafByFace; // identifier of the geometry holding each face, -1 if none

    void visit(const RpkGeometry *geometry, int parentId, const Scene &scene);

  public:
    explicit FaceAncestry(const Scene &scene);

    /** The geometry with this identifier, or null when the hierarchy has none */
    const RpkGeometry *getGeometry(int id) const;

    /** True when one of the geometries containing the face is excluded */
    bool excludesFace(int faceIndex, const RayExclusionSet &exclusions) const;
};

#endif

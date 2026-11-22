#ifndef __FACE_BOUNDS_TABLE__
#define __FACE_BOUNDS_TABLE__

#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/skin/AxisAlignedBoundingBox.h"

/**
Conservative bounds of every face of a scene, in one contiguous array indexed
by the dense face index (Patch::getFaceIndex()). Built once by the scene after
the face indices are assigned, and read-only afterwards: the scene query
reads it, so any number of threads can share it.

C++ port note: the boxes come from Patch::computeAndGetBoundingBox (the box
of the vertices, as the C++ port's Patch::computeBounds); the voxel grid and
shaft culling of this port still read the boxes kept by each patch.
*/
class FaceBoundsTable {
  private:
    BoundingBox *bounds;
    const int numberOfFaces;

    // Not copyable
    FaceBoundsTable(const FaceBoundsTable &);
    FaceBoundsTable &operator=(const FaceBoundsTable &);

  public:
    FaceBoundsTable(Patch *const *faces, int inNumberOfFaces);
    ~FaceBoundsTable();

    int getNumberOfFaces() const;
    const BoundingBox &get(int faceIndex) const;
};

inline int
FaceBoundsTable::getNumberOfFaces() const {
    return numberOfFaces;
}

inline const BoundingBox &
FaceBoundsTable::get(const int faceIndex) const {
    return bounds[faceIndex];
}

#endif

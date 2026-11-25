#ifndef __FACE_BOUNDS_TABLE__
#define __FACE_BOUNDS_TABLE__

#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/skin/RpkAxisAlignedBoundingBox.h"

/**
Conservative bounds of every face of a scene, in one contiguous array indexed
by the dense face index (Patch::getFaceIndex()). Built once by the scene after
the face indices are assigned, from Patch::computeBounds, and read-only
afterwards: the voxel grid and shaft culling (its patch tests and the bounds of
the patch sets it creates) read it, so any number of threads can share it.
*/
class FaceBoundsTable {
  private:
    RpkAxisAlignedBoundingBox *bounds;
    const int numberOfFaces;

  public:
    FaceBoundsTable(Patch *const *faces, int inNumberOfFaces);
    ~FaceBoundsTable();
    FaceBoundsTable(const FaceBoundsTable &) = delete;
    FaceBoundsTable &operator=(const FaceBoundsTable &) = delete;

    int getNumberOfFaces() const;
    const RpkAxisAlignedBoundingBox &get(int faceIndex) const;
};

inline int
FaceBoundsTable::getNumberOfFaces() const {
    return numberOfFaces;
}

inline const RpkAxisAlignedBoundingBox &
FaceBoundsTable::get(const int faceIndex) const {
    return bounds[faceIndex];
}

#endif

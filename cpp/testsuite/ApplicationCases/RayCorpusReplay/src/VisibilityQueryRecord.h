#ifndef VISIBILITY_QUERY_RECORD__
#define VISIBILITY_QUERY_RECORD__

#include <cstdio>

/**
One QUERY record of a ray corpus (format in VisibilityQueryRecorder.h): the
query as the program issued it and the answer it received. An immutable value
once read.
*/
class VisibilityQueryRecord {
  public:
    static const int NUMBER_OF_EXCLUDED_PATCHES = 4;
    static const int NUMBER_OF_EXCLUDED_GEOMETRIES = 2;

  private:
    int path;
    int hitFlags;
    float origin[3];
    float direction[3];
    float minimumDistance;
    float maximumDistance;
    int excludedPatchId[NUMBER_OF_EXCLUDED_PATCHES];
    int excludedGeometryId[NUMBER_OF_EXCLUDED_GEOMETRIES];
    int blockerPatchId;
    float blockerDistance;

  public:
    VisibilityQueryRecord();

    /**
    Reads the fields that follow the QUERY tag.
    @return false on a truncated file
    */
    bool read(FILE *file);

    int getPath() const { return path; }
    int getHitFlags() const { return hitFlags; }
    float getOrigin(const int axis) const { return origin[axis]; }
    float getDirection(const int axis) const { return direction[axis]; }
    float getMinimumDistance() const { return minimumDistance; }
    float getMaximumDistance() const { return maximumDistance; }
    int getExcludedPatchId(const int index) const { return excludedPatchId[index]; }
    int getExcludedGeometryId(const int index) const { return excludedGeometryId[index]; }
    int getBlockerPatchId() const { return blockerPatchId; }
    float getBlockerDistance() const { return blockerDistance; }
};

#endif

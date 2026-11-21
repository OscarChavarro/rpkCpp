#ifndef __VISIBILITY_QUERY_RECORD__
#define __VISIBILITY_QUERY_RECORD__

#include <stdio.h>

/**
One QUERY record of a ray corpus (format in VisibilityQueryRecorder.h): the
query as the program issued it and the answer it received. An immutable value
once read.

C++ port note: the C++ port reads each field with fread in host byte order;
this port decodes the 32-bit little-endian fields explicitly and keeps the
integers in long, since int has 16 bits on Turbo C++ 3.0. Corpus files of
either port are read alike.
*/
class VisibilityQueryRecord {
  public:
    enum {
        NUMBER_OF_EXCLUDED_PATCHES = 4,
        NUMBER_OF_EXCLUDED_GEOMETRIES = 2
    };

  private:
    long path;
    long hitFlags;
    float origin[3];
    float direction[3];
    float minimumDistance;
    float maximumDistance;
    long excludedPatchId[NUMBER_OF_EXCLUDED_PATCHES];
    long excludedGeometryId[NUMBER_OF_EXCLUDED_GEOMETRIES];
    long blockerPatchId;
    float blockerDistance;

  public:
    VisibilityQueryRecord();

    /**
    Reads one int32 field of the corpus.
    @return false at the end of the file
    */
    static bool readInt(FILE *file, long *value);

    /**
    Reads one float field of the corpus.
    @return false at the end of the file
    */
    static bool readFloat(FILE *file, float *value);

    /**
    Reads the fields that follow the QUERY tag.
    @return false on a truncated file
    */
    bool read(FILE *file);

    long getPath() const { return path; }
    long getHitFlags() const { return hitFlags; }
    float getOrigin(const int axis) const { return origin[axis]; }
    float getDirection(const int axis) const { return direction[axis]; }
    float getMinimumDistance() const { return minimumDistance; }
    float getMaximumDistance() const { return maximumDistance; }
    long getExcludedPatchId(const int index) const { return excludedPatchId[index]; }
    long getExcludedGeometryId(const int index) const { return excludedGeometryId[index]; }
    long getBlockerPatchId() const { return blockerPatchId; }
    float getBlockerDistance() const { return blockerDistance; }
};

#endif

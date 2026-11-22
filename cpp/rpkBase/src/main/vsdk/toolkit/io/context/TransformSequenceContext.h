#ifndef TRANSFORM_ARRAY__
#define TRANSFORM_ARRAY__

#include "vsdk/toolkit/io/context/FilePositionContext.h"
#include "vsdk/toolkit/io/context/TransformArrayContext.h"

class TransformSequenceContext {
  public:
    static constexpr int TRANSFORM_MAXIMUM_DIMENSIONS = 8;

    FilePositionContext startingPosition; // Starting position on input
    int numberOfDimensions; // Number of array dimensions
    TransformArrayContext transformArguments[TRANSFORM_MAXIMUM_DIMENSIONS];
};

#endif

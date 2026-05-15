#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __TRANSFORM_ARRAY__
#define __TRANSFORM_ARRAY__

#include "io/context/FilePositionContext.h"
#include "io/context/TransformArrayContext.h"

class TransformSequenceContext {
  public:
    enum{
        TRANSFORM_MAXIMUM_DIMENSIONS = 8
    };

    FilePositionContext startingPosition; // Starting position on input
    int numberOfDimensions; // Number of array dimensions
    TransformArrayContext transformArguments[TRANSFORM_MAXIMUM_DIMENSIONS];
};

#endif

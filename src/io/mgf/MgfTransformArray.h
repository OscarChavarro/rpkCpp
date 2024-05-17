#ifndef __MGF_TRANSFORM_ARRAY__
#define __MGF_TRANSFORM_ARRAY__

#include "io/mgf/MgfTransformArrayArgument.h"
#include "io/mgf/MgfReaderFilePosition.h"

// Maximum array dimensions
#define TRANSFORM_MAXIMUM_DIMENSIONS 8

class MgfTransformArray {
  public:
    MgfReaderFilePosition startingPosition; // Starting position on input
    int numberOfDimensions; // Number of array dimensions
    MgfTransformArrayArgument transformArguments[TRANSFORM_MAXIMUM_DIMENSIONS];
};

#endif

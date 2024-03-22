#ifndef __MGF_TRANSFORM_CONTEXT__
#define __MGF_TRANSFORM_CONTEXT__

#include "io/mgf/mgfDefinitions.h"

// Regular transformation
class MgfTransform {
public:
    MATRIX4Dd xfm; // Transform matrix
    double sca; // Scale factor
};

// Maximum array dimensions
#define TRANSFORM_MAXIMUM_DIMENSIONS 8

class MgfTransformArrayArgument {
public:
    short i; // Current count
    short n; // Current maximum
    char arg[8]; // String argument value
};

class MgfTransformArray {
public:
    MgdReaderFilePosition startingPosition; // Starting position on input
    int numberOfDimensions; // Number of array dimensions
    MgfTransformArrayArgument transformArguments[TRANSFORM_MAXIMUM_DIMENSIONS];
};

class MgfTransformContext {
public:
    long xid; // Unique transform id
    short xac; // Context argument count
    short rev; // Boolean true if vertices reversed
    MgfTransform xf; // Cumulative transformation
    MgfTransformArray *xarr; // Transformation array pointer
    MgfTransformContext *prev; // Previous transformation context
}; // Followed by argument buffer

extern MgfTransformContext *GLOBAL_mgf_xfContext; // Current transform context

#endif

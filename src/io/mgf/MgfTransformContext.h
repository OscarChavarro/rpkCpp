#ifndef __MGF_TRANSFORM_CONTEXT__
#define __MGF_TRANSFORM_CONTEXT__

#include "common/linealAlgebra/Vector3Dd.h"
#include "io/mgf/mgfDefinitions.h"

#define TRANSFORM_ARGC(xf) ( (xf) == nullptr ? 0 : (xf)->xac )
#define TRANSFORM_ARGV(xf) (GLOBAL_mgf_xfLastTransform - (xf)->xac)
#define TRANSFORM_CONTEXT_ARGC TRANSFORM_ARGC(GLOBAL_mgf_xfContext)
#define TRANSFORM_XID(xf) ( (xf) == nullptr ? 0 : (xf)->xid )

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
    MgfTransformArray *transformationArray;
    MgfTransformContext *prev; // Previous transformation context
}; // Followed by argument buffer

extern MgfTransformContext *GLOBAL_mgf_xfContext; // Current transform context

#endif

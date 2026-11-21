#include <string.h>
#include "VisibilityQueryRecord.h"

VisibilityQueryRecord::VisibilityQueryRecord():
    path(0),
    hitFlags(0),
    minimumDistance(0.0f),
    maximumDistance(0.0f),
    blockerPatchId(0),
    blockerDistance(0.0f)
{
    for ( int axis = 0; axis < 3; axis++ ) {
        origin[axis] = 0.0f;
        direction[axis] = 0.0f;
    }
    int i;
    for ( i = 0; i < NUMBER_OF_EXCLUDED_PATCHES; i++ ) {
        excludedPatchId[i] = 0;
    }
    for ( i = 0; i < NUMBER_OF_EXCLUDED_GEOMETRIES; i++ ) {
        excludedGeometryId[i] = 0;
    }
}

/**
Decodes a little-endian two's complement int32, whatever the width of long
(at least 32 bits) and the host byte order
*/
bool
VisibilityQueryRecord::readInt(FILE *file, long *value) {
    unsigned char bytes[4];
    if ( fread(bytes, 1, 4, file) != 4 ) {
        return false;
    }
    const unsigned long bits = ((unsigned long)(bytes[0]))
        | (((unsigned long)(bytes[1])) << 8)
        | (((unsigned long)(bytes[2])) << 16)
        | (((unsigned long)(bytes[3])) << 24);
    if ( (bits & 0x80000000UL) != 0 ) {
        *value = -((long)((~bits) & 0x7FFFFFFFUL)) - 1;
    } else {
        *value = (long)(bits);
    }
    return true;
}

/**
Decodes a little-endian IEEE 754 single precision float. Floats are assumed
to share the byte order of the integers, as in VisibilityQueryRecorder
*/
bool
VisibilityQueryRecord::readFloat(FILE *file, float *value) {
    unsigned char bytes[4];
    if ( fread(bytes, 1, 4, file) != 4 ) {
        return false;
    }
    const unsigned short probe = 1;
    if ( *((const unsigned char *)(&probe)) != 1 ) {
        unsigned char swap = bytes[0];
        bytes[0] = bytes[3];
        bytes[3] = swap;
        swap = bytes[1];
        bytes[1] = bytes[2];
        bytes[2] = swap;
    }
    memcpy(value, bytes, 4);
    return true;
}

bool
VisibilityQueryRecord::read(FILE *file) {
    bool complete = readInt(file, &path) && readInt(file, &hitFlags);
    int axis;
    for ( axis = 0; complete && axis < 3; axis++ ) {
        complete = readFloat(file, &origin[axis]);
    }
    for ( axis = 0; complete && axis < 3; axis++ ) {
        complete = readFloat(file, &direction[axis]);
    }
    complete = complete
        && readFloat(file, &minimumDistance)
        && readFloat(file, &maximumDistance);
    int i;
    for ( i = 0; complete && i < NUMBER_OF_EXCLUDED_PATCHES; i++ ) {
        complete = readInt(file, &excludedPatchId[i]);
    }
    for ( i = 0; complete && i < NUMBER_OF_EXCLUDED_GEOMETRIES; i++ ) {
        complete = readInt(file, &excludedGeometryId[i]);
    }
    complete = complete
        && readInt(file, &blockerPatchId)
        && readFloat(file, &blockerDistance);
    return complete;
}

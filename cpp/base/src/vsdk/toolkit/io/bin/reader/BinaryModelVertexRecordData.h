#ifndef BINARY_MODEL_READER_VERTEX_RECORD__
#define BINARY_MODEL_READER_VERTEX_RECORD__

#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/io/bin/reader/BinaryModelIndexListRef.h"

class BinaryModelVertexRecordData {
  public:
    int id;
    int pointIndex;
    int normalIndex;
    int textureCoordinateIndex;
    ColorRgbMutable color;
    int backIndex;
    int tmp;
    bool hasRadianceData;
    BinaryModelIndexListRef patchIndices;

    BinaryModelVertexRecordData();
};

#endif

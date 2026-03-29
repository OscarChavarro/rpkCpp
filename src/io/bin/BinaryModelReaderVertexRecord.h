#ifndef __BINARY_MODEL_READER_VERTEX_RECORD__
#define __BINARY_MODEL_READER_VERTEX_RECORD__

#include "common/ColorRgb.h"

#include "io/bin/BinaryModelReader.h"
#include "io/bin/BinaryModelReaderIndexListRecord.h"

class BinaryModelReader::VertexRecord {
  public:
    int id;
    int pointIndex;
    int normalIndex;
    int textureCoordinateIndex;
    ColorRgb color;
    int backIndex;
    int tmp;
    bool hasRadianceData;
    IndexListRecord patchIndices;
};

#endif

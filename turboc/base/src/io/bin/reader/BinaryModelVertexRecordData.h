#ifndef BNRY_MDL_RDR_VRTX_RCRD
#define BNRY_MDL_RDR_VRTX_RCRD

#include "common/color/ColorRgb.h"
#include "io/bin/reader/BinaryModelIndexListRef.h"

class BinaryModelVertexRecordData {
  public:
    int id;
    int pointIndex;
    int normalIndex;
    int textureCoordinateIndex;
    ColorRgb color;
    int backIndex;
    int tmp;
    bool hasRadianceData;
    BinaryModelIndexListRef patchIndices;

    BinaryModelVertexRecordData();
};

#endif

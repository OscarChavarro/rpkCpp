#ifndef __K_D_QUERY__
#define __K_D_QUERY__

#include "vsdk/toolkit/common/VSDK.h"

class KDQuery {
  public:
    float *point;
    int wantedN;
    int foundN;
    bool notFilled;
    float **results;
    float *distances;
    float maximumDistance;
    float sqrRadius;
    short excludeFlags;

    KDQuery();
    void print() const;
};

#endif

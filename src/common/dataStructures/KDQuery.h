#ifndef __K_D_QUERY__
#define __K_D_QUERY__

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

#ifndef K_D_QUERY__
#define K_D_QUERY__

template<typename T>
class KDQuery {
  public:
    const float *point;
    int wantedN;
    int foundN;
    bool notFilled;
    T **results;
    float *distances;
    float maximumDistance;
    float sqrRadius;
    short excludeFlags;

    KDQuery(): point(nullptr), wantedN(0), foundN(0), notFilled(false), results(nullptr),
        distances(nullptr), maximumDistance(0), sqrRadius(0), excludeFlags(0) {}
};

#endif

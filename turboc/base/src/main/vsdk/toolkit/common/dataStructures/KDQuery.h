#ifndef K_D_QUERY__
#define K_D_QUERY__

template<typename T>
class KDQuery {
  public:
    const double *point;
    int wantedN;
    int foundN;
    bool notFilled;
    T **results;
    double *distances;
    double maximumDistance;
    double sqrRadius;
    short excludeFlags;

    KDQuery(): point(NULL), wantedN(0), foundN(0), notFilled(false), results(NULL),
        distances(NULL), maximumDistance(0), sqrRadius(0), excludeFlags(0) {}
};

#endif

#ifndef __SPAR_PATH_GROUP__
#define __SPAR_PATH_GROUP__

class SparPathGroupInfo final {
  public:
    static constexpr int MAX_PATH_GROUPS = 2;
};

enum SparPathGroup {
    DISJOINT_GROUP = 0,
    LD_GROUP = 1
};

#endif

#ifndef OPTNS_GRP_BDRCT_RYTRC
#define OPTNS_GRP_BDRCT_RYTRC

#include "vsdk/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"

class OptsGrpBidirRaytr{ public:
    static void parse( int *argc, char **argv, BidirectionalPathTracingState &bidirectionalPathState);

  private:
    class FixedStringBinding{ public:
        char *target;
        int maxLength;
    };

    static int regExpStringLength;

    static bool parseFixedStringBinding(int argc, char **argv, FixedStringBinding &binding);
    static bool parseBoolInt(int argc, char **argv, int &value);
    static void setIntTrue(int &value);
    static void setIntFalse(int &value);
};

#endif

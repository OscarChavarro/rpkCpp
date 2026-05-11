#ifndef OPTIONS_GROUP_BIDIRECTIONAL_RAYTRACING__
#define OPTIONS_GROUP_BIDIRECTIONAL_RAYTRACING__

#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathTracingState.h"

class OptionsGroupBidirectionalRaytracing final {
  public:
    static void parse(
        int *argc,
        char **argv,
        BidirectionalPathTracingState &bidirectionalPathState);

  private:
    class FixedStringBinding {
      public:
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

#include "vsdk/toolkit/common/statistics/Statistics.h"

// Note this class is a singleton
Statistics::Statistics():
    reader(),
    radiance(),
    potential(),
    shadow(),
    rayTracer()
{
}

Statistics &
Statistics::instance() {
    static Statistics instanceValue;
    return instanceValue;
}

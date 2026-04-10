
#include "common/VSDK.h"
#ifndef STCHS_RYTRC_APPRX
#define STCHS_RYTRC_APPRX

enum StochRaytrApprx {
    CONSTANT = 0, // 1
    LINEAR = 1, // 1, u, v
    BI_LINEAR = 2, // 1, u, v, uv
    QUADRATIC = 3, // 1, u, v, uv, u2, v2
    CUBIC = 4 // 1, u, v, uv, u2, v2, u3, u2v, uv2, v3
};

#endif

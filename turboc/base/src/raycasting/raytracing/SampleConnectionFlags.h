#ifndef __SAMPLE_CONNECTION_FLAGS__
#define __SAMPLE_CONNECTION_FLAGS__

#include "common/VSDK.h"

enum SampleConnectionFlags {
    CONNECT_EL = 0x01,  // Compute pdf(E->L) and bsdf(EP -> E -> L)
    CONNECT_LE = 0x02,  // Compute pdf(L->E) and bsdf(LP -> L -> E)
    FILL_OTHER_PDF = 0x10  // If CONNECT_EL or CONNECT_LE then also compute the opposite PDF.
};

#endif

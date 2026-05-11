#ifndef SAMPLE_CONNECTION_FLAGS__
#define SAMPLE_CONNECTION_FLAGS__

enum SampleConnectionFlags : char {
    CONNECT_EL = 0x01,  // Compute pdf(E->L) and bsdf(EP -> E -> L)
    CONNECT_LE = 0x02,  // Compute pdf(L->E) and bsdf(LP -> L -> E)
    FILL_OTHER_PDF = 0x10  // If CONNECT_EL or CONNECT_LE then also compute the opposite PDF.
};

#endif

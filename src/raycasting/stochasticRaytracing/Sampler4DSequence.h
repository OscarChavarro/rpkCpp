#ifndef __SAMPLER_4D_SEQUENCE__
#define __SAMPLER_4D_SEQUENCE__

enum Sampler4DSequence {
    RANDOM,
    HALTON,
    SCRAMBLED_HALTON,
    SOBOL,
    ORIGINAL_FAURE,
    GENERALIZED_FAURE,
    NIEDERREITER
};

#endif

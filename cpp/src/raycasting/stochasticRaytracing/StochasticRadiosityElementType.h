#ifndef __STOCHASTIC_RADIOSITY_ELEMENT_TYPE__
#define __STOCHASTIC_RADIOSITY_ELEMENT_TYPE__

enum StochasticRadiosityElementType {
    ET_TRIANGLE = 0,
    ET_QUAD = 1
};

class StochasticRadiosityElementTypeInfo final {
  public:
    static constexpr int NUMBER_OF_ELEMENT_TYPES = 2;
};

#endif

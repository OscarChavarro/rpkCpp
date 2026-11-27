#ifndef __ELEMENT_SAMPLES__
#define __ELEMENT_SAMPLES__

#include <stddef.h>

class GalerkinElement;

/** Immutable count of the scratch buffer samples a top-level element covers */
class ElementSamples {
  private:
    GalerkinElement *element;
    int samples;

  public:
    ElementSamples(): element(NULL), samples(0) {}
    ElementSamples(GalerkinElement *inElement, int inSamples): element(inElement), samples(inSamples) {}

    GalerkinElement *getElement() const { return element; }
    int getSamples() const { return samples; }
};

#endif

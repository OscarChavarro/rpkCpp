#ifndef __ELEMENT_SAMPLES__
#define __ELEMENT_SAMPLES__

class GalerkinElement;

/** Immutable count of the scratch buffer samples a top-level element covers */
class ElementSamples final {
  private:
    GalerkinElement *element;
    int samples;

  public:
    ElementSamples(): element(nullptr), samples(0) {}
    ElementSamples(GalerkinElement *inElement, int inSamples): element(inElement), samples(inSamples) {}

    GalerkinElement *getElement() const { return element; }
    int getSamples() const { return samples; }
};

#endif

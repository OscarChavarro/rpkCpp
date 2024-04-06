#ifndef __GALERKIN_P__
#define __GALERKIN_P__

#include "GALERKIN/GalerkinElement.h"

inline ColorRgb
galerkinGetRadiance(Patch *patch) {
    return ((GalerkinElement *)(patch->radianceData))->radiance[0];
}

inline void
galerkinSetRadiance(Patch *patch, ColorRgb value) {
    ((GalerkinElement *)(patch->radianceData))->radiance[0] = value;
}

inline ColorRgb
galerkinUnShotRadiance(Patch *patch) {
    return patch->radianceData->unShotRadiance[0];
}

inline float
galerkinGetPotential(Patch *patch) {
    return ((GalerkinElement *)((patch)->radianceData))->potential;
}

inline void
galerkinSetPotential(Patch *patch, float value) {
    ((GalerkinElement *)((patch)->radianceData))->potential = value;
}

inline float
galerkinGetUnShotPotential(Patch *patch) {
    return ((GalerkinElement *)((patch)->radianceData))->unShotPotential;
}

inline void
galerkinSetUnShotPotential(Patch *patch, float value) {
    ((GalerkinElement *)((patch)->radianceData))->unShotPotential = value;
}

inline GalerkinElement*
galerkinGetElement(Patch *patch) {
    if ( patch == nullptr ) {
        fprintf(stderr, "Fatal: Trying to access as GalerkinElement on a null Patch\n");
        exit(1);
    }
    if ( patch->radianceData == nullptr ) {
        fprintf(stderr, "Fatal: Trying to access as GalerkinElement on a Patch with null radianceData\n");
        exit(1);
    }
    if ( patch->radianceData->className != ElementTypes::ELEMENT_GALERKIN ) {
        fprintf(stderr, "Fatal: Trying to access as GalerkinElement a different type of element\n");
        exit(1);
    }
    return (GalerkinElement *)patch->radianceData;
}

#endif

#include "GALERKIN/GatheringStrategy.h"

GatheringStrategy::GatheringStrategy() {
}

GatheringStrategy::~GatheringStrategy() {
}

/**
Makes the representation of potential consistent after an iteration
(potential is always propagated using Jacobi iterations)
*/
float
GatheringStrategy::gatheringPushPullPotential(GalerkinElement *element, float down) {
    down += element->receivedPotential / element->area;
    element->receivedPotential = 0.0f;

    float up = 0.0;

    if ( element->regularSubElements == nullptr && element->irregularSubElements == nullptr ) {
        up = down + element->patch->directPotential;
    }

    if ( element->regularSubElements != nullptr ) {
        for ( int i = 0; i < 4; i++ ) {
            up += 0.25f * GatheringStrategy::gatheringPushPullPotential((GalerkinElement *)element->regularSubElements[i], down);
        }
    }

    if ( element->irregularSubElements != nullptr ) {
        for ( int i = 0; element->irregularSubElements != nullptr && i < element->irregularSubElements->size(); i++ ) {
            GalerkinElement *subElement = (GalerkinElement *)element->irregularSubElements->get(i);
            if ( !element->isCluster() ) {
                // Don't push to irregular surface sub-elements
                down = 0.0;
            }
            up += subElement->area / element->area * GatheringStrategy::gatheringPushPullPotential(subElement, down);
        }
    }

    element->potential = up;
    return element->potential;
}

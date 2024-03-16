#include <cstring>
#include "skin/RadianceMethod.h"

RadianceMethod *GLOBAL_radiance_selectedRadianceMethod = nullptr;

RadianceMethod::RadianceMethod() {
}

RadianceMethod::~RadianceMethod() {
}

static char globalName[20];

char *
getRadianceMethodAlgorithmName(RadianceMethodAlgorithm classZ) {
    switch ( classZ ) {
        case PHOTON_MAP:
            strcpy(globalName, "Photon map");
            break;
        case GALERKIN:
            strcpy(globalName, "Galerkin");
            break;
        case STOCHASTIC_JACOBI:
            strcpy(globalName, "Stochastic Jacobi");
            break;
        case RANDOM_WALK:
            strcpy(globalName, "Random walk");
            break;
        default:
            strcpy(globalName, "<Unknown>");
            break;
    }
    return globalName;
}

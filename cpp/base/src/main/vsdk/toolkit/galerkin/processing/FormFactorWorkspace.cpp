#include "vsdk/toolkit/galerkin/processing/FormFactorWorkspace.h"

FormFactorWorkspace::FormFactorWorkspace():
    lastReceiver(nullptr),
    lastSource(nullptr),
    receiverCubatureRule(nullptr),
    sourceCubatureRule(nullptr),
    receiverNodes(),
    sourceNodes(),
    kernel(),
    maximumKernelValue(0.0),
    visibilityCount(0)
{
    for ( int i = 0; i < CubatureRule::MAXIMUM_NODES; i++ ) {
        for ( int j = 0; j < CubatureRule::MAXIMUM_NODES; j++ ) {
            kernel[i][j] = 0.0;
        }
    }
}

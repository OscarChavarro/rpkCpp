#include "vsdk/toolkit/galerkin/processing/FormFactorWorkspace.h"

FormFactorWorkspace::FormFactorWorkspace():
    lastReceiver(NULL),
    lastSource(NULL),
    receiverCubatureRule(NULL),
    sourceCubatureRule(NULL),
    receiverNodes(),
    sourceNodes(),
    kernel(),
    maximumKernelValue(0.0),
    visibilityCount(0)
{
    for ( int i = 0; i < CUBATURE_MAXIMUM_NODES; i++ ) {
        for ( int j = 0; j < CUBATURE_MAXIMUM_NODES; j++ ) {
            kernel[i][j] = 0.0;
        }
    }
}

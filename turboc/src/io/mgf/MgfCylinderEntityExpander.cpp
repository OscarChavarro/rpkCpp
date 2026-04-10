#include "io/mgf/MgfEntityControl.h"
#include "io/mgf/MgfCylinderEntityExpander.h"

/**
Replace a cylinder with equivalent cone
*/
int
MgfCylinderEntityExpander::handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context) {
    const char *newArgumentValues[6] = {context->entityNames[CONE]};

    if ( argumentCount != 4 ) {
        return MGF_ERRR_WRNG_NUM_O_ARGMN;
    }
    newArgumentValues[1] = argumentValues[1];
    newArgumentValues[2] = argumentValues[2];
    newArgumentValues[3] = argumentValues[3];
    newArgumentValues[4] = argumentValues[2];
    return MgfEntityControl::mgfHandle(CONE, 5, newArgumentValues, context);
}

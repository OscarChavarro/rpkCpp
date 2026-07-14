#include "vsdk/toolkit/io/mgf/MgfEntityControl.h"
#include "vsdk/toolkit/io/mgf/MgfCylinderEntityExpander.h"

/**
Replace a cylinder with equivalent cone
*/
int
MgfCylinderEntityExpander::handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context) {
    const char *newArgumentValues[6] = {context->entityNames[EntityTypeContext::CONE]};

    if ( argumentCount != 4 ) {
        return ParseErrorContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    newArgumentValues[1] = argumentValues[1];
    newArgumentValues[2] = argumentValues[2];
    newArgumentValues[3] = argumentValues[3];
    newArgumentValues[4] = argumentValues[2];
    return MgfEntityControl::mgfHandle(EntityTypeContext::CONE, 5, newArgumentValues, context);
}

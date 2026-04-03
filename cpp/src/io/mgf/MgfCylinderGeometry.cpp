#include "io/mgf/MgfDefinitions.h"
#include "io/mgf/MgfCylinderGeometry.h"

/**
Replace a cylinder with equivalent cone
*/
int
MgfCylinderGeometry::handleEntity(int argumentCount, const char **argumentValues, ParseSession *context) {
    const char *newArgumentValues[6] = {context->entityNames[EntityContext::CONE]};

    if ( argumentCount != 4 ) {
        return ErrorCodeContext::MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    newArgumentValues[1] = argumentValues[1];
    newArgumentValues[2] = argumentValues[2];
    newArgumentValues[3] = argumentValues[3];
    newArgumentValues[4] = argumentValues[2];
    return MgfDefinitions::mgfHandle(EntityContext::CONE, 5, newArgumentValues, context);
}

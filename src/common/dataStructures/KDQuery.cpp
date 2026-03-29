#include "java/lang/System.h"
#include "common/dataStructures/KDQuery.h"

KDQuery::KDQuery():
    point(),
    wantedN(),
    foundN(),
    notFilled(),
    results(),
    distances(),
    maximumDistance(),
    sqrRadius(),
    excludeFlags()
{
}

void
KDQuery::print() const {
    java::lang::System::out.printf("Point X %g, Y %g, Z %g\n", point[0], point[1], point[2]);
    java::lang::System::out.printf("Wanted N: %i, found N: %i\n", wantedN, foundN);
    java::lang::System::out.printf("maximumDistance %g\n", maximumDistance);
    java::lang::System::out.printf("sqrRadius %g\n", sqrRadius);
    java::lang::System::out.printf("excludeFlags %x\n", static_cast<int>(excludeFlags));
}

#include "java/lang/System.h"
#include "vsdk/toolkit/common/dataStructures/KDQuery.h"

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
    java::System::out.printf("Point X %g, Y %g, Z %g\n", point[0], point[1], point[2]);
    java::System::out.printf("Wanted N: %i, found N: %i\n", wantedN, foundN);
    java::System::out.printf("maximumDistance %g\n", maximumDistance);
    java::System::out.printf("sqrRadius %g\n", sqrRadius);
    java::System::out.printf("excludeFlags %x\n", static_cast<int>(excludeFlags));
}

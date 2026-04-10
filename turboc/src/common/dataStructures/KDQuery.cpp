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
    System::out.printf("Point X %g, Y %g, Z %g\n", point[0], point[1], point[2]);
    System::out.printf("Wanted N: %i, found N: %i\n", wantedN, foundN);
    System::out.printf("maximumDistance %g\n", maximumDistance);
    System::out.printf("sqrRadius %g\n", sqrRadius);
    System::out.printf("excludeFlags %x\n", ((int)(excludeFlags)));
}

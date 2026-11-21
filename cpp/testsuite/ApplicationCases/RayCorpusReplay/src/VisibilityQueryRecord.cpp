#include "VisibilityQueryRecord.h"

VisibilityQueryRecord::VisibilityQueryRecord():
    path(0),
    hitFlags(0),
    origin(),
    direction(),
    minimumDistance(0.0F),
    maximumDistance(0.0F),
    excludedPatchId(),
    excludedGeometryId(),
    blockerPatchId(0),
    blockerDistance(0.0F)
{
}

bool
VisibilityQueryRecord::read(FILE *file) {
    bool complete = fread(&path, sizeof(int), 1, file) == 1
        && fread(&hitFlags, sizeof(int), 1, file) == 1
        && fread(origin, sizeof(float), 3, file) == 3
        && fread(direction, sizeof(float), 3, file) == 3
        && fread(&minimumDistance, sizeof(float), 1, file) == 1
        && fread(&maximumDistance, sizeof(float), 1, file) == 1
        && fread(excludedPatchId, sizeof(int), NUMBER_OF_EXCLUDED_PATCHES, file) == NUMBER_OF_EXCLUDED_PATCHES
        && fread(excludedGeometryId, sizeof(int), NUMBER_OF_EXCLUDED_GEOMETRIES, file) == NUMBER_OF_EXCLUDED_GEOMETRIES
        && fread(&blockerPatchId, sizeof(int), 1, file) == 1
        && fread(&blockerDistance, sizeof(float), 1, file) == 1;
    return complete;
}

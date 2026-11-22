#include <cstring>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/scene/VisibilityQueryRecorder.h"

static const char *
pathName(const int path) {
    switch ( path ) {
        case VisibilityQueryRecorder::GALERKIN_RESTRICTED_LIST:
            return "Galerkin restricted list";
        case VisibilityQueryRecorder::GALERKIN_FULL_SCENE:
            return "Galerkin full-scene index";
        case VisibilityQueryRecorder::RAYTRACER_CLOSEST_HIT:
            return "raytracer closest hit";
        case VisibilityQueryRecorder::RAYTRACER_VISIBILITY:
            return "raytracer visibility";
        default:
            return "stochastic shooting";
    }
}

VisibilityQueryRecorder::VisibilityQueryRecorder(const char *inCorpusFileName):
    corpus(nullptr),
    corpusFileName(nullptr),
    cacheResolved(0),
    queries(),
    hits(),
    geometryLists(0)
{
    for ( int i = 0; i < NUMBER_OF_PATHS; i++ ) {
        queries[i] = 0;
        hits[i] = 0;
    }
    if ( inCorpusFileName != nullptr && inCorpusFileName[0] != '\0' ) {
        corpusFileName = new char[strlen(inCorpusFileName) + 1];
        strcpy(corpusFileName, inCorpusFileName);
        corpus = fopen(corpusFileName, "wb");
        if ( corpus == nullptr ) {
            fprintf(stderr, "Ray capture: cannot open corpus file %s, only counting\n", corpusFileName);
        } else {
            fwrite("RPKVQC01", 1, 8, corpus);
            writeInt(1);
        }
    }
}

VisibilityQueryRecorder::~VisibilityQueryRecorder() {
    printSummary();
    if ( corpus != nullptr ) {
        fclose(corpus);
        corpus = nullptr;
    }
    delete[] corpusFileName;
    corpusFileName = nullptr;
}

void
VisibilityQueryRecorder::writeInt(const int value) {
    fwrite(&value, sizeof(int), 1, corpus);
}

void
VisibilityQueryRecorder::writeFloat(const float value) {
    fwrite(&value, sizeof(float), 1, corpus);
}

void
VisibilityQueryRecorder::countCacheResolved() {
    cacheResolved++;
}

void
VisibilityQueryRecorder::recordGeometryList(const java::ArrayList<RpkGeometry *> *geometryList) {
    geometryLists++;
    if ( corpus == nullptr ) {
        return;
    }
    const int count = geometryList != nullptr ? static_cast<int>(geometryList->size()) : 0;
    writeInt(1);
    writeInt(count);
    for ( int i = 0; i < count; i++ ) {
        writeInt(geometryList->get(i)->id);
    }
}

void
VisibilityQueryRecorder::recordQuery(
    const Path path,
    const RpkRay *ray,
    const float minimumDistance,
    const float maximumDistance,
    const int hitFlags,
    const RayExclusionSet &exclusions,
    const RpkRayHit *hit,
    const float hitDistance)
{
    queries[path]++;
    if ( hit != nullptr ) {
        hits[path]++;
    }
    if ( corpus == nullptr ) {
        return;
    }
    writeInt(2);
    writeInt(path);
    writeInt(hitFlags);
    writeFloat(ray->position.x());
    writeFloat(ray->position.y());
    writeFloat(ray->position.z());
    writeFloat(ray->direction.x());
    writeFloat(ray->direction.y());
    writeFloat(ray->direction.z());
    writeFloat(minimumDistance);
    writeFloat(maximumDistance);
    for ( int i = 0; i < MAX_EXCLUDED_PATCHES; i++ ) {
        const Patch *patch = exclusions.getExcludedPatch(i);
        writeInt(patch != nullptr ? static_cast<int>(patch->getId()) : 0);
    }
    writeInt(exclusions.getExcludedGeometry1() != nullptr ? exclusions.getExcludedGeometry1()->id : -1);
    writeInt(exclusions.getExcludedGeometry2() != nullptr ? exclusions.getExcludedGeometry2()->id : -1);
    const Patch *blocker = hit != nullptr ? hit->getPatch() : nullptr;
    writeInt(blocker != nullptr ? static_cast<int>(blocker->getId()) : 0);
    writeFloat(hit != nullptr ? hitDistance : maximumDistance);
}

void
VisibilityQueryRecorder::printSummary() const {
    fprintf(stderr, "Ray capture: shadow rays resolved by the shadow cache: %lld\n", cacheResolved);
    for ( int i = 0; i < NUMBER_OF_PATHS; i++ ) {
        fprintf(stderr, "Ray capture: %s queries: %lld, with hit: %lld\n", pathName(i), queries[i], hits[i]);
    }
    fprintf(stderr, "Ray capture: restricted geometry lists: %lld\n", geometryLists);
    if ( corpusFileName != nullptr ) {
        fprintf(stderr, "Ray capture: corpus written to %s\n", corpusFileName);
    }
}

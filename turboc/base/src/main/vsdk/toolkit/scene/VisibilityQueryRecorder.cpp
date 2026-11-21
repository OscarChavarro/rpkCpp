#include <string.h>
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

/**
True on a little-endian host. Floats are assumed to share the byte order of
the integers, as they do on x86 (the Turbo C++ and the C++ reference
machines) and on the other common architectures
*/
static bool
hostIsLittleEndian() {
    const unsigned short probe = 1;
    return *((const unsigned char *)(&probe)) == 1;
}

/**
Writes the low 32 bits of bits, least significant byte first
*/
static void
writeLittleEndian32(FILE *file, const unsigned long bits) {
    putc(((int)(bits & 0xFFUL)), file);
    putc(((int)((bits >> 8) & 0xFFUL)), file);
    putc(((int)((bits >> 16) & 0xFFUL)), file);
    putc(((int)((bits >> 24) & 0xFFUL)), file);
}

VisibilityQueryRecorder::VisibilityQueryRecorder(const char *inCorpusFileName):
    corpus(NULL),
    corpusFileName(NULL),
    cacheResolved(0.0),
    geometryLists(0.0)
{
    for ( int i = 0; i < NUMBER_OF_PATHS; i++ ) {
        queries[i] = 0.0;
        hits[i] = 0.0;
    }
    if ( inCorpusFileName != NULL && inCorpusFileName[0] != '\0' ) {
        corpusFileName = new char[strlen(inCorpusFileName) + 1];
        strcpy(corpusFileName, inCorpusFileName);
        corpus = fopen(corpusFileName, "wb");
        if ( corpus == NULL ) {
            fprintf(stderr, "Ray capture: cannot open corpus file %s, only counting\n", corpusFileName);
        } else {
            fwrite("RPKVQC01", 1, 8, corpus);
            writeInt(1);
        }
    }
}

VisibilityQueryRecorder::~VisibilityQueryRecorder() {
    printSummary();
    if ( corpus != NULL ) {
        fclose(corpus);
        corpus = NULL;
    }
    delete[] corpusFileName;
    corpusFileName = NULL;
}

/**
A negative value converts to unsigned long modulo 2^N, so its low 32 bits
are its two's complement int32 encoding for any N >= 32
*/
void
VisibilityQueryRecorder::writeInt(const long value) {
    writeLittleEndian32(corpus, ((unsigned long)(value)));
}

void
VisibilityQueryRecorder::writeFloat(const float value) {
    unsigned char bytes[4];
    memcpy(bytes, &value, 4);
    if ( hostIsLittleEndian() ) {
        fwrite(bytes, 1, 4, corpus);
    } else {
        for ( int i = 3; i >= 0; i-- ) {
            putc(bytes[i], corpus);
        }
    }
}

void
VisibilityQueryRecorder::countCacheResolved() {
    cacheResolved += 1.0;
}

void
VisibilityQueryRecorder::recordGeometryList(const ArrayList<Geometry *> *geometryList) {
    geometryLists += 1.0;
    if ( corpus == NULL ) {
        return;
    }
    const long count = geometryList != NULL ? geometryList->size() : 0;
    writeInt(1);
    writeInt(count);
    for ( long i = 0; i < count; i++ ) {
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
    const RayHit *hit,
    const float hitDistance)
{
    queries[path] += 1.0;
    if ( hit != NULL ) {
        hits[path] += 1.0;
    }
    if ( corpus == NULL ) {
        return;
    }
    writeInt(2);
    writeInt(path);
    writeInt(hitFlags);
    writeFloat(ray->position.x);
    writeFloat(ray->position.y);
    writeFloat(ray->position.z);
    writeFloat(ray->direction.x);
    writeFloat(ray->direction.y);
    writeFloat(ray->direction.z);
    writeFloat(minimumDistance);
    writeFloat(maximumDistance);
    for ( int i = 0; i < MAX_EXCLUDED_PATCHES; i++ ) {
        const Patch *patch = exclusions.getExcludedPatch(i);
        writeInt(patch != NULL ? ((long)(patch->id)) : 0L);
    }
    writeInt(exclusions.getExcludedGeometry1() != NULL ? exclusions.getExcludedGeometry1()->id : -1);
    writeInt(exclusions.getExcludedGeometry2() != NULL ? exclusions.getExcludedGeometry2()->id : -1);
    const Patch *blocker = hit != NULL ? hit->getPatch() : NULL;
    writeInt(blocker != NULL ? ((long)(blocker->id)) : 0L);
    writeFloat(hit != NULL ? hitDistance : maximumDistance);
}

void
VisibilityQueryRecorder::printSummary() const {
    fprintf(stderr, "Ray capture: shadow rays resolved by the shadow cache: %.0f\n", cacheResolved);
    for ( int i = 0; i < NUMBER_OF_PATHS; i++ ) {
        fprintf(stderr, "Ray capture: %s queries: %.0f, with hit: %.0f\n", pathName(i), queries[i], hits[i]);
    }
    fprintf(stderr, "Ray capture: restricted geometry lists: %.0f\n", geometryLists);
    if ( corpusFileName != NULL ) {
        fprintf(stderr, "Ray capture: corpus written to %s\n", corpusFileName);
    }
}

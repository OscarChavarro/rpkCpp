#include <string.h>
#include "RayCorpusReader.h"

#ifdef __TURBOC__
static const size_t READ_BUFFER_SIZE = 16384;
#else
static const size_t READ_BUFFER_SIZE = 16 * 1024 * 1024;
#endif
static const long TAG_GEOMETRY_LIST = 1;
static const long TAG_QUERY = 2;
static const int SKIP_BUFFER_SIZE = 1024;

RayCorpusReader::RayCorpusReader(const char *fileName):
    file(NULL),
    buffer(NULL),
    geometryLists(0.0),
    valid(false)
{
    file = fopen(fileName, "rb");
    if ( file == NULL ) {
        return;
    }
    buffer = new char[READ_BUFFER_SIZE];
    setvbuf(file, buffer, _IOFBF, READ_BUFFER_SIZE);
    char magic[8];
    long version = 0;
    valid = fread(magic, 1, 8, file) == 8
        && memcmp(magic, "RPKVQC01", 8) == 0
        && VisibilityQueryRecord::readInt(file, &version)
        && version == 1;
}

RayCorpusReader::~RayCorpusReader() {
    if ( file != NULL ) {
        fclose(file);
        file = NULL;
    }
    delete[] buffer;
    buffer = NULL;
}

bool
RayCorpusReader::next(VisibilityQueryRecord *record) {
    while ( valid ) {
        long tag;
        if ( !VisibilityQueryRecord::readInt(file, &tag) ) {
            return false;
        }
        if ( tag == TAG_QUERY ) {
            if ( !record->read(file) ) {
                valid = false;
                return false;
            }
            return true;
        }
        if ( tag != TAG_GEOMETRY_LIST ) {
            valid = false;
            return false;
        }
        long count;
        if ( !VisibilityQueryRecord::readInt(file, &count) || count < 0 ) {
            valid = false;
            return false;
        }
        char skipped[SKIP_BUFFER_SIZE];
        const long identifiersPerRead = SKIP_BUFFER_SIZE / 4;
        while ( count > 0 ) {
            const long identifiers = count < identifiersPerRead ? count : identifiersPerRead;
            const size_t bytes = ((size_t)(identifiers * 4));
            if ( fread(skipped, 1, bytes, file) != bytes ) {
                valid = false;
                return false;
            }
            count -= identifiers;
        }
        geometryLists += 1.0;
    }
    return false;
}

#include <cstring>
#include "RayCorpusReader.h"

static const size_t READ_BUFFER_SIZE = 16 * 1024 * 1024;
static const int TAG_GEOMETRY_LIST = 1;
static const int TAG_QUERY = 2;

RayCorpusReader::RayCorpusReader(const char *fileName):
    file(nullptr),
    buffer(nullptr),
    geometryLists(0),
    valid(false)
{
    file = fopen(fileName, "rb");
    if ( file == nullptr ) {
        return;
    }
    buffer = new char[READ_BUFFER_SIZE];
    setvbuf(file, buffer, _IOFBF, READ_BUFFER_SIZE);
    char magic[8];
    int version = 0;
    valid = fread(magic, 1, 8, file) == 8
        && memcmp(magic, "RPKVQC01", 8) == 0
        && fread(&version, sizeof(int), 1, file) == 1
        && version == 1;
}

RayCorpusReader::~RayCorpusReader() {
    if ( file != nullptr ) {
        fclose(file);
        file = nullptr;
    }
    delete[] buffer;
    buffer = nullptr;
}

bool
RayCorpusReader::next(VisibilityQueryRecord *record) {
    while ( valid ) {
        int tag;
        if ( fread(&tag, sizeof(int), 1, file) != 1 ) {
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
        int count;
        if ( fread(&count, sizeof(int), 1, file) != 1 || count < 0
             || fseek(file, static_cast<long>(count) * static_cast<long>(sizeof(int)), SEEK_CUR) != 0 ) {
            valid = false;
            return false;
        }
        geometryLists++;
    }
    return false;
}

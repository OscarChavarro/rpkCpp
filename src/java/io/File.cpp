#include "java/io/File.h"

namespace java {
namespace io {

File::File():
    path()
{
}

File::File(const char *path):
    path(path)
{
}

File::File(const java::lang::String &path):
    path(path)
{
}

File::~File() {
    dispose();
}

void
File::dispose() {
    path.dispose();
}

const java::lang::String &
File::getPath() const {
    return path;
}

java::lang::String
File::getName() const {
    const int separator = path.indexOf('/');
    if ( separator < 0 ) {
        return path;
    }
    int tailStart = separator + 1;
    int lastSeparator = separator;
    while ( true ) {
        const int nextSeparator = path.indexOf('/', tailStart);
        if ( nextSeparator < 0 ) {
            break;
        }
        lastSeparator = nextSeparator;
        tailStart = nextSeparator + 1;
    }
    return path.substring(lastSeparator + 1);
}

java::lang::String
File::getParent() const {
    int lastSeparator = -1;
    int searchFrom = 0;
    while ( true ) {
        const int nextSeparator = path.indexOf('/', searchFrom);
        if ( nextSeparator < 0 ) {
            break;
        }
        lastSeparator = nextSeparator;
        searchFrom = nextSeparator + 1;
    }
    if ( lastSeparator < 0 ) {
        return java::lang::String();
    }
    return path.substring(0, lastSeparator);
}

bool
File::isEmpty() const {
    return path.isEmpty();
}

}
}

#include "java/io/BufferedInputStream.h"
#include "java/io/FileInputStream.h"

namespace java {
namespace io {

BufferedInputStream::BufferedInputStream(InputStream *inputStream, bool ownInputStream):
    inputStream(inputStream),
    ownInputStream(ownInputStream)
{
}

BufferedInputStream::~BufferedInputStream() {
    dispose();
}

bool
BufferedInputStream::isOpen() const {
    if ( inputStream == nullptr ) {
        return false;
    }
    const FileInputStream *fileInputStream = dynamic_cast<const FileInputStream *>(inputStream);
    return fileInputStream != nullptr ? fileInputStream->isOpen() : true;
}

int
BufferedInputStream::read() {
    if ( inputStream == nullptr ) {
        return -1;
    }
    return inputStream->read();
}

int
BufferedInputStream::read(unsigned char *buffer, int offset, int length) {
    if ( inputStream == nullptr ) {
        return -1;
    }
    return inputStream->read(buffer, offset, length);
}

long
BufferedInputStream::tell() const {
    if ( inputStream == nullptr ) {
        return -1;
    }
    const FileInputStream *fileInputStream = dynamic_cast<const FileInputStream *>(inputStream);
    return fileInputStream != nullptr ? fileInputStream->tell() : -1;
}

void
BufferedInputStream::close() {
    dispose();
}

void
BufferedInputStream::dispose() {
    if ( inputStream == nullptr ) {
        return;
    }
    inputStream->dispose();
    if ( ownInputStream ) {
        delete inputStream;
    }
    inputStream = nullptr;
}

}
}

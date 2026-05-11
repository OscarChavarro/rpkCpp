#ifndef FILE_UNCOMPRESS_WRAPPER__
#define FILE_UNCOMPRESS_WRAPPER__

#include "vsdk/toolkit/java/io/InputStream.h"
#include "vsdk/toolkit/java/io/OutputStream.h"
#include "vsdk/toolkit/io/wrapper/StreamOpenMode.h"

class FileUncompressWrapper {
  public:
    static java::InputStream *openInputStreamCompressWrapper(const char *fileName, int *isPipe);
    static java::OutputStream *openOutputStreamCompressWrapper(const char *fileName, int *isPipe);
    static void closeInputStream(java::InputStream *stream);
    static void closeOutputStream(java::OutputStream *stream);

  private:
    static const char *modeToLogAction(StreamOpenMode mode);
    static bool isInvalidFileName(const char *fileName);
    static bool buildPipeCommand(const char *fileName, StreamOpenMode openMode, char *command, int commandLength);
    static java::InputStream *openPipeInputStream(const char *command);
    static java::OutputStream *openPipeOutputStream(const char *command);
};

#endif

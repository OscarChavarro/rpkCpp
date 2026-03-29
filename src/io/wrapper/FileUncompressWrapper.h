#ifndef __FILE_UNCOMPRESS_WRAPPER__
#define __FILE_UNCOMPRESS_WRAPPER__

#include "java/io/InputStream.h"
#include "java/io/OutputStream.h"
#include "io/wrapper/StreamOpenMode.h"

class FileUncompressWrapper {
  public:
    static java::io::InputStream *openInputStreamCompressWrapper(const char *fileName, int *isPipe);
    static java::io::OutputStream *openOutputStreamCompressWrapper(const char *fileName, int *isPipe);
    static void closeInputStream(java::io::InputStream *stream);
    static void closeOutputStream(java::io::OutputStream *stream);

  private:
    static const char *modeToLogAction(StreamOpenMode mode);
    static bool isInvalidFileName(const char *fileName);
    static int buildPipeCommand(const char *fileName, StreamOpenMode openMode, char *command, int commandLength);
    static java::io::InputStream *openPipeInputStream(const char *command);
    static java::io::OutputStream *openPipeOutputStream(const char *command);
};

#endif

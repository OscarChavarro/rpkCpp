#ifndef __FILE_UNCOMPRESS_WRAPPER__
#define __FILE_UNCOMPRESS_WRAPPER__

#include "java/io/InputStream.h"
#include "java/io/OutputStream.h"
#include "io/wrapper/StreamOpenMode.h"

class FileUncompressWrapper {
  public:
    static InputStream *openInputStreamCompressWrapper(const char *fileName, int *isPipe);
    static OutputStream *openOutputStreamCompressWrapper(const char *fileName, int *isPipe);
    static void closeInputStream(InputStream *stream);
    static void closeOutputStream(OutputStream *stream);

  private:
    static const char *modeToLogAction(StreamOpenMode mode);
    static bool isInvalidFileName(const char *fileName);
    static bool buildPipeCommand(const char *fileName, StreamOpenMode openMode, char *command, int commandLength);
    static InputStream *openPipeInputStream(const char *command);
    static OutputStream *openPipeOutputStream(const char *command);
};

#endif

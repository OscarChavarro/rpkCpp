#ifndef JAVA_IO_FILE__
#define JAVA_IO_FILE__

#include "vsdk/toolkit/java/lang/String.h"

namespace java {

class File {
  private:
    java::String path;

    static bool
    isValidPath(const char *rawPath);

    static bool
    canOpenWithMode(const char *rawPath, const char *mode, int *errorCode = nullptr);

    static bool
    isDirectoryByReadProbe(const char *rawPath);

  public:
    File();
    explicit File(const char *path);
    explicit File(const java::String &path);
    ~File();

    void
    dispose();

    java::String
    getName() const;

    bool
    exists() const;

    bool
    isDirectory() const;

    bool
    isFile() const;

    bool
    canRead() const;

    bool
    canWrite() const;
};

}

#endif

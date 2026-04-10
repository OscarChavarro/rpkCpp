#ifndef __JAVA_IO_FILE__
#define __JAVA_IO_FILE__

#include "java/lang/String.h"


class File {
  private:
    String path;

    static bool
    isValidPath(const char *rawPath);

    static bool
    canOpenWithMode(const char *rawPath, const char *mode, int *errorCode = NULL);

    static bool
    isDirectoryByReadProbe(const char *rawPath);

  public:
    File();
    explicit File(const char *path);
    explicit File(const String &path);
    ~File();

    void
    dispose();

    String
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


#endif

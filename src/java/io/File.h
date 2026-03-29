#ifndef __JAVA_IO_FILE__
#define __JAVA_IO_FILE__

#include "java/lang/String.h"

namespace java {
namespace io {

class File {
  private:
    java::lang::String path;

    static bool
    isValidPath(const char *rawPath);

    static bool
    canOpenWithMode(const char *rawPath, const char *mode, int *errorCode = nullptr);

    static bool
    isDirectoryByReadProbe(const char *rawPath);

  public:
    File();
    explicit File(const char *path);
    explicit File(const java::lang::String &path);
    ~File();

    void
    dispose();

    java::lang::String
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
}

#endif

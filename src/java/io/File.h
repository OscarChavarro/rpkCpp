#ifndef __JAVA_IO_FILE__
#define __JAVA_IO_FILE__

#include "java/lang/String.h"

namespace java {
namespace io {

class File {
  private:
    java::lang::String path;

  public:
    File();
    explicit File(const char *path);
    explicit File(const java::lang::String &path);
    ~File();

    void
    dispose();

    const java::lang::String &
    getPath() const;

    java::lang::String
    getName() const;

    java::lang::String
    getParent() const;

    bool
    isEmpty() const;

    bool
    exists() const;

    bool
    isDirectory() const;

    bool
    canRead() const;

    bool
    canWrite() const;
};

}
}

#endif

#ifndef __JAVA_IO_FILE__
#define __JAVA_IO_FILE__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
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
    getPath() const;

    java::String
    getName() const;

    /** Path resolved against the current working directory (not normalized,
    as in the JDK). */
    java::String
    getAbsolutePath() const;

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

    bool
    mkdirs() const;

    /**
    Names of the entries of this directory (no "." / ".."), in unspecified
    order as in the JDK. Empty list if this is not a readable directory.
    */
    java::ArrayList<java::String>
    list() const;

    /** Same as list(), but as File objects; the caller owns them. */
    java::ArrayList<java::File*>
    listFiles() const;
};

}

#endif

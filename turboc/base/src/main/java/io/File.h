#ifndef __JAVA_IO_FILE__
#define __JAVA_IO_FILE__

#include <stddef.h>
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"
#include "java/util/ArrayList.h"

class File {
  private:
String path;

static BOOLEAN
isValidPath(const char *rawPath);

static BOOLEAN
canOpenWithMode(const char *rawPath, const char *mode, int *errorCode = NULL);

static BOOLEAN
isDirectoryByReadProbe(const char *rawPath);

  public:
File();
explicit File(const char *path);
explicit File(const String &path);
~File();

void
dispose();

/**
Name for the C library of a path with the standard (long) names of the tree.
With Turbo C++ (DOS) the folders and files of etc/ take their 8.3 names (see
turboc/scripts/mappings_83.csv; map_to_83.py generates their table,
java/io/NAMES83.H) and the other names are cut to 8.3, as DOS does; with the
other compilers it is the path itself.
*/
static String
toPlatformName(const String &standardName);

String
getPath() const;

String
getName() const;

/** Path resolved against the current working directory (not normalized,
as in the JDK). */
String
getAbsolutePath() const;

BOOLEAN
exists() const;

BOOLEAN
isDirectory() const;

BOOLEAN
isFile() const;

BOOLEAN
canRead() const;

BOOLEAN
canWrite() const;

BOOLEAN
mkdirs() const;

/**
Names of the entries of this directory (no "." / ".."), in unspecified
order as in the JDK. Empty list if this is not a readable directory.
*/
ArrayList<String>
list() const;

/** Same as list(), but as File objects; the caller owns them. */
ArrayList<File*>
listFiles() const;
};


#endif

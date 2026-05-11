#ifndef JAVA_LANG_SYSTEM__
#define JAVA_LANG_SYSTEM__

#include "java/io/FileOutputStream.h"
#include "java/io/PrintStream.h"

namespace java {

class System {
  public:
    static java::PrintStream out;
    static java::PrintStream err;
    [[noreturn]] static void exit(int status);
    static long long nanoTime();

  private:
    static java::FileOutputStream standardOutput;
    static java::FileOutputStream standardError;
};

}

#endif

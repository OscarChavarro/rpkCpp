#ifndef __JAVA_LANG_SYSTEM__
#define __JAVA_LANG_SYSTEM__

#include "java/io/FileOutputStream.h"
#include "java/io/PrintStream.h"


class System {
  public:
    static PrintStream out;
    static PrintStream err;
    static void exit(int status);
    static long nanoTime();

  private:
    static FileOutputStream standardOutput;
    static FileOutputStream standardError;
};


#endif

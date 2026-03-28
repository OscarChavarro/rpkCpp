#ifndef __JAVA_LANG_SYSTEM__
#define __JAVA_LANG_SYSTEM__

#include "java/io/PrintStream.h"

namespace java {
namespace lang {

class System {
  public:
    static java::io::PrintStream out;
    static java::io::PrintStream err;
    [[noreturn]] static void exit(int status);
    static long long nanoTime();
};

}
}

#endif

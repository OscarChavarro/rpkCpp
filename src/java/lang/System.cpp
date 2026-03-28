#include "java/lang/System.h"

#include "java/io/FileOutputStream.h"

namespace java {
namespace lang {

namespace {
#if defined(_WIN32)
java::io::FileOutputStream globalSystemOutStream("CONOUT$");
java::io::FileOutputStream globalSystemErrStream("CONOUT$");
#else
java::io::FileOutputStream globalSystemOutStream("/dev/stdout");
java::io::FileOutputStream globalSystemErrStream("/dev/stderr");
#endif
}

java::io::PrintStream System::out(&globalSystemOutStream);
java::io::PrintStream System::err(&globalSystemErrStream);

}
}

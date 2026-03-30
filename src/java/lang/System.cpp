#include <cstdlib>
#include <chrono>

#include "java/io/FileOutputStream.h"
#include "java/lang/System.h"

namespace java {
namespace lang {

java::io::FileOutputStream globalSystemOutStream("/dev/stdout");
java::io::FileOutputStream globalSystemErrStream("/dev/stderr");
java::io::PrintStream System::out(&globalSystemOutStream);
java::io::PrintStream System::err(&globalSystemErrStream);

[[noreturn]] void
System::exit(int status) {
    std::exit(status);
}

long long
System::nanoTime() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

}
}

#include <cstdlib>
#include <chrono>

#include "java/io/FileOutputStream.h"
#include "java/lang/System.h"

namespace java {

java::FileOutputStream standardOutput("/dev/stdout");
java::FileOutputStream standardError("/dev/stderr");
java::PrintStream System::out(&standardOutput);
java::PrintStream System::err(&standardError);

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

#include <cstdlib>
#include <chrono>

#include "java/lang/System.h"

namespace java {

java::FileOutputStream System::standardOutput("/dev/stdout");
java::FileOutputStream System::standardError("/dev/stderr");
java::PrintStream System::out(&System::standardOutput);
java::PrintStream System::err(&System::standardError);

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

#include <stdlib.h>
#include <time.h>

#include "java/lang/System.h"


FileOutputStream System::standardOutput("/dev/stdout");
FileOutputStream System::standardError("/dev/stderr");
PrintStream System::out(&System::standardOutput);
PrintStream System::err(&System::standardError);

void
System::exit(int status) {
    ::exit(status);
}

long
System::nanoTime() {
    const clock_t now = clock();
    if ( now <= 0 ) {
        return 0;
    }
    return ((long)(((double)(now)) * 1000000000.0 / ((double)(CLOCKS_PER_SEC))));
}

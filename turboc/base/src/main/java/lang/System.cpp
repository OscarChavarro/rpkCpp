#include <stdlib.h>
#include <string.h>
#ifdef __TURBOC__
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/concurrent/locks/ReentrantLock.h"
#include "java/util/TreeMap.h"

#include "java/lang/System.h"

// Not private static members, as Turbo C++ 3.0 does not let the
// initializers of out and err use them
static FileOutputStream standardOutput("/dev/stdout");
static FileOutputStream standardError("/dev/stderr");
PrintStream System::out(&standardOutput);
PrintStream System::err(&standardError);

void
System::exit(int status) {
    ::exit(status);
}

#ifdef __TURBOC__
// DOS has no gettimeofday: millisecond resolution wall clock (its 32 bit
// long long holds milliseconds since 1970 until 1994 only, so the times
// count from the first call)
static long long
millisecondsSinceFirstCall() {
    static long firstSecond = -1;
    struct timeb now;
    ftime(&now);
    if ( firstSecond < 0 ) {
        firstSecond = (long)now.time;
    }
    return ((long long)now.time - firstSecond) * 1000 + now.millitm;
}

long long
System::nanoTime() {
    return millisecondsSinceFirstCall() * 1000000;
}

long long
System::currentTimeMillis() {
    return millisecondsSinceFirstCall();
}
#else
long long
System::nanoTime() {
    // C++98 has no monotonic clock: microsecond resolution wall clock
    struct timeval now;
    gettimeofday(&now, NULL);
    return (long long)now.tv_sec * VSDK_LL(1000000000) + (long long)now.tv_usec * VSDK_LL(1000);
}

long long
System::currentTimeMillis() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return (long long)now.tv_sec * VSDK_LL(1000) + (long long)now.tv_usec / VSDK_LL(1000);
}
#endif


static TreeMap<String, String>& properties()
{
    static TreeMap<String, String> table;
    return table;
}

static ReentrantLock& propertiesLock()
{
    static ReentrantLock lock;
    return lock;
}


String
System::getProperty(const String& key) {
    return getProperty(key, "");
}

String
System::getProperty(const String& key, const String& defaultValue) {
    ReentrantLockGuard guard(propertiesLock());
    const String *found = properties().get(key);
    if ( found == NULL ) {
        return defaultValue;
    }
    return *found;
}

void
System::setProperty(const String& key, const String& value) {
    ReentrantLockGuard guard(propertiesLock());
    properties().put(key, value);
}

void
System::setPropertiesFromArguments(int argc, char* argv[]) {
    for ( int i = 1; i < argc; i++ ) {
        if ( argv[i] == NULL || strncmp(argv[i], "-D", 2) != 0 ) {
            continue;
        }
        String definition(argv[i] + 2);
        int equals = definition.indexOf('=');
        if ( equals < 0 ) {
            setProperty(definition, "true");
        }
        else {
            setProperty(definition.substring(0, equals),
                        definition.substring(equals + 1));
        }
    }
}


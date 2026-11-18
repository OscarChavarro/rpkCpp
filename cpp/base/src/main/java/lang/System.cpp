#include "java/lang/String.h"
#include "java/util/TreeMap.txx"
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "java/util/concurrent/locks/ReentrantLock.h"

#include "java/lang/System.h"
namespace java {

// The C standard streams, shared with `printf` and `fprintf(stderr, ...)`
// (opening "/dev/stdout" again would give a second file position that
// overwrites the output redirected to a regular file); autoflush as Java's
java::FileOutputStream System::standardOutput(stdout);
java::FileOutputStream System::standardError(stderr);
java::PrintStream System::out(&System::standardOutput, true);
java::PrintStream System::err(&System::standardError, true);

[[noreturn]] void
System::exit(int status) {
    std::exit(status);
}

long long
System::nanoTime() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<long long>(now.tv_sec) * 1000000000LL + now.tv_nsec;
}

long long
System::currentTimeMillis() {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return static_cast<long long>(now.tv_sec) * 1000LL + now.tv_nsec / 1000000;
}

namespace {

java::TreeMap<java::String, java::String>& properties()
{
    static java::TreeMap<java::String, java::String> table;
    return table;
}

java::ReentrantLock& propertiesLock()
{
    static java::ReentrantLock lock;
    return lock;
}

}

java::String
System::getProperty(const java::String& key) {
    return getProperty(key, "");
}

java::String
System::getProperty(const java::String& key, const java::String& defaultValue) {
    java::ReentrantLockGuard guard(propertiesLock());
    const java::String *found = properties().get(key);
    if ( found == nullptr ) {
        return defaultValue;
    }
    return *found;
}

void
System::setProperty(const java::String& key, const java::String& value) {
    java::ReentrantLockGuard guard(propertiesLock());
    properties().put(key, value);
}

void
System::setPropertiesFromArguments(int argc, char* argv[]) {
    for ( int i = 1; i < argc; i++ ) {
        if ( argv[i] == nullptr || std::strncmp(argv[i], "-D", 2) != 0 ) {
            continue;
        }
        java::String definition(argv[i] + 2);
        int equals = definition.find('=');
        if ( equals == -1 ) {
            setProperty(definition.c_str(), "true");
        }
        else {
            setProperty(definition.substr(0, equals).c_str(),
                        definition.substr(equals + 1).c_str());
        }
    }
}

}

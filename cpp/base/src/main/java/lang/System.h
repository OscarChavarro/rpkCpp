#ifndef __SYSTEM__
#define __SYSTEM__

#include "java/io/FileOutputStream.h"
#include "java/io/PrintStream.h"
#include "java/lang/String.h"
namespace java {

class System {
  public:
    static java::PrintStream out;
    static java::PrintStream err;
    [[noreturn]] static void exit(int status);
    static long long nanoTime();
    static long long currentTimeMillis();

    /**
    @param key name of a system property
    @return its value, or an empty string if it is not set (Java returns
    null)
    */
    static java::String getProperty(const java::String& key);

    /**
    @param key name of a system property
    @param defaultValue value returned if the property is not set
    @return the value of the property, or the default value
    */
    static java::String getProperty(const java::String& key,
                                    const java::String& defaultValue);

    /**
    Sets a system property (as `-Dkey=value` does in Java).
    @param key name of the property
    @param value value of the property
    */
    static void setProperty(const java::String& key, const java::String& value);

    /**
    Sets the system properties given as `-Dkey=value` (or `-Dkey`, which
    means `true`) command line arguments; the others are ignored.
    @param argc number of arguments
    @param argv arguments
    */
    static void setPropertiesFromArguments(int argc, char* argv[]);

  private:
    static java::FileOutputStream standardOutput;
    static java::FileOutputStream standardError;
};

}

#endif

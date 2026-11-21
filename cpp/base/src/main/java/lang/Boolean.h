#ifndef __BOOLEAN__
#define __BOOLEAN__

#include "java/lang/String.h"
#include "java/lang/System.h"

namespace java {

/**
Emulation of the static helpers of Java's `Boolean`.
*/
class Boolean {
  public:
    /**
    @param name name of a system property (see `System::setProperty`)
    @return true if the property exists and is "true", ignoring case
    */
    static bool getBoolean(const java::String& name) {
        return System::getProperty(name).equalsIgnoreCase("true");
    }

  private:
    Boolean() {}
};

}

#endif

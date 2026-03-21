#include "java/lang/System.h"

namespace java {
namespace lang {

java::io::PrintStream System::out(stdout);
java::io::PrintStream System::err(stderr);

}
}

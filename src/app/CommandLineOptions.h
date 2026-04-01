#ifndef __APP_COMMAND_LINE_OPTIONS_TYPE__
#define __APP_COMMAND_LINE_OPTIONS_TYPE__

#include "java/io/PrintStream.h"

class CommandLineOptions {
  public:
    bool (*get)(void *value, void *data); // Retrieves a argument value
    void (*print)(java::PrintStream *stream, void *value, void *data); // Prints an argument value
    void *dummy; // Pointer to "dummy" argument value storage
    void *data; // Pointer to additional data
};

#endif

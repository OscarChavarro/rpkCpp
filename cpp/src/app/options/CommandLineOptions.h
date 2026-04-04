#ifndef __APP_COMMAND_LINE_OPTIONS_TYPE__
#define __APP_COMMAND_LINE_OPTIONS_TYPE__

#include "java/io/PrintStream.h"

enum class OptionKind {
    BOOL,
    INT,
    FLOAT,
    STRING,
    VECTOR3D,
    COLORRGB,
    UNKNOWN
};

struct OptionValueWrapper {
    void *ptr;
    OptionKind kind;

    OptionValueWrapper():
        ptr(nullptr),
        kind(OptionKind::UNKNOWN) {
    }

    OptionValueWrapper(void *p):
        ptr(p),
        kind(OptionKind::UNKNOWN) {
    }

    OptionValueWrapper(void *p, OptionKind k):
        ptr(p),
        kind(k) {
    }
};

class CommandLineOptions {
  public:
    bool (*get)(OptionValueWrapper value, void *data);
    void (*print)(java::PrintStream *stream, OptionValueWrapper value, void *data);
    OptionValueWrapper dummy;
    void *data;
};

#endif

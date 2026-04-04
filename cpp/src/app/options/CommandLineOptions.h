#ifndef __APP_COMMAND_LINE_OPTIONS_TYPE__
#define __APP_COMMAND_LINE_OPTIONS_TYPE__

enum class OptionKind {
    BOOL,
    INT,
    FLOAT,
    STRING,
    VECTOR3D,
    COLORRGB,
    UNKNOWN
};

enum class OptionDispatch {
    AUTO,
    ENUM,
    FIXED_STRING,
    SET_TRUE,
    SET_FALSE,
    CIE_XY
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

#endif

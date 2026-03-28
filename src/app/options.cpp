/**
Command line options and defaults
*/

#include <cstring>
#include <cstdlib>
#include "java/lang/System.h"
#include "java/lang/Integer.h"
#include <cerrno>
#include <cstdint>

#include "java/util/ArrayList.txx"
#include "java/util/Formatter.h"
#include "common/linealAlgebra/Vector3D.h"
#include "common/ColorRgb.h"
#include "app/options.h"

char *GLOBAL_option_dummyVal = nullptr;
int GLOBAL_options_dummyVal = 0;

static int *globalArgumentCount;
static char **globalArguments = nullptr;
static int globalCurrentArgumentIndex = 0;
static int globalDummyInt = 0;
static char *globalDummyString = nullptr;
static int globalDummyTrue = true;
static int globalDummyFalse = false;
static float globalDummyFloat = 0.0f;
static Vector3D globalDummyVector3D = {0.0f, 0.0f, 0.0f};
static ColorRgb globalDummyRgb{};
static float globalDummyCieXy[2] = {0.0f, 0.0f};
static java::ArrayList<char *> *globalStringsToDelete = new java::ArrayList<char *>();

/**
Initializes the global variables above
*/
static void
optionsInitArguments(int *argc, char **argv) {
    globalArgumentCount = argc;
    globalArguments = argv;
    globalCurrentArgumentIndex = 0;
}

static const char *
optionsCurrentArgumentValue() {
    if ( globalArguments == nullptr || globalArgumentCount == nullptr ) {
        return nullptr;
    }
    if ( globalCurrentArgumentIndex < 0 || globalCurrentArgumentIndex >= *globalArgumentCount ) {
        return nullptr;
    }
    return globalArguments[globalCurrentArgumentIndex];
}

/**
Tests whether arguments remain
*/
static bool
optionsArgumentsRemaining() {
    return globalCurrentArgumentIndex < *globalArgumentCount;
}

/**
Skips to next argument value
*/
static void
optionsNextArgument() {
    globalCurrentArgumentIndex++;
}

/**
Consumes the current argument value, that is: removes it from the list
*/
static void
optionsConsumeArgument() {
    for ( int i = globalCurrentArgumentIndex; i < *globalArgumentCount - 1; i++ ) {
        globalArguments[i] = globalArguments[i + 1];
    }
    globalArguments[*globalArgumentCount - 1] = nullptr;
    (*globalArgumentCount)--;
}

/**
Scans the current argument value for a value of given format
*/
static bool
optionsGetArgumentIntValue(int *res) {
    const char *currentArgument = optionsCurrentArgumentValue();
    if ( currentArgument == nullptr ) {
        return false;
    }

    errno = 0;
    char *endPointer = nullptr;
    const long parsedValue = strtol(currentArgument, &endPointer, 10);

    if ( endPointer == currentArgument || *endPointer != '\0' ) {
        return false;
    }
    if ( errno == ERANGE
         || parsedValue < static_cast<long>(java::Integer::MIN_VALUE)
         || parsedValue > static_cast<long>(java::Integer::MAX_VALUE) ) {
        return false;
    }

    *res = static_cast<int>(parsedValue);
    return true;
}

/**
Scans the current argument value for a value of given format
*/
static bool
optionsGetArgumentFloatValue(const char * /*format*/, float *res) {
    const char *currentArgument = optionsCurrentArgumentValue();
    if ( currentArgument == nullptr || res == nullptr ) {
        return false;
    }

    errno = 0;
    char *endPointer = nullptr;
    const float parsedValue = strtof(currentArgument, &endPointer);
    if ( endPointer == currentArgument || *endPointer != '\0' || errno == ERANGE ) {
        return false;
    }

    *res = parsedValue;
    return true;
}

/**
Integer option values
*/
static int
optionsGetInt(void *value, void * /*data*/) {
    int *n = static_cast<int *>(value);
    if ( !optionsGetArgumentIntValue(n) ) {
        java::lang::System::err.printf("'%s' is not a valid integer value\n", optionsCurrentArgumentValue());
        return false;
    }
    return true;
}

static void
optionsPrintInt(java::io::PrintStream *stream, void *value, void * /*data*/) {
    int *n = static_cast<int *>(value);
    if ( stream != nullptr ) {
        stream->printf("%d", *n);
    }
}

CommandLineOptions GLOBAL_options_intType = {
    optionsGetInt,
    optionsPrintInt,
    static_cast<void *>(&globalDummyInt),
    nullptr
};

/**
String option values
*/
static int
optionsGetString(void *value, void * /*data*/) {
    char **s = static_cast<char **>(value);
    const char *currentArgument = optionsCurrentArgumentValue();
    if ( currentArgument == nullptr ) {
        return false;
    }
    unsigned long n = strlen(currentArgument) + 1;
    *s = new char[n];

    if ( globalStringsToDelete != nullptr ) {
        globalStringsToDelete->add(*s);
    }
    java::util::Formatter::formatToBuffer(*s, static_cast<int>(n), "%s", currentArgument);
    return true;
}

static void
optionsPrintString(java::io::PrintStream *stream, void *value, void * /*data*/) {
    char **s = static_cast<char **>(value);
    if ( stream != nullptr ) {
        stream->printf("'%s'", *s ? *s : "");
    }
}

CommandLineOptions GLOBAL_options_stringType = {
    optionsGetString,
    optionsPrintString,
    static_cast<void *>(&globalDummyString),
    nullptr
};

/**
Copied string (maxlength n) option values
*/
int
optionsStringGet(void *value, void *data) {
    char *s = static_cast<char *>(value);
    int n = static_cast<int>(reinterpret_cast<intptr_t>(data));
    const char *currentArgument = optionsCurrentArgumentValue();
    if ( s != nullptr && currentArgument != nullptr ) {
        strncpy(s, currentArgument, n);
        s[n - 1] = '\0';  // Ensure zero ending c-string
    }

    return true;
}

void
optionsStringPrint(java::io::PrintStream *stream, void *value, void * /*data*/) {
    const char *s = static_cast<const char *>(value);
    if ( stream != nullptr ) {
        stream->printf("'%s'", s ? s : "");
    }
}

/**
Enumerated type option values
*/
static void
optionsPrintEnumValues(const ENUMDESC *tab) {
    for ( int i = 0; tab != nullptr && tab[i].name != nullptr; i++ ) {
        java::lang::System::err.printf("\t%s\n", tab[i].name);
    }
}

int
optionsEnumGet(void *value, void *data) {
    int *v = static_cast<int *>(value);
    const ENUMDESC *tab = static_cast<const ENUMDESC *>(data);
    const ENUMDESC *tabSave = tab;
    const char *currentArgument = optionsCurrentArgumentValue();
    if ( currentArgument == nullptr ) {
        return false;
    }
    for ( int i = 0; tab != nullptr && tab[i].name != nullptr; i++ ) {
        if ( strncasecmp(currentArgument, tab[i].name, tab[i].abbrev) == 0 ) {
            *v = tab[i].value;
            return true;
        }
    }
    java::lang::System::err.printf("Invalid option argument '%s'. Should be one of:\n", currentArgument);
    optionsPrintEnumValues(tabSave);
    return false;
}

void
optionsEnumPrint(java::io::PrintStream *stream, void *value, void *data) {
    const int *v = static_cast<const int *>(value);
    const ENUMDESC *tab = static_cast<const ENUMDESC *>(data);
    if ( stream == nullptr ) {
        return;
    }
    for ( int i = 0; tab != nullptr && tab[i].name != nullptr; i++ ) {
        if ( *v == tab[i].value ) {
            stream->printf("%s", tab[i].name);
            return;
        }
    }
    stream->printf("%s", "INVALID ENUM VALUE!!!");
}

/* ------------------- boolean (yes|no) option values-------------------- */
/* implemented as an enumeration type */

static ENUMDESC boolTable[] = {
        {true,  "yes",   1},
        {false, "no",    1},
        {true,  "true",  1},
        {false, "false", 1},
        {0, nullptr,        0}
};

CommandLineOptions GLOBAL_options_boolType = {
    optionsEnumGet,
    optionsEnumPrint,
    static_cast<void *>(&GLOBAL_options_dummyVal),
    static_cast<void *>(boolTable)
};

/* ------------------- set true/false option values --------------------- */

static int
optionsSetTrue(void *value, void * /*data*/) {
    int *x = static_cast<int *>(value);
    // No option expected on command line, nothing consumed

    *x = true;
    return true;
}

static int
optionsSetFalse(void *value, void * /*data*/) {
    int *x = static_cast<int *>(value);
    /* No option expected on command line, nothing consumed */

    *x = false;
    return true;
}

static void
optionsPrintOther(java::io::PrintStream *stream, void * /*x*/, void * /*data*/) {
    if ( stream != nullptr ) {
        stream->printf("%s", "other");
    }
}


CommandLineOptions GLOBAL_options_setTrueType = {
    optionsSetTrue,
    optionsPrintOther,
    static_cast<void *>(&globalDummyTrue),
    nullptr
};

CommandLineOptions GLOBAL_options_setFalseType = {
    optionsSetFalse,
    optionsPrintOther,
    static_cast<void *>(&globalDummyFalse),
    nullptr
};

/* ------------------- float option values --------------------- */
static int
optionsGetfloat(void *value, void * /*data*/) {
    float *x = static_cast<float *>(value);
    if ( !optionsGetArgumentFloatValue("%f", x) ) {
        java::lang::System::err.printf("'%s' is not a valid floating point value\n", optionsCurrentArgumentValue());
        return false;
    }
    return true;
}

static void
optionsPrintFloat(java::io::PrintStream *stream, void *value, void * /*data*/) {
    const float *x = static_cast<const float *>(value);
    if ( stream != nullptr ) {
        stream->printf("%g", *x);
    }
}

CommandLineOptions GLOBAL_options_floatType = {
        optionsGetfloat,
        optionsPrintFloat,
        static_cast<void *>(&globalDummyFloat),
        nullptr
};

/**
Vector3D option values
*/
static int
optionsGetVector(void *value, void * /*data*/) {
    Vector3D *v = static_cast<Vector3D *>(value);
    int ok = optionsGetArgumentFloatValue("%f", &v->x);
    if ( ok ) {
        optionsConsumeArgument();
        ok &= optionsArgumentsRemaining() && optionsGetArgumentFloatValue("%f", &v->y);
    }
    if ( ok ) {
        optionsConsumeArgument();
        ok &= optionsArgumentsRemaining() && optionsGetArgumentFloatValue("%f", &v->z);
    }
    if ( !ok ) {
        java::lang::System::err.printf("invalid vector argument value");
    }

    return ok;
}

static void
optionsPrintVector(java::io::PrintStream *stream, void *value, void * /*data*/) {
    const Vector3D *v = static_cast<const Vector3D *>(value);
    if ( stream != nullptr && v != nullptr ) {
        stream->printf("%g %g %g", v->x, v->y, v->z);
    }
}

CommandLineOptions GLOBAL_options_vectorType = {
    optionsGetVector,
    optionsPrintVector,
    static_cast<void *>(&globalDummyVector3D),
    nullptr
};

/**
RGB option values
*/
static int
optionsGetRgb(void *value, void * /*data*/) {
    ColorRgb *c = static_cast<ColorRgb *>(value);
    int ok = optionsGetArgumentFloatValue("%f", &c->r);
    if ( ok ) {
        optionsConsumeArgument();
        ok &= optionsArgumentsRemaining() && optionsGetArgumentFloatValue("%f", &c->g);
    }
    if ( ok ) {
        optionsConsumeArgument();
        ok &= optionsArgumentsRemaining() && optionsGetArgumentFloatValue("%f", &c->b);
    }
    if ( !ok ) {
        java::lang::System::err.printf("invalid RGB color argument value");
    }

    return ok;
}

static void
optionsPrintRgb(java::io::PrintStream *stream, void *value, void * /*data*/) {
    const ColorRgb *v = static_cast<const ColorRgb *>(value);
    if ( stream != nullptr && v != nullptr ) {
        stream->printf("%g %g %g", v->r, v->g, v->b);
    }
}

CommandLineOptions GLOBAL_options_rgbType = {
    optionsGetRgb,
    optionsPrintRgb,
    static_cast<void *>(&globalDummyRgb),
    nullptr
};

/**
CIE xy option values
*/

static int
optionsGetCieXy(void *value, void * /*data*/) {
    float *c = static_cast<float *>(value);
    int ok = optionsGetArgumentFloatValue("%f", &c[0]);
    if ( ok ) {
        optionsConsumeArgument();
        ok &= optionsArgumentsRemaining() && optionsGetArgumentFloatValue("%f", &c[1]);
    }
    if ( !ok ) {
        java::lang::System::err.printf("invalid CIE xy color argument value");
    }

    return ok;
}

static void
optionsPrintCieXyCallBack(java::io::PrintStream *stream, void *value, void * /*data*/) {
    const float *c = static_cast<const float *>(value);
    if ( stream != nullptr && c != nullptr ) {
        stream->printf("%g %g", c[0], c[1]);
    }
}

CommandLineOptions GLOBAL_options_xyType = {
    optionsGetCieXy,
    optionsPrintCieXyCallBack,
    static_cast<void *>(&globalDummyCieXy),
    nullptr
};

CommandLineOptions *const OPTIONS_TYPE_BOOL = &GLOBAL_options_boolType;
CommandLineOptions *const OPTIONS_TYPE_SET_TRUE = &GLOBAL_options_setTrueType;
CommandLineOptions *const OPTIONS_TYPE_SET_FALSE = &GLOBAL_options_setFalseType;
CommandLineOptions *const OPTIONS_TYPE_STRING = &GLOBAL_options_stringType;
CommandLineOptions *const OPTIONS_TYPE_FLOAT = &GLOBAL_options_floatType;
CommandLineOptions *const OPTIONS_TYPE_VECTOR = &GLOBAL_options_vectorType;
CommandLineOptions *const OPTIONS_TYPE_RGB = &GLOBAL_options_rgbType;
CommandLineOptions *const OPTIONS_TYPE_XY = &GLOBAL_options_xyType;
void (*const DEFAULT_ACTION)(void *) = nullptr;

/**
Argument parsing
*/

static unsigned long
unsignedLongMax(unsigned long a, unsigned long b) {
    return a > b ? a : b;
}

static CommandLineOptionDescription *
optionsLookupOption(const char *s, CommandLineOptionDescription *options) {
    if ( s == nullptr ) {
        return nullptr;
    }
    for ( int i = 0; options[i].name != nullptr; i++ ) {
        CommandLineOptionDescription *opt = &options[i];
        if ( strncmp(s, opt->name,
             unsignedLongMax(opt->abbreviationLength > 0 ? opt->abbreviationLength : strlen(opt->name), strlen(s))) ==
             0 ) {
            return opt;
        }
    }
    return nullptr;
}

static void
optionsProcessArguments(CommandLineOptionDescription *options) {
    CommandLineOptionDescription *opt = optionsLookupOption(optionsCurrentArgumentValue(), options);
    if ( opt ) {
        int ok = true;
        if ( opt->type ) {
            if ((opt->type == &GLOBAL_options_setTrueType) ||
                (opt->type == &GLOBAL_options_setFalseType) ) {
                if ( !opt->type->get(opt->value ? opt->value : opt->type->dummy, opt->type->data) ) {
                    ok = false;
                }
            } else {
                optionsConsumeArgument();
                if ( optionsArgumentsRemaining() ) {
                    if ( !opt->type->get(opt->value ? opt->value : opt->type->dummy, opt->type->data) ) {
                        ok = false;
                    }
                } else {
                    java::lang::System::err.printf("Option argument missing.\n");
                    ok = false;
                }
            }
        }
        if ( ok && opt->action ) {
            if ( opt->value != nullptr ) {
                opt->action(opt->value);
            } else {
                opt->action(opt->type ? opt->type->dummy : nullptr);
            }
        }
        optionsConsumeArgument();
    } else optionsNextArgument();
}

/**
Scans for options mentioned in the 'options' command line description
list, parses their value, executes their associated actions, and
removes them from the argv list (decreasing argc)
*/
void
parseGeneralOptions(CommandLineOptionDescription *options, int *argc, char **argv) {
    optionsInitArguments(argc, argv);
    while ( optionsArgumentsRemaining() ) {
        optionsProcessArguments(options);
    }
}

void
deleteOptionsMemory() {
    if ( globalStringsToDelete != nullptr ) {
        for ( int i = 0; i < globalStringsToDelete->size(); i++ ) {
            delete[] globalStringsToDelete->get(i);
        }
        delete globalStringsToDelete;
        globalStringsToDelete = nullptr;
    }
}

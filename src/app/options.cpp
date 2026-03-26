/**
Command line options and defaults
*/

#include <cstring>
#include "java/lang/System.h"
#include <cerrno>
#include <climits>
#include <cstdint>

#include "java/util/ArrayList.txx"
#include "common/linealAlgebra/Vector3D.h"
#include "common/ColorRgb.h"
#include "app/options.h"

char *GLOBAL_option_dummyVal = nullptr;
int GLOBAL_options_dummyVal = 0;

static int *globalArgumentCount;
static char **globalCurrentArgumentValue = nullptr;
static char **globalFirstArgument;
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
    globalCurrentArgumentValue = globalFirstArgument = argv;
}

/**
Tests whether arguments remain
*/
static bool
optionsArgumentsRemaining() {
    return globalCurrentArgumentValue - globalFirstArgument < *globalArgumentCount;
}

/**
Skips to next argument value
*/
static void
optionsNextArgument() {
    globalCurrentArgumentValue++;
}

/**
Consumes the current argument value, that is: removes it from the list
*/
static void
optionsConsumeArgument() {
    char **av = globalCurrentArgumentValue;
    while ( av - globalFirstArgument < *globalArgumentCount - 1 ) {
        *av = *(av + 1);
        av++;
    }
    *av = nullptr;
    (*globalArgumentCount)--;
}

/**
Scans the current argument value for a value of given format
*/
static bool
optionsGetArgumentIntValue(int *res) {
    if ( globalCurrentArgumentValue == nullptr || *globalCurrentArgumentValue == nullptr ) {
        return false;
    }

    errno = 0;
    char *endPointer = nullptr;
    const long parsedValue = strtol(*globalCurrentArgumentValue, &endPointer, 10);

    if ( endPointer == *globalCurrentArgumentValue || *endPointer != '\0' ) {
        return false;
    }
    if ( errno == ERANGE || parsedValue < INT_MIN || parsedValue > INT_MAX ) {
        return false;
    }

    *res = static_cast<int>(parsedValue);
    return true;
}

/**
Scans the current argument value for a value of given format
*/
static bool
optionsGetArgumentFloatValue(const char *format, float *res) {
    return (sscanf(*globalCurrentArgumentValue, format, res) == 1);
}

/**
Integer option values
*/
static int
optionsGetInt(void *value, void * /*data*/) {
    int *n = static_cast<int *>(value);
    if ( !optionsGetArgumentIntValue(n) ) {
        java::lang::System::err.printf("'%s' is not a valid integer value\n", *globalCurrentArgumentValue);
        return false;
    }
    return true;
}

static void
optionsPrintInt(FILE *fp, void *value, void * /*data*/) {
    int *n = static_cast<int *>(value);
    fprintf(fp, "%d", *n);
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
    unsigned long n = strlen(*globalCurrentArgumentValue) + 1;
    *s = new char[n];

    if ( globalStringsToDelete != nullptr ) {
        globalStringsToDelete->add(*s);
    }
    snprintf(*s, n, "%s", *globalCurrentArgumentValue);
    return true;
}

static void
optionsPrintString(FILE *fp, void *value, void * /*data*/) {
    char **s = static_cast<char **>(value);
    fprintf(fp, "'%s'", *s ? *s : "");
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
    if ( s != nullptr ) {
        strncpy(s, *globalCurrentArgumentValue, n);
        s[n - 1] = '\0';  // Ensure zero ending c-string
    }

    return true;
}

void
optionsStringPrint(FILE *fp, void *value, void * /*data*/) {
    const char *s = static_cast<const char *>(value);
    fprintf(fp, "'%s'", s ? s : "");
}

/**
Enumerated type option values
*/
static void
optionsPrintEnumValues(const ENUMDESC *tab) {
    while ( tab && tab->name ) {
        java::lang::System::err.printf("\t%s\n", tab->name);
        tab++;
    }
}

int
optionsEnumGet(void *value, void *data) {
    int *v = static_cast<int *>(value);
    const ENUMDESC *tab = static_cast<const ENUMDESC *>(data);
    const ENUMDESC *tabSave = tab;
    while ( tab && tab->name ) {
        if ( strncasecmp(*globalCurrentArgumentValue, tab->name, tab->abbrev) == 0 ) {
            *v = tab->value;
            return true;
        }
        tab++;
    }
    java::lang::System::err.printf("Invalid option argument '%s'. Should be one of:\n", *globalCurrentArgumentValue);
    optionsPrintEnumValues(tabSave);
    return false;
}

void
optionsEnumPrint(FILE *fp, void *value, void *data) {
    const int *v = static_cast<const int *>(value);
    const ENUMDESC *tab = static_cast<const ENUMDESC *>(data);
    while ( tab && tab->name ) {
        if ( *v == tab->value ) {
            fprintf(fp, "%s", tab->name);
            return;
        }
        tab++;
    }
    fprintf(fp, "INVALID ENUM VALUE!!!");
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
optionsPrintOther(FILE *fp, void * /*x*/, void * /*data*/) {
    fprintf(fp, "other");
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
        java::lang::System::err.printf("'%s' is not a valid floating point value\n", *globalCurrentArgumentValue);
        return false;
    }
    return true;
}

static void
optionsPrintFloat(FILE *fp, void *value, void * /*data*/) {
    const float *x = static_cast<const float *>(value);
    fprintf(fp, "%g", *x);
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
optionsPrintVector(FILE *fp, void *value, void * /*data*/) {
    const Vector3D *v = static_cast<const Vector3D *>(value);
    v->print(fp);
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
optionsPrintRgb(FILE *fp, void *value, void * /*data*/) {
    const ColorRgb *v = static_cast<const ColorRgb *>(value);
    v->print(fp);
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
optionsPrintCieXyCallBack(FILE *fp, void *value, void * /*data*/) {
    const float *c = static_cast<const float *>(value);
    fprintf(fp, "%g %g", c[0], c[1]);
}

CommandLineOptions GLOBAL_options_xyType = {
    optionsGetCieXy,
    optionsPrintCieXyCallBack,
    static_cast<void *>(&globalDummyCieXy),
    nullptr
};

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
    CommandLineOptionDescription *opt = options;
    while ( opt->name ) {
        if ( strncmp(s, opt->name,
             unsignedLongMax(opt->abbreviationLength > 0 ? opt->abbreviationLength : strlen(opt->name), strlen(s))) ==
             0 ) {
            return opt;
        }
        opt++;
    }
    return nullptr;
}

static void
optionsProcessArguments(CommandLineOptionDescription *options) {
    CommandLineOptionDescription *opt = optionsLookupOption(*globalCurrentArgumentValue, options);
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

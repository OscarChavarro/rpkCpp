/**
Command line options and defaults
*/

#include <cstring>

#include "java/util/ArrayList.txx"
#include "common/linealAlgebra/Vector3D.h"
#include "common/ColorRgb.h"
#include "common/options.h"

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
    *res = (int)strtol(*globalCurrentArgumentValue, nullptr, 10);
    return *res == 1;
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
optionsGetInt(int *n, const void * /*data*/) {
    if ( !optionsGetArgumentIntValue(n) ) {
        fprintf(stderr, "'%s' is not a valid integer value\n", *globalCurrentArgumentValue);
        return false;
    }
    return true;
}

static void
optionsPrintInt(FILE *fp, const int *n, const void * /*data*/) {
    fprintf(fp, "%d", *n);
}

CommandLineOptions GLOBAL_options_intType = {
    (int (*)(void *, void *))optionsGetInt,
    (void (*)(FILE *, void *, void *))optionsPrintInt,
    (void *) &globalDummyInt,
    nullptr
};

/**
String option values
*/
static int
optionsGetString(char **s, const void * /*data*/) {
    unsigned long n = strlen(*globalCurrentArgumentValue) + 1;
    *s = new char[n];

    if ( globalStringsToDelete != nullptr ) {
        globalStringsToDelete->add(*s);
    }
    snprintf(*s, n, "%s", *globalCurrentArgumentValue);
    return true;
}

static void
optionsPrintString(FILE *fp, char **s, const void * /*data*/) {
    fprintf(fp, "'%s'", *s ? *s : "");
}

CommandLineOptions GLOBAL_options_stringType = {
    (int (*)(void *, void *)) optionsGetString,
    (void (*)(FILE *, void *, void *)) optionsPrintString,
    (void *) &globalDummyString,
    nullptr
};

/**
Copied string (maxlength n) option values
*/
int
optionsStringGet(char *s, int n) {
    if ( s != nullptr ) {
        strncpy(s, *globalCurrentArgumentValue, n);
        s[n - 1] = '\0';  // Ensure zero ending c-string
    }

    return true;
}

void
optionsStringPrint(FILE *fp, const char *s, int /*n*/) {
    fprintf(fp, "'%s'", s ? s : "");
}

/**
Enumerated type option values
*/
static void
optionsPrintEnumValues(const ENUMDESC *tab) {
    while ( tab && tab->name ) {
        fprintf(stderr, "\t%s\n", tab->name);
        tab++;
    }
}

int
optionsEnumGet(int *v, ENUMDESC *tab) {
    ENUMDESC *tabSave = tab;
    while ( tab && tab->name ) {
        if ( strncasecmp(*globalCurrentArgumentValue, tab->name, tab->abbrev) == 0 ) {
            *v = tab->value;
            return true;
        }
        tab++;
    }
    fprintf(stderr, "Invalid option argument '%s'. Should be one of:\n", *globalCurrentArgumentValue);
    optionsPrintEnumValues(tabSave);
    return false;
}

void
optionsEnumPrint(FILE *fp, const int *v, ENUMDESC *tab) {
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
    (int (*)(void *, void *)) optionsEnumGet,
    (void (*)(FILE *, void *, void *))optionsEnumPrint,
    (void *) &GLOBAL_options_dummyVal,
    (void *) boolTable
};

/* ------------------- set true/false option values --------------------- */

static int
optionsSetTrue(int *x, const void * /*data*/) {
    // No option expected on command line, nothing consumed

    *x = true;
    return true;
}

static int
optionsSetFalse(int *x, const void * /*data*/) {
    /* No option expected on command line, nothing consumed */

    *x = false;
    return true;
}

static void
optionsPrintOther(FILE *fp, const void * /*x*/, const void * /*data*/) {
    fprintf(fp, "other");
}


CommandLineOptions GLOBAL_options_setTrueType = {
    (int (*)(void *, void *)) optionsSetTrue,
    (void (*)(FILE *, void *, void *)) optionsPrintOther,
    (void *) &globalDummyTrue,
    nullptr
};

CommandLineOptions GLOBAL_options_setFalseType = {
    (int (*)(void *, void *)) optionsSetFalse,
    (void (*)(FILE *, void *, void *)) optionsPrintOther,
    (void *) &globalDummyFalse,
    nullptr
};

/* ------------------- float option values --------------------- */
static int
optionsGetfloat(float *x, const void * /*data*/) {
    if ( !optionsGetArgumentFloatValue("%f", x) ) {
        fprintf(stderr, "'%s' is not a valid floating point value\n", *globalCurrentArgumentValue);
        return false;
    }
    return true;
}

static void
optionsPrintFloat(FILE *fp, const float *x, const void * /*data*/) {
    fprintf(fp, "%g", *x);
}

CommandLineOptions GLOBAL_options_floatType = {
        (int (*)(void *, void *)) optionsGetfloat,
        (void (*)(FILE *, void *, void *)) optionsPrintFloat,
        (void *) &globalDummyFloat,
        nullptr
};

/**
Vector3D option values
*/
static int
optionsGetVector(Vector3D *v, const void * /*data*/) {
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
        fprintf(stderr, "invalid vector argument value");
    }

    return ok;
}

static void
optionsPrintVector(FILE *fp, const Vector3D *v, const void * /*data*/) {
    v->print(fp);
}

CommandLineOptions GLOBAL_options_vectorType = {
    (int (*)(void *, void *)) optionsGetVector,
    (void (*)(FILE *, void *, void *)) optionsPrintVector,
    (void *) &globalDummyVector3D,
    nullptr
};

/**
RGB option values
*/
static int
optionsGetRgb(ColorRgb *c, const void * /*data*/) {
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
        fprintf(stderr, "invalid RGB color argument value");
    }

    return ok;
}

static void
optionsPrintRgb(FILE *fp, const ColorRgb *v, const void * /*data*/) {
    v->print(fp);
}

CommandLineOptions GLOBAL_options_rgbType = {
    (int (*)(void *, void *)) optionsGetRgb,
    (void (*)(FILE *, void *, void *)) optionsPrintRgb,
    (void *) &globalDummyRgb,
    nullptr
};

/**
CIE xy option values
*/

static int
optionsGetCieXy(float *c, const void * /*data*/) {
    int ok = optionsGetArgumentFloatValue("%f", &c[0]);
    if ( ok ) {
        optionsConsumeArgument();
        ok &= optionsArgumentsRemaining() && optionsGetArgumentFloatValue("%f", &c[1]);
    }
    if ( !ok ) {
        fprintf(stderr, "invalid CIE xy color argument value");
    }

    return ok;
}

static void
optionsPrintCieXy(FILE *fp, const float *c, void * /*data*/) {
    fprintf(fp, "%g %g", c[0], c[1]);
}

CommandLineOptions GLOBAL_options_xyType = {
    (int (*)(void *, void *)) optionsGetCieXy,
    (void (*)(FILE *, void *, void *)) optionsPrintCieXy,
    (void *) &globalDummyCieXy,
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
                    fprintf(stderr, "Option argument missing.\n");
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

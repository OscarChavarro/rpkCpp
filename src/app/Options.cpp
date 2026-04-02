/**
Command line options and defaults
*/

#include <cstdlib>
#include <cstring>

#include "java/lang/Integer.h"
#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "java/util/Formatter.h"
#include "common/linealAlgebra/Vector3D.h"
#include "common/ColorRgb.h"
#include "app/Options.h"

static int *globalArgumentCount;
static char **globalArguments = nullptr;
static int globalCurrentArgumentIndex = 0;
static java::ArrayList<char *> *globalStringsToDelete = new java::ArrayList<char *>();
static java::ArrayList<int *> *globalStringLengthsToDelete = new java::ArrayList<int *>();

/**
Initializes the global variables above
*/
void
Options::optionsInitArguments(int *argc, char **argv) {
    globalArgumentCount = argc;
    globalArguments = argv;
    globalCurrentArgumentIndex = 0;
}

const char *
Options::optionsCurrentArgumentValue() {
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
bool
Options::optionsArgumentsRemaining() {
    return globalCurrentArgumentIndex < *globalArgumentCount;
}

/**
Skips to next argument value
*/
void
Options::optionsNextArgument() {
    globalCurrentArgumentIndex++;
}

/**
Consumes the current argument value, that is: removes it from the list
*/
void
Options::optionsConsumeArgument() {
    for ( int i = globalCurrentArgumentIndex; i < *globalArgumentCount - 1; i++ ) {
        globalArguments[i] = globalArguments[i + 1];
    }
    globalArguments[*globalArgumentCount - 1] = nullptr;
    (*globalArgumentCount)--;
}

/**
Scans the current argument value for a value of given format
*/
bool
Options::optionsGetArgumentIntValue(int *res) {
    const char *currentArgument = Options::optionsCurrentArgumentValue();
    if ( currentArgument == nullptr ) {
        return false;
    }

    char *endPointer = nullptr;
    const long parsedValue = strtol(currentArgument, &endPointer, 10);

    if ( endPointer == currentArgument || *endPointer != '\0' ) {
        return false;
    }
    if ( parsedValue < static_cast<long>(java::Integer::MIN_VALUE)
         || parsedValue > static_cast<long>(java::Integer::MAX_VALUE) ) {
        return false;
    }

    *res = static_cast<int>(parsedValue);
    return true;
}

/**
Scans the current argument value for a value of given format
*/
bool
Options::optionsGetArgumentFloatValue(const char * /*format*/, float *res) {
    const char *currentArgument = Options::optionsCurrentArgumentValue();
    if ( currentArgument == nullptr || res == nullptr ) {
        return false;
    }

    char *endPointer = nullptr;
    const float parsedValue = strtof(currentArgument, &endPointer);
    if ( endPointer == currentArgument || *endPointer != '\0' ) {
        return false;
    }

    *res = parsedValue;
    return true;
}

/**
Integer option values
*/
bool
Options::optionsGetInt(void *value, void * /*data*/) {
    int *n = static_cast<int *>(value);
    if ( !Options::optionsGetArgumentIntValue(n) ) {
        java::System::err.printf("'%s' is not a valid integer value\n", Options::optionsCurrentArgumentValue());
        return false;
    }
    return true;
}

void
Options::optionsPrintInt(java::PrintStream *stream, void *value, void * /*data*/) {
    int *n = static_cast<int *>(value);
    if ( stream != nullptr ) {
        stream->printf("%d", *n);
    }
}

/**
String option values
*/
bool
Options::optionsGetString(void *value, void * /*data*/) {
    char **s = static_cast<char **>(value);
    const char *currentArgument = Options::optionsCurrentArgumentValue();
    if ( currentArgument == nullptr ) {
        return false;
    }
    unsigned long n = strlen(currentArgument) + 1;
    *s = new char[n];

    if ( globalStringsToDelete != nullptr ) {
        globalStringsToDelete->add(*s);
    }
    java::Formatter::format(*s, static_cast<int>(n), "%s", currentArgument);
    return true;
}

void
Options::optionsPrintString(java::PrintStream *stream, void *value, void * /*data*/) {
    char **s = static_cast<char **>(value);
    if ( stream != nullptr ) {
        stream->printf("'%s'", *s ? *s : "");
    }
}

/**
Copied string (maxlength n) option values
*/
bool
Options::optionsStringGet(void *value, void *data) {
    char *s = static_cast<char *>(value);
    int *nPointer = static_cast<int *>(data);
    const char *currentArgument = Options::optionsCurrentArgumentValue();
    if ( s != nullptr && currentArgument != nullptr && nPointer != nullptr && *nPointer > 0 ) {
        const int n = *nPointer;
        strncpy(s, currentArgument, n);
        s[n - 1] = '\0';  // Ensure zero ending c-string
    }

    return true;
}

int *
Options::optionsCreateStringLengthStorage(int n) {
    int *storage = new int(n);
    if ( globalStringLengthsToDelete != nullptr ) {
        globalStringLengthsToDelete->add(storage);
    }
    return storage;
}

void
Options::optionsStringPrint(java::PrintStream *stream, void *value, void * /*data*/) {
    const char *s = static_cast<const char *>(value);
    if ( stream != nullptr ) {
        stream->printf("'%s'", s ? s : "");
    }
}

/**
Enumerated type option values
*/
void
Options::optionsPrintEnumValues(const EnumDesc *tab) {
    for ( int i = 0; tab != nullptr && tab[i].name != nullptr; i++ ) {
        java::System::err.printf("\t%s\n", tab[i].name);
    }
}

bool
Options::optionsEnumGet(void *value, void *data) {
    int *v = static_cast<int *>(value);
    const EnumDesc *tab = static_cast<const EnumDesc *>(data);
    const EnumDesc *tabSave = tab;
    const char *currentArgument = Options::optionsCurrentArgumentValue();
    if ( currentArgument == nullptr ) {
        return false;
    }
    for ( int i = 0; tab != nullptr && tab[i].name != nullptr; i++ ) {
        if ( strncasecmp(currentArgument, tab[i].name, tab[i].abbrev) == 0 ) {
            *v = tab[i].value;
            return true;
        }
    }
    java::System::err.printf("Invalid option argument '%s'. Should be one of:\n", currentArgument);
    Options::optionsPrintEnumValues(tabSave);
    return false;
}

void
Options::optionsEnumPrint(java::PrintStream *stream, void *value, void *data) {
    const int *v = static_cast<const int *>(value);
    const EnumDesc *tab = static_cast<const EnumDesc *>(data);
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

/* ------------------- set true/false option values --------------------- */

bool
Options::optionsSetTrue(void *value, void * /*data*/) {
    int *x = static_cast<int *>(value);
    // No option expected on command line, nothing consumed

    *x = true;
    return true;
}

bool
Options::optionsSetFalse(void *value, void * /*data*/) {
    int *x = static_cast<int *>(value);
    /* No option expected on command line, nothing consumed */

    *x = false;
    return true;
}

void
Options::optionsPrintOther(java::PrintStream *stream, void * /*x*/, void * /*data*/) {
    if ( stream != nullptr ) {
        stream->printf("%s", "other");
    }
}

/* ------------------- float option values --------------------- */
bool
Options::optionsGetfloat(void *value, void * /*data*/) {
    float *x = static_cast<float *>(value);
    if ( !Options::optionsGetArgumentFloatValue("%f", x) ) {
        java::System::err.printf("'%s' is not a valid floating point value\n", Options::optionsCurrentArgumentValue());
        return false;
    }
    return true;
}

void
Options::optionsPrintFloat(java::PrintStream *stream, void *value, void * /*data*/) {
    const float *x = static_cast<const float *>(value);
    if ( stream != nullptr ) {
        stream->printf("%g", *x);
    }
}

/**
Vector3D option values
*/
bool
Options::optionsGetVector(void *value, void * /*data*/) {
    Vector3D *v = static_cast<Vector3D *>(value);
    bool ok = Options::optionsGetArgumentFloatValue("%f", &v->x);
    if ( ok ) {
        Options::optionsConsumeArgument();
        ok &= Options::optionsArgumentsRemaining() && Options::optionsGetArgumentFloatValue("%f", &v->y);
    }
    if ( ok ) {
        Options::optionsConsumeArgument();
        ok &= Options::optionsArgumentsRemaining() && Options::optionsGetArgumentFloatValue("%f", &v->z);
    }
    if ( !ok ) {
        java::System::err.printf("invalid vector argument value");
    }

    return ok;
}

void
Options::optionsPrintVector(java::PrintStream *stream, void *value, void * /*data*/) {
    const Vector3D *v = static_cast<const Vector3D *>(value);
    if ( stream != nullptr && v != nullptr ) {
        stream->printf("%g %g %g", v->x, v->y, v->z);
    }
}

/**
RGB option values
*/
bool
Options::optionsGetRgb(void *value, void * /*data*/) {
    ColorRgb *c = static_cast<ColorRgb *>(value);
    bool ok = Options::optionsGetArgumentFloatValue("%f", &c->r);
    if ( ok ) {
        Options::optionsConsumeArgument();
        ok &= Options::optionsArgumentsRemaining() && Options::optionsGetArgumentFloatValue("%f", &c->g);
    }
    if ( ok ) {
        Options::optionsConsumeArgument();
        ok &= Options::optionsArgumentsRemaining() && Options::optionsGetArgumentFloatValue("%f", &c->b);
    }
    if ( !ok ) {
        java::System::err.printf("invalid RGB color argument value");
    }

    return ok;
}

void
Options::optionsPrintRgb(java::PrintStream *stream, void *value, void * /*data*/) {
    const ColorRgb *v = static_cast<const ColorRgb *>(value);
    if ( stream != nullptr && v != nullptr ) {
        stream->printf("%g %g %g", v->r, v->g, v->b);
    }
}

/**
CIE xy option values
*/

bool
Options::optionsGetCieXy(void *value, void * /*data*/) {
    float *c = static_cast<float *>(value);
    bool ok = Options::optionsGetArgumentFloatValue("%f", &c[0]);
    if ( ok ) {
        Options::optionsConsumeArgument();
        ok &= Options::optionsArgumentsRemaining() && Options::optionsGetArgumentFloatValue("%f", &c[1]);
    }
    if ( !ok ) {
        java::System::err.printf("invalid CIE xy color argument value");
    }

    return ok;
}

void
Options::optionsPrintCieXyCallBack(java::PrintStream *stream, void *value, void * /*data*/) {
    const float *c = static_cast<const float *>(value);
    if ( stream != nullptr && c != nullptr ) {
        stream->printf("%g %g", c[0], c[1]);
    }
}

void (*const DEFAULT_ACTION)(void *) = nullptr;

/**
Argument parsing
*/

unsigned long
Options::unsignedLongMax(unsigned long a, unsigned long b) {
    return a > b ? a : b;
}

CommandLineOptionDescription *
Options::optionsLookupOption(const char *s, CommandLineOptionDescription *options) {
    if ( s == nullptr ) {
        return nullptr;
    }
    for ( int i = 0; options[i].name != nullptr; i++ ) {
        CommandLineOptionDescription *opt = &options[i];
        if ( strncmp(s, opt->name,
             Options::unsignedLongMax(opt->abbreviationLength > 0 ? opt->abbreviationLength : strlen(opt->name), strlen(s))) ==
             0 ) {
            return opt;
        }
    }
    return nullptr;
}

void *
Options::optionsValueOrDummy(CommandLineOptionDescription *opt) {
    if ( opt == nullptr ) {
        return nullptr;
    }
    if ( opt->value != nullptr ) {
        return opt->value;
    }
    if ( opt->type != nullptr ) {
        return opt->type->dummy;
    }
    return nullptr;
}

bool
Options::optionsTypeConsumesCommandLineArgument(const CommandLineOptions *type) {
    if ( type == nullptr || type->get == nullptr ) {
        return false;
    }
    return type->get != Options::optionsSetTrue && type->get != Options::optionsSetFalse;
}

void
Options::optionsProcessArguments(CommandLineOptionDescription *options) {
    CommandLineOptionDescription *opt = Options::optionsLookupOption(Options::optionsCurrentArgumentValue(), options);
    if ( opt ) {
        bool ok = true;
        if ( opt->type ) {
            if ( !Options::optionsTypeConsumesCommandLineArgument(opt->type) ) {
                if ( !opt->type->get(Options::optionsValueOrDummy(opt), opt->type->data) ) {
                    ok = false;
                }
            } else {
                Options::optionsConsumeArgument();
                if ( Options::optionsArgumentsRemaining() ) {
                    if ( !opt->type->get(Options::optionsValueOrDummy(opt), opt->type->data) ) {
                        ok = false;
                    }
                } else {
                    java::System::err.printf("Option argument missing.\n");
                    ok = false;
                }
            }
        }
        if ( ok && opt->action ) {
            if ( opt->value != nullptr ) {
                opt->action(opt->value);
            } else {
                opt->action(Options::optionsValueOrDummy(opt));
            }
        }
        Options::optionsConsumeArgument();
    } else Options::optionsNextArgument();
}

/**
Scans for options mentioned in the 'options' command line description
list, parses their value, executes their associated actions, and
removes them from the argv list (decreasing argc)
*/
void
Options::parseGeneralOptions(CommandLineOptionDescription *options, int *argc, char **argv) {
    Options::optionsInitArguments(argc, argv);
    while ( Options::optionsArgumentsRemaining() ) {
        Options::optionsProcessArguments(options);
    }
}

void
Options::deleteOptionsMemory() {
    if ( globalStringLengthsToDelete != nullptr ) {
        for ( int i = 0; i < globalStringLengthsToDelete->size(); i++ ) {
            delete globalStringLengthsToDelete->get(i);
        }
        delete globalStringLengthsToDelete;
        globalStringLengthsToDelete = nullptr;
    }

    if ( globalStringsToDelete != nullptr ) {
        for ( int i = 0; i < globalStringsToDelete->size(); i++ ) {
            delete[] globalStringsToDelete->get(i);
        }
        delete globalStringsToDelete;
        globalStringsToDelete = nullptr;
    }
}

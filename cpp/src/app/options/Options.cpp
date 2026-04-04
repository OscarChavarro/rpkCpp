/**
Command line options and defaults
*/

#include <cstdlib>
#include <cstring>

#include "java/lang/System.h"
#include "java/util/ArrayList.txx"
#include "java/util/Formatter.h"
#include "common/linealAlgebra/Vector3D.h"
#include "common/ColorRgb.h"
#include "app/options/Options.h"
#include "app/options/ValueParser.h"
#include "app/options/OptionParser.h"

int *Options::argumentCount;
char **Options::arguments = nullptr;
int Options::currentArgumentIndex = 0;
java::ArrayList<char *> *Options::stringsToDelete = new java::ArrayList<char *>();
java::ArrayList<int *> *Options::stringLengthsToDelete = new java::ArrayList<int *>();

template <typename T>
static T *optionsTypedValue(OptionValueWrapper value, OptionKind expectedKind) {
    if ( value.ptr == nullptr ) {
        return nullptr;
    }
    if ( value.kind != OptionKind::UNKNOWN && value.kind != expectedKind ) {
        return nullptr;
    }
    return static_cast<T *>(value.ptr);
}

template <typename T>
static const T *optionsTypedConstValue(OptionValueWrapper value, OptionKind expectedKind) {
    if ( value.ptr == nullptr ) {
        return nullptr;
    }
    if ( value.kind != OptionKind::UNKNOWN && value.kind != expectedKind ) {
        return nullptr;
    }
    return static_cast<const T *>(value.ptr);
}

/**
Initializes the class static parsing state
*/
void
Options::optionsInitArguments(int *argc, char **argv) {
    argumentCount = argc;
    arguments = argv;
    currentArgumentIndex = 0;
}

const char *
Options::optionsCurrentArgumentValue() {
    if ( arguments == nullptr || argumentCount == nullptr ) {
        return nullptr;
    }
    if ( currentArgumentIndex < 0 || currentArgumentIndex >= *argumentCount ) {
        return nullptr;
    }
    return arguments[currentArgumentIndex];
}

/**
Tests whether arguments remain
*/
bool
Options::optionsArgumentsRemaining() {
    return currentArgumentIndex < *argumentCount;
}

/**
Skips to next argument value
*/
void
Options::optionsNextArgument() {
    currentArgumentIndex++;
}

/**
Consumes the current argument value, that is: removes it from the list
*/
void
Options::optionsConsumeArgument() {
    if ( arguments == nullptr || argumentCount == nullptr ) {
        return;
    }
    if ( currentArgumentIndex < 0 || currentArgumentIndex >= *argumentCount ) {
        return;
    }
    arguments[currentArgumentIndex] = nullptr;
    currentArgumentIndex++;
}

bool
Options::optionsParseInt(OptionValueWrapper value, void * /*data*/) {
    int *n = optionsTypedValue<int>(value, OptionKind::INT);
    if ( n == nullptr ) {
        return false;
    }
    if ( !ValueParser<int>::parse(Options::optionsCurrentArgumentValue(), *n) ) {
        java::System::err.printf("'%s' is not a valid integer value\n", Options::optionsCurrentArgumentValue());
        return false;
    }
    return true;
}

void
Options::optionsPrintInt(java::PrintStream *stream, OptionValueWrapper value, void * /*data*/) {
    const int *n = optionsTypedConstValue<int>(value, OptionKind::INT);
    if ( n == nullptr ) {
        return;
    }
    if ( stream != nullptr ) {
        stream->printf("%d", *n);
    }
}

bool
Options::optionsParseString(OptionValueWrapper value, void * /*data*/) {
    char **s = optionsTypedValue<char *>(value, OptionKind::STRING);
    if ( s == nullptr ) {
        return false;
    }
    if ( !ValueParser<char *>::parse(Options::optionsCurrentArgumentValue(), *s) ) {
        return false;
    }
    if ( stringsToDelete != nullptr ) {
        stringsToDelete->add(*s);
    }
    return true;
}

void
Options::optionsPrintString(java::PrintStream *stream, OptionValueWrapper value, void * /*data*/) {
    char * const *s = optionsTypedConstValue<char *>(value, OptionKind::STRING);
    if ( s == nullptr ) {
        return;
    }
    if ( stream != nullptr ) {
        stream->printf("'%s'", *s ? *s : "");
    }
}

bool
Options::optionsParseFixedString(OptionValueWrapper value, void *data) {
    char *s = optionsTypedValue<char>(value, OptionKind::STRING);
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
    if ( stringLengthsToDelete != nullptr ) {
        stringLengthsToDelete->add(storage);
    }
    return storage;
}

void
Options::optionsStringPrint(java::PrintStream *stream, OptionValueWrapper value, void * /*data*/) {
    const char *s = optionsTypedConstValue<char>(value, OptionKind::STRING);
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
Options::optionsParseEnum(OptionValueWrapper value, void *data) {
    int *v = optionsTypedValue<int>(value, OptionKind::BOOL);
    if ( v == nullptr ) {
        v = optionsTypedValue<int>(value, OptionKind::INT);
    }
    if ( v == nullptr ) {
        return false;
    }
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
Options::optionsEnumPrint(java::PrintStream *stream, OptionValueWrapper value, void *data) {
    const int *v = optionsTypedConstValue<int>(value, OptionKind::BOOL);
    if ( v == nullptr ) {
        v = optionsTypedConstValue<int>(value, OptionKind::INT);
    }
    if ( v == nullptr ) {
        return;
    }
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
Options::optionsSetTrue(OptionValueWrapper value, void * /*data*/) {
    int *x = optionsTypedValue<int>(value, OptionKind::BOOL);
    if ( x == nullptr ) {
        return false;
    }
    // No option expected on command line, nothing consumed

    *x = true;
    return true;
}

bool
Options::optionsSetFalse(OptionValueWrapper value, void * /*data*/) {
    int *x = optionsTypedValue<int>(value, OptionKind::BOOL);
    if ( x == nullptr ) {
        return false;
    }
    /* No option expected on command line, nothing consumed */

    *x = false;
    return true;
}

void
Options::optionsPrintOther(java::PrintStream *stream, OptionValueWrapper /*x*/, void * /*data*/) {
    if ( stream != nullptr ) {
        stream->printf("%s", "other");
    }
}

bool
Options::optionsParseFloat(OptionValueWrapper value, void * /*data*/) {
    float *x = optionsTypedValue<float>(value, OptionKind::FLOAT);
    if ( x == nullptr ) {
        return false;
    }
    if ( !ValueParser<float>::parse(Options::optionsCurrentArgumentValue(), *x) ) {
        java::System::err.printf("'%s' is not a valid floating point value\n", Options::optionsCurrentArgumentValue());
        return false;
    }
    return true;
}

void
Options::optionsPrintFloat(java::PrintStream *stream, OptionValueWrapper value, void * /*data*/) {
    const float *x = optionsTypedConstValue<float>(value, OptionKind::FLOAT);
    if ( x == nullptr ) {
        return;
    }
    if ( stream != nullptr ) {
        stream->printf("%g", *x);
    }
}

bool
Options::optionsParseVector(OptionValueWrapper value, void * /*data*/) {
    Vector3D *v = optionsTypedValue<Vector3D>(value, OptionKind::VECTOR3D);
    if ( v == nullptr ) {
        return false;
    }
    bool ok = ValueParser<float>::parse(Options::optionsCurrentArgumentValue(), v->x);
    if ( ok ) {
        Options::optionsConsumeArgument();
        ok &= Options::optionsArgumentsRemaining() && ValueParser<float>::parse(Options::optionsCurrentArgumentValue(), v->y);
    }
    if ( ok ) {
        Options::optionsConsumeArgument();
        ok &= Options::optionsArgumentsRemaining() && ValueParser<float>::parse(Options::optionsCurrentArgumentValue(), v->z);
    }
    if ( !ok ) {
        java::System::err.printf("invalid vector argument value");
    }

    return ok;
}

void
Options::optionsPrintVector(java::PrintStream *stream, OptionValueWrapper value, void * /*data*/) {
    const Vector3D *v = optionsTypedConstValue<Vector3D>(value, OptionKind::VECTOR3D);
    if ( stream != nullptr && v != nullptr ) {
        stream->printf("%g %g %g", v->x, v->y, v->z);
    }
}

bool
Options::optionsParseRgb(OptionValueWrapper value, void * /*data*/) {
    ColorRgb *c = optionsTypedValue<ColorRgb>(value, OptionKind::COLORRGB);
    if ( c == nullptr ) {
        return false;
    }
    bool ok = ValueParser<float>::parse(Options::optionsCurrentArgumentValue(), c->r);
    if ( ok ) {
        Options::optionsConsumeArgument();
        ok &= Options::optionsArgumentsRemaining() && ValueParser<float>::parse(Options::optionsCurrentArgumentValue(), c->g);
    }
    if ( ok ) {
        Options::optionsConsumeArgument();
        ok &= Options::optionsArgumentsRemaining() && ValueParser<float>::parse(Options::optionsCurrentArgumentValue(), c->b);
    }
    if ( !ok ) {
        java::System::err.printf("invalid RGB color argument value");
    }

    return ok;
}

void
Options::optionsPrintRgb(java::PrintStream *stream, OptionValueWrapper value, void * /*data*/) {
    const ColorRgb *v = optionsTypedConstValue<ColorRgb>(value, OptionKind::COLORRGB);
    if ( stream != nullptr && v != nullptr ) {
        stream->printf("%g %g %g", v->r, v->g, v->b);
    }
}

bool
Options::optionsParseCieXy(OptionValueWrapper value, void * /*data*/) {
    float *c = optionsTypedValue<float>(value, OptionKind::FLOAT);
    if ( c == nullptr ) {
        return false;
    }
    bool ok = ValueParser<float>::parse(Options::optionsCurrentArgumentValue(), c[0]);
    if ( ok ) {
        Options::optionsConsumeArgument();
        ok &= Options::optionsArgumentsRemaining() && ValueParser<float>::parse(Options::optionsCurrentArgumentValue(), c[1]);
    }
    if ( !ok ) {
        java::System::err.printf("invalid CIE xy color argument value");
    }

    return ok;
}

void
Options::optionsPrintCieXyCallBack(java::PrintStream *stream, OptionValueWrapper value, void * /*data*/) {
    const float *c = optionsTypedConstValue<float>(value, OptionKind::FLOAT);
    if ( stream != nullptr && c != nullptr ) {
        stream->printf("%g %g", c[0], c[1]);
    }
}

/**
Argument parsing
*/

void
Options::parseGeneralOptions(CommandLineOptionDescription *options, int *argc, char **argv) {
    LegacyOptionRegistry registry = {options, legacyOptionRegistryCount(options)};
    OptionParser::configureLegacy(registry, argc);
    OptionParser::parse(*argc, argv);
}

void
Options::deleteOptionsMemory() {
    if ( stringLengthsToDelete != nullptr ) {
        for ( int i = 0; i < stringLengthsToDelete->size(); i++ ) {
            delete stringLengthsToDelete->get(i);
        }
        delete stringLengthsToDelete;
        stringLengthsToDelete = nullptr;
    }

    if ( stringsToDelete != nullptr ) {
        for ( int i = 0; i < stringsToDelete->size(); i++ ) {
            delete[] stringsToDelete->get(i);
        }
        delete stringsToDelete;
        stringsToDelete = nullptr;
    }
}

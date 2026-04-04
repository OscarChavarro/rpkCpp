#include "java/lang/System.h"
#include "app/options/Options.h"
#include "app/options/OptionParser.h"

LegacyOptionRegistry OptionParser::legacyRegistry = {nullptr, 0};
int *OptionParser::argumentCount = nullptr;

void OptionParser::configureLegacy(LegacyOptionRegistry registry, int *argc) {
    legacyRegistry = registry;
    argumentCount = argc;
}

OptionValueWrapper OptionParser::valueOrDummy(CommandLineOptionDescription *opt) {
    if ( opt == nullptr ) {
        return OptionValueWrapper();
    }
    if ( opt->value.ptr != nullptr ) {
        OptionValueWrapper value = opt->value;
        if ( value.kind == OptionKind::UNKNOWN && opt->type != nullptr ) {
            value.kind = opt->type->dummy.kind;
        }
        return value;
    }
    if ( opt->type != nullptr ) {
        return opt->type->dummy;
    }
    return OptionValueWrapper();
}

bool OptionParser::typeConsumesArgument(const CommandLineOptions *type) {
    if ( type == nullptr || type->get == nullptr ) {
        return false;
    }
    return type->get != Options::optionsSetTrue && type->get != Options::optionsSetFalse;
}

bool OptionParser::processOne() {
    CommandLineOptionDescription *opt = legacyOptionRegistryLookup(legacyRegistry, Options::optionsCurrentArgumentValue());
    if ( opt == nullptr ) {
        Options::optionsNextArgument();
        return true;
    }
    bool ok = true;
    if ( opt->typedOption != nullptr ) {
        if ( opt->typedParser != nullptr ) {
            Options::optionsConsumeArgument();
            if ( Options::optionsArgumentsRemaining() ) {
                if ( !opt->typedParser(opt->typedOption, Options::optionsCurrentArgumentValue()) ) {
                    ok = false;
                }
            } else {
                java::System::err.printf("Option argument missing.\n");
                ok = false;
            }
        }
        if ( ok && opt->typedAction != nullptr ) {
            opt->typedAction(opt->typedOption);
        }
    } else {
        if ( opt->type != nullptr ) {
            if ( !typeConsumesArgument(opt->type) ) {
                if ( !opt->type->get(valueOrDummy(opt), opt->type->data) ) {
                    ok = false;
                }
            } else {
                Options::optionsConsumeArgument();
                if ( Options::optionsArgumentsRemaining() ) {
                    if ( !opt->type->get(valueOrDummy(opt), opt->type->data) ) {
                        ok = false;
                    }
                } else {
                    java::System::err.printf("Option argument missing.\n");
                    ok = false;
                }
            }
        }
        if ( ok && opt->action != nullptr ) {
            if ( opt->value.ptr != nullptr ) {
                opt->action(opt->value);
            } else {
                opt->action(valueOrDummy(opt));
            }
        }
    }
    Options::optionsConsumeArgument();
    return ok;
}

void OptionParser::compactArguments(char **argv, int originalArgc) {
    int writeIndex = 0;
    for ( int readIndex = 0; readIndex < originalArgc; readIndex++ ) {
        if ( argv[readIndex] != nullptr ) {
            argv[writeIndex++] = argv[readIndex];
        }
    }
    for ( int i = writeIndex; i < originalArgc; i++ ) {
        argv[i] = nullptr;
    }
    if ( argumentCount != nullptr ) {
        *argumentCount = writeIndex;
    }
}

bool OptionParser::parse(int argc, char **argv) {
    int fallbackCount = argc;
    int *countPointer = argumentCount != nullptr ? argumentCount : &fallbackCount;
    if ( legacyRegistry.options == nullptr ) {
        if ( argumentCount != nullptr ) {
            *argumentCount = argc;
        }
        return true;
    }
    Options::optionsInitArguments(countPointer, argv);
    if ( countPointer != nullptr ) {
        *countPointer = argc;
    }
    while ( Options::optionsArgumentsRemaining() ) {
        if ( !processOne() ) {
            compactArguments(argv, argc);
            return false;
        }
    }
    compactArguments(argv, argc);
    return true;
}

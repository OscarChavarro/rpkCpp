#include <cstring>

#include "java/lang/System.h"
#include "common/ColorRgb.h"
#include "common/linealAlgebra/Vector3D.h"
#include "app/options/Options.h"
#include "app/options/OptionParser.h"
#include "app/options/ValueParser.h"

LegacyOptionRegistry OptionParser::legacyRegistry = {nullptr, 0};
int *OptionParser::argumentCount = nullptr;

void OptionParser::configureLegacy(LegacyOptionRegistry registry, int *argc) {
    legacyRegistry = registry;
    argumentCount = argc;
}

bool OptionParser::optionConsumesArgument(const CommandLineOptionDescription *opt) {
    if ( opt == nullptr ) {
        return false;
    }
    if ( opt->dispatch == OptionDispatch::AUTO && opt->kind == OptionKind::UNKNOWN ) {
        return false;
    }
    return opt->dispatch != OptionDispatch::SET_TRUE && opt->dispatch != OptionDispatch::SET_FALSE;
}

bool OptionParser::parseLegacyValue(CommandLineOptionDescription *opt, OptionValueWrapper value) {
    if ( opt == nullptr ) {
        return false;
    }
    if ( value.ptr == nullptr ) {
        return false;
    }

    switch ( opt->dispatch ) {
    case OptionDispatch::SET_TRUE:
        *static_cast<int *>(value.ptr) = true;
        return true;
    case OptionDispatch::SET_FALSE:
        *static_cast<int *>(value.ptr) = false;
        return true;
    case OptionDispatch::ENUM:
        return Options::optionsParseEnum(value, opt->data);
    case OptionDispatch::FIXED_STRING:
        return Options::optionsParseFixedString(value, opt->data);
    case OptionDispatch::CIE_XY:
        return Options::optionsParseCieXy(value, opt->data);
    case OptionDispatch::AUTO:
    default:
        break;
    }

    switch ( opt->kind ) {
    case OptionKind::INT:
        return Options::optionsParseInt(value, opt->data);
    case OptionKind::FLOAT:
        return Options::optionsParseFloat(value, opt->data);
    case OptionKind::STRING:
        return Options::optionsParseString(value, opt->data);
    case OptionKind::VECTOR3D:
        return Options::optionsParseVector(value, opt->data);
    case OptionKind::COLORRGB:
        return Options::optionsParseRgb(value, opt->data);
    case OptionKind::BOOL: {
        bool parsed = false;
        if ( !ValueParser<bool>::parse(Options::optionsCurrentArgumentValue(), parsed) ) {
            java::System::err.printf("'%s' is not a valid boolean value\n", Options::optionsCurrentArgumentValue());
            return false;
        }
        *static_cast<int *>(value.ptr) = parsed ? 1 : 0;
        return true;
    }
    case OptionKind::UNKNOWN:
        return true;
    default:
        return false;
    }
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
        Options::optionsConsumeArgument();
        return ok;
    }

    int intScratch = 0;
    float floatScratch = 0.0f;
    char *stringScratch = nullptr;
    Vector3D vectorScratch(0.0, 0.0, 0.0);
    ColorRgb colorScratch(0.0, 0.0, 0.0);
    float cieXyScratch[2] = {0.0f, 0.0f};
    OptionValueWrapper target = opt->value;
    if ( target.ptr == nullptr ) {
        switch ( opt->dispatch ) {
        case OptionDispatch::CIE_XY:
            target = OptionValueWrapper(static_cast<void *>(cieXyScratch), OptionKind::FLOAT);
            break;
        case OptionDispatch::SET_TRUE:
        case OptionDispatch::SET_FALSE:
            target = OptionValueWrapper(static_cast<void *>(&intScratch), OptionKind::BOOL);
            break;
        case OptionDispatch::AUTO:
        case OptionDispatch::ENUM:
        case OptionDispatch::FIXED_STRING:
        default:
            switch ( opt->kind ) {
            case OptionKind::INT:
            case OptionKind::BOOL:
                target = OptionValueWrapper(static_cast<void *>(&intScratch), opt->kind);
                break;
            case OptionKind::FLOAT:
                target = OptionValueWrapper(static_cast<void *>(&floatScratch), OptionKind::FLOAT);
                break;
            case OptionKind::STRING:
                if ( opt->dispatch == OptionDispatch::FIXED_STRING ) {
                    target = OptionValueWrapper(static_cast<void *>(nullptr), OptionKind::STRING);
                } else {
                    target = OptionValueWrapper(static_cast<void *>(&stringScratch), OptionKind::STRING);
                }
                break;
            case OptionKind::VECTOR3D:
                target = OptionValueWrapper(static_cast<void *>(&vectorScratch), OptionKind::VECTOR3D);
                break;
            case OptionKind::COLORRGB:
                target = OptionValueWrapper(static_cast<void *>(&colorScratch), OptionKind::COLORRGB);
                break;
            case OptionKind::UNKNOWN:
            default:
                target = OptionValueWrapper();
                break;
            }
            break;
        }
    }

    if ( optionConsumesArgument(opt) ) {
        Options::optionsConsumeArgument();
        if ( Options::optionsArgumentsRemaining() ) {
            ok = parseLegacyValue(opt, target);
        } else {
            java::System::err.printf("Option argument missing.\n");
            ok = false;
        }
    } else {
        ok = parseLegacyValue(opt, target);
    }

    if ( ok && opt->action != nullptr ) {
        opt->action(target);
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

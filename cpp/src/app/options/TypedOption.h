#ifndef __TYPED_OPTION__
#define __TYPED_OPTION__

#include <cstring>

#include "app/options/DefaultParser.h"

template<typename T>
struct Option {
    const char *name;
    T *target;
    int consumesValue;
    void (*onSet)(T &);
    bool (*parseArgs)(int, char **, T &);
};

struct OptionBase {
    const char *name;
    int abbreviationLength;
    int (*consumesValue)(void *);
    bool (*apply)(void *, int, char **);
    void *option;
};

template<typename T>
bool applyOption(Option<T> &opt, int argc, char **argv) {
    if ( opt.target == nullptr ) {
        return false;
    }
    if ( opt.consumesValue == 0 ) {
        if ( opt.parseArgs != nullptr ) {
            if ( !opt.parseArgs(0, nullptr, *opt.target) ) {
                return false;
            }
        }
        if ( opt.onSet != nullptr ) {
            opt.onSet(*opt.target);
        }
        return true;
    }
    if ( argc < opt.consumesValue ) {
        return false;
    }
    T value = *opt.target;
    bool parsed = false;
    if ( opt.parseArgs != nullptr ) {
        parsed = opt.parseArgs(opt.consumesValue, argv, value);
    } else if ( opt.consumesValue == 1 ) {
        parsed = DefaultParser<T>::parse(argv[0], value);
    }
    if ( !parsed ) {
        return false;
    }
    *opt.target = value;
    if ( opt.onSet != nullptr ) {
        opt.onSet(*opt.target);
    }
    return true;
}

template<typename T>
bool applyAdapter(void *opt, int argc, char **argv) {
    if ( opt == nullptr ) {
        return false;
    }
    return applyOption(*(Option<T> *)opt, argc, argv);
}

template<typename T>
int consumesValueAdapter(void *opt) {
    if ( opt == nullptr ) {
        return 1;
    }
    return ((Option<T> *)opt)->consumesValue;
}

#define REGISTER_OPTION(type, optionInstance, abbr) \
    { (optionInstance).name, abbr, &consumesValueAdapter<type>, &applyAdapter<type>, (void *)&(optionInstance) }

inline bool matchOption(const char *input, const char *name, int abbrLen) {
    if ( input == nullptr || name == nullptr ) {
        return false;
    }
    if ( strcmp(input, name) == 0 ) {
        return true;
    }
    if ( abbrLen <= 0 ) {
        return false;
    }
    const unsigned long inputLength = static_cast<unsigned long>(strlen(input));
    if ( inputLength > static_cast<unsigned long>(abbrLen) ) {
        return false;
    }
    return strncmp(input, name, inputLength) == 0;
}

inline bool parseOptionBaseRegistryInPlace(OptionBase *registry, int registryCount, int *argc, char **argv) {
    if ( registry == nullptr || argc == nullptr || argv == nullptr ) {
        return false;
    }

    for ( int i = 0; i < *argc; i++ ) {
        if ( argv[i] == nullptr ) {
            continue;
        }
        for ( int j = 0; j < registryCount; j++ ) {
            if ( registry[j].name == nullptr ) {
                continue;
            }
            if ( !matchOption(argv[i], registry[j].name, registry[j].abbreviationLength) ) {
                continue;
            }
            int consumesValue = 1;
            if ( registry[j].consumesValue != nullptr ) {
                consumesValue = registry[j].consumesValue(registry[j].option);
            }
            if ( consumesValue != 0 ) {
                if ( i + consumesValue >= *argc ) {
                    return false;
                }
                bool missingValue = false;
                for ( int k = 1; k <= consumesValue; k++ ) {
                    if ( argv[i + k] == nullptr ) {
                        missingValue = true;
                        break;
                    }
                }
                if ( missingValue ) {
                    return false;
                }
                if ( registry[j].apply == nullptr || !registry[j].apply(registry[j].option, consumesValue, argv + i + 1) ) {
                    return false;
                }
                argv[i] = nullptr;
                for ( int k = 1; k <= consumesValue; k++ ) {
                    argv[i + k] = nullptr;
                }
                i += consumesValue;
            } else {
                if ( registry[j].apply == nullptr || !registry[j].apply(registry[j].option, 0, nullptr) ) {
                    return false;
                }
                argv[i] = nullptr;
            }
            break;
        }
    }

    int writeIndex = 0;
    for ( int readIndex = 0; readIndex < *argc; readIndex++ ) {
        if ( argv[readIndex] != nullptr ) {
            argv[writeIndex++] = argv[readIndex];
        }
    }
    for ( int i = writeIndex; i < *argc; i++ ) {
        argv[i] = nullptr;
    }
    *argc = writeIndex;
    return true;
}

#endif

#ifndef __TYPED_OPTION__
#define __TYPED_OPTION__

#include <cstring>

#include "app/options/CommandLineOptionDescription.h"
#include "app/options/DefaultParser.h"

template<typename T>
struct Option {
    const char *name;
    T *target;
    int consumesValue;
    void (*onSet)(T &);
};

struct OptionBase {
    const char *name;
    int abbreviationLength;
    int (*consumesValue)(void *);
    bool (*apply)(void *, const char *);
    void *option;
};

template<typename T>
bool applyOption(Option<T> &opt, const char *arg) {
    if ( opt.target == nullptr ) {
        return false;
    }
    if ( opt.consumesValue == 0 ) {
        *opt.target = static_cast<T>(true);
        if ( opt.onSet != nullptr ) {
            opt.onSet(*opt.target);
        }
        return true;
    }
    T value;
    if ( !DefaultParser<T>::parse(arg, value) ) {
        return false;
    }
    *opt.target = value;
    if ( opt.onSet != nullptr ) {
        opt.onSet(*opt.target);
    }
    return true;
}

template<typename T>
bool applyAdapter(void *opt, const char *arg) {
    if ( opt == nullptr ) {
        return false;
    }
    return applyOption(*(Option<T> *)opt, arg);
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
    if ( abbrLen == 0 ) {
        return strcmp(input, name) == 0;
    }
    return strncmp(input, name, static_cast<unsigned long>(abbrLen)) == 0;
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
                if ( i + 1 >= *argc || argv[i + 1] == nullptr ) {
                    return false;
                }
                if ( registry[j].apply == nullptr || !registry[j].apply(registry[j].option, argv[i + 1]) ) {
                    return false;
                }
                argv[i] = nullptr;
                argv[i + 1] = nullptr;
                i++;
            } else {
                if ( registry[j].apply == nullptr || !registry[j].apply(registry[j].option, nullptr) ) {
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

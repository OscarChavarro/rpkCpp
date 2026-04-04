#ifndef __OPTION_REGISTRY__
#define __OPTION_REGISTRY__

#include <cstring>

#include "app/options/TypedOption.h"
#include "app/options/CommandLineOptionDescription.h"

template<typename T>
struct OptionRegistry {
    Option<T> *options;
    int count;
};

struct LegacyOptionRegistry {
    CommandLineOptionDescription *options;
    int count;
};

inline int legacyOptionRegistryCount(CommandLineOptionDescription *options) {
    int count = 0;
    if ( options == nullptr ) {
        return 0;
    }
    while ( options[count].name != nullptr ) {
        count++;
    }
    return count;
}

inline CommandLineOptionDescription *legacyOptionRegistryLookup(const LegacyOptionRegistry &registry, const char *name) {
    if ( registry.options == nullptr || name == nullptr ) {
        return nullptr;
    }
    for ( int i = 0; i < registry.count; i++ ) {
        CommandLineOptionDescription *opt = &registry.options[i];
        unsigned long lhs = static_cast<unsigned long>(opt->abbreviationLength > 0 ? opt->abbreviationLength : static_cast<int>(strlen(opt->name)));
        unsigned long rhs = static_cast<unsigned long>(strlen(name));
        unsigned long n = lhs > rhs ? lhs : rhs;
        if ( strncmp(name, opt->name, n) == 0 ) {
            return opt;
        }
    }
    return nullptr;
}

#endif

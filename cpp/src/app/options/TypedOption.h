#ifndef __TYPED_OPTION__
#define __TYPED_OPTION__

#include "app/options/CommandLineOptionDescription.h"

template<typename T>
struct Option {
    const char *name;
    int abbreviationLength;
    T *target;
    bool (*parser)(const char *, T &);
    void (*action)(T &);
};

template<typename T>
struct TypedOptionAdapter {
    static bool parse(const void *typedOption, const char *input) {
        Option<T> *option = (Option<T> *)typedOption;
        if ( option == nullptr || option->target == nullptr || option->parser == nullptr ) {
            return false;
        }
        return option->parser(input, *option->target);
    }

    static void act(const void *typedOption) {
        Option<T> *option = (Option<T> *)typedOption;
        if ( option == nullptr || option->target == nullptr || option->action == nullptr ) {
            return;
        }
        option->action(*option->target);
    }
};

template<typename T>
inline CommandLineOptionDescription makeTypedOptionDescription(Option<T> *option, const char *description) {
    CommandLineOptionDescription desc = {
        option->name,
        option->abbreviationLength,
        OptionKind::UNKNOWN,
        OptionValueWrapper(),
        nullptr,
        description,
        nullptr,
        OptionDispatch::AUTO,
        (const void *)option,
        &TypedOptionAdapter<T>::parse,
        &TypedOptionAdapter<T>::act
    };
    return desc;
}

#endif

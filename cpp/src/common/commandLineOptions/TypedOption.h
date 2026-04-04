#ifndef __OPTION_CORE_TYPED_OPTION__
#define __OPTION_CORE_TYPED_OPTION__

#include <cstring>

#include "common/commandLineOptions/DefaultParser.h"

template<typename T>
class TypedOption {
  public:
    TypedOption();
    TypedOption(const char *name, T *target, int consumesValue, void (*onSet)(T &), bool (*parseArgs)(int, char **, T &));

    const char *getName() const;
    int getConsumesValue() const;
    bool apply(int argc, char **argv);

  private:
    const char *name_;
    T *target_;
    int consumesValue_;
    void (*onSet_)(T &);
    bool (*parseArgs_)(int, char **, T &);
};

class OptionBase {
  public:
    OptionBase();
    OptionBase(
        const char *name,
        int abbreviationLength,
        int (*consumesValue)(void *),
        bool (*apply)(void *, int, char **),
        void *option);

    bool isConfigured() const;
    const char *getName() const;
    int getAbbreviationLength() const;
    int consumesValue() const;
    bool apply(int argc, char **argv) const;

  private:
    const char *name_;
    int abbreviationLength_;
    int (*consumesValue_)(void *);
    bool (*apply_)(void *, int, char **);
    void *option_;
};

template<typename T>
bool applyOption(TypedOption<T> &opt, int argc, char **argv);

template<typename T>
bool applyAdapter(void *opt, int argc, char **argv);

template<typename T>
int consumesValueAdapter(void *opt);

#define REGISTER_OPTION(type, optionInstance, abbr) \
    OptionBase((optionInstance).getName(), abbr, &consumesValueAdapter<type>, &applyAdapter<type>, (void *)&(optionInstance))

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

#include "common/commandLineOptions/TypedOption.txx"

#endif

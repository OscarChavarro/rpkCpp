#ifndef __TYPED_OPTION__
#define __TYPED_OPTION__

#include <string.h>

#include "vsdk/toolkit/common/commandLineOptions/DefaultParser.h"

class OptionValue {
  public:
    virtual ~OptionValue() {}
    virtual const char *getName() const = 0;
    virtual int getConsumesValue() const = 0;
    virtual bool apply(int argc, char **argv) = 0;
};

template<typename T>
class TypedOption : public OptionValue {
  public:
    TypedOption();
    TypedOption(const char *name, T *target, int consumesValue, void (*onSet)(T &), bool (*parseArgs)(int, char **, T &));
    TypedOption(const char *name, T *target, int consumesValue, void (*onSet)(T &), bool (*parseArgs)(int, char **, T &), bool (*validate)(T &), void (*transform)(T &));

    const char *getName() const;
    int getConsumesValue() const;
    bool apply(int argc, char **argv);

  private:
    const char *name_;
    T *target_;
    int consumesValue_;
    void (*onSet_)(T &);
    bool (*parseArgs_)(int, char **, T &);
    bool (*validate_)(T &);
    void (*transform_)(T &);
};

class OptionBase {
  public:
    OptionBase();
    OptionBase(OptionValue *option, int abbreviationLength);

    bool isConfigured() const;
    const char *getName() const;
    int getAbbreviationLength() const;
    int consumesValue() const;
    bool apply(int argc, char **argv) const;

  private:
    OptionValue *option_;
    int abbreviationLength_;
};

#define REGISTER_OPTION(type, optionInstance, abbr) \
    OptionBase(&(optionInstance), abbr)

inline bool matchOption(const char *input, const char *name, int abbrLen) {
    if ( input == NULL || name == NULL ) {
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

#include "vsdk/toolkit/common/commandLineOptions/TypedOption.txx"

#endif

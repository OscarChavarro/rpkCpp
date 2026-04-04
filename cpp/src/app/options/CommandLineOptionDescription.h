#ifndef __COMMAND_LINE_OPTION_DESCRIPTION__
#define __COMMAND_LINE_OPTION_DESCRIPTION__

#include "app/options/CommandLineOptions.h"

class CommandLineOptionDescription {
  public:
    const char *name;
    int abbreviationLength;
    OptionKind kind;
    OptionValueWrapper value;
    void (*action)(OptionValueWrapper);
    const char *description;
    void *data;
    OptionDispatch dispatch;
    const void *typedOption;
    bool (*typedParser)(const void *, const char *);
    void (*typedAction)(const void *);
};

#endif

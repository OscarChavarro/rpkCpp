#ifndef __COMMAND_LINE_OPTION_DESCRIPTION__
#define __COMMAND_LINE_OPTION_DESCRIPTION__

#include "app/options/CommandLineOptions.h"

class CommandLineOptionDescription {
  public:
    const char *name; // Command line options name
    int abbreviationLength; // Minimum number of characters in command ine option name abbreviation or
				 // 0 if no abbreviation is allowed
    CommandLineOptions *type; // Value type, or TYPELESS
    OptionValueWrapper value;
    void (*action)(OptionValueWrapper);
    const char *description; // Short description of the option. For printing command line option usage
};

#endif

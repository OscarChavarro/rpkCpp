#ifndef __COMMAND_LINE_OPTION_DESCRIPTION__
#define __COMMAND_LINE_OPTION_DESCRIPTION__

class CommandLineOptions;

class CommandLineOptionDescription {
  public:
    const char *name; // Command line options name
    int abbreviationLength; // Minimum number of characters in command ine option name abbreviation or
				 // 0 if no abbreviation is allowed
    CommandLineOptions *type; // Value type, or TYPELESS
    void *value; // Pointer to value, or nullptr if TYPELESS option or to store value in temporary variable
    void (*action)(void *); // Action called after parsing the value, can be a nullptr pointer. A pointer to the
				 // parsed option value (or nullptr if TYPELESS option) is passed as the argument
    const char *description; // Short description of the option. For printing command line option usage
};

#endif

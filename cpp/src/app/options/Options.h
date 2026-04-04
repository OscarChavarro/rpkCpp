/**
Command line options and defaults
*/

#ifndef __OPTIONS__
#define __OPTIONS__

#include "app/options/CommandLineOptions.h"
#include "app/options/CommandLineOptionDescription.h"
#include "app/options/EnumDesc.h"
#include "java/util/ArrayList.h"

struct OptionParser;

class Options final {
  public:
    // Default action; no action.
    static constexpr void (*DEFAULT_ACTION)(OptionValueWrapper) = nullptr;

    static void parseGeneralOptions(CommandLineOptionDescription *options, int *argc, char **argv);
    static bool optionsParseEnum(OptionValueWrapper value, void *data);
    static void optionsEnumPrint(java::PrintStream *stream, OptionValueWrapper value, void *data);
    static bool optionsParseFixedString(OptionValueWrapper value, void *data);
    static void optionsStringPrint(java::PrintStream *stream, OptionValueWrapper value, void *data);
    static bool optionsParseInt(OptionValueWrapper value, void *data);
    static void optionsPrintInt(java::PrintStream *stream, OptionValueWrapper value, void *data);
    static bool optionsParseString(OptionValueWrapper value, void *data);
    static void optionsPrintString(java::PrintStream *stream, OptionValueWrapper value, void *data);
    static bool optionsSetTrue(OptionValueWrapper value, void *data);
    static bool optionsSetFalse(OptionValueWrapper value, void *data);
    static void optionsPrintOther(java::PrintStream *stream, OptionValueWrapper x, void *data);
    static bool optionsParseFloat(OptionValueWrapper value, void *data);
    static void optionsPrintFloat(java::PrintStream *stream, OptionValueWrapper value, void *data);
    static bool optionsParseVector(OptionValueWrapper value, void *data);
    static void optionsPrintVector(java::PrintStream *stream, OptionValueWrapper value, void *data);
    static bool optionsParseRgb(OptionValueWrapper value, void *data);
    static void optionsPrintRgb(java::PrintStream *stream, OptionValueWrapper value, void *data);
    static bool optionsParseCieXy(OptionValueWrapper value, void *data);
    static void optionsPrintCieXyCallBack(java::PrintStream *stream, OptionValueWrapper value, void *data);
    static int *optionsCreateStringLengthStorage(int n);
    static void deleteOptionsMemory();

    /**
    The following helper function builds an enumerated value options type:
    example usage:

    static EnumDesc kinds = {
      { 1, "firstkind", 5 },
      { 2, "secondkind", 6 },
      { 0, nullptr, 0 }
    };
    static CommandLineOptions kindTypeStruct = makeEnumOptTypeStruct(kinds);
    "&kindTypeStruct" then can be used as option value type in a CMDLINEOPTDESC record
    */
    static inline CommandLineOptions makeEnumOptTypeStruct(EnumDesc *enumvaltab) {
        CommandLineOptions optionsType = {
            Options::optionsParseEnum,
            Options::optionsEnumPrint,
            OptionValueWrapper(),
            static_cast<void *>(enumvaltab)
        };
        return optionsType;
    }

    /**
    The following helper function builds an n string value options type:
    example usage:
    static CommandLineOptions nStringTypeStruct = makeNStringTypeStruct(n);
    "&nStringTypeStruct" then can be used as option value type in a CMDLINEOPTDESC record
    */
    static inline CommandLineOptions makeNStringTypeStruct(int n) {
        CommandLineOptions optionsType = {
            Options::optionsParseFixedString,
            Options::optionsStringPrint,
            OptionValueWrapper(),
            static_cast<void *>(Options::optionsCreateStringLengthStorage(n))
        };
        return optionsType;
    }

  private:
    friend struct OptionParser;

    static int *argumentCount;
    static char **arguments;
    static int currentArgumentIndex;
    static java::ArrayList<char *> *stringsToDelete;
    static java::ArrayList<int *> *stringLengthsToDelete;

    static void optionsInitArguments(int *argc, char **argv);
    static const char *optionsCurrentArgumentValue();
    static bool optionsArgumentsRemaining();
    static void optionsNextArgument();
    static void optionsConsumeArgument();
    static void optionsPrintEnumValues(const EnumDesc *tab);
};

#endif

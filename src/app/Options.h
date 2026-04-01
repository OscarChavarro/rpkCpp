/**
Command line options and defaults
*/

#ifndef __OPTIONS__
#define __OPTIONS__

#include "app/CommandLineOptions.h"
#include "app/CommandLineOptionDescription.h"
#include "app/EnumDesc.h"

extern CommandLineOptions GLOBAL_options_intType;
extern CommandLineOptions GLOBAL_options_boolType;
extern CommandLineOptions GLOBAL_options_setTrueType;
extern CommandLineOptions GLOBAL_options_setFalseType;
extern CommandLineOptions GLOBAL_options_stringType;
extern CommandLineOptions GLOBAL_options_floatType;
extern CommandLineOptions GLOBAL_options_vectorType;
extern CommandLineOptions GLOBAL_options_rgbType;
extern CommandLineOptions GLOBAL_options_xyType;
extern char *GLOBAL_option_dummyVal;

/**
Shorthands for specifying command line argument type, the 'type'
field of the CMD_LINE_OPT_DESC structure below
*/
extern CommandLineOptions *const OPTIONS_TYPE_BOOL;
extern CommandLineOptions *const OPTIONS_TYPE_SET_TRUE;
extern CommandLineOptions *const OPTIONS_TYPE_SET_FALSE;
extern CommandLineOptions *const OPTIONS_TYPE_STRING;
extern CommandLineOptions *const OPTIONS_TYPE_FLOAT;
extern CommandLineOptions *const OPTIONS_TYPE_VECTOR;
extern CommandLineOptions *const OPTIONS_TYPE_RGB;
extern CommandLineOptions *const OPTIONS_TYPE_XY;

// Default action; no action.
extern void (*const DEFAULT_ACTION)(void *);

class Options final {
  public:
    static void setDummyIntValueReference(int &dummyValue);
    static void parseGeneralOptions(CommandLineOptionDescription *options, int *argc, char **argv);
    static bool optionsEnumGet(void *value, void *data);
    static void optionsEnumPrint(java::PrintStream *stream, void *value, void *data);
    static bool optionsStringGet(void *value, void *data);
    static void optionsStringPrint(java::PrintStream *stream, void *value, void *data);
    static bool optionsGetInt(void *value, void *data);
    static void optionsPrintInt(java::PrintStream *stream, void *value, void *data);
    static bool optionsGetString(void *value, void *data);
    static void optionsPrintString(java::PrintStream *stream, void *value, void *data);
    static bool optionsSetTrue(void *value, void *data);
    static bool optionsSetFalse(void *value, void *data);
    static void optionsPrintOther(java::PrintStream *stream, void *x, void *data);
    static bool optionsGetfloat(void *value, void *data);
    static void optionsPrintFloat(java::PrintStream *stream, void *value, void *data);
    static bool optionsGetVector(void *value, void *data);
    static void optionsPrintVector(java::PrintStream *stream, void *value, void *data);
    static bool optionsGetRgb(void *value, void *data);
    static void optionsPrintRgb(java::PrintStream *stream, void *value, void *data);
    static bool optionsGetCieXy(void *value, void *data);
    static void optionsPrintCieXyCallBack(java::PrintStream *stream, void *value, void *data);
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
            Options::optionsEnumGet,
            Options::optionsEnumPrint,
            nullptr,
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
            Options::optionsStringGet,
            Options::optionsStringPrint,
            static_cast<void *>(&GLOBAL_option_dummyVal),
            static_cast<void *>(Options::optionsCreateStringLengthStorage(n))
        };
        return optionsType;
    }

  private:
    static void optionsInitArguments(int *argc, char **argv);
    static const char *optionsCurrentArgumentValue();
    static bool optionsArgumentsRemaining();
    static void optionsNextArgument();
    static void optionsConsumeArgument();
    static bool optionsGetArgumentIntValue(int *res);
    static bool optionsGetArgumentFloatValue(const char *format, float *res);
    static void optionsPrintEnumValues(const EnumDesc *tab);
    static unsigned long unsignedLongMax(unsigned long a, unsigned long b);
    static CommandLineOptionDescription *optionsLookupOption(const char *s, CommandLineOptionDescription *options);
    static void optionsProcessArguments(CommandLineOptionDescription *options);
};

#endif

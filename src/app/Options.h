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
extern int GLOBAL_options_dummyVal;
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

extern void parseGeneralOptions(CommandLineOptionDescription *options, int *argc, char **argv);

/**
Enumerated type options: let the enumTypeStruct.data field point to an array
of EnumDesc entries. These entries describe the integer options values and
their names. abbrev indicates the minimum number of characters in abbreviations
of the names. The last entry shall be {0, nullptr, 0} (sentinel)
*/
extern int optionsEnumGet(void *value, void *data);
extern void optionsEnumPrint(java::io::PrintStream *stream, void *value, void *data);

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
inline CommandLineOptions
makeEnumOptTypeStruct(EnumDesc *enumvaltab) {
    CommandLineOptions optionsType = {
        optionsEnumGet,
        optionsEnumPrint,
        static_cast<void *>(&GLOBAL_options_dummyVal),
        static_cast<void *>(enumvaltab)
    };
    return optionsType;
}

// n string options: let the nstringTypeStruct.data field point to a maximum string length

extern int optionsStringGet(void *value, void *data);
extern void optionsStringPrint(java::io::PrintStream *stream, void *value, void *data);
extern int *optionsCreateStringLengthStorage(int n);

/**
The following helper function builds an n string value options type:
example usage:
static CommandLineOptions nStringTypeStruct = makeNStringTypeStruct(n);
"&nStringTypeStruct" then can be used as option value type in a CMDLINEOPTDESC record
*/
inline CommandLineOptions
makeNStringTypeStruct(int n) {
    CommandLineOptions optionsType = {
        optionsStringGet,
        optionsStringPrint,
        static_cast<void *>(&GLOBAL_option_dummyVal),
        static_cast<void *>(optionsCreateStringLengthStorage(n))
    };
    return optionsType;
}

extern void deleteOptionsMemory();

#endif

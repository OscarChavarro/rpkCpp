/**
Command line options and defaults
*/

#ifndef __OPTIONS__
#define __OPTIONS__

#include <cstdio>

#include "skin/PatchSet.h"

/**
Command line option value type structures
*/
class CommandLineOptions {
  public:
    int (*get)(void *value, void *data); // Retrieves a argument value
    void (*print)(FILE *fp, void *value, void *data); // Prints an argument value
    void *dummy; // Pointer to "dummy" argument value storage
    void *data; // Pointer to additional data
};

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
field of the CMDLINEOPTDESC structure below
*/
#define TYPELESS nullptr
#define Tint (&GLOBAL_options_intType)
#define Tbool (&GLOBAL_options_boolType)
#define Tsettrue (&GLOBAL_options_setTrueType)
#define Tsetfalse (&GLOBAL_options_setFalseType)
#define Tstring (&GLOBAL_options_stringType)
#define Tfloat (&GLOBAL_options_floatType)
#define TVECTOR (&GLOBAL_options_vectorType)
#define TRGB (&GLOBAL_options_rgbType)
#define Txy (&GLOBAL_options_xyType)

// Default action; no action
#define DEFAULT_ACTION (void (*)(void *))nullptr

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

extern void parseOptions(CommandLineOptionDescription *options, int *argc, char **argv);

/**
Enumerated type options: let the enumTypeStruct.data field point to an array
of ENUMDESC entries. These entries describe the integer options values and
their names. abbrev indicates the minimum number of characters in abbreviations
of the names. The last entry shall be {0, nullptr, 0} (sentinel)
*/
class ENUMDESC {
  public:
    int value;
    const char *name;
    int abbrev;
};

extern int optionsEnumGet(int *, ENUMDESC *);
extern void optionsEnumPrint(FILE *fp, const int *v, ENUMDESC *tab);

/**
The following macro declares an enumerated value options type:
example usage:

static ENUMDESC kinds = {
  { 1, "firstkind", 5 },
  { 2, "secondkind", 6 },
  { 0, nullptr, 0 }
};
MakeEnumTypeStruct(kindTypeStruct, kinds);
#define Tkind (&kindTypeStruct)
"Tkind" then can be used as option value type in a CMDLINEOPTDESC record
*/
#define MakeEnumOptTypeStruct(enumTypeStructName, enumvaltab) \
static CommandLineOptions enumTypeStructName = { \
  (int (*)(void *, void *))optionsEnumGet, \
  (void (*)(FILE *, void *, void *))optionsEnumPrint, \
  (void *)&GLOBAL_options_dummyVal, \
  (void *)enumvaltab \
}

// n string options: let the nstringTypeStruct.data field point to a maximum string length

extern int optionsStringGet(char *, int);
extern void optionsStringPrint(FILE *fp, const char *s, int n);

/**
The following macro declares an n string value options type:
example usage:
MakeNStringTypeStruct(nStringTypeStruct, n);
#define Tnstring (&nStringTypeStruct)
"Tnstring" then can be used as option value type in a CMDLINEOPTDESC record
*/
#define MakeNStringTypeStruct(nstringTypeStructName, n) \
static CommandLineOptions nstringTypeStructName = { \
  (int (*)(void *, void *))optionsStringGet, \
  (void (*)(FILE *, void *, void *))optionsStringPrint, \
  (void *)&GLOBAL_option_dummyVal, \
  (void *)n \
}

#endif

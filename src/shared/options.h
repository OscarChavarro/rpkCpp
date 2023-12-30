/* options.h: command line options and defaults */

#ifndef _RPK_OPTIONS_H_
#define _RPK_OPTIONS_H_

#include <cstdio>

/* command line option value type structures. See options.c for examples. */
class CMDLINEOPTTYPE {
  public:
    int (*get)(void *value, void *data);    /* retrieves a argument value */
    void (*print)(FILE *fp, void *value, void *data);    /* prints an argument value */
    void *dummy;                /* pointer to "dummy" argument value storage */
    void *data;                /* pointer to additional data */
};

extern CMDLINEOPTTYPE intTypeStruct, boolTypeStruct, setTrueTypeStruct, setFalseTypeStruct,
        stringTypeStruct, nstringTypeStruct, floatTypeStruct,
        vectorTypeStruct, rgbTypeStruct, xyTypeStruct, enumTypeStruct;

/* shorthands for specifying command line argument type, the 'type'
 * field of the CMDLINEOPTDESC structure below. */
#define TYPELESS    (CMDLINEOPTTYPE *)0
#define Tint        (&intTypeStruct)    /* int* value pointer */
#define Tbool        (&boolTypeStruct)    /* int* value pointer */
#define Tsettrue    (&setTrueTypeStruct)    /* int* value pointer */
#define Tsetfalse    (&setFalseTypeStruct)    /* int* value pointer */
#define Tstring        (&stringTypeStruct)    /* char** value pointer */
#define Tfloat        (&floatTypeStruct)    /* float* value pointer */
#define TVECTOR        (&vectorTypeStruct)    /* Vector3D* value pointer */
#define TRGB        (&rgbTypeStruct)    /* RGB* value pointer */
#define Txy        (&xyTypeStruct)        /* CIE xy color value pair (float [2]) */

/* default action; no action */
#define DEFAULT_ACTION (void (*)(void *))0

/* command line option description structure */
class CMDLINEOPTDESC {
  public:
    const char *name;            /* command line options name */
    int abbrevlength;        /* minimum number of characters in
				 * command ine option name abbreviation or
				 * 0 if no abbreviation is allowed. */
    CMDLINEOPTTYPE *type;        /* value type, or TYPELESS */
    void *value;            /* pointer to value, or nullptr if TYPELESS
				 * option or to store value in temporary
				 * variable. */
    void (*action)(void *);    /* action called after parsing the value, can
				 * be a nullptr pointer. A pointer to the
				 * parsed option value (or nullptr if
				 * TYPELESS option) is passed as
				 * the argument. */
    const char *description;        /* short description of the option. For
				 * printing command line option usage. */
};

/* scans for options mentionned in the 'options' command line description 
 * list, parses their value, executes their associated actions, and
 * removes them from the argv list (decreasing argc) */
extern void ParseOptions(CMDLINEOPTDESC *options, int *argc, char **argv);

/* enumerated type options: let the enumTypeStruct.data field point to an array
 * of ENUMDESC entries. These entries describe the integer options values and
 * their names. abbrev indicates the minimum number of characters in abbreviations
 * of the names. The last entry shall be {0, nullptr, 0} (sentinel). */
class ENUMDESC {
  public:
    int value;
    const char *name;
    int abbrev;
};

extern int Tenum_get(int *, ENUMDESC *);

extern void Tenum_print(FILE *, int *, ENUMDESC *);

extern int Tenum_dummy_val;

/* The following macro declares an enumerated value options type:
 * example usage:
 *
 * static ENUMDESC kinds = {
 *   { 1, "firstkind", 5 },
 *   { 2, "secondkind", 6 },
 *   { 0, nullptr, 0 }
 * };
 * MakeEnumTypeStruct(kindTypeStruct, kinds);
 * #define Tkind (&kindTypeStruct)
 * "Tkind" then can be used as option value type in a CMDLINEOPTDESC record
 */
#define MakeEnumOptTypeStruct(enumTypeStructName, enumvaltab) \
static CMDLINEOPTTYPE enumTypeStructName = {        \
  (int (*)(void *, void *))Tenum_get,            \
  (void (*)(FILE *, void *, void *))Tenum_print,    \
  (void *)&Tenum_dummy_val,                \
  (void *)enumvaltab                    \
}


/* n string options: let the nstringTypeStruct.data field point to a maximum string length */

extern int Tnstring_get(char *, int);

extern void Tnstring_print(FILE *, char *, int);

extern char *Tnstring_dummy_val;

/* The following macro declares an n string value options type:
 * example usage:
 * MakeNStringTypeStruct(nStringTypeStruct, n);
 * #define Tnstring (&nStringTypeStruct)
 * "Tnstring" then can be used as option value type in a CMDLINEOPTDESC record
 */
#define MakeNStringTypeStruct(nstringTypeStructName, n) \
static CMDLINEOPTTYPE nstringTypeStructName = {        \
  (int (*)(void *, void *))Tnstring_get,            \
  (void (*)(FILE *, void *, void *))Tnstring_print,    \
  (void *)&Tnstring_dummy_val,                \
  (void *)n                    \
}

#endif
